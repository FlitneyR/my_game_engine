#include <engine.hpp>

#include <set>
#include <sstream>
#include <fstream>
#include <iostream>

namespace mge {

const char* Engine::s_engineName = "My Game Engine";
const char* Engine::s_gameName = "My Game";
const uint32_t Engine::s_engineVersion = VK_MAKE_VERSION(0, 1, 0);
const uint32_t Engine::s_gameVersion = VK_MAKE_VERSION(0, 1, 0);

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
    m_window = glfwCreateWindow(initWidth, initHeight, s_gameName, nullptr, nullptr);

    createInstance();
    getSurface();
    getPhysicalDevice();
    createLogicalDevice();
    createSwapchain();
    createSynchronisers();
    createDepthBuffer();
    createRenderPass();
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

        auto millisSinceStart = std::chrono::duration_cast<std::chrono::milliseconds>(frameStart - m_startTime).count();
        auto millisSinceLastFrame = std::chrono::duration_cast<std::chrono::milliseconds>(frameStart - m_lastFrameTime).count();
        float time = static_cast<float>(millisSinceStart) / 1000.f;
        float deltaTime = static_cast<float>(millisSinceLastFrame) / 1000.f;
        
        glfwPollEvents();
        draw();
        update(time, deltaTime);

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
        .setApplicationVersion(s_gameVersion)
        .setEngineVersion(s_engineVersion)
        .setPApplicationName(s_gameName)
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

    auto createInfo = vk::DeviceCreateInfo {}
        .setPEnabledLayerNames(requiredLayers)
        .setPEnabledExtensionNames(requiredExtensions)
        .setQueueCreateInfos(queueCreateInfos)
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
        .setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
        .setMinImageCount(std::min(surfaceCaps.minImageCount + 1, surfaceCaps.maxImageCount))
        .setPresentMode(vk::PresentModeKHR::eFifo)
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

    m_device.destroySwapchainKHR(m_swapchain);
    for (auto& imageView : m_swapchainImageViews) m_device.destroyImageView(imageView);
    for (auto& framebuffer : m_framebuffers) m_device.destroyFramebuffer(framebuffer);

    createSwapchain();
    createDepthBuffer();
    createFramebuffers();
}

void Engine::createSynchronisers() {
    vk::SemaphoreCreateInfo createInfo;
    m_imageAvailableSemaphore = m_device.createSemaphore(createInfo);
    m_renderFinishedSemaphore = m_device.createSemaphore(createInfo);

    auto fenceCreateInfo = vk::FenceCreateInfo {}.setFlags(vk::FenceCreateFlagBits::eSignaled);
    m_commandBufferReadyFence = m_device.createFence(fenceCreateInfo);
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
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment)
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
    auto renderTarget = vk::AttachmentDescription {}
        .setFinalLayout(vk::ImageLayout::ePresentSrcKHR)
        .setFormat(m_swapchainFormat.format)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        ;

    auto depthTarget = vk::AttachmentDescription {}
        .setFinalLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        .setFormat(m_depthImageFormat)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setLoadOp(vk::AttachmentLoadOp::eClear)
        .setStoreOp(vk::AttachmentStoreOp::eDontCare)
        .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
        .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
        ;
    
    std::vector<vk::AttachmentDescription> attachments { renderTarget, depthTarget };
    
    auto renderAttachment = vk::AttachmentReference {}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        ;
    
    auto depthAttachment = vk::AttachmentReference {}
        .setAttachment(1)
        .setLayout(vk::ImageLayout::eDepthStencilAttachmentOptimal)
        ;

    std::vector<vk::SubpassDescription> subpass = {
        vk::SubpassDescription {}
            .setColorAttachments(renderAttachment)
            .setPDepthStencilAttachment(&depthAttachment)
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            ,
        };
    
    std::vector<vk::SubpassDependency> dependencies {
        vk::SubpassDependency {}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
                           | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput
                           | vk::PipelineStageFlagBits::eEarlyFragmentTests)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite
                            | vk::AccessFlagBits::eDepthStencilAttachmentWrite)
    };

    auto createInfo = vk::RenderPassCreateInfo {}
        .setAttachments(attachments)
        .setSubpasses(subpass)
        .setDependencies(dependencies)
        ;
    
    m_renderPass = m_device.createRenderPass(createInfo);
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
            m_swapchainImageViews[i], m_depthImageView
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
        .setCommandBufferCount(1)
        .setCommandPool(m_commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        ;

    m_commandBuffer = m_device.allocateCommandBuffers(allocInfo)[0];
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
    vk::resultCheck(m_device.waitForFences(m_commandBufferReadyFence, true, UINT64_MAX), "Failed to wait for command-buffer-ready fence");
    m_device.resetFences(m_commandBufferReadyFence);

    auto nextImageResult = m_device.acquireNextImageKHR(m_swapchain, UINT64_MAX, m_imageAvailableSemaphore);

    if (nextImageResult.result != vk::Result::eSuboptimalKHR)
        vk::resultCheck(nextImageResult.result, "Failed to acquire next image");
    
    auto imageIndex = nextImageResult.value;

    auto now = std::chrono::high_resolution_clock::now();

    m_commandBuffer.reset();

    vk::CommandBufferBeginInfo beginInfo;
    m_commandBuffer.begin(beginInfo);

    auto viewport = vk::Viewport {}
        .setX(0.f)
        .setY(0.f)
        .setWidth(static_cast<float>(m_swapchainExtent.width))
        .setHeight(static_cast<float>(m_swapchainExtent.height))
        .setMinDepth(0.f)
        .setMaxDepth(1.f)
        ;

    m_commandBuffer.setViewport(0, viewport);

    auto scissor = vk::Rect2D {}
        .setOffset({ 0, 0 })
        .setExtent(m_swapchainExtent)
        ;
    
    m_commandBuffer.setScissor(0, scissor);

    std::vector<vk::ClearValue> clearValues {
        vk::ClearValue {}
            .setColor({ 0.f, 0.f, 0.f, 0.f })
            ,
        vk::ClearValue {}
            .setDepthStencil({ 1.0f, 0 })
    };

    auto renderPassBeginInfo = vk::RenderPassBeginInfo {}
        .setClearValues(clearValues)
        .setFramebuffer(m_framebuffers[imageIndex])
        .setRenderArea({{ 0, 0 }, { m_swapchainExtent.width, m_swapchainExtent.height }})
        .setRenderPass(m_renderPass)
        ;

    m_commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);

    recordDrawCommands(m_commandBuffer);

    m_commandBuffer.endRenderPass();
    m_commandBuffer.end();

    vk::Queue graphicsQueue = m_device.getQueue(*m_queueFamilies.graphicsFamily, 0);
    
    vk::PipelineStageFlags waitDstStageMask = vk::PipelineStageFlagBits::eTopOfPipe;

    std::vector<vk::SubmitInfo> submits {
        vk::SubmitInfo {}
            .setCommandBuffers(m_commandBuffer)
            .setWaitSemaphores(m_imageAvailableSemaphore)
            .setWaitDstStageMask(waitDstStageMask)
            .setSignalSemaphores(m_renderFinishedSemaphore)
            ,
    };

    graphicsQueue.submit(submits, m_commandBufferReadyFence);

    auto presentInfo = vk::PresentInfoKHR {}
        .setImageIndices(imageIndex)
        .setSwapchains(m_swapchain)
        .setWaitSemaphores(m_renderFinishedSemaphore)
        ;

    vk::Queue presentQueue = m_device.getQueue(*m_queueFamilies.presentFamily, 0);
    auto presentResult = presentQueue.presentKHR(presentInfo);

    if (presentResult == vk::Result::eSuboptimalKHR || presentResult == vk::Result::eErrorOutOfDateKHR) {
        rebuildSwapchain();
    } else vk::resultCheck(presentResult, "Failed to present render result");
}

void Engine::cleanup() {
    m_device.destroyImage(m_depthImage);
    m_device.destroyImageView(m_depthImageView);
    m_device.freeMemory(m_depthImageMemory);

    m_device.destroyCommandPool(m_commandPool);

    m_device.destroySemaphore(m_imageAvailableSemaphore);
    m_device.destroySemaphore(m_renderFinishedSemaphore);
    m_device.destroyFence(m_commandBufferReadyFence);

    for (auto& imageView : m_swapchainImageViews)
        m_device.destroyImageView(imageView);
    
    m_device.destroyRenderPass(m_renderPass);

    for (auto& framebuffer : m_framebuffers)
        m_device.destroyFramebuffer(framebuffer);
    
    m_device.destroySwapchainKHR(m_swapchain);

    m_device.destroy();

    m_instance.destroySurfaceKHR(m_surface);
    m_instance.destroy();

    glfwDestroyWindow(m_window);
}

}
