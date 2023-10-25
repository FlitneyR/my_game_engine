#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <libraries.hpp>
#include <engine.hpp>
#include <stb_image.h>

namespace mge {

struct SimpleMeshInstance {
    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        return {};
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return {};
    }
};

struct SimpleMaterialInstance {
    SimpleMaterialInstance(Engine& engine) {}

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup() {}
    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {}
    void cleanup() {}
};

struct ModelTransformMeshInstance {
    glm::mat4 m_modelTransform;

    ModelTransformMeshInstance() :
        m_modelTransform(glm::mat4(1.f))
    {}

    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        return std::vector<vk::VertexInputAttributeDescription> {
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 0 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 1)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 1 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 2)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 2 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setLocation(firstAvailableLocation + 3)
                .setOffset(offsetof(ModelTransformMeshInstance, m_modelTransform) + 3 * sizeof(glm::vec4))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            };
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return std::vector<vk::VertexInputBindingDescription> {
            vk::VertexInputBindingDescription {}
                .setBinding(1)
                .setInputRate(vk::VertexInputRate::eInstance)
                .setStride(sizeof(ModelTransformMeshInstance))
                ,
            };
    }
};

struct Texture {
    std::vector<uint8_t> m_data;
    uint32_t m_width, m_height;

    Texture() = default;

    Texture(const char* filename) {
        int channels, width, height;
        uint8_t* data = stbi_load(filename, &width, &height, &channels, 4);
        
        if (!data) throw std::runtime_error("Failed to load image: " + std::string(filename));

        m_width = width;
        m_height = height;

        m_data.resize(m_width * m_height * 4);
        memcpy(m_data.data(), data, m_data.size());

        stbi_image_free(data);
    }
};

class SingleTextureMaterialInstance {
public:
    Engine* r_engine;

    SingleTextureMaterialInstance(Engine& engine) :
        r_engine(&engine)
    {}

    vk::Image m_image;
    vk::ImageView m_imageView;
    vk::Sampler m_sampler;
    vk::DeviceMemory m_bufferMemory;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSet m_descriptorSet;

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup(const Texture& texture) {
        setupImage(texture);
        allocateMemory();
        setupImageView();
        setupSampler();
        fillImage(texture);
        createDescriptorSetLayout();
        setupDescriptorPool();
        setupDescriptorSet();
        transitionImageLayout();
    }

    void setupImage(const Texture& texture);
    void allocateMemory();
    void setupImageView();
    void setupSampler();
    void fillImage(const Texture& texture);
    void createDescriptorSetLayout();
    void setupDescriptorPool();
    void setupDescriptorSet();
    void transitionImageLayout();

    void cleanup() {
        if (s_descriptorSetLayout != VK_NULL_HANDLE) {
            r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);
            s_descriptorSetLayout = VK_NULL_HANDLE;
        }

        r_engine->m_device.destroyImageView(m_imageView);
        r_engine->m_device.destroyImage(m_image);
        r_engine->m_device.destroySampler(m_sampler);
        r_engine->m_device.freeMemory(m_bufferMemory);
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
    }

    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 1, m_descriptorSet, nullptr);
    }
};

}

#endif