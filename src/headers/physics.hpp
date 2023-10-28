#ifndef PHYSICS_HPP
#define PHYSICS_HPP
#include <optional>

#include <libraries.hpp>
#include <gameobject.hpp>

namespace mge {

struct AABB {
    glm::vec3 m_min;
    glm::vec3 m_max;

    bool overlaps(const AABB& other) const {
        return m_min.x < other.m_max.x
            && m_max.x > other.m_min.x
            && m_min.y < other.m_max.y
            && m_max.y > other.m_min.y
            && m_min.z < other.m_max.z
            && m_max.z > other.m_min.z;
    }
};

class RigidBody;

struct CollisionData {
    RigidBody *m_self;
    RigidBody *m_other;
    glm::vec3 m_normal;
    glm::vec3 m_impactPoint;

    void resolve();
};

class CollisionPrimitive {
public:
    glm::vec3 m_position;

    std::optional<AABB> m_aabb;

    virtual ~CollisionPrimitive() {}

    virtual AABB getAABB() = 0;
    virtual void invalidateAABB() { m_aabb.reset(); }

    virtual bool checkCollision(const CollisionPrimitive& other) {
        CollisionData ignore1, ignore2;
        return checkCollision(other, ignore1, ignore2);
    }

    virtual bool checkCollision(const CollisionPrimitive& other, CollisionData& data, CollisionData& otherData) const = 0;

    template<typename PrimitiveA, typename PrimitiveB>
    static bool checkCollision(const PrimitiveA& a, const PrimitiveB& b, CollisionData& da, CollisionData& db);
};

class RigidBody : public GameObject {
public:
    std::unique_ptr<CollisionPrimitive> m_collider;

    glm::vec3 m_linearAcceleration;
    glm::vec3 m_linearVelocity;
    glm::vec3 m_angularVelocity;
    float m_mass;

    void physicsUpdate(float deltaTime) override {
        GameObject::physicsUpdate(deltaTime);

        m_linearVelocity += m_linearAcceleration * deltaTime;
        m_position += m_linearVelocity * deltaTime;

        float angle = glm::length(m_angularVelocity) * deltaTime;
        glm::vec3 axis = glm::normalize(m_angularVelocity);
        m_rotation = glm::angleAxis(angle, axis) * m_rotation;

        m_collider->m_position = m_position;
    }

    virtual void onCollision(const CollisionData& data) {
        glm::vec3 relativeVelocity = data.m_other->m_linearVelocity - m_linearVelocity;

        float alignedVelocity = glm::dot(relativeVelocity, data.m_normal);
        float massRatio = 2.f * data.m_other->m_mass / (m_mass + data.m_other->m_mass);

        m_linearVelocity += alignedVelocity * massRatio * data.m_normal;
    }
};

void CollisionData::resolve() {
    m_self->onCollision(*this);
}

class BSPT {
public:
    std::vector<RigidBody*> m_children;
    std::unique_ptr<BSPT> m_left, m_right;
    int m_depth = 0;

    enum Axis { X, Y, Z };

    int m_maxDepth = 10;
    int m_minCount = 10;

    void split() {
        if (m_depth >= m_maxDepth) return;
        if (m_children.size() <= m_minCount) return;

        Axis axis;
        float splitPoint;
        selectSplitPlane(axis, splitPoint);

        m_left = std::make_unique<BSPT>();
        m_right = std::make_unique<BSPT>();

        for (auto& child : m_children) {
            AABB aabb = child->m_collider->getAABB();
            switch (axis) {
            case X:
                if (aabb.m_min.x < splitPoint) m_left->m_children.push_back(child);
                if (aabb.m_max.x > splitPoint) m_right->m_children.push_back(child);
                break;
            case Y:
                if (aabb.m_min.y < splitPoint) m_left->m_children.push_back(child);
                if (aabb.m_max.y > splitPoint) m_right->m_children.push_back(child);
                break;
            case Z:
                if (aabb.m_min.z < splitPoint) m_left->m_children.push_back(child);
                if (aabb.m_max.z > splitPoint) m_right->m_children.push_back(child);
                break;
            }
        }

        m_children.clear();

        m_left->m_depth = m_right->m_depth = m_depth + 1;

        m_left->split();
        m_right->split();
    }

    void selectSplitPlane(Axis& axis, float& splitPoint) {
        glm::vec3 mean(0.f);
        for (const auto& child : m_children) mean += child->m_position;
        mean /= static_cast<float>(m_children.size());

        glm::vec3 variance(0.f);
        for (const auto& child : m_children) {
            glm::vec3 sqDiff = child->m_position - mean;
            sqDiff *= sqDiff;
            variance += sqDiff;
        }
        variance /= static_cast<float>(m_children.size() - 1);

        axis = X;
        if (variance.y > variance.x) axis = Y;
        if ((axis == X && variance.z > variance.x) || (axis == Y && variance.z > variance.y)) axis = Z;

        switch (axis) {
        case X: splitPoint = mean.x; break;
        case Y: splitPoint = mean.y; break;
        case Z: splitPoint = mean.z; break;
        }
    }

    void generateCollisions(std::vector<CollisionData>& collisions) {
        CollisionData da, db;

        for (const auto& a : m_children)
        for (const auto& b : m_children)
        if (a != b && a->m_collider->checkCollision(*b->m_collider, da, db)) {
            da.m_self = db.m_other = a;
            db.m_self = da.m_other = b;
            collisions.push_back(da);
            collisions.push_back(db);
        }

        if (m_left) m_left->generateCollisions(collisions);
        if (m_right) m_right->generateCollisions(collisions);
    }
};

class SphereCollider : public CollisionPrimitive {
public:
    float m_radius;

    SphereCollider() = default;

    SphereCollider(glm::vec3 position, float radius) :
        m_radius(radius)
    {
        m_position = position;
    }

    AABB getAABB() override {
        if (!m_aabb) {
            m_aabb->m_min = m_aabb->m_max = m_position;
            m_aabb->m_min.x -= m_radius;
            m_aabb->m_min.y -= m_radius;
            m_aabb->m_min.z -= m_radius;
            m_aabb->m_max.x += m_radius;
            m_aabb->m_max.y += m_radius;
            m_aabb->m_max.z += m_radius;
        }

        return *m_aabb;
    }

    bool checkCollision(const CollisionPrimitive& other, CollisionData& data, CollisionData& otherData) const override;
};

template<>
bool CollisionPrimitive::checkCollision(const SphereCollider& a, const SphereCollider& b, CollisionData& da, CollisionData& db) {
    float minDistance = a.m_radius + b.m_radius;
    if (glm::distance2(a.m_position, b.m_position) > minDistance * minDistance) return false;

    da.m_impactPoint = db.m_impactPoint = (a.m_position * b.m_radius + b.m_position * a.m_radius) / minDistance;

    da.m_normal = glm::normalize(a.m_position - b.m_position);
    db.m_normal = -da.m_normal;

    return true;
}

bool SphereCollider::checkCollision(const CollisionPrimitive& other, CollisionData& data, CollisionData& otherData) const {
    if (auto _other = dynamic_cast<const SphereCollider*>(&other))
        return CollisionPrimitive::checkCollision(*this, *_other, data, otherData);

    return false;
}

}

#endif