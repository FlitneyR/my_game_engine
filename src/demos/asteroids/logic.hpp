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
    float m_fireCooldown = 0.f;
};

class AsteroidSystem : public mge::ecs::System<AsteroidComponent> {
public:
    static constexpr float MAX_DISTANCE = 500.f;

    void wrapAsteroids() {
        auto spaceshipSystem = r_ecsManager->getSystem<SpaceshipComponent>("Spaceship");
        auto transformSystem = r_ecsManager->getSystem<mge::ecs::TransformComponent>("Transform");

        auto spaceshipEntity = spaceshipSystem->m_components.begin()->first;
        auto spaceshipTransform = transformSystem->getComponent(spaceshipEntity);
        glm::vec3 spaceshipPosition = spaceshipTransform->getPosition();

        for (auto& [ entity, component ] : m_components) {
            auto asteroidTransform = transformSystem->getComponent(entity);

            glm::vec3 asteroidPosition = asteroidTransform->getPosition();
            glm::vec3 relpos = asteroidPosition - spaceshipPosition; // asteroid relative position
            
            if (relpos.x >  MAX_DISTANCE) relpos.x -= 2.f * MAX_DISTANCE;
            if (relpos.x < -MAX_DISTANCE) relpos.x += 2.f * MAX_DISTANCE;
            if (relpos.y >  MAX_DISTANCE) relpos.y -= 2.f * MAX_DISTANCE;
            if (relpos.y < -MAX_DISTANCE) relpos.y += 2.f * MAX_DISTANCE;
            if (relpos.z >  MAX_DISTANCE) relpos.z -= 2.f * MAX_DISTANCE;
            if (relpos.z < -MAX_DISTANCE) relpos.z += 2.f * MAX_DISTANCE;

            asteroidTransform->setPosition(spaceshipPosition + relpos);
        }
    }

    void breakApart(const mge::ecs::Entity& entity) {
        auto transformSystem = r_ecsManager->getSystem<mge::ecs::TransformComponent>("Transform");
        auto collisionSystem = r_ecsManager->getSystem<mge::ecs::CollisionComponent>("Collision");
        auto rigidbodySystem = r_ecsManager->getSystem<mge::ecs::RigidbodyComponent>("Rigidbody");

        auto asteroid = getComponent(entity);
        auto oldTransform = transformSystem->getComponent(entity);
        auto oldRigidbody = rigidbodySystem->getComponent(entity);
        auto oldCollision = collisionSystem->getComponent(entity);

        float radius = oldCollision->m_shapeParameters.u_sphereParameters.m_radius;

        if (radius <= 3.f) return;

        auto breakAxis = mge::Engine::randomUnitVector() * mge::Engine::randomRangeFloat(10.f, 50.f);

        for (float i = -1.f; i <= 1.f; i += 2.f) {
            float newRadius = radius * 0.5f;

            auto newEntity = r_ecsManager->makeEntityFromTemplate("Asteroid");

            auto transform = transformSystem->getComponent(newEntity);
            auto collision = collisionSystem->getComponent(newEntity);
            auto rigidbody = rigidbodySystem->getComponent(newEntity);

            transform->setPosition(oldTransform->getPosition() + glm::normalize(breakAxis) * i * newRadius);
            transform->setScale(glm::vec3 { newRadius });

            collision->setupSphere(newRadius);

            rigidbody->m_mass = newRadius * newRadius * newRadius;
            
            rigidbody->m_velocity = oldRigidbody->m_velocity;
            rigidbody->m_velocity += breakAxis * i * oldRigidbody->m_mass / (8.f * rigidbody->m_mass);

            rigidbody->m_angularVelocity = oldRigidbody->m_angularVelocity;
            rigidbody->m_angularVelocity += mge::Engine::randomUnitVector() * mge::Engine::randomRangeFloat(3.f, 10.f);
        }
    }
};

class BulletSystem : public mge::ecs::System<BulletComponent> {
public:
    static constexpr float MAX_AGE = 3.f;

    void destroyOldBullets(float deltaTime) {
        std::vector<mge::ecs::Entity> entitiesToDestroy;

        for (auto& [ entity, comp ] : m_components)
            if ((comp.m_age += deltaTime) > MAX_AGE)
                entitiesToDestroy.push_back(entity);
        
        for (const auto& entity : entitiesToDestroy)
            r_ecsManager->destroyEntity(entity);
    }

    void handleCollisions(const std::vector<mge::ecs::CollisionComponent::CollisionEvent>& collisionEvents) {
        auto asteroidSystem = static_cast<AsteroidSystem*>(r_ecsManager->getSystem<AsteroidComponent>("Asteroid"));

        std::vector<mge::ecs::Entity> entitiesToDestroy;

        for (const auto& collisionEvent : collisionEvents)
        if (getComponent(collisionEvent.m_thisEntity))
        if (auto hitAsteroid = asteroidSystem->getComponent(collisionEvent.m_otherEntity)) {
            entitiesToDestroy.push_back(collisionEvent.m_thisEntity);
            entitiesToDestroy.push_back(collisionEvent.m_otherEntity);

            asteroidSystem->breakApart(hitAsteroid->m_entity);
        }

        for (auto& entity : entitiesToDestroy) r_ecsManager->destroyEntity(entity);
    }
};

class SpaceshipSystem : public mge::ecs::System<SpaceshipComponent> {
public:
    constexpr static float PITCH_RATE = 0.75f * glm::pi<float>();
    constexpr static float TURN_RATE = 0.75f * glm::pi<float>();
    constexpr static float ACCELERATION_RATE = 25.f;
    constexpr static float RATE_OF_FIRE = 0.125f;

    void checkForAsteroidCollision(const std::vector<mge::ecs::CollisionComponent::CollisionEvent>& collisions) {
        auto asteroidSystem = r_ecsManager->getSystem<AsteroidComponent>("Asteroid");
        auto rigidbodySystem = r_ecsManager->getSystem<mge::ecs::RigidbodyComponent>("Rigidbody");

        for (auto& collision : collisions)
        if (auto spaceship = getComponent(collision.m_thisEntity))
        if (spaceship->m_alive)
        if (asteroidSystem->getComponent(collision.m_otherEntity)) {
            spaceship->m_alive = false;

            if (auto rigidbody = rigidbodySystem->getComponent(spaceship->m_entity)) {
                rigidbody->m_angularVelocity = mge::Engine::randomUnitVector() * mge::Engine::randomRangeFloat(0.f, 3.f);
                rigidbody->m_acceleration = glm::vec3 { 0.f };
            }
        }
    }

    void update(float deltaTime, bool accelerate, bool pitchUp, bool pitchDown, bool turnLeft, bool turnRight, bool fire) {
        auto transformSystem = r_ecsManager->getSystem<mge::ecs::TransformComponent>("Transform");
        auto rigidbodySystem = r_ecsManager->getSystem<mge::ecs::RigidbodyComponent>("Rigidbody");

        static float turnInput = 0.f, pitchInput = 0.f;

        turnInput = glm::mix(turnInput, static_cast<float>(turnLeft - turnRight), glm::clamp(2.f * deltaTime, 0.f, 1.f));
        pitchInput = glm::mix(pitchInput, static_cast<float>(pitchDown - pitchUp), glm::clamp(2.f * deltaTime, 0.f, 1.f));

        for (auto& [ entity, comp ] : m_components)
        if (comp.m_alive)
        if (auto transform = transformSystem->getComponent(entity))
        if (auto rigidbody = rigidbodySystem->getComponent(entity)) {
            comp.m_fireCooldown = glm::max(0.f, comp.m_fireCooldown - deltaTime);
            float pitchDelta = pitchInput * PITCH_RATE * deltaTime;
            float turnDelta = turnInput * TURN_RATE * deltaTime;

            transform->setRotation(
                glm::angleAxis(turnDelta, transform->getUp()) *
                glm::angleAxis(-0.5f * turnDelta, transform->getForward()) *
                glm::angleAxis(pitchDelta, transform->getRight()) *
                transform->getRotation()
            );

            rigidbody->m_velocity *= glm::clamp(1.f - 0.1f * deltaTime, 0.f, 1.f);
            rigidbody->m_acceleration = transform->getForward() * (ACCELERATION_RATE * accelerate);

            static float side = -1.f;

            if (fire)
            if (comp.m_fireCooldown <= 0.f) {
                comp.m_fireCooldown = RATE_OF_FIRE;

                auto bulletEntity = r_ecsManager->makeEntityFromTemplate("Bullet");

                auto bulletTransform = transformSystem->getComponent(bulletEntity);
                auto bulletRigidbody = rigidbodySystem->getComponent(bulletEntity);

                bulletTransform->setPosition(transform->getPosition() + transform->getForward() * 3.f + transform->getRight() * side);
                bulletTransform->setRotation(transform->getRotation() * glm::angleAxis(glm::half_pi<float>(), glm::vec3 { 1.f, 0.f, 0.f }));

                bulletRigidbody->m_velocity = rigidbody->m_velocity + transform->getForward() * 100.f;

                side *= -1.f;
            }
        }
    }
};

#endif