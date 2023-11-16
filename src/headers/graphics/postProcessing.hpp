#ifndef POSTPROCESSING_HPP
#define POSTPROCESSING_HPP

#include <libraries.hpp>
#include <engine.hpp>

namespace mge {

class HDRColourCorrection {
    Engine* r_engine;

    vk::RenderPass m_renderPass;
    vk::Framebuffer m_framebuffer;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline;
    vk::ShaderModule m_vertexStage, m_fragmentStage;

    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorSet m_descriptorSet;
    vk::Sampler m_sampler;

public:

    HDRColourCorrection() = default;
    HDRColourCorrection(Engine& engine) : r_engine(&engine) {}

    void setup() {
        setupRenderPass();
        setupFramebuffer();
        setupDescriptorSet();
        setupPipelineLayout();
        setupPipeline();
    }

    void cleanup() {
        r_engine->m_device.destroyRenderPass(m_renderPass);
        r_engine->m_device.destroyFramebuffer(m_framebuffer);
        r_engine->m_device.destroyPipelineLayout(m_pipelineLayout);
        r_engine->m_device.destroyPipeline(m_pipeline);
        r_engine->m_device.destroyShaderModule(m_vertexStage);
        r_engine->m_device.destroyShaderModule(m_fragmentStage);
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
        r_engine->m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);
        r_engine->m_device.destroySampler(m_sampler);
    }

    void setupRenderPass();
    void setupFramebuffer();
    void setupDescriptorSet();
    void setupPipelineLayout();
    void setupPipeline();

    void draw(vk::CommandBuffer cmd);
};

}

#endif