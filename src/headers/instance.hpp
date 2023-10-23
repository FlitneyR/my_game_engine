#ifndef INSTANCE_HPP
#define INSTANCE_HPP

#include <libraries.hpp>

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

}

#endif