#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <component.hpp>
#include <system.hpp>
#include <transform.hpp>

namespace mge::ecs {

class CollisionComponent : public Component {
public:
    enum Shape {
        e_sphere,
    };

    union ShapeParameters {
        struct SphereParameters {
            float m_radius;
        } u_sphereParameters;
    };

    struct CollisionEvent {
        Entity m_thisEntity, m_otherEntity;
        glm::vec3 m_normal, m_collisionPoint;
    };

    System<TransformComponent>& transformSystem;

private:
    Shape m_shape;
    ShapeParameters m_shapeParameters;

public:
    void setupSphere(float radius) {
        m_shape = e_sphere;
        m_shapeParameters.u_sphereParameters.m_radius = radius;
    }

    bool checkCollision(const CollisionComponent& other, CollisionEvent& event) const {
        switch (m_shape) {
        case e_sphere:
            switch (other.m_shape) {
            case e_sphere: return checkSphereSphereCollision(other, event);
            }
            break;
        }

        return false;
    }

    bool checkSphereSphereCollision(const CollisionComponent& other, CollisionEvent& event) const {
        float myRadius = m_shapeParameters.u_sphereParameters.m_radius;
        float otherRadius = other.m_shapeParameters.u_sphereParameters.m_radius;

        glm::vec3 myPosition = transformSystem.getComponent(m_parent)->getPosition();
        glm::vec3 otherPosition = transformSystem.getComponent(other.m_parent)->getPosition();

        float minDist = myRadius + otherRadius;
        float sqDist = glm::distance2(myPosition, otherPosition);

        if (sqDist > minDist * minDist) return false;

        event.m_thisEntity = m_parent;
        event.m_otherEntity = other.m_parent;
        event.m_normal = glm::normalize(myPosition - otherPosition);
        event.m_collisionPoint = (myPosition * otherRadius + otherPosition * myRadius ) / minDist;

        return true;
    }
};

class CollisionSystem : public System<CollisionComponent> {
public:
    std::vector<CollisionComponent::CollisionEvent> getCollisionEvents() const {
        std::vector<CollisionComponent::CollisionEvent> events;

        CollisionComponent::CollisionEvent event;
        for (auto [ thisEntity, thisComp ] : m_components)
        for (auto [ otherEntity, otherComp ] : m_components)
        if (thisEntity != otherEntity)
        if (thisComp.checkCollision(otherComp, event))
            events.push_back(event);

        return events;
    }
};

}

#endif