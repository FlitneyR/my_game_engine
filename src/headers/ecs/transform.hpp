#ifndef TRANSFORM_HPP
#define TRANSFORM_HPP

#include <libraries.hpp>

namespace mge::ecs {

class TransformComponent {
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    bool m_validMatrix = false;
    glm::mat4 m_matrix;

public:
    glm::vec3 getPosition() { return m_position; }
    glm::quat getRotation() { return m_rotation; }
    glm::vec3 getScale() { return m_scale; }

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

    glm::mat4 getMatrix() {
        if (!m_validMatrix) {
            m_matrix = glm::mat4 { 1.0 };

            m_matrix = glm::translate(m_matrix, m_position);
            m_matrix = glm::toMat4(m_rotation) * m_matrix;
            m_matrix = glm::scale(m_matrix, m_scale);
        }

        return m_matrix;
    }
};

}

#endif