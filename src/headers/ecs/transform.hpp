#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <libraries.hpp>
#include <component.hpp>

namespace mge::ecs {

class TransformComponent : public Component {
    glm::vec3 m_position { 0.f };
    glm::quat m_rotation { 0.f, { 0.f, 1.f, 0.f } };
    glm::vec3 m_scale { 1.f };

    bool m_validMatrix = false;
    glm::mat4 m_matrix;

public:
    glm::vec3 getPosition() { return m_position; }
    glm::quat getRotation() { return m_rotation; }
    glm::vec3 getScale() { return m_scale; }

    glm::vec3 getRight() {
        return glm::normalize(getMat3()[0]);
    }

    glm::vec3 getForward() {
        return glm::normalize(getMat3()[1]);
    }

    glm::vec3 getUp() {
        return glm::normalize(getMat3()[2]);
    }

    void setPosition(glm::vec3 position) {
        m_position = position;
        m_validMatrix = false;
    }

    void setRotation(glm::quat rotation) {
        m_rotation = rotation;
        m_validMatrix = false;
    }

    void setScale(glm::vec3 scale) {
        m_scale = scale;
        m_validMatrix = false;
    }

    void updateMatrix() {
        m_matrix = glm::translate(glm::mat4 { 1.f }, m_position) * glm::scale(glm::mat4 { 1.f }, m_scale) * glm::toMat4(m_rotation);
    }

    glm::mat4 getMat4() {
        if (!m_validMatrix) updateMatrix();
        return m_matrix;
    }

    glm::mat3 getMat3() {
        if (!m_validMatrix) updateMatrix();
        return glm::mat3 { m_matrix };
    }
};

}

#endif