#define STB_IMAGE_IMPLEMENTATION
#include <instance.hpp>

namespace mge {

vk::DescriptorSetLayout SimpleMaterialInstance::s_descriptorSetLayout = VK_NULL_HANDLE;
vk::DescriptorSetLayout SingleTextureMaterialInstance::s_descriptorSetLayout = VK_NULL_HANDLE;

void SingleTextureMaterialInstance::setupImage(const Texture& texture) {
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

    m_image = r_engine->m_device.createImage(createInfo);
}

void SingleTextureMaterialInstance::setupImageView() {
    auto createInfo = vk::ImageViewCreateInfo {}
        .setFormat(vk::Format::eR8G8B8A8Srgb)
        .setImage(m_image)
        .setViewType(vk::ImageViewType::e2D)
        ;
    
    createInfo.subresourceRange
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setBaseMipLevel(0)
        .setLayerCount(1)
        .setLevelCount(1)
        ;

    m_imageView = r_engine->m_device.createImageView(createInfo);
}

void SingleTextureMaterialInstance::setupSampler() {
    auto properties = r_engine->m_physicalDevice.getProperties();

    auto createInfo = vk::SamplerCreateInfo {}
        .setAddressModeU(vk::SamplerAddressMode::eRepeat)
        .setAddressModeV(vk::SamplerAddressMode::eRepeat)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setMaxAnisotropy(properties.limits.maxSamplerAnisotropy)
        ;

    m_sampler = r_engine->m_device.createSampler(createInfo);
}

void SingleTextureMaterialInstance::allocateMemory() {
    auto memReqs = r_engine->m_device.getImageMemoryRequirements(m_image);

    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal
                                       | vk::MemoryPropertyFlagBits::eHostVisible
                                       ;

    auto allocInfo = vk::MemoryAllocateInfo {}
        .setAllocationSize(memReqs.size)
        .setMemoryTypeIndex(r_engine->findMemoryType(memReqs.memoryTypeBits, properties))
        ;

    m_bufferMemory = r_engine->m_device.allocateMemory(allocInfo);
    r_engine->m_device.bindImageMemory(m_image, m_bufferMemory, 0);
}

void SingleTextureMaterialInstance::fillImage(const Texture& texture) {
    void* mappedMemory = r_engine->m_device.mapMemory(m_bufferMemory, 0, texture.m_data.size());
    memcpy(mappedMemory, texture.m_data.data(), texture.m_data.size());

    auto mappedMemoryRange = vk::MappedMemoryRange {}
        .setMemory(m_bufferMemory)
        .setOffset(0)
        .setSize(texture.m_data.size())
        ;

    r_engine->m_device.flushMappedMemoryRanges(mappedMemoryRange);
    r_engine->m_device.unmapMemory(m_bufferMemory);
}

void SingleTextureMaterialInstance::createDescriptorSetLayout() {
    if (s_descriptorSetLayout != VK_NULL_HANDLE) return;

    auto binding = vk::DescriptorSetLayoutBinding {}
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
        ;

    auto createInfo = vk::DescriptorSetLayoutCreateInfo {}.setBindings(binding);
    s_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(createInfo);
}

void SingleTextureMaterialInstance::setupDescriptorPool() {
    auto poolSize = vk::DescriptorPoolSize {}
        .setDescriptorCount(1)
        .setType(vk::DescriptorType::eCombinedImageSampler)
        ;

    auto createInfo = vk::DescriptorPoolCreateInfo {}
        .setMaxSets(1)
        .setPoolSizes(poolSize)
        ;
    
    m_descriptorPool = r_engine->m_device.createDescriptorPool(createInfo);
}

void SingleTextureMaterialInstance::setupDescriptorSet() {
    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(s_descriptorSetLayout)
        ;

    m_descriptorSet = r_engine->m_device.allocateDescriptorSets(allocInfo)[0];

    auto imageInfo = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setImageView(m_imageView)
        .setSampler(m_sampler)
        ;

    auto descriptorSetWrite = vk::WriteDescriptorSet {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDstArrayElement(0)
        .setDstBinding(0)
        .setDstSet(m_descriptorSet)
        .setImageInfo(imageInfo)
        ;

    r_engine->m_device.updateDescriptorSets(descriptorSetWrite, nullptr);
}

void SingleTextureMaterialInstance::transitionImageLayout() {
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

    auto imageBarrier = vk::ImageMemoryBarrier {}
        .setImage(m_image)
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
    
    cmd.end();
    
    auto queue = r_engine->m_device.getQueue(*r_engine->m_queueFamilies.graphicsFamily, 0);

    auto submitInfo = vk::SubmitInfo {}
        .setCommandBuffers(cmd)
        ;

    queue.submit(submitInfo);
    r_engine->m_device.waitIdle();

    r_engine->m_device.freeCommandBuffers(r_engine->m_commandPool, cmd);
}

}