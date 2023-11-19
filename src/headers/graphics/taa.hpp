#ifndef TAA_HPP
#define TAA_HPP

#include <engine.hpp>

namespace mge {

class TAA {
    Engine* r_engine;

    vk::Image m_previousFrame, m_taaTarget;
    vk::ImageView m_previousFrameView, m_taaTargetView;
    vk::DeviceMemory m_previousFrameMemory, m_taaTargetMemory;

    vk::Framebuffer m_taaFramebuffer, m_sharpenFramebuffer;
    vk::RenderPass m_taaRenderPass, m_sharpenRenderPass;

    vk::Sampler m_previousFrameSampler, m_currentFrameSampler, m_velocitySampler, m_depthSampler, m_taaOutputSampler;
    vk::DescriptorSet m_aaDescriptorSet, m_sharpenDescriptorSet;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSetLayout m_taaDescriptorSetLayout, m_sharpenDescriptorSetLayout;

    vk::ShaderModule m_vertexShader, m_taaFragmentShader, m_sharpenFragmentShader;
    vk::PipelineLayout m_taaPipelineLayout, m_sharpenPipelineLayout;
    vk::Pipeline m_aaPipeline, m_sharpenPipeline;

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
        setupDescriptorSets();

        setupRenderPasses();
        setupFramebuffers();

        setupPipelineLayouts();
        setupPipelines();
    }

    void draw(vk::CommandBuffer cmd);

    void cleanup() {
        r_engine->m_device.destroyImage(m_previousFrame);
        r_engine->m_device.destroyImage(m_taaTarget);

        r_engine->m_device.destroyImageView(m_previousFrameView);
        r_engine->m_device.destroyImageView(m_taaTargetView);

        r_engine->m_device.freeMemory(m_previousFrameMemory);
        r_engine->m_device.freeMemory(m_taaTargetMemory);

        r_engine->m_device.destroyFramebuffer(m_taaFramebuffer);
        r_engine->m_device.destroyFramebuffer(m_sharpenFramebuffer);
        
        r_engine->m_device.destroyRenderPass(m_taaRenderPass);
        r_engine->m_device.destroyRenderPass(m_sharpenRenderPass);

        r_engine->m_device.destroySampler(m_previousFrameSampler);
        r_engine->m_device.destroySampler(m_currentFrameSampler);
        r_engine->m_device.destroySampler(m_velocitySampler);
        r_engine->m_device.destroySampler(m_depthSampler);
        r_engine->m_device.destroySampler(m_taaOutputSampler);

        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
        r_engine->m_device.destroyDescriptorSetLayout(m_taaDescriptorSetLayout);
        r_engine->m_device.destroyDescriptorSetLayout(m_sharpenDescriptorSetLayout);

        r_engine->m_device.destroyShaderModule(m_vertexShader);
        r_engine->m_device.destroyShaderModule(m_taaFragmentShader);
        r_engine->m_device.destroyShaderModule(m_sharpenFragmentShader);

        r_engine->m_device.destroyPipelineLayout(m_taaPipelineLayout);
        r_engine->m_device.destroyPipelineLayout(m_sharpenPipelineLayout);

        r_engine->m_device.destroyPipeline(m_aaPipeline);
        r_engine->m_device.destroyPipeline(m_sharpenPipeline);
    }

    void setupImages();
    void setupImageViews();
    void allocateImages();
    void transitionImageLayouts();

    void setupDescriptorSetLayout();
    void setupDescriptorPool();
    void setupSamplers();
    void setupDescriptorSets();

    void setupFramebuffers();
    void setupRenderPasses();

    void setupPipelineLayouts();
    void setupPipelines();
};

}

#endif