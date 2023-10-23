#ifndef VERTEX_HPP
#define VERTEX_HPP

#include <libraries.hpp>

namespace mge {

class PointColorVertex {
    glm::vec3 m_position;
    glm::vec4 m_colour;

public:
    PointColorVertex(glm::vec3 position, glm::vec4 colour) :
        m_position(position), m_colour(colour)
    {}

    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes() {
        return std::vector<vk::VertexInputAttributeDescription> {
            vk::VertexInputAttributeDescription {}
                .setBinding(0)
                .setLocation(0)
                .setOffset(offsetof(PointColorVertex, m_position))
                .setFormat(vk::Format::eR32G32B32Sfloat)
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(0)
                .setLocation(1)
                .setOffset(offsetof(PointColorVertex, m_colour))
                .setFormat(vk::Format::eR32G32B32A32Sfloat)
                ,
            };
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return std::vector<vk::VertexInputBindingDescription> {
            vk::VertexInputBindingDescription {}
                .setBinding(0)
                .setInputRate(vk::VertexInputRate::eVertex)
                .setStride(sizeof(PointColorVertex))
                ,
            };
    }
};

}

#endif