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
    m_currentFrame = r_engine->m_device.createImage(createInfo);
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
    m_currentFrameMemory = r_engine->m_device.allocateMemory(allocInfo);
    
    r_engine->m_device.bindImageMemory(m_previousFrame, m_previousFrameMemory, {});
    r_engine->m_device.bindImageMemory(m_currentFrame, m_currentFrameMemory, {});
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
        vk::DependencyFlagBits::eByRegion,
        {}, {},
        {
            barrier.setImage(m_previousFrame),
            barrier.setImage(m_currentFrame)
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
    m_currentFrameView = r_engine->m_device.createImageView(createInfo.setImage(m_currentFrame));
}

void TAA::setupDescriptorSetLayout() {
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
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    m_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(bindings)
        );
}

void TAA::setupDescriptorPool() {
    std::vector<vk::DescriptorPoolSize> poolsizes {
        vk::DescriptorPoolSize {}
            .setDescriptorCount(1)
            .setType(vk::DescriptorType::eCombinedImageSampler)
            ,
        vk::DescriptorPoolSize {}
            .setDescriptorCount(1)
            .setType(vk::DescriptorType::eCombinedImageSampler)
            ,
    };

    m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setMaxSets(1)
        .setPoolSizes(poolsizes)
        );
}

void TAA::setupSamplers() {
    auto createInfo = vk::SamplerCreateInfo {};

    m_previousFrameSampler = r_engine->m_device.createSampler(createInfo);
    m_thisFrameSampler = r_engine->m_device.createSampler(createInfo);
}

void TAA::setupDescriptorSet() {
    m_descriptorSet = r_engine->m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(m_descriptorSetLayout)
        )[0];
    
    r_engine->m_device.updateDescriptorSets(std::vector<vk::WriteDescriptorSet> {
        vk::WriteDescriptorSet {}
            .setDstBinding(0)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDstSet(m_descriptorSet)
            .setImageInfo(vk::DescriptorImageInfo {}
                .setImageView(m_previousFrameView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSampler(m_previousFrameSampler))
            ,
        vk::WriteDescriptorSet {}
            .setDstBinding(1)
            .setDescriptorCount(1)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDstSet(m_descriptorSet)
            .setImageInfo(vk::DescriptorImageInfo {}
                .setImageView(m_currentFrameView)
                .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                .setSampler(m_thisFrameSampler))
            ,
        }, {});
}

void TAA::setupRenderPass() {
    std::vector<vk::AttachmentDescription> attachmentDescriptions {
        vk::AttachmentDescription {}
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setFinalLayout(vk::ImageLayout::eTransferSrcOptimal)
            .setFormat(r_engine->m_emissiveFormat)
            .setLoadOp(vk::AttachmentLoadOp::eDontCare)
    };

    auto emissiveAttachment = vk::AttachmentReference {}
        .setAttachment(0)
        .setLayout(vk::ImageLayout::eColorAttachmentOptimal)
        ;

    std::vector<vk::SubpassDescription> subpasses {
        vk::SubpassDescription {}
            .setColorAttachments(emissiveAttachment)
    };

    std::vector<vk::SubpassDependency> dependencies {
        vk::SubpassDependency {}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eTransfer)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
            ,
        vk::SubpassDependency {}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstStageMask(vk::PipelineStageFlagBits::eFragmentShader)
            .setSrcAccessMask(vk::AccessFlagBits::eColorAttachmentWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setDependencyFlags(vk::DependencyFlagBits::eByRegion)
            ,
    };

    m_renderPass = r_engine->m_device.createRenderPass(vk::RenderPassCreateInfo {}
        .setAttachments(attachmentDescriptions)
        .setDependencies(dependencies)
        .setSubpasses(subpasses)
        );
}

void TAA::setupFramebuffer() {
    std::vector<vk::ImageView> attachments {
        r_engine->m_emissiveImageView,
    };

    m_framebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
        .setRenderPass(m_renderPass)
        .setAttachments(attachments)
        .setWidth(r_engine->m_swapchainExtent.width)
        .setHeight(r_engine->m_swapchainExtent.height)
        .setLayers(1)
        );
}

void TAA::setupPipelineLayout() {
    m_pipelineLayout = r_engine->m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_descriptorSetLayout)
        );
}

void TAA::setupPipeline() {
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
            .setModule(m_vertexShader = r_engine->loadShaderModule("build/fullscreen.vert.spv"))
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eVertex)
            ,
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_fragmentShader = r_engine->loadShaderModule("build/taa.frag.spv"))
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eFragment)
            ,
    };

    auto pipeline = r_engine->m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_pipelineLayout)
        .setPDynamicState(&dynamicState)
        .setPVertexInputState(&vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizationState)
        .setPDepthStencilState(&depthStencilState)
        .setPMultisampleState(&multisampleState)
        .setPColorBlendState(&colourBlendState)
        .setRenderPass(m_renderPass)
        .setStages(stages)
        );
    
    vk::resultCheck(pipeline.result, "Failed to create pipeline");
    m_pipeline = pipeline.value;
}

void TAA::draw(vk::CommandBuffer cmd) {
    auto blitInfo = vk::ImageBlit {}
        .setSrcOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D {
                static_cast<int32_t>(r_engine->m_swapchainExtent.width),
                static_cast<int32_t>(r_engine->m_swapchainExtent.height),
                1
            },
        })
        .setDstOffsets({
            vk::Offset3D { 0, 0, 0 },
            vk::Offset3D {
                static_cast<int32_t>(r_engine->m_swapchainExtent.width),
                static_cast<int32_t>(r_engine->m_swapchainExtent.height),
                1
            },
        })
        .setSrcSubresource(vk::ImageSubresourceLayers {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(0))
        .setDstSubresource(vk::ImageSubresourceLayers {}
            .setAspectMask(vk::ImageAspectFlagBits::eColor)
            .setBaseArrayLayer(0)
            .setLayerCount(1)
            .setMipLevel(0))
        ;
    
    cmd.blitImage(
        r_engine->m_emissiveImage, vk::ImageLayout::eTransferSrcOptimal,
        m_currentFrame, vk::ImageLayout::eTransferDstOptimal,
        blitInfo, vk::Filter::eLinear
    );

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
        vk::DependencyFlagBits::eByRegion,
        {}, {}, {
            barrier.setImage(m_currentFrame),
            barrier.setImage(m_previousFrame),
        }
    );

    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setFramebuffer(m_framebuffer)
        .setRenderPass(m_renderPass)
        .setRenderArea({ {}, r_engine->m_swapchainExtent })
        , vk::SubpassContents::eInline);
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_descriptorSet, {});
    cmd.draw(3, 1, 0, 0);
    
    cmd.endRenderPass();

    cmd.pipelineBarrier(
        vk::PipelineStageFlagBits::eFragmentShader,
        vk::PipelineStageFlagBits::eTransfer,
        vk::DependencyFlagBits::eByRegion,
        {}, {}, {
            barrier.setImage(m_previousFrame)
                .setNewLayout(vk::ImageLayout::eTransferDstOptimal)
                .setSrcAccessMask(vk::AccessFlagBits::eShaderRead)
                .setDstAccessMask(vk::AccessFlagBits::eTransferWrite)
                ,
            barrier.setImage(m_currentFrame)
        }
    );

    cmd.blitImage(
        r_engine->m_emissiveImage, vk::ImageLayout::eTransferSrcOptimal,
        m_previousFrame, vk::ImageLayout::eTransferDstOptimal,
        blitInfo,
        vk::Filter::eLinear
    );
}

}