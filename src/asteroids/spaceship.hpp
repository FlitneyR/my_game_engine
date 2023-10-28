#ifndef SPACESHIP_HPP
#define SPACESHIP_HPP

#include "asteroid.hpp"

#include <engine.hpp>

struct Spaceship {
    glm::vec3 m_position;
    glm::vec3 m_velocity;
    
    glm::quat m_rotation;
    glm::vec3 m_angularVelocity;

    float m_yaw;
    float m_roll;

    uint32_t m_modelInstanceId;

    bool m_alive = true;

    static constexpr float TURN_RATE = glm::pi<float>();
    static constexpr float ACCELERATION = 1.f;

    void update(bool turnLeft, bool turnRight, bool accelerate, float deltaTime) {
        m_velocity *= 1.f - 0.2f * deltaTime;

        if (m_alive) {
            static float turn = 0;

            turn = glm::lerp(turn, -1.f * turnLeft + 1.f * turnRight, deltaTime * 3.f);

            m_roll = glm::lerp(m_roll, turn * glm::half_pi<float>(), deltaTime * 3.f);
            // m_roll = turn * glm::half_pi<float>();
            m_yaw += turn * TURN_RATE * deltaTime;
            m_velocity += getForward() * ACCELERATION * deltaTime * (accelerate ? 1.0f : 0.f);
        } else {
            m_velocity += getForward() * ACCELERATION * deltaTime;
            m_rotation = glm::angleAxis(glm::length(m_angularVelocity) * deltaTime, glm::normalize(m_angularVelocity)) * m_rotation;
        }

        m_position += m_velocity;
    }

    bool isColliding(const Asteroid& asteroid) {
        static constexpr float radius = 3.f;

        return glm::distance(m_position, asteroid.m_position) < radius + asteroid.m_radius;
    }

    void die() {
        if (m_alive) {
            m_alive = false;
            m_angularVelocity = mge::Engine::randomRangeFloat(0.5f, 3.f) * glm::pi<float>() * mge::Engine::randomUnitVector();
            m_rotation = glm::angleAxis(m_yaw, glm::vec3(0, 0, 1)) * glm::angleAxis(m_roll, glm::vec3(0, 1, 0));
        }
    }

    glm::mat4 getTransform() {
        glm::mat4 result(1.f);

        result = glm::translate(result, m_position);
        
        if (m_alive) {
            result = glm::rotate(result, m_yaw, glm::vec3(0, 0, 1));
            result = glm::rotate(result, m_roll, glm::vec3(0, -1, 0));
        } else result = result * glm::toMat4(m_rotation);

        return result;
    }

    glm::vec3 getForward() {
        glm::mat4 transform = getTransform();

        return transform * glm::vec4(0, 1, 0, 0);

        // return glm::normalize(glm::vec3(-glm::sin(m_yaw), glm::cos(m_yaw), 0.f));
    }

    glm::vec3 getRight() {
        glm::mat4 transform = getTransform();

        return transform * glm::vec4(1, 0, 0, 0);
        // return glm::normalize(glm::cross(getForward(), glm::vec3(0.f, 0.f, 1.f)));
    }

    glm::vec3 getUp() {
        glm::mat4 transform = getTransform();

        return transform * glm::vec4(0, 0, 1, 0);
        // return glm::cross(getRight(), getForward());
    }

    glm::vec3 getCameraPosition() {
        return m_position - getForward() * 10.f + getUp() * 3.f + glm::vec3(0.f, 0.f, 7.f);
    }

    glm::vec3 getCameraTarget() {
        return m_position + getForward() * 15.f;
    }
};

#endif