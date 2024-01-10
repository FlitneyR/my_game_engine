#include <bloom.hpp>

namespace mge {

void Bloom::makeImages() {
    int sideLength = std::max(r_engine->m_swapchainExtent.width, r_engine->m_swapchainExtent.height);
    m_mipLevels = std::ceil(std::log2(sideLength));

    auto createInfo = vk::ImageCreateInfo {}
        .setArrayLayers(1)
        .setExtent(vk::Extent3D { r_engine->m_swapchainExtent, 1 })
        .setFormat(r_engine->m_emissiveFormat)
        .setImageType(vk::ImageType::e2D)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(m_mipLevels)
        .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        .setUsage(vk::ImageUsageFlagBits::eColorAttachment
                | vk::ImageUsageFlagBits::eSampled)
        ;

    m_vBlurImage = r_engine->m_device.createImage(createInfo);
    m_hBlurImage = r_engine->m_device.createImage(createInfo);
}

void Bloom::allocateImages() {
    auto memreqs = r_engine->m_device.getImageMemoryRequirements(m_vBlurImage);

    auto allocInfo = vk::MemoryAllocateInfo {}
        .setAllocationSize(memreqs.size)
        .setMemoryTypeIndex(r_engine->findMemoryType(memreqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
        ;
    
    m_vBlurImageMemory = r_engine->m_device.allocateMemory(allocInfo);
    m_hBlurImageMemory = r_engine->m_device.allocateMemory(allocInfo);

    r_engine->m_device.bindImageMemory(m_vBlurImage, m_vBlurImageMemory, {});
    r_engine->m_device.bindImageMemory(m_hBlurImage, m_hBlurImageMemory, {});
}

void Bloom::makeImageViews() {
    auto createInfo = vk::ImageViewCreateInfo {}
        .setFormat(r_engine->m_emissiveFormat)
        .setViewType(vk::ImageViewType::e2D)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(m_mipLevels)
            )
        ;

    m_hBlurImageView = r_engine->m_device.createImageView(createInfo.setImage(m_hBlurImage));
    m_vBlurImageView = r_engine->m_device.createImageView(createInfo.setImage(m_vBlurImage));

    createInfo.subresourceRange.setLevelCount(1);

    for (int i = 0; i < m_mipLevels; i++) {
        createInfo.subresourceRange.setBaseMipLevel(i);

        m_hBlurImageViews.push_back(r_engine->m_device.createImageView(createInfo.setImage(m_hBlurImage)));
        m_vBlurImageViews.push_back(r_engine->m_device.createImageView(createInfo.setImage(m_vBlurImage)));
    }
}

void Bloom::makeRenderPass() {
    auto attachmentDescription = vk::AttachmentDescription {}
        .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setFormat(r_engine->m_emissiveFormat)
        .setLoadOp(vk::AttachmentLoadOp::eLoad)
        .setStoreOp(vk::AttachmentStoreOp::eStore)
        .setSamples(vk::SampleCountFlagBits::e1)
        ;

    auto attachmentReference = vk::AttachmentReference {}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        ;

    std::vector<vk::SubpassDescription> subpasses {
        vk::SubpassDescription {}
            .setColorAttachments(attachmentReference)
            ,
    };

    std::vector<vk::SubpassDependency> dependencies {
        vk::SubpassDependency {}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            ,
    };

    m_renderPass = r_engine->m_device.createRenderPass(vk::RenderPassCreateInfo {}
        .setAttachments(attachmentDescription)
        .setSubpasses(subpasses)
        .setDependencies(dependencies)
        );
}

void Bloom::makeFramebuffers() {
    uint32_t width = r_engine->m_swapchainExtent.width;
    uint32_t height = r_engine->m_swapchainExtent.height;

    m_emissiveFramebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
        .setWidth(width)
        .setHeight(height)
        .setLayers(1)
        .setRenderPass(m_renderPass)
        .setAttachments(r_engine->m_emissiveImageView)
        );

    for (int i = 0; i < m_mipLevels; i++) {
        auto createInfo = vk::FramebufferCreateInfo {}
            .setWidth(width)
            .setHeight(height)
            .setLayers(1)
            .setRenderPass(m_renderPass)
            ;

        m_hBlurFramebuffers.push_back(r_engine->m_device.createFramebuffer(createInfo
            .setAttachments(m_hBlurImageViews[i])
            ));
        
        m_vBlurFramebuffers.push_back(r_engine->m_device.createFramebuffer(createInfo
            .setAttachments(m_vBlurImageViews[i])
            ));

        m_extentSizes.push_back(vk::Extent2D { width, height });
        
        width = std::max(1U, width / 2);
        height = std::max(1U, height / 2);
    }
}

void Bloom::makeDescriptorSetLayout() {
    m_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(vk::DescriptorSetLayoutBinding {}
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            .setDescriptorCount(1)
            .setBinding(0)
            )
        );
}

void Bloom::makePipelineLayout() {
    auto pushConstantRange = vk::PushConstantRange {}
        .setOffset(0)
        .setSize(sizeof(PushConstants))
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        ;

    m_pipelineLayout = r_engine->m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setPushConstantRanges(pushConstantRange)
        .setSetLayouts(m_descriptorSetLayout)
        );
}

void Bloom::makePipeline() {
    m_vertexShader = r_engine->loadShaderModule("fullscreen.vert.spv");
    m_fragmentShader = r_engine->loadShaderModule("bloom.frag.spv");

    std::vector<vk::PipelineShaderStageCreateInfo> stages {
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_vertexShader)
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eVertex)
            ,
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_fragmentShader)
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    auto rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setLineWidth(1.0)
        ;

    auto multisampleState = vk::PipelineMultisampleStateCreateInfo {};
    auto vertexInputState = vk::PipelineVertexInputStateCreateInfo {};
    
    auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        ;

    auto viewport = vk::Viewport {}
        .setX(0)
        .setY(0)
        .setWidth(r_engine->m_swapchainExtent.width)
        .setHeight(r_engine->m_swapchainExtent.height)
        .setMinDepth(0)
        .setMaxDepth(1)
        ;
    
    auto scissor = vk::Rect2D {}
        .setExtent(r_engine->m_swapchainExtent)
        ;

    auto viewportState = vk::PipelineViewportStateCreateInfo {}
        .setViewports(viewport)
        .setScissors(scissor)
        ;

    auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setBlendEnable(true)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setColorWriteMask(vk::ColorComponentFlagBits::eR
                        | vk::ColorComponentFlagBits::eG
                        | vk::ColorComponentFlagBits::eB
                        | vk::ColorComponentFlagBits::eA)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eZero)
        .setDstColorBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eOne)
        ;

    auto colourBlendState = vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachment)
        ;

    auto createInfo = vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_pipelineLayout)
        .setPDynamicState(nullptr)
        .setPVertexInputState(&vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPTessellationState(nullptr)
        .setPDepthStencilState(nullptr)
        .setPRasterizationState(&rasterizationState)
        .setPMultisampleState(&multisampleState)
        .setPColorBlendState(&colourBlendState)
        .setPViewportState(&viewportState)
        .setRenderPass(m_renderPass)
        .setStages(stages)
        .setSubpass(0)
        ;

    auto pipelineResultValue = r_engine->m_device.createGraphicsPipeline({}, createInfo);
    vk::resultCheck(pipelineResultValue.result, "Failed to create bloom up sample pipeline");
    m_addPipeline = pipelineResultValue.value;

    attachment.setBlendEnable(false);
    colourBlendState.setAttachments(attachment);
    
    pipelineResultValue = r_engine->m_device.createGraphicsPipeline({}, createInfo);
    vk::resultCheck(pipelineResultValue.result, "Failed to create bloom down sample pipeline");
    m_replacePipeline = pipelineResultValue.value;
}

void Bloom::makeDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolSizes {
        vk::DescriptorPoolSize {}
            .setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(2 * m_mipLevels + 1)
            ,
    };

    m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setMaxSets(2 * m_mipLevels + 1)
        .setPoolSizes(poolSizes)
        );
}

void Bloom::makeDescriptorSets() {
    m_sampler = r_engine->m_device.createSampler(vk::SamplerCreateInfo {}
        .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
        .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
        .setBorderColor(vk::BorderColor::eFloatOpaqueBlack)
        .setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear)
        );

    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setSetLayouts(m_descriptorSetLayout)
        ;

    m_emissiveDescriptorSet = r_engine->m_device.allocateDescriptorSets(allocInfo)[0];

    std::vector<vk::DescriptorSetLayout> setLayouts(m_mipLevels, m_descriptorSetLayout);
    allocInfo.setSetLayouts(setLayouts);
    m_vBlurDescriptorSets = r_engine->m_device.allocateDescriptorSets(allocInfo);
    m_hBlurDescriptorSets = r_engine->m_device.allocateDescriptorSets(allocInfo);
    
    auto imageInfo = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSampler(m_sampler)
        ;

    auto writeDescriptorSet = vk::WriteDescriptorSet {}
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        .setDstBinding(0)
        .setDescriptorCount(1)
        ;

    r_engine->m_device.updateDescriptorSets(
        vk::WriteDescriptorSet { writeDescriptorSet }
            .setDstSet(m_emissiveDescriptorSet)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }.setImageView(r_engine->m_emissiveImageView))
        , {}
    );

    for (int i = 0; i < m_mipLevels; i++) {
        r_engine->m_device.updateDescriptorSets(
            vk::WriteDescriptorSet { writeDescriptorSet }
                .setDstSet(m_vBlurDescriptorSets[i])
                .setImageInfo(vk::DescriptorImageInfo { imageInfo }.setImageView(m_vBlurImageViews[i]))
            , {}
        );

        r_engine->m_device.updateDescriptorSets(
            vk::WriteDescriptorSet { writeDescriptorSet }
                .setDstSet(m_hBlurDescriptorSets[i])
                .setImageInfo(vk::DescriptorImageInfo { imageInfo }.setImageView(m_hBlurImageViews[i]))
            , {}
        );
    }
}

void Bloom::filterHighlights(vk::CommandBuffer cmd) {
    PushConstants pc;
    pc.m_stage = e_filterHighlights;
    pc.m_threshold = m_threshold;

    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setClearValues(vk::ClearValue {}.setColor(vk::ClearColorValue { 0, 0, 0, 0 }))
        .setFramebuffer(m_vBlurFramebuffers[minMipLevel()])
        .setRenderPass(m_renderPass)
        .setRenderArea(vk::Rect2D { { 0, 0 }, m_extentSizes[minMipLevel()] })
        , vk::SubpassContents::eInline);

    pc.m_viewportSize = glm::vec2 { m_extentSizes[minMipLevel()].width, m_extentSizes[minMipLevel()].height };
    cmd.pushConstants<PushConstants>(m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_emissiveDescriptorSet, {});
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_replacePipeline);
    cmd.draw(3, 1, 0, 0);
    
    cmd.endRenderPass();
}

void Bloom::blurDownMipChain(vk::CommandBuffer cmd) {
    PushConstants pc;
    pc.m_stage = e_blur;
    
    for (int i = minMipLevel(); i < maxMipLevel(); i++) {
        cmd.beginRenderPass(vk::RenderPassBeginInfo {}
            .setClearValues(vk::ClearValue {}.setColor(vk::ClearColorValue { 0, 0, 0, 0 }))
            .setFramebuffer(m_hBlurFramebuffers[i])
            .setRenderPass(m_renderPass)
            .setRenderArea(vk::Rect2D { { 0, 0 }, m_extentSizes[i] })
            , vk::SubpassContents::eInline);
    
        pc.m_direction = e_horizontal;
        pc.m_viewportSize = glm::vec2 { m_extentSizes[i].width, m_extentSizes[i].height };
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_vBlurDescriptorSets[std::max(0, i - 1)], {});
        cmd.pushConstants<PushConstants>(m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
        cmd.draw(3, 1, 0, 0);
    
        cmd.endRenderPass();

        cmd.beginRenderPass(vk::RenderPassBeginInfo {}
            .setClearValues(vk::ClearValue {}.setColor(vk::ClearColorValue { 0, 0, 0, 0 }))
            .setFramebuffer(m_vBlurFramebuffers[i])
            .setRenderPass(m_renderPass)
            .setRenderArea(vk::Rect2D { { 0, 0 }, m_extentSizes[i] })
            , vk::SubpassContents::eInline);

        pc.m_direction = e_vertical;
        pc.m_viewportSize = glm::vec2 { m_extentSizes[i].width, m_extentSizes[i].height };
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_hBlurDescriptorSets[i], {});
        cmd.pushConstants<PushConstants>(m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
        cmd.draw(3, 1, 0, 0);

        cmd.endRenderPass();
    }
}

void Bloom::combineMipChain(vk::CommandBuffer cmd) {
    PushConstants pc;
    pc.m_stage = e_combine;
    pc.m_combineFactor = m_combineFactor;

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_addPipeline);

    for (int i = maxMipLevel() - 2; i >= minMipLevel(); i--) {
        cmd.beginRenderPass(vk::RenderPassBeginInfo {}
            .setClearValues(vk::ClearValue {}.setColor(vk::ClearColorValue { 0, 0, 0, 0 }))
            .setFramebuffer(m_vBlurFramebuffers[i])
            .setRenderPass(m_renderPass)
            .setRenderArea(vk::Rect2D { { 0, 0 }, m_extentSizes[i] })
            , vk::SubpassContents::eInline);
        
        pc.m_viewportSize = glm::vec2 { m_extentSizes[i].width, m_extentSizes[i].height };
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_vBlurDescriptorSets[i + 1], {});
        cmd.pushConstants<PushConstants>(m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
        cmd.draw(3, 1, 0, 0);

        cmd.endRenderPass();
    }
}

void Bloom::overlayBloomOntoEmissive(vk::CommandBuffer cmd) {
    PushConstants pc;
    pc.m_stage = e_overlay;
    pc.m_overlayFactor = m_overlayFactor;
    
    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setClearValues(vk::ClearValue {}.setColor(vk::ClearColorValue { 0, 0, 0, 0 }))
        .setFramebuffer(m_emissiveFramebuffer)
        .setRenderPass(m_renderPass)
        .setRenderArea(vk::Rect2D { { 0, 0 }, m_extentSizes[0] })
        , vk::SubpassContents::eInline);
    
    pc.m_viewportSize = glm::vec2 { m_extentSizes[0].width, m_extentSizes[0].height };
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_vBlurDescriptorSets[minMipLevel()], {});
    cmd.pushConstants<PushConstants>(m_pipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, pc);
    cmd.draw(3, 1, 0, 0);

    cmd.endRenderPass();
}

void Bloom::draw(vk::CommandBuffer cmd) {
    auto imageBarrier = vk::ImageMemoryBarrier{}
        .setOldLayout(vk::ImageLayout::eUndefined)
        .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .setSubresourceRange(vk::ImageSubresourceRange {}
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setBaseArrayLayer(0)
            .setLevelCount(m_mipLevels)
            .setLayerCount(1)
        );

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eFragmentShader,
        {}, {}, {},
        {
            vk::ImageMemoryBarrier { imageBarrier }
                .setImage(m_vBlurImage)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                ,
            vk::ImageMemoryBarrier { imageBarrier }
                .setImage(m_hBlurImage)
                .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                ,
        }
        );

    filterHighlights(cmd);
    blurDownMipChain(cmd);
    combineMipChain(cmd);
    overlayBloomOntoEmissive(cmd);
}

}