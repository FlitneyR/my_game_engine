#include <postProcessing.hpp>

namespace mge {

void HDRColourCorrection::setupRenderPass() {
    std::vector<vk::AttachmentDescription> attachmentDescriptions {
        vk::AttachmentDescription {}
            .setInitialLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setFinalLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
            .setFormat(r_engine->m_emissiveFormat)
            .setLoadOp(vk::AttachmentLoadOp::eLoad)
            .setStoreOp(vk::AttachmentStoreOp::eStore)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
    };

    std::vector<vk::AttachmentReference> attachmentReferences {
        vk::AttachmentReference {}
            .setAttachment(0)
            .setLayout(vk::ImageLayout::eGeneral)
    };

    std::vector<vk::SubpassDependency> dependencies {
        // vk::SubpassDependency {}
        //     .set
    };

    std::vector<vk::SubpassDescription> subpasses {
        vk::SubpassDescription {}
            .setColorAttachments(attachmentReferences)
            .setInputAttachments(attachmentReferences)
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
    };

    m_renderPass = r_engine->m_device.createRenderPass(vk::RenderPassCreateInfo {}
        .setAttachments(attachmentDescriptions)
        .setDependencies(dependencies)
        .setSubpasses(subpasses)
        );
}

void HDRColourCorrection::setupFramebuffer() {
    m_framebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
        .setAttachments(r_engine->m_emissiveImageView)
        .setWidth(r_engine->m_swapchainExtent.width)
        .setHeight(r_engine->m_swapchainExtent.height)
        .setLayers(1)
        .setRenderPass(m_renderPass)
        );
}

void HDRColourCorrection::setupDescriptorSet() {
    auto binding = vk::DescriptorSetLayoutBinding {}
        .setBinding(0)
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setStageFlags(vk::ShaderStageFlagBits::eFragment)
        ;

    m_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
        .setBindings(binding)
        );

    auto poolSize = vk::DescriptorPoolSize {}
        .setDescriptorCount(1)
        .setType(vk::DescriptorType::eInputAttachment)
        ;

    m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
        .setMaxSets(1)
        .setPoolSizes(poolSize)
        );

    m_descriptorSet = r_engine->m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(1)
        .setSetLayouts(m_descriptorSetLayout)
        )[0];

    m_sampler = r_engine->m_device.createSampler(vk::SamplerCreateInfo {});
    
    auto imageInfo = vk::DescriptorImageInfo {}
        .setImageLayout(vk::ImageLayout::eGeneral)
        .setImageView(r_engine->m_emissiveImageView)
        .setSampler(m_sampler)
        ;

    r_engine->m_device.updateDescriptorSets(vk::WriteDescriptorSet {}
        .setDescriptorCount(1)
        .setDescriptorType(vk::DescriptorType::eInputAttachment)
        .setDstArrayElement(0)
        .setDstBinding(0)
        .setDstSet(m_descriptorSet)
        .setImageInfo(imageInfo),
        {});
}

void HDRColourCorrection::setupPipelineLayout() {
    m_pipelineLayout = r_engine->m_device.createPipelineLayout(vk::PipelineLayoutCreateInfo {}
        .setSetLayouts(m_descriptorSetLayout)
        );
}

void HDRColourCorrection::setupPipeline() {
    auto vertexInputState = vk::PipelineVertexInputStateCreateInfo {};
    
    auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask(vk::ColorComponentFlagBits::eR
                         | vk::ColorComponentFlagBits::eG
                         | vk::ColorComponentFlagBits::eB
                         | vk::ColorComponentFlagBits::eA)
        ;

    auto colorBlendState = vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachment);
    
    std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor,
    };
    
    auto dynamicState = vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStates(dynamicStates);
    
    auto inputAssemblyState = vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList);
    
    auto rasterizationState = vk::PipelineRasterizationStateCreateInfo {}
        .setLineWidth(1.f)
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eClockwise)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        ;

    auto multisampleState = vk::PipelineMultisampleStateCreateInfo {}
        .setRasterizationSamples(vk::SampleCountFlagBits::e1)
        ;

    m_vertexStage = r_engine->loadShaderModule("build/fullscreen.vert.spv");
    m_fragmentStage = r_engine->loadShaderModule("build/hdrColourCorrection.frag.spv");
    
    std::vector<vk::PipelineShaderStageCreateInfo> stages {
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_fragmentStage)
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eFragment)
            ,
        vk::PipelineShaderStageCreateInfo {}
            .setModule(m_vertexStage)
            .setPName("main")
            .setStage(vk::ShaderStageFlagBits::eVertex)
            ,
    };

    auto viewportState = vk::PipelineViewportStateCreateInfo {}
        .setViewports(vk::Viewport {}
            .setX(0.f)
            .setY(0.f)
            .setWidth(r_engine->m_swapchainExtent.width)
            .setHeight(r_engine->m_swapchainExtent.height)
            .setMinDepth(0.f)
            .setMaxDepth(1.f)
            )
        .setScissors(vk::Rect2D {}
            .setOffset({ 0, 0 })
            .setExtent(r_engine->m_swapchainExtent)
            )
        ;

    auto depthStencilState = vk::PipelineDepthStencilStateCreateInfo {};

    auto pipelineResultValue = r_engine->m_device.createGraphicsPipeline(nullptr, vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_pipelineLayout)
        .setPDynamicState(&dynamicState)
        .setPVertexInputState(&vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPViewportState(&viewportState)
        .setPRasterizationState(&rasterizationState)
        .setPDepthStencilState(&depthStencilState)
        .setPMultisampleState(&multisampleState)
        .setPColorBlendState(&colorBlendState)
        .setRenderPass(m_renderPass)
        .setStages(stages)
        );

    vk::resultCheck(pipelineResultValue.result, "Failed to create graphics pipeline");
    
    m_pipeline = pipelineResultValue.value;
}

void HDRColourCorrection::draw(vk::CommandBuffer cmd) {
    cmd.beginRenderPass(vk::RenderPassBeginInfo {}
        .setRenderPass(m_renderPass)
        .setRenderArea({ {}, r_engine->m_swapchainExtent })
        .setFramebuffer(m_framebuffer),
        vk::SubpassContents::eInline
        );
    
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 0, m_descriptorSet, {});
    cmd.draw(3, 1, 0, 0);

    cmd.endRenderPass();
}

}
