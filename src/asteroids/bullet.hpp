#ifndef BULLET_HPP
#define BULLET_HPP

#include <engine.hpp>
#include "asteroid.hpp"

struct Bullet {
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    uint32_t m_meshInstanceId;

    glm::mat4 getTransform() {
        glm::mat4 mat(1.f);
        
        // mat = glm::translate(mat, m_position);
        // glm::rotate(mat, glm::half_pi<float>(), glm::vec3(0, 0, 1));
        mat = glm::inverse(glm::lookAt(m_position, m_position + m_velocity, glm::vec3(0, 0, 1)));

        return mat;
    }

    void update(float deltaTime) {
        m_position += m_velocity * deltaTime;
    }

    bool isColliding(const Asteroid& asteroid) {
        return glm::distance(m_position, asteroid.m_position) <= asteroid.m_radius;
    }
};

#endif