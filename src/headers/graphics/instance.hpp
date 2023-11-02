#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <libraries.hpp>
#include <engine.hpp>
#include <stb_image.h>

namespace mge {

class MeshInstanceBase {};

class MaterialInstanceBase {};

struct SimpleMeshInstance : public MeshInstanceBase {
    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        return {};
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return {};
    }
};

struct SimpleMaterialInstance : public MaterialInstanceBase {
    SimpleMaterialInstance(Engine& engine) {}

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup() {}
    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {}
    void cleanup() {}
};

struct ModelTransformMeshInstance : public MeshInstanceBase {
    glm::mat4 m_modelTransform;

    ModelTransformMeshInstance() :
        m_modelTransform(glm::mat4(1.f))
    {}

    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        return std::vector<vk::VertexInputAttributeDescription> {
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 0 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 1)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 1 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 2)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 2 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 3)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 3 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            };
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return std::vector<vk::VertexInputBindingDescription> {
            vk::VertexInputBindingDescription {}
                .setBinding(1)
                .setInputRate(vk::VertexInputRate::eInstance)
                .setStride(sizeof(ModelTransformMeshInstance))
                ,
            };
    }
};

struct Texture {
    std::vector<uint8_t> m_data;
    uint32_t m_width, m_height;

    Texture() = default;

    Texture(const char* filename) {
        int channels, width, height;
        uint8_t* data = stbi_load(filename, &width, &height, &channels, 4);
        
        if (!data) throw std::runtime_error("Failed to load image: " + std::string(filename));

        m_width = width;
        m_height = height;

        m_data.resize(m_width * m_height * 4);
        memcpy(m_data.data(), data, m_data.size());

        stbi_image_free(data);
    }
};

#define NTEXTURE_TEMPLATE template<size_t N>
#define NTEXTURE_MATERIAL_INSTANCE NTextureMaterialInstance<N>

NTEXTURE_TEMPLATE
class NTextureMaterialInstance : public MaterialInstanceBase {
public:
    Engine* r_engine;

    NTextureMaterialInstance(Engine& engine) :
        r_engine(&engine)
    {}

    std::array<vk::Image, N> m_images;
    std::array<vk::ImageView, N> m_imageViews;
    std::array<vk::Sampler, N> m_samplers;
    std::array<vk::DeviceMemory, N> m_bufferMemories;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSet m_descriptorSet;

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup(const std::array<Texture, N>& textures) {
        setupImages(textures);
        allocateMemories();
        setupImageViews();
        setupSamplers();
        fillImages(textures);
        createDescriptorSetLayout();
        setupDescriptorPool();
        setupDescriptorSet();
        transitionImageLayout();
    }

    void setupImages(const std::array<Texture, N>& textures);
    void allocateMemories();
    void setupImageViews();
    void setupSamplers();
    void fillImages(const std::array<Texture, N>& textures);
    void createDescriptorSetLayout();
    void setupDescriptorPool();
    void setupDescriptorSet();
    void transitionImageLayout();

    void cleanup() {
        if (s_descriptorSetLayout != VK_NULL_HANDLE) {
            r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);
            s_descriptorSetLayout = VK_NULL_HANDLE;
        }

        for (auto& imageView : m_imageViews)
            r_engine->m_device.destroyImageView(imageView);
        
        for (auto& image : m_images)
            r_engine->m_device.destroyImage(image);
        
        for (auto& sampler : m_samplers)
            r_engine->m_device.destroySampler(sampler);
        
        for (auto& bufferMemory : m_bufferMemories)
            r_engine->m_device.freeMemory(bufferMemory);
        
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
    }

    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, m_descriptorSet, nullptr);
    }
};

NTEXTURE_TEMPLATE
vk::DescriptorSetLayout NTEXTURE_MATERIAL_INSTANCE::s_descriptorSetLayout = VK_NULL_HANDLE;

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupImages(const std::array<Texture, N>& textures) {
    for (int i = 0; i < N; i++) {
        auto& texture = textures[i];

        auto createInfo = vk::ImageCreateInfo {}
            .setArrayLayers(1)
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setImageType(vk::ImageType::e2D)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setMipLevels(1)
            .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled)
            ;
        
        createInfo.extent
            .setWidth(texture.m_width)
            .setHeight(texture.m_height)
            .setDepth(1)
            ;

        m_images[i] = r_engine->m_device.createImage(createInfo);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupImageViews() {
    for (int i = 0; i < N; i++) {
        auto createInfo = vk::ImageViewCreateInfo {}
            .setFormat(vk::Format::eR8G8B8A8Srgb)
            .setImage(m_images[i])
            .setViewType(vk::ImageViewType::e2D)
            ;
        
        createInfo.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1)
            ;

        m_imageViews[i] = r_engine->m_device.createImageView(createInfo);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupSamplers() {
    for (int i = 0; i < N; i++) {
        auto properties = r_engine->m_physicalDevice.getProperties();

        auto createInfo = vk::SamplerCreateInfo {}
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
            ;

        m_samplers[i] = r_engine->m_device.createSampler(createInfo);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::allocateMemories() {
    for (int i = 0; i < N; i++) {
        auto memReqs = r_engine->m_device.getImageMemoryRequirements(m_images[i]);

        vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
                                        | vk::MemoryPropertyFlagBits::eHostVisible
                                        ;

        auto allocInfo = vk::MemoryAllocateInfo {}
            .setAllocationSize(memReqs.size)
            .setMemoryTypeIndex(r_engine->findMemoryType(memReqs.memoryTypeBits, properties))
            ;

        m_bufferMemories[i] = r_engine->m_device.allocateMemory(allocInfo);
        r_engine->m_device.bindImageMemory(m_images[i], m_bufferMemories[i], 0);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::fillImages(const std::array<Texture, N>& textures) {
    for (int i = 0; i < N; i++) {
        void* mappedMemory = r_engine->m_device.mapMemory(m_bufferMemories[i], 0, textures[i].m_data.size());
        memcpy(mappedMemory, textures[i].m_data.data(), textures[i].m_data.size());

        auto mappedMemoryRange = vk::MappedMemoryRange {}
            .setMemory(m_bufferMemories[i])
            .setOffset(0)
            .setSize(textures[i].m_data.size())
            ;

        r_engine->m_device.flushMappedMemoryRanges(mappedMemoryRange);
        r_engine->m_device.unmapMemory(m_bufferMemories[i]);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::createDescriptorSetLayout() {
    if (s_descriptorSetLayout != VK_NULL_HANDLE) return;

    std::array<vk::DescriptorSetLayoutBinding, N> bindings;

    for (int i = 0; i < N; i++)
        bindings[i] = vk::DescriptorSetLayoutBinding {}
            .setBinding(i)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
            ;

    auto createInfo = vk::DescriptorSetLayoutCreateInfo {}.setBindings(bindings);
    s_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(createInfo);
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupDescriptorPool() {
    auto poolSize = vk::DescriptorPoolSize {}
        .setDescriptorCount(N)
        .setType(vk::DescriptorType::eCombinedImageSampler)
        ;

    auto createInfo = vk::DescriptorPoolCreateInfo {}
        .setMaxSets(1)
        .setPoolSizes(poolSize)
        ;
    
    m_descriptorPool = r_engine->m_device.createDescriptorPool(createInfo);
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupDescriptorSet() {
    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(s_descriptorSetLayout)
        ;

    m_descriptorSet = r_engine->m_device.allocateDescriptorSets(allocInfo)[0];

    std::array<vk::DescriptorImageInfo, N> imageInfos;

    for (int i = 0; i < N; i++)
        imageInfos[i] = vk::DescriptorImageInfo {}
            .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setImageView(m_imageViews[i])
            .setSampler(m_samplers[i])
            ;

    auto descriptorSetWrite = vk::WriteDescriptorSet {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDstArrayElement(0)
        .setDstBinding(0)
        .setDstSet(m_descriptorSet)
        .setImageInfo(imageInfos)
        ;

    r_engine->m_device.updateDescriptorSets(descriptorSetWrite, nullptr);
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::transitionImageLayout() {
    auto cmdAllocInfo = vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(r_engine->m_commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        ;

    vk::CommandBuffer cmd = r_engine->m_device.allocateCommandBuffers(cmdAllocInfo)[0];

    auto beginInfo = vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        ;

    cmd.begin(beginInfo);

    for (int i = 0; i < N; i++) {
        auto imageBarrier = vk::ImageMemoryBarrier {}
            .setImage(m_images[i])
            .setOldLayout(vk::ImageLayout::eUndefined)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setSrcQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            .setDstQueueFamilyIndex(VK_QUEUE_FAMILY_IGNORED)
            ;
        
        imageBarrier.subresourceRange
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1)
            ;

        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTopOfPipe,
            vk::PipelineStageFlagBits::eFragmentShader,
            {}, {}, {},
            imageBarrier
            );
    }
    
    cmd.end();
    
    auto queue = r_engine->m_device.getQueue(*r_engine->m_queueFamilies.graphicsFamily, 0);

    auto submitInfo = vk::SubmitInfo {}
        .setCommandBuffers(cmd)
        ;

    queue.submit(submitInfo);
    r_engine->m_device.waitIdle();

    r_engine->m_device.freeCommandBuffers(r_engine->m_commandPool, cmd);
}

#undef NTEXTURE_TEMPLATE
#undef NTEXTURE_MATERIAL_INSTANCE

}

#endif