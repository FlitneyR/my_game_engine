#ifndef ASTEROID_HPP
#define ASTEROID_HPP

#include <engine.hpp>
#include <physics.hpp>

constexpr float MAX_ASTEROID_DISTANCE = 1000.f;

// class Asteroid : public mge::RigidBody {
// public:
//     Asteroid() {
//         m_collider = std::make_unique<mge::SphereCollider>();
//     }

//     void start() override {
//         mge::RigidBody::start();

//         m_rotation = glm::angleAxis(
//             mge::Engine::randomRangeFloat(-1.f, 1.f) * glm::pi<float>(),
//             mge::Engine::randomUnitVector()
//         );

//         m_angularVelocity = mge::Engine::randomUnitVector() * mge::Engine::randomRangeFloat(0.f, 0.25f) * glm::pi<float>();

//         m_position = glm::vec3(
//             mge::Engine::randomRangeFloat(-1.f, 1.f) * MAX_ASTEROID_DISTANCE,
//             mge::Engine::randomRangeFloat(-1.f, 1.f) * MAX_ASTEROID_DISTANCE,
//             0
//         );

//         m_linearVelocity = glm::vec3(
//             mge::Engine::randomRangeFloat(-1.f, 1.f) * 5.f,
//             mge::Engine::randomRangeFloat(-1.f, 1.f) * 5.f,
//             0
//         );

//         float radius = glm::pow(mge::Engine::randomRangeFloat(1.f, 2.f), 4.f);
//         m_scale = glm::vec3(radius);

//         dynamic_cast<mge::SphereCollider*>(m_collider.get())->m_radius = radius;
//     }

//     void update(float deltaTime) override {
//         mge::RigidBody::update(deltaTime);

//         if (m_position.x >  MAX_ASTEROID_DISTANCE) m_position.x -= 2 * MAX_ASTEROID_DISTANCE;
//         if (m_position.x < -MAX_ASTEROID_DISTANCE) m_position.x += 2 * MAX_ASTEROID_DISTANCE;
//         if (m_position.y >  MAX_ASTEROID_DISTANCE) m_position.y -= 2 * MAX_ASTEROID_DISTANCE;
//         if (m_position.y < -MAX_ASTEROID_DISTANCE) m_position.y += 2 * MAX_ASTEROID_DISTANCE;
//         if (m_position.z >  MAX_ASTEROID_DISTANCE) m_position.z -= 2 * MAX_ASTEROID_DISTANCE;
//         if (m_position.z < -MAX_ASTEROID_DISTANCE) m_position.z += 2 * MAX_ASTEROID_DISTANCE;
//     }
// };

struct Asteroid {
    glm::vec3 m_position;
    glm::quat m_rotation;
    float m_radius;

    uint32_t m_modelInstanceId;

    glm::vec3 m_linearVelocity;

    glm::vec3 m_angularVelocity;

    glm::mat4 getTransform() {
        glm::mat4 mat(1.f);

        mat = glm::translate(mat, m_position);
        mat = glm::scale(mat, glm::vec3(m_radius));
        mat = mat * glm::toMat4(m_rotation);

        return mat;
    }

    float mass() {
        return m_radius;
    }

    void start() {
        m_position = glm::vec3(
            MAX_ASTEROID_DISTANCE * mge::Engine::randomRangeFloat(-1.f, 1.f),
            MAX_ASTEROID_DISTANCE * mge::Engine::randomRangeFloat(-1.f, 1.f),
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
            mge::Engine::randomRangeFloat(-1.f, 1.f)
        };

        glm::vec3 rotationAxis = glm::normalize(randomAxis);
        float rotationRate = mge::Engine::randomRangeFloat(-1.f, 1.f);
        m_angularVelocity = rotationAxis * rotationRate;

        m_rotation = glm::angleAxis(
            mge::Engine::randomRangeFloat(-glm::pi<float>(), glm::pi<float>()),
            glm::normalize(glm::vec3(
                mge::Engine::randomRangeFloat(-1.f, 1.f),
                mge::Engine::randomRangeFloat(-1.f, 1.f),
                mge::Engine::randomRangeFloat(-1.f, 1.f)
            ))
        );

        m_radius = glm::pow(mge::Engine::randomRangeFloat(1.f, 2.f), 4.f);
    }

    void update(float deltaTime, glm::vec3 cameraPosition) {
        m_position += m_linearVelocity * deltaTime;

        glm::quat identity { 0, 0, 0, 0 };
        m_rotation = glm::angleAxis(glm::length(m_angularVelocity) * deltaTime, glm::normalize(m_angularVelocity)) * m_rotation;

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
        float minDist = m_radius + other.m_radius;
        float sqDist = glm::length2(m_position - other.m_position);
        return sqDist < minDist * minDist;
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

        m_linearVelocity += collisionNormal * vd1 * 0.9f;
        other.m_linearVelocity += collisionNormal * vd2 * 0.9f;

        float overlap = 0.01f + m_radius + other.m_radius - glm::distance(m_position, other.m_position);

        m_position += collisionNormal * overlap * m1 / m2;
        other.m_position -= collisionNormal * overlap * m2 / m1;
    }
};

#endif