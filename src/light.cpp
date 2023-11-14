#include <light.hpp>

namespace mge {

void LightInstance::updateShadowMapView(Camera& shadowMapView) {
    shadowMapView.m_position = m_position;
    shadowMapView.m_forward = m_direction;
    shadowMapView.m_near = m_near;
    shadowMapView.m_far = m_far;

    switch (m_type) {
    case e_directional:
        shadowMapView.m_projectionType = shadowMapView.e_orthographic;
        shadowMapView.m_shadowMapRange = glm::vec2 { m_shadowRange, m_shadowRange };
        break;
    case e_spot:
        shadowMapView.m_projectionType = shadowMapView.e_perspective;
        shadowMapView.m_fov = m_angle * 2.f;
        shadowMapView.m_shadowMapRange = std::nullopt;
        break;
    default: throw std::runtime_error("Cannot get shadow map view for this type of light");
    }

    shadowMapView.updateBuffer();
}

vk::PipelineRasterizationStateCreateInfo LightMaterial::getRasterizationState() {
    return vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eFront)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setLineWidth(1.f)
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        ;
}

vk::PipelineColorBlendStateCreateInfo LightMaterial::getColorBlendState() {
    static auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask( vk::ColorComponentFlagBits::eR
                        |   vk::ColorComponentFlagBits::eG
                        |   vk::ColorComponentFlagBits::eB
                        |   vk::ColorComponentFlagBits::eA)
        .setBlendEnable(true)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOne)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        ;

    return vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachment)
        ;
}

void LightMaterial::bindPipeline(vk::CommandBuffer cmd) {
    Material<PointVertex, LightInstance, SimpleMaterialInstance, Camera>::bindPipeline(cmd);
    std::vector<uint32_t> offsets;
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1, r_engine.m_gbufferDescriptorSet, offsets);
}

void LightMaterial::createDescriptorSetLayouts() {
    Material<PointVertex, LightInstance, SimpleMaterialInstance, Camera>::createDescriptorSetLayouts();

    m_descriptorSetLayouts.push_back(r_engine.m_gbufferDescriptorSetLayout);
}

void LightMaterial::modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) {
    createInfo.setSubpass(1);
}

void ShadowMappedLightMaterialInstance::setup(uint32_t width, uint32_t height) {
    m_shadowMapView.setup();

    m_shadowMapTexture = Texture(width, height,
        r_engine->m_depthImageFormat,
        vk::ImageUsageFlagBits::eDepthStencilAttachment,
        Texture::e_depth
        );

    setupImage();
    allocateImageMemory();
    setupImageView();

    setupFramebuffer();

    createDescriptorSetLayout();
    setupDescriptorPool();
    
    setupSampler();

    setupDescriptorSet();
}

void ShadowMappedLightMaterialInstance::bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {
    cmd.bindDescriptorSets(
        vk::PipelineBindPoint::eGraphics,
        pipelineLayout, 1,
        m_descriptorSets[r_engine->m_currentInFlightFrame],
        nullptr
    );
}

void ShadowMappedLightMaterialInstance::cleanup() {
    m_shadowMapView.cleanup();

    r_engine->m_device.destroyFramebuffer(m_framebuffer);
    r_engine->m_device.destroyImage(m_image);
    r_engine->m_device.destroyImageView(m_imageView);
    r_engine->m_device.destroySampler(m_sampler);
    r_engine->m_device.freeMemory(m_imageMemory);
    // r_engine->m_device.destroyBuffer(m_lightViewBuffer);
    // r_engine->m_device.freeMemory(m_lightViewBufferMemory);
    r_engine->m_device.destroyDescriptorPool(m_descriptorPool);

    if (s_descriptorSetLayout != VK_NULL_HANDLE) {
        r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);
        s_descriptorSetLayout = VK_NULL_HANDLE;
    }
}

void ShadowMappedLightMaterialInstance::updateViewBuffer(LightInstance& lightInstance) {
    lightInstance.updateShadowMapView(m_shadowMapView);
}

void ShadowMappedLightMaterialInstance::beginShadowMapRenderPass(vk::CommandBuffer cmd, LightInstance &lightInstance) {
    updateViewBuffer(lightInstance);

    auto clearValue = vk::ClearValue {}
        .setDepthStencil({ 1.0f, 0 })
        ;
    
    auto viewport = vk::Viewport {}
        .setX(0.f)
        .setY(0.f)
        .setWidth(m_shadowMapTexture.m_width)
        .setHeight(m_shadowMapTexture.m_height)
        .setMinDepth(0.f)
        .setMaxDepth(1.f)
        ;

    auto scissor = vk::Rect2D {}
        .setOffset({ 0, 0 })
        .setExtent({ m_shadowMapTexture.m_width, m_shadowMapTexture.m_height })
        ;

    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setClearValues(clearValue)
        .setFramebuffer(m_framebuffer)
        .setRenderPass(r_engine->m_shadowMappingRenderPass)
        .setRenderArea(scissor),
        vk::SubpassContents::eInline
        );
    
    cmd.setViewport(0, viewport);
    cmd.setScissor(0, scissor);
}

void ShadowMappedLightMaterialInstance::setupImage() {
    m_image = r_engine->m_device.createImage(vk::ImageCreateInfo {}
        .setArrayLayers(1)
        .setExtent({ m_shadowMapTexture.m_width, m_shadowMapTexture.m_height, 1 })
        .setFormat(m_shadowMapTexture.m_format)
        .setImageType(vk::ImageType::e2D)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(1)
        .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setTiling(vk::ImageTiling::eOptimal)
        .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment
                | vk::ImageUsageFlagBits::eSampled)
        );
}

void ShadowMappedLightMaterialInstance::allocateImageMemory() {
    auto memreqs = r_engine->m_device.getImageMemoryRequirements(m_image);

    m_imageMemory = r_engine->m_device.allocateMemory(vk::MemoryAllocateInfo {}
        .setAllocationSize(memreqs.size)
        .setMemoryTypeIndex(r_engine->findMemoryType(memreqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
        );
    
    r_engine->m_device.bindImageMemory(m_image, m_imageMemory, {});
}

void ShadowMappedLightMaterialInstance::setupImageView() {
    m_imageView = r_engine->m_device.createImageView(vk::ImageViewCreateInfo {}
        .setFormat(m_shadowMapTexture.m_format)
        .setImage(m_image)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eDepth)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1)
            )
        .setViewType(vk::ImageViewType::e2D)
        );
}

void ShadowMappedLightMaterialInstance::setupFramebuffer() {
    m_framebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
        .setRenderPass(r_engine->m_shadowMappingRenderPass)
        .setAttachments(m_imageView)
        .setWidth(m_shadowMapTexture.m_width)
        .setHeight(m_shadowMapTexture.m_height)
        .setLayers(1)
        );
}

void ShadowMappedLightMaterialInstance::createDescriptorSetLayout() {
    if (s_descriptorSetLayout != VK_NULL_HANDLE) return;

    std::vector<vk::DescriptorSetLayoutBinding> bindings {
        vk::DescriptorSetLayoutBinding {}
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
        vk::DescriptorSetLayoutBinding {}
            .setBinding(1)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        };

    s_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(bindings));
}

void ShadowMappedLightMaterialInstance::setupDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolSizes {
        vk::DescriptorPoolSize {}
            .setDescriptorCount(r_engine->getMaxFramesInFlight())
            .setType(vk::DescriptorType::eCombinedImageSampler)
            ,
        vk::DescriptorPoolSize {}
            .setDescriptorCount(r_engine->getMaxFramesInFlight())
            .setType(vk::DescriptorType::eUniformBuffer)
    };

    m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setMaxSets(r_engine->getMaxFramesInFlight())
        .setPoolSizes(poolSizes)
        );
}

void ShadowMappedLightMaterialInstance::setupSampler() {
    m_sampler = r_engine->m_device.createSampler(vk::SamplerCreateInfo {}
        .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
        .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
        .setMagFilter(vk::Filter::eLinear)
        .setMinFilter(vk::Filter::eLinear)
        .setMipmapMode(vk::SamplerMipmapMode::eLinear)
        .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
        );
}

void ShadowMappedLightMaterialInstance::setupDescriptorSet() {
    std::vector<vk::DescriptorSetLayout> layouts(r_engine->getMaxFramesInFlight(), s_descriptorSetLayout);

    m_descriptorSets = r_engine->m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(r_engine->getMaxFramesInFlight())
        .setSetLayouts(layouts)
        );
    
    for (int i = 0; i < m_descriptorSets.size(); i++) {
        r_engine->m_device.updateDescriptorSets(vk::WriteDescriptorSet {}
            .setDstSet(m_descriptorSets[i])
            .setDstBinding(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setImageInfo(vk::DescriptorImageInfo {}
                .setImageView(m_imageView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSampler(m_sampler)
                ),
            {});
        
        auto memreqs = r_engine->m_device.getBufferMemoryRequirements(m_shadowMapView.m_buffers[i]);
        
        r_engine->m_device.updateDescriptorSets(vk::WriteDescriptorSet {}
            .setDstSet(m_descriptorSets[i])
            .setDstBinding(1)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setBufferInfo(vk::DescriptorBufferInfo {}
                .setBuffer(m_shadowMapView.m_buffers[i])
                .setOffset({})
                .setRange(memreqs.size)),
            {});
    }
}

vk::DescriptorSetLayout ShadowMappedLightMaterialInstance::s_descriptorSetLayout = VK_NULL_HANDLE;

void ShadowMappedLightMaterial::bindPipeline(vk::CommandBuffer cmd) {
    ShadowMappedLight::Material::bindPipeline(cmd);
    std::vector<uint32_t> offsets;
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 2, r_engine.m_gbufferDescriptorSet, offsets);
}

vk::PipelineRasterizationStateCreateInfo ShadowMappedLightMaterial::getRasterizationState() {
    return vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eFront)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setLineWidth(1.f)
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        ;
}

vk::PipelineColorBlendStateCreateInfo ShadowMappedLightMaterial::getColorBlendState() {
    static auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask( vk::ColorComponentFlagBits::eR
                        |   vk::ColorComponentFlagBits::eG
                        |   vk::ColorComponentFlagBits::eB
                        |   vk::ColorComponentFlagBits::eA)
        .setBlendEnable(true)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOne)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        ;

    return vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachment)
        ;
}

void ShadowMappedLightMaterial::createDescriptorSetLayouts() {
    ShadowMappedLight::Material::createDescriptorSetLayouts();

    m_descriptorSetLayouts.push_back(r_engine.m_gbufferDescriptorSetLayout);
}

void ShadowMappedLightMaterial::modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) {
    createInfo.setSubpass(1);
}

}