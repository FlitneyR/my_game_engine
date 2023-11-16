#include <engine.hpp>

#include <set>
#include <sstream>
#include <fstream>

namespace mge {

const char* Engine::s_engineName = "My Game Engine";
const uint32_t Engine::s_engineVersion = VK_MAKE_VERSION(0, 1, 0);

const std::vector<std::string> Engine::s_requiredInstanceLayers {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<std::string> Engine::s_requiredDeviceLayers {
    "VK_LAYER_KHRONOS_validation",
};

const std::vector<std::string> Engine::s_requiredInstanceExtensions {
    "VK_KHR_portability_enumeration",
    "VK_KHR_surface",
    "VK_EXT_metal_surface",
};

const std::vector<std::string> Engine::s_requiredDeviceExtensions {
    "VK_KHR_portability_subset",
    "VK_KHR_swapchain",
};

void Engine::init(uint32_t initWidth, uint32_t initHeight) {
    glfwInit();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
    m_window = glfwCreateWindow(initWidth, initHeight, getGameName().c_str(), nullptr, nullptr);

    createInstance();
    getSurface();
    getPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createSynchronisers();
    createDepthBuffer();
    createRenderPass();
    createShadowMappingRenderPass();
    createGBufferDescriptorSetLayout();
    createGBuffer();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
}

void Engine::main() {
    m_startTime = std::chrono::high_resolution_clock::now();
    m_lastFrameTime = m_startTime;

    start();

    while (!glfwWindowShouldClose(m_window)) {
        auto frameStart = std::chrono::high_resolution_clock::now();

        auto microsSinceStart = std::chrono::duration_cast<std::chrono::microseconds>(frameStart - m_startTime).count();
        auto microsSinceLastFrame = std::chrono::duration_cast<std::chrono::microseconds>(frameStart - m_lastFrameTime).count();
        double time = static_cast<double>(microsSinceStart) / 1'000'000.f;
        double deltaTime = static_cast<double>(microsSinceLastFrame) / 1'000'000.f;
        
        glfwPollEvents();
        physicsUpdate(deltaTime);
        update(deltaTime);
        draw();

        m_lastFrameTime = frameStart;
    }

    m_device.waitIdle();

    end();
}

void Engine::checkInstanceLayers() {
    auto supportedLayers = vk::enumerateInstanceLayerProperties();
    std::set<std::string> missingLayers(s_requiredInstanceLayers.begin(), s_requiredInstanceLayers.end());
    for (const auto& layer : supportedLayers) missingLayers.erase(std::string(layer.layerName));

    if (!missingLayers.empty()) {
        std::ostringstream errorMsg; errorMsg << "Missing required layers: ";
        std::copy(missingLayers.begin(), missingLayers.end(), std::ostream_iterator<std::string>(errorMsg, ", "));
        throw std::runtime_error(errorMsg.str());
    }
}

void Engine::checkInstanceExtensions() {
    auto supportedExtensions = vk::enumerateInstanceExtensionProperties();
    std::set<std::string> missingExtensions(s_requiredInstanceExtensions.begin(), s_requiredInstanceExtensions.end());
    for (const auto& extension : supportedExtensions) missingExtensions.erase(std::string(extension.extensionName));

    if (!missingExtensions.empty()) {
        std::ostringstream errorMsg; errorMsg << "Missing required extensions: ";
        std::copy(missingExtensions.begin(), missingExtensions.end(), std::ostream_iterator<std::string>(errorMsg, ", "));
        throw std::runtime_error(errorMsg.str());
    }
}

bool Engine::checkDeviceLayers(vk::PhysicalDevice device) {
    auto supportedLayers = device.enumerateDeviceLayerProperties();
    std::set<std::string> missingLayers(s_requiredDeviceLayers.begin(), s_requiredDeviceLayers.end());
    for (const auto& layer : supportedLayers) missingLayers.erase(std::string(layer.layerName));

    return missingLayers.empty();
}

bool Engine::checkDeviceExtensions(vk::PhysicalDevice device) {
    auto supportedExtensions = device.enumerateDeviceExtensionProperties();
    std::set<std::string> missingExtensions(s_requiredDeviceExtensions.begin(), s_requiredDeviceExtensions.end());
    for (const auto& extension : supportedExtensions) missingExtensions.erase(std::string(extension.extensionName));

    return missingExtensions.empty();
}

void Engine::createInstance() {
    checkInstanceLayers();
    checkInstanceExtensions();

    std::vector<const char*> requiredLayers;
    requiredLayers.reserve(s_requiredInstanceLayers.size());
    for (const auto& layer : s_requiredInstanceLayers)
        requiredLayers.push_back(layer.c_str());

    std::vector<const char*> requiredExtensions;
    requiredExtensions.reserve(s_requiredInstanceExtensions.size());
    for (const auto& extension : s_requiredInstanceExtensions)
        requiredExtensions.push_back(extension.c_str());

    auto appInfo = vk::ApplicationInfo {}
        .setApiVersion(VK_API_VERSION_1_3)
        .setApplicationVersion(getGameVersion())
        .setEngineVersion(s_engineVersion)
        .setPApplicationName(getGameName().c_str())
        .setPEngineName(s_engineName)
        ;

    auto createInfo = vk::InstanceCreateInfo {}
        .setPEnabledLayerNames(requiredLayers)
        .setPEnabledExtensionNames(requiredExtensions)
        .setPApplicationInfo(&appInfo)
        .setFlags(vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR)
        ;

    m_instance = vk::createInstance(createInfo);
}

void Engine::getSurface() {
    VkSurfaceKHR surface;
    vk::Result result { glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface) };

    if (result == vk::Result::eErrorExtensionNotPresent) {
        uint32_t missingExtensionCount;
        const char** _missingExtensions = glfwGetRequiredInstanceExtensions(&missingExtensionCount);

        std::vector<const char*> missingExtensions(missingExtensionCount);
        memcpy(missingExtensions.data(), _missingExtensions, missingExtensionCount * sizeof(const char*));

        std::ostringstream errorMsg; errorMsg << "One of the following extensions is missing: ";
        std::copy(_missingExtensions, _missingExtensions + missingExtensionCount, std::ostream_iterator<const char*>(errorMsg, ", "));

        vk::resultCheck(result, errorMsg.str().c_str());
    } else vk::resultCheck(result, "Failed to get surface from window");

    m_surface = surface;
}

Engine::QueueFamilies Engine::getQueueFamilies(vk::PhysicalDevice device) const {
    QueueFamilies result;

    auto queueFamilies = device.getQueueFamilyProperties();

    int index = -1;
    for (const auto& queueFamily : queueFamilies) { index++;
        if (queueFamily.queueFlags & vk::QueueFlagBits::eGraphics)
            result.graphicsFamily = index;
        
        if (device.getSurfaceSupportKHR(index, m_surface))
            result.presentFamily = index;
        
        if (result.is_complete()) break;
    }

    return result;
}

void Engine::getPhysicalDevice() {
    auto devices = m_instance.enumeratePhysicalDevices();

    for (const auto& device : devices) {
        auto queueFamilies = getQueueFamilies(device);

        if (queueFamilies.is_complete() && checkDeviceLayers(device) && checkDeviceExtensions(device)) {
            m_physicalDevice = device;
            m_queueFamilies = queueFamilies;

            return;
        }
    }

    throw std::runtime_error("Failed to find a suitable physical device");
}

void Engine::createLogicalDevice() {
    std::vector<const char*> requiredLayers;
    requiredLayers.reserve(s_requiredDeviceLayers.size());
    for (const auto& layer : s_requiredDeviceLayers)
        requiredLayers.push_back(layer.c_str());

    std::vector<const char*> requiredExtensions;
    requiredExtensions.reserve(s_requiredDeviceExtensions.size());
    for (const auto& extension : s_requiredDeviceExtensions)
        requiredExtensions.push_back(extension.c_str());

    std::vector<const float> priorities { 1.0f };

    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos {
        vk::DeviceQueueCreateInfo {}
            .setQueueCount(1)
            .setQueueFamilyIndex(*m_queueFamilies.graphicsFamily)
            .setQueuePriorities(priorities)
            ,
    };

    auto features = m_physicalDevice.getFeatures();

    auto createInfo = vk::DeviceCreateInfo {}
        .setPEnabledLayerNames(requiredLayers)
        .setPEnabledExtensionNames(requiredExtensions)
        .setQueueCreateInfos(queueCreateInfos)
        .setPEnabledFeatures(&features)
        ;
    
    m_device = m_physicalDevice.createDevice(createInfo);
}

void Engine::createSwapchain() {
    auto surfaceCaps = m_physicalDevice.getSurfaceCapabilitiesKHR(m_surface);
    auto surfaceFormats = m_physicalDevice.getSurfaceFormatsKHR(m_surface);

    vk::SurfaceFormatKHR surfaceFormat = surfaceFormats[0];

    for (const auto& sf : surfaceFormats) {
        if (sf.format != vk::Format::eB8G8R8A8Srgb) continue;
        if (sf.colorSpace != vk::ColorSpaceKHR::eSrgbNonlinear) continue;
        
        surfaceFormat = sf;
        break;
    }

    auto createInfo = vk::SwapchainCreateInfoKHR {}
        .setClipped(true)
        .setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
        .setImageArrayLayers(1)
        .setImageColorSpace(surfaceFormat.colorSpace)
        .setImageExtent(surfaceCaps.currentExtent)
        .setImageFormat(surfaceFormat.format)
        .setImageSharingMode(vk::SharingMode::eExclusive)
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eTransferDst)
        .setMinImageCount(std::min(surfaceCaps.minImageCount + 1, surfaceCaps.maxImageCount))
        .setPresentMode(vk::PresentModeKHR::eMailbox)
        .setSurface(m_surface)
        .setPreTransform(surfaceCaps.currentTransform)
        ;
    
    if (createInfo.imageExtent == vk::Extent2D { UINT32_MAX, UINT32_MAX }) {
        int _width, _height;
        glfwGetFramebufferSize(m_window, &_width, &_height);

        uint32_t width = _width, height = _height;
        width = std::clamp(width, surfaceCaps.minImageExtent.width, surfaceCaps.maxImageExtent.width);
        height = std::clamp(height, surfaceCaps.minImageExtent.height, surfaceCaps.maxImageExtent.height);

        createInfo.setImageExtent({ width, height });
    }

    std::vector<uint32_t> queueFamilyIndices {
        *m_queueFamilies.graphicsFamily,
        *m_queueFamilies.presentFamily
    };

    if (m_queueFamilies.graphicsFamily != m_queueFamilies.presentFamily)
        createInfo
            .setImageSharingMode(vk::SharingMode::eConcurrent)
            .setQueueFamilyIndices(queueFamilyIndices)
            ;

    m_swapchainFormat = surfaceFormat;
    m_swapchainExtent = createInfo.imageExtent;
    m_swapchain = m_device.createSwapchainKHR(createInfo);
    m_swapchainImages = m_device.getSwapchainImagesKHR(m_swapchain);

    m_swapchainImageViews.resize(m_swapchainImages.size());
    auto imageViewCreateInfo = vk::ImageViewCreateInfo {}
        .setViewType(vk::ImageViewType::e2D)
        .setFormat(m_swapchainFormat.format)
        ;
    
    imageViewCreateInfo.components
        .setR(vk::ComponentSwizzle::eR)
        .setG(vk::ComponentSwizzle::eG)
        .setB(vk::ComponentSwizzle::eB)
        .setA(vk::ComponentSwizzle::eA)
        ;
    
    imageViewCreateInfo.subresourceRange
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1)
        ;

    for (int i = 0; i < m_swapchainImageViews.size(); i++)
        m_swapchainImageViews[i] = m_device.createImageView(imageViewCreateInfo.setImage(m_swapchainImages[i]));
}

void Engine::rebuildSwapchain() {
    m_device.waitIdle();

    m_device.destroyImage(m_depthImage);
    m_device.destroyImageView(m_depthImageView);
    m_device.freeMemory(m_depthImageMemory);

    m_device.destroyImage(m_albedoImage);
    m_device.destroyImageView(m_albedoImageView);
    m_device.freeMemory(m_albedoImageMemory);

    m_device.destroyImage(m_normalImage);
    m_device.destroyImageView(m_normalImageView);
    m_device.freeMemory(m_normalImageMemory);

    m_device.destroyImage(m_armImage);
    m_device.destroyImageView(m_armImageView);
    m_device.freeMemory(m_armImageMemory);

    m_device.destroyImage(m_emissiveImage);
    m_device.destroyImageView(m_emissiveImageView);
    m_device.freeMemory(m_emissiveImageMemory);

    m_device.destroyDescriptorPool(m_gbufferDescriptorPool);

    m_device.destroySwapchainKHR(m_swapchain);
    for (auto& imageView : m_swapchainImageViews) m_device.destroyImageView(imageView);
    for (auto& framebuffer : m_framebuffers) m_device.destroyFramebuffer(framebuffer);

    createSwapchain();
    createDepthBuffer();
    createGBuffer();
    createFramebuffers();
}

void Engine::createSynchronisers() {
    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    auto fenceCreateInfo = vk::FenceCreateInfo {}.setFlags(vk::FenceCreateFlagBits::eSignaled);

    m_imageAvailableSemaphores.resize(getMaxFramesInFlight());
    m_renderFinishedSemaphores.resize(getMaxFramesInFlight());
    m_commandBufferReadyFences.resize(getMaxFramesInFlight());

    for (int i = 0; i < getMaxFramesInFlight(); i++) {
        m_imageAvailableSemaphores[i] = m_device.createSemaphore(semaphoreCreateInfo);
        m_renderFinishedSemaphores[i] = m_device.createSemaphore(semaphoreCreateInfo);
        m_commandBufferReadyFences[i] = m_device.createFence(fenceCreateInfo);
    }
}

void Engine::createDepthBuffer() {
    // select image format
    const static std::vector<vk::Format> potentialDepthFormats {
        vk::Format::eD32Sfloat,
        vk::Format::eD24UnormS8Uint,
        vk::Format::eD16UnormS8Uint,
        vk::Format::eD16Unorm,
    };

    for (const auto& format : potentialDepthFormats) {
        auto properties = m_physicalDevice.getFormatProperties(format);
        
        if (properties.optimalTilingFeatures & vk::FormatFeatureFlagBits::eDepthStencilAttachment) {
            m_depthImageFormat = format;
            break;
        }
    }

    // create image
    auto bufferCreateInfo = vk::ImageCreateInfo {}
        .setArrayLayers(1)
        .setExtent(vk::Extent3D(m_swapchainExtent, 1))
        .setFormat(m_depthImageFormat)
        .setImageType(vk::ImageType::e2D)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(1)
        .setQueueFamilyIndices(*m_queueFamilies.graphicsFamily)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment | vk::ImageUsageFlagBits::eInputAttachment)
        ;

    m_depthImage = m_device.createImage(bufferCreateInfo);

    // allocate memory
    auto memReqs = m_device.getImageMemoryRequirements(m_depthImage);
    auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

    auto allocInfo = vk::MemoryAllocateInfo {}
        .setMemoryTypeIndex(findMemoryType(memReqs.memoryTypeBits, properties))
        .setAllocationSize(memReqs.size)
        ;

    m_depthImageMemory = m_device.allocateMemory(allocInfo);
    m_device.bindImageMemory(m_depthImage, m_depthImageMemory, 0);

    // create image view
    auto viewCreateInfo = vk::ImageViewCreateInfo {}
        .setFormat(m_depthImageFormat)
        .setImage(m_depthImage)
        .setViewType(vk::ImageViewType::e2D)
        ;
    
    viewCreateInfo.subresourceRange
        .setAspectMask(vk::ImageAspectFlagBits::eDepth)
        .setBaseMipLevel(0)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setLevelCount(1)
        ;

    m_depthImageView = m_device.createImageView(viewCreateInfo);
}

void Engine::createRenderPass() {
    auto templateAttachment = vk::AttachmentDescription {}
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        ;

    auto renderTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eTransferDstOptimal)
        .setFormat(m_swapchainFormat.format)
        .setLoadOp(vk::AttachmentLoadOp::eDontCare)
        ;

    auto depthTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setFormat(m_depthImageFormat)
        ;

    auto albedoTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setFormat(m_albedoFormat)
        ;

    auto normalTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setFormat(m_normalFormat)
        ;

    auto armTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eColorAttachmentOptimal)
        .setFormat(m_armFormat)
        ;

    auto emissiveTarget = vk::AttachmentDescription { templateAttachment }
        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setFormat(m_emissiveFormat)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        ;
    
    std::vector<vk::AttachmentDescription> attachments {
        renderTarget,
        depthTarget,
        albedoTarget,
        normalTarget,
        armTarget,
        emissiveTarget
        };

    auto gBufferAttachments = std::vector<vk::AttachmentReference> {
        vk::AttachmentReference {}
            .setAttachment(2)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(3)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(4)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(5)
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
            ,
        };
    
    auto lightingAttachments = std::vector<vk::AttachmentReference> {
        vk::AttachmentReference {}
            .setAttachment(1)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(2)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(3)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            ,
        vk::AttachmentReference {}
            .setAttachment(4)
            .setLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            ,
    };
    
    auto emissiveAttachment = vk::AttachmentReference {}
        .setAttachment(5)
        .setLayout(vk::ImageLayout::eGeneral)
        ;
    
    auto depthAttachment = vk::AttachmentReference {}
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        ;

    std::vector<vk::SubpassDescription> subpasses = {
        vk::SubpassDescription {}
            .setColorAttachments(gBufferAttachments)
            .setPDepthStencilAttachment(&depthAttachment)
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            ,
        vk::SubpassDescription {}
            .setColorAttachments(emissiveAttachment)
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setInputAttachments(lightingAttachments)
            ,
        // vk::SubpassDescription {}
        //     .setColorAttachments(emissiveAttachment)
        //     .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        //     .setInputAttachments(emissiveAttachment)
        //     ,
        };
    
    std::vector<vk::SubpassDependency> dependencies {
        vk::SubpassDependency {} // prepare depth stencil for writing
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
                           | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
                           | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
                            | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            ,
        vk::SubpassDependency {} // prepare model textures for reading
            .setSrcSubpass(0)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eTopOfPipe)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            ,
        vk::SubpassDependency {} // shadow map dependency
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(1)
            .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests
                           | vk::PipelineStageFlagBits::eLateFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            ,
        vk::SubpassDependency {} // prepare gbuffer for reading
            .setSrcSubpass(0)
            .setDstSubpass(1)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
                           | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
                            | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
            ,
        vk::SubpassDependency {} // prepare emissive for display
            .setSrcSubpass(1)
            .setDstSubpass(1)
            .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask(vk::AccessFlagBits::eInputAttachmentRead)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
            ,
    };

    auto createInfo = vk::RenderPassCreateInfo {}
        .setAttachments(attachments)
        .setSubpasses(subpasses)
        .setDependencies(dependencies)
        ;
    
    m_renderPass = m_device.createRenderPass(createInfo);
}

void Engine::createShadowMappingRenderPass() {
    auto depthStencilAttachment = vk::AttachmentReference {}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        ;

    auto subpasses = vk::SubpassDescription {}
        .setPDepthStencilAttachment(&depthStencilAttachment)
        .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
        ;

    auto dependencies = std::vector<vk::SubpassDependency> {
        vk::SubpassDependency {}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDstStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests 
                           | vk::PipelineStageFlagBits::eLateFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            ,
        vk::SubpassDependency {}
            .setSrcSubpass(0)
            .setDstSubpass(VK_SUBPASS_EXTERNAL)
            .setSrcStageMask(vk::PipelineStageFlagBits::eEarlyFragmentTests
                           | vk::PipelineStageFlagBits::eLateFragmentTests)
            .setSrcAccessMask(vk::AccessFlagBits::eDepthStencilAttachmentWrite)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            ,
        };

    m_shadowMappingRenderPass = m_device.createRenderPass(vk::RenderPassCreateInfo {}
        .setAttachments(vk::AttachmentDescription {}
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setFormat(m_depthImageFormat)
            .setLoadOp(vk::AttachmentLoadOp::eClear)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            )
        .setSubpasses(subpasses)
        .setDependencies(dependencies)
        );
}

vk::ShaderModule Engine::loadShaderModule(std::string path) {
    std::ifstream file(path, std::ios::ate | std::ios::binary);
    unsigned int num_bytes = file.tellg(); file.seekg(0);

    std::vector<uint32_t> code(num_bytes / 4);
    file.read(reinterpret_cast<char*>(code.data()), num_bytes);

    return compileShaderModule(code);
}

vk::ShaderModule Engine::compileShaderModule(std::vector<uint32_t>& code) {
    auto createInfo = vk::ShaderModuleCreateInfo {}.setCode(code);
    return m_device.createShaderModule(createInfo);
}

void Engine::createGBufferDescriptorSetLayout() {
    auto binding = vk::DescriptorSetLayoutBinding {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        ;
    
    std::vector<vk::DescriptorSetLayoutBinding> bindings {
        binding.setBinding(0),
        binding.setBinding(1),
        binding.setBinding(2),
        binding.setBinding(3),
    };

    auto createInfo = vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(bindings)
        ;
    
    m_gbufferDescriptorSetLayout = m_device.createDescriptorSetLayout(createInfo);
}

void Engine::createGBuffer() {
    // create images
    auto imageCreateInfo = vk::ImageCreateInfo {}
        .setArrayLayers(1)
        .setExtent(vk::Extent3D { m_swapchainExtent, 1 })
        .setImageType(vk::ImageType::e2D)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(1)
        .setQueueFamilyIndices(*m_queueFamilies.graphicsFamily)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eColorAttachment
                // | vk::ImageUsageFlagBits::eTransientAttachment
                | vk::ImageUsageFlagBits::eInputAttachment)
        ;

    m_albedoImage = m_device.createImage(imageCreateInfo.setFormat(m_albedoFormat));
    m_normalImage = m_device.createImage(imageCreateInfo.setFormat(m_normalFormat));
    m_armImage = m_device.createImage(imageCreateInfo.setFormat(m_armFormat));
    m_emissiveImage = m_device.createImage(imageCreateInfo
        .setFormat(m_emissiveFormat)
        .setUsage(imageCreateInfo.usage | vk::ImageUsageFlagBits::eTransferSrc));

    auto imageViewCreateInfo = vk::ImageViewCreateInfo {}
        .setSubresourceRange(vk::ImageSubresourceRange {
            vk::ImageAspectFlagBits::eColor,
            0, 1,
            0, 1
        })
        .setViewType(vk::ImageViewType::e2D)
        ;

    std::vector<vk::Image*> images { &m_albedoImage, &m_normalImage, &m_armImage, &m_emissiveImage };
    std::vector<vk::DeviceMemory*> memory { &m_albedoImageMemory, &m_normalImageMemory, &m_armImageMemory, &m_emissiveImageMemory };

    for (int i = 0; i < images.size(); i++) {
        auto memreqs = m_device.getImageMemoryRequirements(*images[i]);
        auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        auto allocateInfo = vk::MemoryAllocateInfo {}
            .setAllocationSize(memreqs.size)
            .setMemoryTypeIndex(findMemoryType(memreqs.memoryTypeBits, properties))
            ;

        *memory[i] = m_device.allocateMemory(allocateInfo);

        m_device.bindImageMemory(*images[i], *memory[i], 0);
    }

    m_albedoImageView = m_device.createImageView(imageViewCreateInfo
        .setFormat(m_albedoFormat)
        .setImage(m_albedoImage));

    m_normalImageView = m_device.createImageView(imageViewCreateInfo
        .setImage(m_normalImage)
        .setFormat(m_normalFormat));

    m_armImageView = m_device.createImageView(imageViewCreateInfo
        .setImage(m_armImage)
        .setFormat(m_armFormat));

    m_emissiveImageView = m_device.createImageView(imageViewCreateInfo
        .setImage(m_emissiveImage)
        .setFormat(m_emissiveFormat));
    
    createGBufferDescriptorSet();
}

void Engine::createGBufferDescriptorSet() {
    auto poolSize = vk::DescriptorPoolSize {}
        .setDescriptorCount(4)
        .setType(vk::DescriptorType::eInputAttachment)
        ;

    auto poolCreateInfo = vk::DescriptorPoolCreateInfo {}
        .setMaxSets(1)
        .setPoolSizes(poolSize)
        ;

    m_gbufferDescriptorPool = m_device.createDescriptorPool(poolCreateInfo);

    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_gbufferDescriptorPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(m_gbufferDescriptorSetLayout)
        ;

    m_gbufferDescriptorSet = m_device.allocateDescriptorSets(allocInfo)[0];

    auto imageInfo = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSampler(nullptr)
        ;

    auto imageInfos = std::vector<vk::DescriptorImageInfo> {
        imageInfo.setImageView(m_depthImageView),
        imageInfo.setImageView(m_albedoImageView),
        imageInfo.setImageView(m_normalImageView),
        imageInfo.setImageView(m_armImageView),
    };

    auto descriptorSetWrite = vk::WriteDescriptorSet {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setDstArrayElement(0)
        .setDstBinding(0)
        .setDstSet(m_gbufferDescriptorSet)
        .setImageInfo(imageInfos)
        ;

    m_device.updateDescriptorSets(descriptorSetWrite, nullptr);
}

void Engine::createFramebuffers() {
    auto createInfo = vk::FramebufferCreateInfo {}
        .setWidth(m_swapchainExtent.width)
        .setHeight(m_swapchainExtent.height)
        .setLayers(1)
        .setRenderPass(m_renderPass)
        ;
    
    m_framebuffers.clear();
    m_framebuffers.resize(m_swapchainImageViews.size());

    for (int i = 0; i < m_framebuffers.size(); i++) {
        std::vector<vk::ImageView> attachments {
            m_swapchainImageViews[i],
            m_depthImageView,
            m_albedoImageView,
            m_normalImageView,
            m_armImageView,
            m_emissiveImageView
        };

        createInfo.setAttachments(attachments);
        m_framebuffers[i] = m_device.createFramebuffer(createInfo);
    }
}

void Engine::createCommandPool() {
    auto createInfo = vk::CommandPoolCreateInfo {}
        .setQueueFamilyIndex(*m_queueFamilies.graphicsFamily)
        .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer)
        ;
    
    m_commandPool = m_device.createCommandPool(createInfo);
}

void Engine::createCommandBuffer() {
    auto allocInfo = vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(getMaxFramesInFlight())
        .setCommandPool(m_commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        ;

    m_commandBuffers = m_device.allocateCommandBuffers(allocInfo);
}

uint32_t Engine::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    auto memProps = m_physicalDevice.getMemoryProperties();

    for (int index = 0; index < memProps.memoryTypeCount; index++) {
        if (!(typeFilter & (1 << index))) continue;
        if ((memProps.memoryTypes[index].propertyFlags & properties) == properties) return index;
    }

    throw std::runtime_error("Failed to find a valid memory type");
}

void Engine::draw() {
    auto waitResult = m_device.waitForFences(m_commandBufferReadyFences[m_currentInFlightFrame], true, UINT64_MAX);
    vk::resultCheck(waitResult, "Failed to wait for command-buffer-ready fence");
    m_device.resetFences(m_commandBufferReadyFences[m_currentInFlightFrame]);

    auto nextImageResult = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_imageAvailableSemaphores[m_currentInFlightFrame]);

    if (nextImageResult.result != vk::Result::eSuboptimalKHR)
        vk::resultCheck(nextImageResult.result, "Failed to acquire next image");
    
    auto imageIndex = nextImageResult.value;

    vk::CommandBuffer cmd = m_commandBuffers[m_currentInFlightFrame];

    cmd.reset();

    vk::CommandBufferBeginInfo beginInfo;
    cmd.begin(beginInfo);

    recordShadowMapDrawCommands(cmd);

    auto viewport = vk::Viewport {}
        .setX(0.f)
        .setY(0.f)
        .setWidth(static_cast<float>(m_swapchainExtent.width))
        .setHeight(static_cast<float>(m_swapchainExtent.height))
        .setMinDepth(0.f)
        .setMaxDepth(1.f)
        ;

    auto scissor = vk::Rect2D {}
        .setOffset({ 0, 0 })
        .setExtent(m_swapchainExtent)
        ;

    std::vector<vk::ClearValue> clearValues {
        vk::ClearValue {}.setColor({ 0.f, 0.f, 0.f, 1.f }),    // final colour
        vk::ClearValue {}.setDepthStencil({ 1.0f, 0 }),        // depth
        vk::ClearValue {}.setColor({ 0.f, 0.f, 0.f, 1.f }),    // albedo
        vk::ClearValue {}.setColor({ 0.f, 0.f, 1.f, 1.f }),    // normal
        vk::ClearValue {}.setColor({ 0.f, 0.f, 0.f, 1.f }),    // arm 
        vk::ClearValue {}.setColor({ 0.f, 0.f, 0.f, 1.f }),    // emissive
    };

    auto renderPassBeginInfo = vk::RenderPassBeginInfo {}
        .setClearValues(clearValues)
        .setFramebuffer(m_framebuffers[imageIndex])
        .setRenderArea({ {}, m_swapchainExtent })
        .setRenderPass(m_renderPass)
        ;

    cmd.setViewport(0, viewport);
    cmd.setScissor(0, scissor);
    cmd.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    recordGBufferDrawCommands(cmd);
    cmd.nextSubpass(vk::SubpassContents::eInline);
    recordLightingDrawCommands(cmd);

    cmd.endRenderPass();

    recordPostProcessingDrawCommands(cmd);

    auto region = vk::ImageBlit {}
        .setSrcSubresource(vk::ImageSubresourceLayers {
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1
        })
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D {
                static_cast<int32_t>(m_swapchainExtent.width),
                static_cast<int32_t>(m_swapchainExtent.height),
                1
            },
        })
        .setDstSubresource(vk::ImageSubresourceLayers {
            vk::ImageAspectFlagBits::eColor,
            0, 0, 1
        })
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D {
                static_cast<int32_t>(m_swapchainExtent.width),
                static_cast<int32_t>(m_swapchainExtent.height),
                1
            },
        })
        ;

    cmd.blitImage(
        m_emissiveImage, vk::ImageLayout::eTransferSrcOptimal,
        m_swapchainImages[imageIndex], vk::ImageLayout::eTransferDstOptimal,
        region, vk::Filter::eLinear
    );

    auto imageBarrier = vk::ImageMemoryBarrier {}
        .setSrcQueueFamilyIndex(*m_queueFamilies.graphicsFamily)
        .setDstQueueFamilyIndex(*m_queueFamilies.graphicsFamily)
        .setSubresourceRange(vk::ImageSubresourceRange {
            vk::ImageAspectFlagBits::eColor,
            0, 1,
            0, 1
        })
        ;

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        imageBarrier
            .setImage(m_swapchainImages[imageIndex])
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::ePresentSrcKHR)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask({})
        );

    cmd.end();

    vk::Queue graphicsQueue = m_device.getQueue(*m_queueFamilies.graphicsFamily, 0);
    
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTopOfPipe;

    std::vector<vk::SubmitInfo> submits {
        vk::SubmitInfo {}
            .setCommandBuffers(cmd)
            .setWaitSemaphores(m_imageAvailableSemaphores[m_currentInFlightFrame])
            .setWaitDstStageMask(waitDstStageMask)
            .setSignalSemaphores(m_renderFinishedSemaphores[m_currentInFlightFrame])
            ,
    };

    graphicsQueue.submit(submits, m_commandBufferReadyFences[m_currentInFlightFrame]);

    auto presentInfo = vk::PresentInfoKHR {}
        .setImageIndices(imageIndex)
        .setSwapchains(m_swapchain)
        .setWaitSemaphores(m_renderFinishedSemaphores[m_currentInFlightFrame])
        ;

    vk::Queue presentQueue = m_device.getQueue(*m_queueFamilies.presentFamily, 0);
    auto presentResult = presentQueue.presentKHR(presentInfo);

    if (presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR) {
        rebuildSwapchain();
    } else vk::resultCheck(presentResult, "Failed to present render result");

    m_currentInFlightFrame = (m_currentInFlightFrame + 1) % getMaxFramesInFlight();
}

void Engine::cleanup() {
    m_device.destroyImage(m_depthImage);
    m_device.destroyImageView(m_depthImageView);
    m_device.freeMemory(m_depthImageMemory);

    m_device.destroyImage(m_albedoImage);
    m_device.destroyImageView(m_albedoImageView);
    m_device.freeMemory(m_albedoImageMemory);

    m_device.destroyImage(m_normalImage);
    m_device.destroyImageView(m_normalImageView);
    m_device.freeMemory(m_normalImageMemory);

    m_device.destroyImage(m_armImage);
    m_device.destroyImageView(m_armImageView);
    m_device.freeMemory(m_armImageMemory);

    m_device.destroyImage(m_emissiveImage);
    m_device.destroyImageView(m_emissiveImageView);
    m_device.freeMemory(m_emissiveImageMemory);

    m_device.destroyDescriptorSetLayout(m_gbufferDescriptorSetLayout);
    m_device.destroyDescriptorPool(m_gbufferDescriptorPool);

    m_device.destroyCommandPool(m_commandPool);

    for (int i = 0; i < getMaxFramesInFlight(); i++) {
        m_device.destroySemaphore(m_imageAvailableSemaphores[i]);
        m_device.destroySemaphore(m_renderFinishedSemaphores[i]);
        m_device.destroyFence(m_commandBufferReadyFences[i]);
    }

    for (auto& imageView : m_swapchainImageViews)
        m_device.destroyImageView(imageView);
    
    m_device.destroyRenderPass(m_renderPass);
    m_device.destroyRenderPass(m_shadowMappingRenderPass);

    for (auto& framebuffer : m_framebuffers)
        m_device.destroyFramebuffer(framebuffer);
    
    m_device.destroySwapchainKHR(m_swapchain);

    m_device.destroy();

    m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
}

}
