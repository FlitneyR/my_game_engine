#ifndef ASTEROID_HPP
#define ASTEROID_HPP

#include <engine.hpp>

constexpr float MAX_ASTEROID_DISTANCE = 500.f;

struct Asteroid {
    glm::vec3 m_position;
    glm::mat4 m_rotation;
    float m_radius;

    uint32_t m_meshInstanceId;

    glm::vec3 m_linearVelocity;

    glm::vec3 m_rotationAxis;
    float m_angularVelocity;

    glm::mat4 getTransform() {
        // return glm::mat4 {
        //     glm::vec4(m_radius * m_rotation[0], 0.f),
        //     glm::vec4(m_radius * m_rotation[1], 0.f),
        //     glm::vec4(m_radius * m_rotation[2], 0.f),
        //     glm::vec4(m_position, 1.f)
        // };
        glm::mat4 mat(1.f);

        mat = glm::translate(mat, m_position);
        mat = glm::scale(mat, glm::vec3(m_radius));
        mat = mat * m_rotation;

        return mat;
    }

    float mass() {
        return m_radius;
    }

    void start() {
        m_position = glm::vec3(
            MAX_ASTEROID_DISTANCE * mge::Engine::randomRangeFloat(-1.f, 1.f),
            MAX_ASTEROID_DISTANCE * mge::Engine::randomRangeFloat(-1.f, 1.f),
            // mge::Engine::randomRangeFloat(-MAX_ASTEROID_DISTANCE, MAX_ASTEROID_DISTANCE)
            0
        );

        m_linearVelocity = glm::vec3(
            mge::Engine::randomRangeFloat(-5.f, 5.f),
            mge::Engine::randomRangeFloat(-5.f, 5.f),
            0
        );

        glm::vec3 randomAxis {
            mge::Engine::randomRangeFloat(-1.f, 1.f),
            mge::Engine::randomRangeFloat(-1.f, 1.f),
            // mge::Engine::randomRangeFloat(-1.f, 1.f)
            0
        };

        m_rotationAxis = glm::normalize(randomAxis);
        m_angularVelocity = mge::Engine::randomRangeFloat(-1.f, 1.f);
        m_rotation = glm::mat4(1.f);

        m_radius = mge::Engine::randomRangeFloat(1.f, 5.f);
    }

    void update(float deltaTime, glm::vec3 cameraPosition) {
        m_position += m_linearVelocity * deltaTime;

        m_rotation = glm::rotate(m_rotation, deltaTime * m_angularVelocity, m_rotationAxis);

        glm::vec3 relpos = m_position - cameraPosition;

        if (relpos.x >  MAX_ASTEROID_DISTANCE) relpos.x = -MAX_ASTEROID_DISTANCE;
        if (relpos.x < -MAX_ASTEROID_DISTANCE) relpos.x =  MAX_ASTEROID_DISTANCE;
        if (relpos.y >  MAX_ASTEROID_DISTANCE) relpos.y = -MAX_ASTEROID_DISTANCE;
        if (relpos.y < -MAX_ASTEROID_DISTANCE) relpos.y =  MAX_ASTEROID_DISTANCE;
        if (relpos.z >  MAX_ASTEROID_DISTANCE) relpos.z = -MAX_ASTEROID_DISTANCE;
        if (relpos.z < -MAX_ASTEROID_DISTANCE) relpos.z =  MAX_ASTEROID_DISTANCE;

        m_position = cameraPosition + relpos;
    }

    bool isColliding(const Asteroid& other) const {
        float mindist = m_radius + other.m_radius;
        if (glm::distance(m_position.x, other.m_position.x) > mindist) return false;
        if (glm::distance(m_position.y, other.m_position.y) > mindist) return false;
        return glm::distance(m_position, other.m_position) < mindist;
    }

    void resolveCollision(Asteroid& other) {
        glm::vec3 collisionNormal = glm::normalize(m_position - other.m_position);

        float m1 = mass();
        float m2 = other.mass();

        float vi1 = glm::dot(m_linearVelocity, collisionNormal);
        float vi2 = glm::dot(other.m_linearVelocity, collisionNormal);

        float vf1 = m2 * vi2 / m1;
        float vf2 = m1 * vi1 / m2;

        float vd1 = vf1 - vi1;
        float vd2 = vf2 - vi2;

        m_linearVelocity += collisionNormal * vd1 * 0.75f;
        other.m_linearVelocity += collisionNormal * vd2 * 0.75f;

        float overlap = 0.01f + m_radius + other.m_radius - glm::distance(m_position, other.m_position);

        m_position += collisionNormal * overlap * m1 / m2;
        other.m_position -= collisionNormal * overlap * m2 / m1;
    }
};

#endif