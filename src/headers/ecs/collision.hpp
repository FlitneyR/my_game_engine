#ifndef COLLISION_HPP
#define COLLISION_HPP

#include <component.hpp>
#include <system.hpp>
#include <ecsManager.hpp>
#include <transform.hpp>
#include <iostream>
#include <memory>

namespace mge::ecs {

struct CollisionEvent {
    Entity m_thisEntity, m_otherEntity;
    glm::vec3 m_normal, m_collisionPoint;
    float m_collisionDepth;
};

struct AABB {
    float m_minX, m_maxX;
    float m_minY, m_maxY;
    float m_minZ, m_maxZ;

    bool checkIntersection(const AABB& other) const;
};

class Collider {
public:
    TransformComponent* r_transform;

    virtual ~Collider() = default;

    /**
     * @brief Get the Support Point for the object
     * 
     * @param direction MUST BE NORMALIZED
     * @return glm::vec3 The furthest point within the collider in the given direction
     */
    virtual glm::vec3 getSupportPoint(const glm::vec3& direction) const = 0;
    virtual glm::vec3 getClosestPoint(const glm::vec3& position) const = 0;
    virtual void addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const = 0;

    virtual AABB getAABB() const = 0;

    bool checkCollisionAlongDirection(const Collider& other, const glm::vec3& normal, CollisionEvent& event) const;
    bool checkCollisionSAT(const Collider& other, CollisionEvent& event) const;
    bool checkCollision(const Collider& other, CollisionEvent& event) const;
};

class SphereCollider : public Collider {
public:
    float m_radius;

    SphereCollider(float radius) : m_radius(radius) {}

    AABB getAABB() const override;
    glm::vec3 getSupportPoint(const glm::vec3& direction) const override;
    glm::vec3 getClosestPoint(const glm::vec3& position) const override;
    void addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const override;
};

class CapsuleCollider : public Collider {
public:
    float m_radius;
    float m_halfHeight;

    CapsuleCollider(float radius, float halfHeight) : m_radius(radius), m_halfHeight(halfHeight) {}

    AABB getAABB() const override;
    glm::vec3 getSupportPoint(const glm::vec3& direction) const override;
    glm::vec3 getClosestPoint(const glm::vec3& position) const override;
    void addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const override;
};

class OBBCollider : public Collider {
public:
    float m_width;
    float m_height;
    float m_depth;

    OBBCollider(float width, float height, float depth) :
        m_width(width),
        m_height(height),
        m_depth(depth)
    {}

    std::array<glm::vec3, 8> getModelSpaceCorners() const;
    std::array<glm::vec3, 8> getWorldSpaceCorners() const;

    AABB getAABB() const override;
    glm::vec3 getSupportPoint(const glm::vec3& direction) const override;
    glm::vec3 getClosestPoint(const glm::vec3& position) const override;
    void addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const override;
};

class CollisionComponent : public Component {
public:
    TransformComponent* r_transform;
    std::unique_ptr<Collider> m_collider;

    template<typename ColliderType>
    void setCollider(ColliderType collider) {
        m_collider = std::make_unique<ColliderType>(collider);
        m_collider->r_transform = r_transform;
    }

    AABB getAABB() const {
        return m_collider->getAABB();
    }

    bool checkCollision(const CollisionComponent& other, CollisionEvent& event) const {
        return m_collider->checkCollision(*other.m_collider, event);
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

        void findSplitPlane(Axis& axis, float& distance);

    public:
        void split(int depth = 0);
        bool isLeaf() { return !(m_left || m_right); }
        void generateCollisionEvents(std::vector<CollisionEvent>& collisionEvents);
    };

    std::vector<CollisionEvent> getCollisionEvents();
};

}

#endif