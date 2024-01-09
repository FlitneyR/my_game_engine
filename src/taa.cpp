#include <taa.hpp>

namespace mge {

void TAA::setupImages() {
    auto createInfo = vk::ImageCreateInfo {}
        .setArrayLayers(1)
        .setExtent(vk::Extent3D { r_engine->m_swapchainExtent, 1 })
        .setFormat(r_engine->m_emissiveFormat)
        .setImageType(vk::ImageType::e2D)
        .setInitialLayout(vk::ImageLayout::eUndefined)
        .setMipLevels(1)
        .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        .setSamples(vk::SampleCountFlagBits::e1)
        .setUsage(vk::ImageUsageFlagBits::eTransferDst
                | vk::ImageUsageFlagBits::eSampled);

    m_previousFrame = r_engine->m_device.createImage(createInfo);

    m_taaTarget = r_engine->m_device.createImage(createInfo
        .setUsage(vk::ImageUsageFlagBits::eColorAttachment
                | vk::ImageUsageFlagBits::eTransferSrc
                | vk::ImageUsageFlagBits::eSampled)
        );
}

void TAA::allocateImages() {
    auto memreqs = r_engine->m_device.getImageMemoryRequirements(m_previousFrame);

    auto allocInfo = vk::MemoryAllocateInfo {}
        .setAllocationSize(memreqs.size)
        .setMemoryTypeIndex(r_engine->findMemoryType(
            memreqs.memoryTypeBits,
            vk::MemoryPropertyFlagBits::eDeviceLocal
        ));

    m_previousFrameMemory = r_engine->m_device.allocateMemory(allocInfo);
    m_taaTargetMemory = r_engine->m_device.allocateMemory(allocInfo);
    
    r_engine->m_device.bindImageMemory(m_previousFrame, m_previousFrameMemory, {});
    r_engine->m_device.bindImageMemory(m_taaTarget, m_taaTargetMemory, {});
}

void TAA::transitionImageLayouts() {
    vk::CommandBuffer cmd = r_engine->m_device.allocateCommandBuffers(vk::CommandBufferAllocateInfo {}
        .setCommandBufferCount(1)
        .setCommandPool(r_engine->m_commandPool)
        .setLevel(vk::CommandBufferLevel::ePrimary)
        )[0];
    
    cmd.begin(vk::CommandBufferBeginInfo {}
        .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit)
        );
    
    auto barrier = vk::ImageMemoryBarrier {}
        .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1));
    
    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {}, {}, {},
        {
            barrier.setImage(m_previousFrame),
        }
    );
    
    cmd.end();

    auto queue = r_engine->m_device.getQueue(*r_engine->m_queueFamilies.graphicsFamily, 0);

    queue.submit(vk::SubmitInfo {} .setCommandBuffers(cmd));

    r_engine->m_device.waitIdle();
    r_engine->m_device.freeCommandBuffers(r_engine->m_commandPool, cmd);
}

void TAA::setupImageViews() {
    auto createInfo = vk::ImageViewCreateInfo {}
        .setFormat(r_engine->m_emissiveFormat)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setBaseMipLevel(0)
            .setLayerCount(1)
            .setLevelCount(1))
        .setViewType(vk::ImageViewType::e2D);

    m_previousFrameView = r_engine->m_device.createImageView(createInfo.setImage(m_previousFrame));
    m_taaTargetView = r_engine->m_device.createImageView(createInfo.setImage(m_taaTarget));
}

void TAA::setupDescriptorSetLayout() {
    std::vector<vk::DescriptorSetLayoutBinding> aaBindings {
        vk::DescriptorSetLayoutBinding {} // previous frame
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
        vk::DescriptorSetLayoutBinding {} // current frame
            .setBinding(1)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
        vk::DescriptorSetLayoutBinding {} // velocity
            .setBinding(2)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
        vk::DescriptorSetLayoutBinding {} // depth
            .setBinding(3)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    m_taaDescriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(aaBindings)
        );
    
    std::vector<vk::DescriptorSetLayoutBinding> sharpenBindings {
        vk::DescriptorSetLayoutBinding {} // taa output
            .setBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    m_sharpenDescriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(sharpenBindings)
        );
}

void TAA::setupDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolsizes {
        vk::DescriptorPoolSize {}
            .setDescriptorCount(5)
            .setType(vk::DescriptorType::eCombinedImageSampler)
            ,
    };

    m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setMaxSets(2)
        .setPoolSizes(poolsizes)
        );
}

void TAA::setupSamplers() {
    auto createInfo = vk::SamplerCreateInfo {}
        .setAddressModeU(vk::SamplerAddressMode::eClampToEdge)
        .setAddressModeV(vk::SamplerAddressMode::eClampToEdge)
        .setMinFilter(vk::Filter::eLinear)
        .setMagFilter(vk::Filter::eLinear)
        ;

    m_previousFrameSampler = r_engine->m_device.createSampler(createInfo);
    m_currentFrameSampler = r_engine->m_device.createSampler(createInfo);
    m_velocitySampler = r_engine->m_device.createSampler(createInfo);
    m_depthSampler = r_engine->m_device.createSampler(createInfo);
    m_taaOutputSampler = r_engine->m_device.createSampler(createInfo);
}

void TAA::setupDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts {
        m_taaDescriptorSetLayout, m_sharpenDescriptorSetLayout
    };

    auto descriptorSets = r_engine->m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setSetLayouts(layouts)
        );
    
    m_aaDescriptorSet = descriptorSets[0];
    m_sharpenDescriptorSet = descriptorSets[1];

    auto imageInfo = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        ;

    auto descriptorWrite = vk::WriteDescriptorSet {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
        ;
    
    r_engine->m_device.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
        vk::WriteDescriptorSet { descriptorWrite }
            .setDstSet(m_aaDescriptorSet)
            .setDstBinding(0)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }
                .setImageView(m_previousFrameView)
                .setSampler(m_previousFrameSampler))
            ,
        vk::WriteDescriptorSet { descriptorWrite }
            .setDstSet(m_aaDescriptorSet)
            .setDstBinding(1)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }
                .setImageView(r_engine->m_emissiveImageView)
                .setSampler(m_currentFrameSampler))
            ,
        vk::WriteDescriptorSet { descriptorWrite }
            .setDstSet(m_aaDescriptorSet)
            .setDstBinding(2)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }
                .setImageView(r_engine->m_velocityImageView)
                .setSampler(m_velocitySampler))
            ,
        vk::WriteDescriptorSet { descriptorWrite }
            .setDstSet(m_aaDescriptorSet)
            .setDstBinding(3)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }
                .setImageView(r_engine->m_depthImageView)
                .setSampler(m_depthSampler))
            ,
        vk::WriteDescriptorSet { descriptorWrite }
            .setDstSet(m_sharpenDescriptorSet)
            .setDstBinding(0)
            .setImageInfo(vk::DescriptorImageInfo { imageInfo }
                .setImageView(m_taaTargetView)
                .setSampler(m_taaOutputSampler))
            ,
        }, {});
}

void TAA::setupRenderPasses() {
    auto outputAttachment = vk::AttachmentReference {}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        ;

    std::vector<vk::SubpassDescription> subpasses {
        vk::SubpassDescription {}
            .setColorAttachments(outputAttachment)
            ,
    };

    {   // taa render pass
        std::vector<vk::AttachmentDescription> attachmentDescriptions {
            vk::AttachmentDescription {}
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setFormat(r_engine->m_emissiveFormat)
                .setLoadOp(vk::AttachmentLoadOp::eDontCare)
                ,
        };

        std::vector<vk::SubpassDependency> dependencies {
            vk::SubpassDependency {}
                .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eTransfer)
                .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                ,
            vk::SubpassDependency {}
                .setSrcSubpass(VK_SUBPASS_EXTERNAL)
                .setDstSubpass(0)
                .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
                .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
                .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
                .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
                ,
        };

        m_taaRenderPass = r_engine->m_device.createRenderPass(vk::RenderPassCreateInfo {}
            .setAttachments(attachmentDescriptions)
            .setDependencies(dependencies)
            .setSubpasses(subpasses)
            );
    }

    {   // sharpening render pass
        std::vector<vk::AttachmentDescription> attachmentDescriptions {
            vk::AttachmentDescription {}
                .setInitialLayout(vk::ImageLayout::eUndefined)
                .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setFormat(r_engine->m_emissiveFormat)
                .setLoadOp(vk::AttachmentLoadOp::eDontCare)
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

        m_sharpenRenderPass = r_engine->m_device.createRenderPass(vk::RenderPassCreateInfo {}
            .setAttachments(attachmentDescriptions)
            .setDependencies(dependencies)
            .setSubpasses(subpasses)
            );
    }
}

void TAA::setupFramebuffers() {
    {   // taa framebuffer
        std::vector<vk::ImageView> attachments {
            m_taaTargetView,
        };

        m_taaFramebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
            .setRenderPass(m_taaRenderPass)
            .setAttachments(attachments)
            .setWidth(r_engine->m_swapchainExtent.width)
            .setHeight(r_engine->m_swapchainExtent.height)
            .setLayers(1)
            );
    }

    {   // sharpen framebuffer
        std::vector<vk::ImageView> attachments {
            r_engine->m_emissiveImageView,
        };

        m_sharpenFramebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
            .setRenderPass(m_sharpenRenderPass)
            .setAttachments(attachments)
            .setWidth(r_engine->m_swapchainExtent.width)
            .setHeight(r_engine->m_swapchainExtent.height)
            .setLayers(1)
            );
    }
}

void TAA::setupPipelineLayouts() {
    m_taaPipelineLayout = r_engine->m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_taaDescriptorSetLayout)
        );
    
    m_sharpenPipelineLayout = r_engine->m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_sharpenDescriptorSetLayout)
        );
}

void TAA::setupPipelines() {
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    auto dynamicState = vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStates(dynamicStates);

    auto vertexInputState = vk::PipelineVertexInputStateCreateInfo {};

    auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList);

    auto viewport = vk::Viewport {};
    auto scissor = vk::Rect2D {};

    auto viewportState = vk::PipelineViewportStateCreateInfo {}
        .setViewports(viewport)
        .setScissors(scissor);

    auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo {};

    auto rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setLineWidth(1.f);

    auto multisampleState = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(vk::SampleCountFlagBits::e1);

    auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask(vk::ColorComponentFlagBits::eR
                         | vk::ColorComponentFlagBits::eG
                         | vk::ColorComponentFlagBits::eB
                         | vk::ColorComponentFlagBits::eA)
        ;

    auto colourBlendState = vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachment);

    std::vector<vk::PipelineShaderStageCreateInfo> stages {
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_vertexShader = r_engine->loadShaderModule("fullscreen.vert.spv"))
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eVertex)
            ,
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_taaFragmentShader = r_engine->loadShaderModule("taa.frag.spv"))
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    {   // taa pipeline
        auto pipeline = r_engine->m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo {}
            .setLayout(m_taaPipelineLayout)
            .setPDynamicState(&dynamicState)
            .setPVertexInputState(&vertexInputState)
            .setPInputAssemblyState(&inputAssemblyState)
            .setPViewportState(&viewportState)
            .setPRasterizationState(&rasterizationState)
            .setPDepthStencilState(&depthStencilState)
            .setPMultisampleState(&multisampleState)
            .setPColorBlendState(&colourBlendState)
            .setRenderPass(m_taaRenderPass)
            .setStages(stages)
            );
        
        vk::resultCheck(pipeline.result, "Failed to create pipeline");
        m_aaPipeline = pipeline.value;
    }

    {   // sharpen pipeline
        stages[1].setModule(m_sharpenFragmentShader = r_engine->loadShaderModule("sharpen.frag.spv"));

        auto pipeline = r_engine->m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo {}
            .setLayout(m_sharpenPipelineLayout)
            .setPDynamicState(&dynamicState)
            .setPVertexInputState(&vertexInputState)
            .setPInputAssemblyState(&inputAssemblyState)
            .setPViewportState(&viewportState)
            .setPRasterizationState(&rasterizationState)
            .setPDepthStencilState(&depthStencilState)
            .setPMultisampleState(&multisampleState)
            .setPColorBlendState(&colourBlendState)
            .setRenderPass(m_sharpenRenderPass)
            .setStages(stages)
            );
        
        vk::resultCheck(pipeline.result, "Failed to create pipeline");
        m_sharpenPipeline = pipeline.value;
    }
}

void TAA::draw(vk::CommandBuffer cmd) {
    auto barrier = vk::ImageMemoryBarrier {}
        .setNewLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
        .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
        .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
        .setSubresourceRange(vk::ImageSubresourceRange {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseMipLevel(0)
            .setLevelCount(1)
            .setBaseArrayLayer(0)
            .setLayerCount(1))
        ;

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eTransfer,
        vk::PipelineStageFlagBits::eFragmentShader,
        {}, {}, {}, {
            vk::ImageMemoryBarrier { barrier }.setImage(m_previousFrame),
        }
    );

    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setFramebuffer(m_taaFramebuffer)
        .setRenderPass(m_taaRenderPass)
        .setRenderArea({ {}, r_engine->m_swapchainExtent })
        , vk::SubpassContents::eInline);
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_aaPipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_taaPipelineLayout, 0, m_aaDescriptorSet, {});
    cmd.draw(3, 1, 0, 0);
    
    cmd.endRenderPass();

    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setFramebuffer(m_sharpenFramebuffer)
        .setRenderPass(m_sharpenRenderPass)
        .setRenderArea({ {}, r_engine->m_swapchainExtent })
        , vk::SubpassContents::eInline);

    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_sharpenPipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_sharpenPipelineLayout, 0, m_sharpenDescriptorSet, {});
    cmd.draw(3, 1, 0, 0);
    
    cmd.endRenderPass();

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eTransfer,
        {}, {}, {}, {
            vk::ImageMemoryBarrier { barrier }
                .setImage(m_previousFrame)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                ,
            vk::ImageMemoryBarrier { barrier }
                .setImage(m_taaTarget)
                .setNewLayout(vk::ImageLayout::eTransferSrcOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                .setDstAccessMask(vk::AccessFlagBits::eTransferRead)
                ,
        }
    );

    auto subresource = vk::ImageSubresourceLayers {}
        .setAspectMask(vk::ImageAspectFlagBits::eColor)
        .setBaseArrayLayer(0)
        .setLayerCount(1)
        .setMipLevel(0);

    cmd.copyImage(
        m_taaTarget, vk::ImageLayout::eTransferSrcOptimal,
        m_previousFrame, vk::ImageLayout::eTransferDstOptimal,
        vk::ImageCopy {}
            .setExtent(vk::Extent3D { r_engine->m_swapchainExtent, 1 })
            .setSrcSubresource(subresource)
            .setDstSubresource(subresource)
    );
}

}