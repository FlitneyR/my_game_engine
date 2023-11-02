#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <libraries.hpp>
#include <transform.hpp>
#include <collision.hpp>
#include <component.hpp>
#include <system.hpp>

namespace mge::ecs {

class RigidBodyComponent : public Component {
public:
    glm::vec3 m_velocity;
    glm::vec3 m_acceleration;
    glm::vec3 m_angularVelocity;
    float m_mass;

    enum PhysicsType {
        e_dynamic,
        e_kinematic,
    } m_physicsType;
};

class RigidBodySystem : public System<RigidBodyComponent> {
public:
    System<TransformComponent>& r_transformSystem;

    void update(float deltaTime) {
        for (auto& [ entity, rigidbody ] : m_components)
        if (auto transform = r_transformSystem.getComponent(entity)) {
            rigidbody.m_velocity += rigidbody.m_acceleration * deltaTime;

            glm::vec3 angularVelocity = rigidbody.m_angularVelocity;
            glm::quat deltaRotation = glm::angleAxis(glm::length(angularVelocity), glm::normalize(angularVelocity));

            transform->setPosition(transform->getPosition() + rigidbody.m_velocity * deltaTime);
            transform->setRotation(deltaRotation * transform->getRotation());
        }
    }

    void resolveCollisions(std::vector<CollisionComponent::CollisionEvent> collisionEvents) {
        for (const auto& collisionEvent : collisionEvents)
        if (auto thisRigidBody = getComponent(collisionEvent.m_thisEntity))
        if (thisRigidBody->m_physicsType == RigidBodyComponent::e_dynamic)
        if (auto otherRigidBody = getComponent(collisionEvent.m_otherEntity)) {
            /* TODO! Resolve collision events */
        }
    }
};

}

#endif