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
    glm::mat4 m_previousModelTransform;

    ModelTransformMeshInstance() :
        m_modelTransform(glm::mat4(1.f)),
        m_previousModelTransform(glm::mat4(1.f))
    {}

    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        auto binding = vk::VertexInputAttributeDescription {}
            .setBinding(1)
            .setLocation(firstAvailableLocation)
            .setFormat(vk::Format::eR32G32B32A32Sfloat);

        return std::vector<vk::VertexInputAttributeDescription> {
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(ModelTransformMeshInstance, m_previousModelTransform))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
                ,
            binding
                .setLocation(firstAvailableLocation++)
                .setOffset(binding.offset + sizeof(glm::vec4))
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
    uint32_t m_width, m_height, m_mipMapLevels = 1;
    vk::Format m_format;
    vk::ImageUsageFlags m_specialUsage;

    enum Type {
        e_colour, e_depth
    } m_type = e_colour;

    Texture() = default;

    Texture(uint32_t width, uint32_t height, vk::Format format, vk::ImageUsageFlags specialUsage, Type type) :
        m_width(width), m_height(height), m_format(format), m_specialUsage(specialUsage), m_type(type)
    {}

    Texture(const char* filename, vk::Format format = vk::Format::eR8G8B8A8Srgb, bool generateMipMaps = true) :
        m_format(format)
    {
        int channels, width, height;
        uint8_t* data = stbi_load(filename, &width, &height, &channels, 4);

        if (generateMipMaps) m_mipMapLevels = static_cast<uint32_t>(
            glm::floor(glm::log2(static_cast<float>(
                glm::max(width, height))))) + 1;
        
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

    std::array<Texture, N> m_textures;
    std::array<vk::Image, N> m_images;
    std::array<vk::ImageView, N> m_imageViews;
    std::array<vk::Sampler, N> m_samplers;
    std::array<vk::DeviceMemory, N> m_bufferMemories;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSet m_descriptorSet;

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup(const std::array<Texture, N>& textures) {
        m_textures = textures;
        setupImages();
        allocateMemories();
        setupImageViews();
        setupSamplers();
        fillImages();
        createDescriptorSetLayout();
        setupDescriptorPool();
        setupDescriptorSet();
        // transitionImageLayout();
    }

    void setupImages();
    void allocateMemories();
    void setupImageViews();
    void setupSamplers();
    void fillImages();
    void generateMipMaps(uint32_t index);
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
void NTEXTURE_MATERIAL_INSTANCE::setupImages() {
    for (int i = 0; i < N; i++) {
        auto& texture = m_textures[i];

        m_images[i] = r_engine->m_device.createImage(vk::ImageCreateInfo {}
            .setArrayLayers(1)
            .setFormat(texture.m_format)
            .setImageType(vk::ImageType::e2D)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setMipLevels(texture.m_mipMapLevels)
            .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(//vk::ImageUsageFlagBits::eStorage
                      vk::ImageUsageFlagBits::eSampled
                    | vk::ImageUsageFlagBits::eTransferSrc
                    | vk::ImageUsageFlagBits::eTransferDst
                    | texture.m_specialUsage)
            .setExtent(vk::Extent3D {}
                .setWidth(texture.m_width)
                .setHeight(texture.m_height)
                .setDepth(1))
            );
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupImageViews() {
    for (int i = 0; i < N; i++) {
        vk::ImageAspectFlags aspectMask;
        switch (m_textures[i].m_type) {
        case Texture::e_colour: aspectMask = vk::ImageAspectFlagBits::eColor; break;
        case Texture::e_depth: aspectMask = vk::ImageAspectFlagBits::eDepth; break;
        }

        m_imageViews[i] = r_engine->m_device.createImageView(vk::ImageViewCreateInfo {}
            .setFormat(m_textures[i].m_format)
            .setImage(m_images[i])
            .setViewType(vk::ImageViewType::e2D)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(aspectMask)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setLayerCount(1)
                .setLevelCount(m_textures[i].m_mipMapLevels))
            );
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::setupSamplers() {
    auto properties = r_engine->m_physicalDevice.getProperties();

    for (int i = 0; i < N; i++) {
        m_samplers[i] = r_engine->m_device.createSampler(vk::SamplerCreateInfo {}
            .setAddressModeU(vk::SamplerAddressMode::eRepeat)
            .setAddressModeV(vk::SamplerAddressMode::eRepeat)
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setMinLod(0)
            .setMaxLod(m_textures[i].m_mipMapLevels)
            .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
            .setAnisotropyEnable(r_engine->m_physicalDevice.getFeatures().samplerAnisotropy)
            );
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::allocateMemories() {
    for (int i = 0; i < N; i++) {
        auto memReqs = r_engine->m_device.getImageMemoryRequirements(m_images[i]);
        
        vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal;

        auto allocInfo = vk::MemoryAllocateInfo {}
            .setAllocationSize(memReqs.size)
            .setMemoryTypeIndex(r_engine->findMemoryType(memReqs.memoryTypeBits, properties))
            ;

        m_bufferMemories[i] = r_engine->m_device.allocateMemory(allocInfo);
        r_engine->m_device.bindImageMemory(m_images[i], m_bufferMemories[i], 0);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::fillImages() {
    for (int i = 0; i < N; i++)
    if (m_textures[i].m_data.size() > 0) {
        uint32_t width = m_textures[i].m_width;
        uint32_t height = m_textures[i].m_height;
        uint32_t pixelSize = m_textures[i].m_data.size() / (width * height);

        r_engine->copyDataToImage(m_images[i], m_textures[i].m_data.data(), width, height, pixelSize);

        if (m_textures[i].m_mipMapLevels > 1) generateMipMaps(i);
    }
}

NTEXTURE_TEMPLATE
void NTEXTURE_MATERIAL_INSTANCE::generateMipMaps(uint32_t index) {
    vk::CommandBuffer cmd = r_engine->m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(r_engine->m_commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        )[0];
    
    cmd.begin(vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    auto imageBarrierToWrite = vk::ImageMemoryBarrier {}
        .setImage(m_images[index])
        .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setBaseMipLevel(0)
            .setLevelCount(1));

    auto imageBarrierToRead = vk::ImageMemoryBarrier {}
        .setImage(m_images[index])
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
        .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
        .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setBaseMipLevel(0)
            .setLevelCount(1));
    
    int32_t width = m_textures[index].m_width, height = m_textures[index].m_height;

    for (int level = 0; level < m_textures[index].m_mipMapLevels - 1; level++) {
        imageBarrierToRead.subresourceRange
                .setBaseMipLevel(level);
        
        imageBarrierToWrite.subresourceRange
                .setBaseMipLevel(level + 1);
        
        cmd.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer,
            vk::PipelineStageFlagBits::eTransfer,
            {}, {}, {},
            { imageBarrierToRead, imageBarrierToWrite }
            );

        cmd.blitImage(
            m_images[index], vk::ImageLayout::eTransferSrcOptimal,
            m_images[index], vk::ImageLayout::eTransferDstOptimal,
            vk::ImageBlit {}
                .setSrcOffsets({
                    vk::Offset3D { 0, 0, 0 },
                    vk::Offset3D { width, height, 1 },
                    })
                .setDstOffsets({
                    vk::Offset3D { 0, 0, 0 },
                    vk::Offset3D { std::max(width /= 2, 1), std::max(height /= 2, 1), 1 },
                    })
                .setSrcSubresource(vk::ImageSubresourceLayers {}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0).setLayerCount(1)
                    .setMipLevel(level))
                .setDstSubresource(vk::ImageSubresourceLayers {}
                    .setAspectMask(vk::ImageAspectFlagBits::eColor)
                    .setBaseArrayLayer(0).setLayerCount(1)
                    .setMipLevel(level + 1)),
            vk::Filter::eLinear
            );
    }


    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eAllGraphics,
        {}, {}, {},
        vk::ImageMemoryBarrier {}
            .setImage(m_images[index])
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setOldLayout(vk::ImageLayout::eTransferDstOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
                .setLevelCount(1)
                .setBaseMipLevel(m_textures[index].m_mipMapLevels - 1))
    );

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eAllGraphics,
        {}, {}, {},
        vk::ImageMemoryBarrier {}
            .setImage(m_images[index])
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setOldLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(vk::ImageAspectFlagBits::eColor)
                .setBaseArrayLayer(0)
                .setLayerCount(1)
                .setBaseMipLevel(0)
                .setLevelCount(m_textures[index].m_mipMapLevels - 1))
    );
    
    cmd.end();

    vk::Queue queue = r_engine->m_device.getQueue(*r_engine->m_queueFamilies.graphicsFamily, 0);
    queue.submit(vk::SubmitInfo {} .setCommandBuffers(cmd));

    r_engine->m_device.waitIdle();
    r_engine->m_device.freeCommandBuffers(r_engine->m_commandPool, cmd);
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

    for (int i = 0; i < N; i++)
    if (m_textures[i].m_data.size() > 0) {
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
            .setLevelCount(m_textures[i].m_mipMapLevels)
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