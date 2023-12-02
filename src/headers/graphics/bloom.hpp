#ifndef BLOOM_HPP
#define BLOOM_HPP

#include <engine.hpp>
#include <optional>

namespace mge {

class Bloom {
    Engine* r_engine;

    vk::Image m_vBlurImage, m_hBlurImage;
    vk::ImageView m_vBlurImageView, m_hBlurImageView;

    std::vector<vk::ImageView> m_vBlurImageViews, m_hBlurImageViews;
    std::vector<vk::Extent2D> m_extentSizes;

    vk::DeviceMemory m_vBlurImageMemory, m_hBlurImageMemory;

    int m_mipLevels;

    vk::RenderPass m_renderPass;
    vk::Framebuffer m_emissiveFramebuffer;
    std::vector<vk::Framebuffer> m_hBlurFramebuffers, m_vBlurFramebuffers;

    vk::ShaderModule m_vertexShader, m_fragmentShader;

    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_replacePipeline, m_addPipeline;

    vk::Sampler m_sampler;

    vk::DescriptorSet m_emissiveDescriptorSet;
    std::vector<vk::DescriptorSet> m_vBlurDescriptorSets, m_hBlurDescriptorSets;

    vk::DescriptorSetLayout m_descriptorSetLayout;
    vk::DescriptorPool m_descriptorPool;

    enum Stage : uint32_t {
        e_filterHighlights = 0,
        e_blur,
        e_combine,
        e_overlay,
    };

    enum Direction : uint32_t {
        e_horizontal = 0,
        e_vertical,
    };

    struct PushConstants {
        Stage m_stage;
        Direction m_direction;
        glm::vec2 m_viewportSize;
        float m_threshold;
        float m_combineFactor;
        float m_overlayFactor;
    };

public:
    Bloom() = default;
    Bloom(Engine& engine) : r_engine(&engine) {}

    std::optional<int> m_minMipLevel, m_maxMipLevel;

    float m_threshold = 0.5;
    float m_combineFactor = 0.75;
    float m_overlayFactor = 0.25;

    int minMipLevel() { return m_minMipLevel.has_value() ? m_minMipLevel.value() : 0; }
    int maxMipLevel() { return m_maxMipLevel.has_value() ? std::min(m_mipLevels, m_maxMipLevel.value()) : m_mipLevels; }

    void setup() {
        makeImages();
        allocateImages();
        makeImageViews();

        makeRenderPass();
        makeFramebuffers();

        makeDescriptorSetLayout();
        makePipelineLayout();
        makePipeline();

        makeDescriptorPool();
        makeDescriptorSets();
    }

    void draw(vk::CommandBuffer cmd);

    void cleanup() {
        r_engine->m_device.destroyImage(m_vBlurImage);
        r_engine->m_device.destroyImage(m_hBlurImage);

        r_engine->m_device.destroyShaderModule(m_vertexShader);
        r_engine->m_device.destroyShaderModule(m_fragmentShader);

        r_engine->m_device.destroyImageView(m_vBlurImageView);
        r_engine->m_device.destroyImageView(m_hBlurImageView);

        for (int i = 0; i < m_mipLevels; i++) {
            r_engine->m_device.destroyImageView(m_vBlurImageViews[i]);
            r_engine->m_device.destroyImageView(m_hBlurImageViews[i]);
        }
        m_vBlurImageViews.clear();
        m_hBlurImageViews.clear();

        r_engine->m_device.freeMemory(m_vBlurImageMemory);
        r_engine->m_device.freeMemory(m_hBlurImageMemory);

        r_engine->m_device.destroyRenderPass(m_renderPass);

        r_engine->m_device.destroyFramebuffer(m_emissiveFramebuffer);

        for (auto framebuffer : m_hBlurFramebuffers)
            r_engine->m_device.destroyFramebuffer(framebuffer);
        
        m_hBlurFramebuffers.clear();

        for (auto framebuffer : m_vBlurFramebuffers)
            r_engine->m_device.destroyFramebuffer(framebuffer);
        
        m_vBlurFramebuffers.clear();

        m_extentSizes.clear();

        r_engine->m_device.destroyPipelineLayout(m_pipelineLayout);
        r_engine->m_device.destroyPipeline(m_addPipeline);
        r_engine->m_device.destroyPipeline(m_replacePipeline);

        r_engine->m_device.destroyDescriptorSetLayout(m_descriptorSetLayout);
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
        r_engine->m_device.destroySampler(m_sampler);
    }

private:
    void makeImages();
    void allocateImages();
    void makeImageViews();

    void makeRenderPass();
    void makeFramebuffers();

    void makeDescriptorSetLayout();

    void makePipelineLayout();
    void makePipeline();

    void makeDescriptorPool();
    void makeDescriptorSets();

    void clearBloomImages(vk::CommandBuffer cmd);
    void filterHighlights(vk::CommandBuffer cmd);
    void blurDownMipChain(vk::CommandBuffer cmd);
    void combineMipChain(vk::CommandBuffer cmd);
    void overlayBloomOntoEmissive(vk::CommandBuffer cmd);
};

}

#endif 