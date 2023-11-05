#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <component.hpp>
#include <system.hpp>
#include <ecsManager.hpp>
#include <transform.hpp>
#include <iostream>

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
        float m_collisionDepth;
    };

    struct AABB {
        float m_minX, m_maxX;
        float m_minY, m_maxY;
        float m_minZ, m_maxZ;
    };

    TransformComponent* r_transform;

    Shape m_shape;
    ShapeParameters m_shapeParameters;

public:
    AABB getAABB() const {
        AABB aabb;

        glm::vec3 position = r_transform->getPosition();
        
        switch (m_shape) {
        case e_sphere: {
            float radius = m_shapeParameters.u_sphereParameters.m_radius;
            aabb.m_minX = position.x - radius;
            aabb.m_maxX = position.x + radius;
            aabb.m_minY = position.y - radius;
            aabb.m_maxY = position.y + radius;
            aabb.m_minZ = position.z - radius;
            aabb.m_maxZ = position.z + radius;
        }   break;
        }

        return aabb;
    }

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

        glm::vec3 myPosition = r_transform->getPosition();
        glm::vec3 otherPosition = other.r_transform->getPosition();

        float minDist = myRadius + otherRadius;
        float sqDist = glm::distance2(myPosition, otherPosition);

        if (sqDist > minDist * minDist) return false;

        event.m_thisEntity = m_entity;
        event.m_otherEntity = other.m_entity;
        event.m_normal = glm::normalize(myPosition - otherPosition);
        event.m_collisionPoint = (myPosition * otherRadius + otherPosition * myRadius ) / minDist;
        event.m_collisionDepth = myRadius - glm::distance(myPosition, event.m_collisionPoint);

        return true;
    }
};

class CollisionSystem : public System<CollisionComponent> {
public:
    class BSPT {
    public:
        std::unique_ptr<BSPT> m_left, m_right;
        std::vector<CollisionComponent*> m_children;

        static constexpr uint32_t MAX_DEPTH = 10;
        static constexpr uint32_t MIN_CHILDREN = 10;

    private:
        enum Axis { X, Y, Z };

        void findSplitPlane(Axis& axis, float& distance) {
            glm::vec3 variance, meanPosition;

            for (const auto& child : m_children)
                meanPosition += child->r_transform->getPosition();
            meanPosition /= static_cast<float>(m_children.size());

            for (const auto& child : m_children) {
                glm::vec3 diff = child->r_transform->getPosition() - meanPosition;
                variance += diff * diff;
            }

            variance /= static_cast<float>(m_children.size());

            axis = X;
            if (variance.y > variance.x) axis = Y;
            if (variance.z > (axis == X ? variance.x : variance.y)) axis = Z;

            switch (axis) {
            case X: distance = meanPosition.x; break;
            case Y: distance = meanPosition.y; break;
            case Z: distance = meanPosition.z; break;
            }
        }

    public:
        void split(int depth = 0) {
            if (depth >= MAX_DEPTH || m_children.size() <= MIN_CHILDREN) return;

            m_left = std::make_unique<BSPT>();
            m_right = std::make_unique<BSPT>();

            Axis axis;
            float distance;
            findSplitPlane(axis, distance);

            for (const auto child : m_children) {
                bool onLeft = false;
                bool onRight = false;
                
                auto aabb = child->getAABB();

                switch (axis) {
                case X:
                    onLeft = aabb.m_minX < distance;
                    onRight = aabb.m_maxX > distance;
                    break;
                case Y:
                    onLeft = aabb.m_minY < distance;
                    onRight = aabb.m_maxY > distance;
                    break;
                case Z:
                    onLeft = aabb.m_minZ < distance;
                    onRight = aabb.m_maxZ > distance;
                    break;
                }

                if (onLeft) m_left->m_children.push_back(child);
                if (onRight) m_right->m_children.push_back(child);
            }

            m_children.clear();

            m_left->split(depth + 1);
            m_right->split(depth + 1);
        }

        bool isLeaf() { return !(m_left || m_right); }

        void generateCollisionEvents(std::vector<CollisionComponent::CollisionEvent>& collisionEvents) {
            if (isLeaf()) {
                CollisionComponent::CollisionEvent event;
                for (auto& collider1 : m_children)
                for (auto& collider2 : m_children)
                if (collider1->m_entity != collider2->m_entity)
                if (collider1->checkCollision(*collider2, event))
                    collisionEvents.push_back(event);
            } else {
                if (m_left) m_left->generateCollisionEvents(collisionEvents);
                if (m_right) m_right->generateCollisionEvents(collisionEvents);
            }
        }
    };

    std::vector<CollisionComponent::CollisionEvent> getCollisionEvents() {
        auto transformSystem = r_ecsManager->getSystem<TransformComponent>("Transform");

        BSPT bspt;
        for (auto& [ entity, comp ] : m_components) {
            comp.r_transform = transformSystem->getComponent(entity);
            bspt.m_children.push_back(&comp);
        }

        bspt.split();

        std::vector<CollisionComponent::CollisionEvent> events;

        bspt.generateCollisionEvents(events);

        return events;
    }
};

}

#endif