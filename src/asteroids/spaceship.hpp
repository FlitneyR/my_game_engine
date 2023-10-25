#ifndef SPACESHIP_HPP
#define SPACESHIP_HPP

#include <engine.hpp>

struct Spaceship {
    glm::vec3 m_position;
    glm::vec3 m_velocity;

    float m_yaw;
    float m_roll;

    uint32_t m_meshInstanceId;

    static constexpr float TURN_RATE = glm::pi<float>();
    static constexpr float ACCELERATION = 1.f;

    void update(bool turnLeft, bool turnRight, bool accelerate, float deltaTime) {
        static float turn = 0;

        turn = glm::lerp(turn, -1.f * turnLeft + 1.f * turnRight, deltaTime);

        m_roll = glm::lerp(m_roll, turn * glm::half_pi<float>(), deltaTime);
        m_yaw += turn * TURN_RATE * deltaTime;

        m_velocity *= 0.99f;

        m_velocity += getForward() * ACCELERATION * deltaTime * (accelerate ? 1.0f : 0.f);
        m_position += m_velocity;
    }

    glm::mat4 getTransform() {
        glm::mat4 result(1.f);

        result = glm::translate(result, m_position);
        result = glm::rotate(result, m_yaw, glm::vec3(0, 0, 1));
        result = glm::rotate(result, m_roll, glm::vec3(0, -1, 0));

        return result;
    }

    glm::vec3 getForward() {
        return glm::normalize(glm::vec3(-glm::sin(m_yaw), glm::cos(m_yaw), 0.f));
    }

    glm::vec3 getRight() {
        return glm::normalize(glm::cross(getForward(), glm::vec3(0.f, 0.f, 1.f)));
    }
};

#endif