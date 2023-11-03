#ifndef LOGIC_HPP
#define LOGIC_HPP

#include <system.hpp>
#include <component.hpp>
#include <collision.hpp>
#include <rigidbody.hpp>
#include <transform.hpp>
#include <ecsManager.hpp>
#include <modelInstance.hpp>

struct AsteroidComponent : public mge::ecs::Component {};

struct BulletComponent : public mge::ecs::Component {
    float m_age = 0.f;
};

struct SpaceshipComponent : public mge::ecs::Component {
    bool m_alive = true;
};

class AsteroidSystem : public mge::ecs::System<AsteroidComponent> {
public:
    mge::ecs::System<SpaceshipComponent>* r_spaceshipSystem;
    mge::ecs::System<mge::ecs::TransformComponent>* r_transformSystem;

    static constexpr float MAX_DISTANCE = 500.f;

    void wrapAsteroids() {
        auto spaceshipEntity = r_spaceshipSystem->m_components.begin()->first;
        auto spaceshipTransform = r_transformSystem->getComponent(spaceshipEntity);
        glm::vec3 spaceshipPosition = spaceshipTransform->getPosition();

        for (auto& [ entity, component ] : m_components) {
            auto asteroidTransform = r_transformSystem->getComponent(entity);

            glm::vec3 asteroidPosition = asteroidTransform->getPosition();
            glm::vec3 relpos = asteroidPosition - spaceshipPosition; // asteroid relative position
            
            if (relpos.x > MAX_DISTANCE) relpos.x -= 2.f * MAX_DISTANCE;
            if (relpos.x < -MAX_DISTANCE) relpos.x += 2.f * MAX_DISTANCE;
            if (relpos.y > MAX_DISTANCE) relpos.y -= 2.f * MAX_DISTANCE;
            if (relpos.y < -MAX_DISTANCE) relpos.y += 2.f * MAX_DISTANCE;
            if (relpos.z > MAX_DISTANCE) relpos.z -= 2.f * MAX_DISTANCE;
            if (relpos.z < -MAX_DISTANCE) relpos.z += 2.f * MAX_DISTANCE;

            // spaceshipPosition.z *= 0.f;

            asteroidTransform->setPosition(spaceshipPosition + relpos);
        }
    }
};

class BulletSystem : public mge::ecs::System<BulletComponent> {
public:
    mge::ecs::System<AsteroidComponent>* r_asteroidSystem;

    static constexpr float MAX_AGE = 5.f;

    void destroyOldBullets(float deltaTime) {
        std::vector<mge::ecs::Entity> entitiesToDestroy;

        for (auto& [ entity, comp ] : m_components)
            if ((comp.m_age += deltaTime) > MAX_AGE)
                entitiesToDestroy.push_back(entity);
        
        for (const auto& entity : entitiesToDestroy)
            r_ecsManager->destroyEntity(entity);
    }

    void handleCollisions(const std::vector<mge::ecs::CollisionComponent::CollisionEvent>& collisionEvents) {
        std::vector<mge::ecs::Entity> entitiesToDestroy;

        for (const auto& collisionEvent : collisionEvents)
        if (getComponent(collisionEvent.m_thisEntity))
        if (r_asteroidSystem->getComponent(collisionEvent.m_otherEntity)) {
            entitiesToDestroy.push_back(collisionEvent.m_thisEntity);
            entitiesToDestroy.push_back(collisionEvent.m_otherEntity);
        }

        for (auto& entity : entitiesToDestroy) r_ecsManager->destroyEntity(entity);
    }
};

class SpaceshipSystem : public mge::ecs::System<SpaceshipComponent> {
public:
    mge::ecs::System<BulletComponent>* r_bulletSystem;
    mge::ecs::System<mge::ecs::ModelComponent>* r_bulletModelSystem;
    mge::ecs::System<mge::ecs::CollisionComponent>* r_collisionSystem;

    mge::ecs::System<AsteroidComponent>* r_asteroidSystem;
    mge::ecs::System<mge::ecs::TransformComponent>* r_transformSystem;
    mge::ecs::System<mge::ecs::RigidbodyComponent>* r_rigidBodySystem;

    constexpr static float PITCH_RATE = 0.75f * glm::pi<float>();
    constexpr static float TURN_RATE = 0.75f * glm::pi<float>();
    constexpr static float ACCELERATION_RATE = 25.f;

    void checkForAsteroidCollision(const std::vector<mge::ecs::CollisionComponent::CollisionEvent>& collisions) {
        for (auto& collision : collisions)
        if (auto spaceship = getComponent(collision.m_thisEntity))
        if (spaceship->m_alive)
        if (r_asteroidSystem->getComponent(collision.m_otherEntity)) {
            spaceship->m_alive = false;

            if (auto rigidbody = r_rigidBodySystem->getComponent(spaceship->m_entity)) {
                rigidbody->m_angularVelocity = glm::vec3 {
                    mge::Engine::randomRangeFloat(-1.f, 1.f),
                    mge::Engine::randomRangeFloat(-1.f, 1.f),
                    1.f
                } * mge::Engine::randomRangeFloat(-10.f, 10.f);
            }
        }
    }

    void update(float deltaTime, bool accelerate, bool pitchUp, bool pitchDown, bool turnLeft, bool turnRight, bool fire) {
        static float turnInput = 0.f, pitchInput = 0.f;

        turnInput = glm::mix(turnInput, static_cast<float>(turnRight - turnLeft), glm::clamp(2.f * deltaTime, 0.f, 1.f));
        pitchInput = glm::mix(pitchInput, static_cast<float>(pitchUp - pitchDown), glm::clamp(2.f * deltaTime, 0.f, 1.f));

        for (auto& [ entity, comp ] : m_components)
        if (auto transform = r_transformSystem->getComponent(entity))
        if (auto rigidbody = r_rigidBodySystem->getComponent(entity)) {
            if (!comp.m_alive) {
                rigidbody->m_acceleration = transform->getForward() * ACCELERATION_RATE;
            } else {
                float pitchDelta = pitchInput * -PITCH_RATE * deltaTime;
                float turnDelta = turnInput * TURN_RATE * deltaTime;

                transform->setRotation(
                    glm::angleAxis(turnDelta, transform->getUp()) *
                    glm::angleAxis(-0.5f * turnDelta, transform->getForward()) *
                    glm::angleAxis(pitchDelta, transform->getRight()) *
                    transform->getRotation()
                );

                rigidbody->m_velocity *= glm::clamp(1.f - 0.25f * deltaTime, 0.f, 1.f);
                rigidbody->m_acceleration = transform->getForward() * (ACCELERATION_RATE * accelerate);

                if (fire)
                for (float i = -1.f; i <= 1.f; i += 2.f) {
                    auto bulletEntity = r_ecsManager->makeEntity();

                    r_bulletSystem->addComponent(bulletEntity);
                    r_bulletModelSystem->addComponent(bulletEntity);
                    auto bulletTransform = r_transformSystem->addComponent(bulletEntity);
                    auto bulletRigidbody = r_rigidBodySystem->addComponent(bulletEntity);
                    auto bulletCollision = r_collisionSystem->addComponent(bulletEntity);

                    bulletTransform->setPosition(transform->getPosition() + transform->getForward() * 3.f + transform->getRight() * i);
                    bulletTransform->setRotation(transform->getRotation() * glm::angleAxis(glm::half_pi<float>(), glm::vec3 { 1.f, 0.f, 0.f }));
                    bulletTransform->setScale(glm::vec3 { 1.f });

                    bulletRigidbody->m_physicsType = bulletRigidbody->e_dynamic;
                    bulletRigidbody->m_mass = 0.001f;
                    bulletRigidbody->m_velocity = rigidbody->m_velocity + transform->getForward() * 50.f;
                    bulletRigidbody->m_acceleration = glm::vec3 { 0.f };
                    bulletRigidbody->m_angularVelocity = glm::vec3 { 0.f };

                    bulletCollision->setupSphere(1.f);
                }
            }
        }
    }
};

#endif