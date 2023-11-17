#ifndef TAA_HPP
#define TAA_HPP

#include <engine.hpp>

namespace mge {

class TAA {
    Engine* r_engine;

    vk::Image m_currentFrame, m_previousFrame;
    vk::ImageView m_currentFrameView, m_previousFrameView;
    vk::DeviceMemory m_currentFrameMemory, m_previousFrameMemory;

    vk::Framebuffer m_framebuffer;
    vk::RenderPass m_renderPass;

    vk::Sampler m_previousFrameSampler, m_currentFrameSampler, m_velocitySampler, m_depthSampler;
    vk::DescriptorSet m_descriptorSet;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSetLayout m_descriptorSetLayout;

    vk::ShaderModule m_vertexShader, m_fragmentShader;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline;

public:
    TAA() = default;
    TAA(Engine& engine) : r_engine(&engine) {}

    void setup() {
        setupImages();
        allocateImages();
        setupImageViews();
        transitionImageLayouts();

        setupDescriptorSetLayout();
        setupDescriptorPool();
        setupSamplers();
        setupDescriptorSet();

        setupRenderPass();
        setupFramebuffer();

        setupPipelineLayout();
        setupPipeline();
    }

    void draw(vk::CommandBuffer cmd);

    void cleanup() {
        r_engine->m_device.destroyImage(m_previousFrame);
        r_engine->m_device.destroyImage(m_currentFrame);

        r_engine->m_device.destroyImageView(m_previousFrameView);
        r_engine->m_device.destroyImageView(m_currentFrameView);

        r_engine->m_device.freeMemory(m_previousFrameMemory);
        r_engine->m_device.freeMemory(m_currentFrameMemory);

        r_engine->m_device.destroyFramebuffer(m_framebuffer);
        r_engine->m_device.destroyRenderPass(m_renderPass);

        r_engine->m_device.destroySampler(m_previousFrameSampler);
        r_engine->m_device.destroySampler(m_currentFrameSampler);
        r_engine->m_device.destroySampler(m_velocitySampler);
        r_engine->m_device.destroySampler(m_depthSampler);

        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
        r_engine->m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);

        r_engine->m_device.destroyShaderModule(m_vertexShader);
        r_engine->m_device.destroyShaderModule(m_fragmentShader);
        r_engine->m_device.destroyPipelineLayout(m_pipelineLayout);
        r_engine->m_device.destroyPipeline(m_pipeline);
    }

    void setupImages();
    void setupImageViews();
    void allocateImages();
    void transitionImageLayouts();

    void setupDescriptorSetLayout();
    void setupDescriptorPool();
    void setupSamplers();
    void setupDescriptorSet();

    void setupFramebuffer();
    void setupRenderPass();

    void setupPipelineLayout();
    void setupPipeline();
};

}

#endif