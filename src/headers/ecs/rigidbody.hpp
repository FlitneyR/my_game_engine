#ifndef RIGIDBODY_HPP
#define RIGIDBODY_HPP

#include <libraries.hpp>
#include <transform.hpp>
#include <collision.hpp>
#include <component.hpp>
#include <system.hpp>

namespace mge::ecs {

class RigidbodyComponent : public Component {
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

class RigidbodySystem : public System<RigidbodyComponent> {
public:
    System<TransformComponent>* r_transformSystem;

    void update(float deltaTime) {
        for (auto& [ entity, rigidbody ] : m_components)
        if (auto transform = r_transformSystem->getComponent(entity)) {
            rigidbody.m_velocity += rigidbody.m_acceleration * deltaTime;

            transform->setPosition(transform->getPosition() + rigidbody.m_velocity * deltaTime);

            glm::vec3 angularVelocity = rigidbody.m_angularVelocity * deltaTime;

            if (angularVelocity != glm::vec3 { 0.f }) {
                glm::quat deltaRotation = glm::angleAxis(glm::length(angularVelocity), glm::normalize(angularVelocity));
                transform->setRotation(deltaRotation * transform->getRotation());
            }
        }
    }

    void resolveCollisions(std::vector<CollisionComponent::CollisionEvent> collisionEvents) {
        for (const auto& collisionEvent : collisionEvents)
        if (auto thisRigidBody = getComponent(collisionEvent.m_thisEntity))
        if (thisRigidBody->m_physicsType == RigidbodyComponent::e_dynamic)
        if (auto otherRigidBody = getComponent(collisionEvent.m_otherEntity)) {
            float m = 2.f * otherRigidBody->m_mass / (thisRigidBody->m_mass + otherRigidBody->m_mass);
            float v = glm::dot(thisRigidBody->m_velocity - otherRigidBody->m_velocity, collisionEvent.m_normal);

            thisRigidBody->m_velocity -= collisionEvent.m_normal * m * v * 0.75f;
            
            auto transform = r_transformSystem->getComponent(collisionEvent.m_thisEntity);

            glm::vec3 newPosition = transform->getPosition();

            newPosition += collisionEvent.m_normal * collisionEvent.m_collisionDepth;

            transform->setPosition(newPosition);
        }
    }
};

}

#endif