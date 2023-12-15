#include <collision.hpp>

namespace mge::ecs {

bool AABB::checkIntersection(const AABB& other) const {
    bool xIntersection = !(m_maxX < other.m_minX && m_minX > other.m_maxX);
    bool yIntersection = !(m_maxY < other.m_minY && m_minY > other.m_maxY);
    bool zIntersection = !(m_maxZ < other.m_minZ && m_minZ > other.m_maxZ);
    return xIntersection && yIntersection && zIntersection;
}

bool Collider::checkCollisionAlongDirection(const Collider& other, const glm::vec3& normal, CollisionEvent& event) const {
    glm::vec3 antiNormal = -normal;

    glm::vec3 A = getSupportPoint(antiNormal);
    glm::vec3 B = getSupportPoint(normal);
    glm::vec3 C = other.getSupportPoint(antiNormal);
    glm::vec3 D = other.getSupportPoint(normal);

    float a = glm::dot(normal, A);
    float b = glm::dot(normal, B);
    float c = glm::dot(normal, C);
    float d = glm::dot(normal, D);

    if (a < c && b > c) {
        event.m_normal = antiNormal;
        event.m_collisionDepth = b - c;
        event.m_collisionPoint = B + normal * event.m_collisionDepth;
        return true;
    }

    if (c < a && d > a) {
        event.m_normal = normal;
        event.m_collisionDepth = d - a;
        event.m_collisionPoint = A + normal * event.m_collisionDepth;
        return true;
    }

    return false;
}

bool Collider::checkCollisionSAT(const Collider& other, CollisionEvent& event) const {
    std::vector<glm::vec3> normalsToCheck;

    addNormalsToVector(normalsToCheck, other);
    other.addNormalsToVector(normalsToCheck, *this);

    bool first = true;
    for (const glm::vec3& normal : normalsToCheck) {
        CollisionEvent _event;
        if (!checkCollisionAlongDirection(other, normal, first ? event : _event))
            return false;
        
        if (!first)
        if (_event.m_collisionDepth < event.m_collisionDepth)
            event = _event;

        first = false;
    }

    return true;
}

bool Collider::checkCollision(const Collider& other, CollisionEvent& event) const {
    if (!getAABB().checkIntersection(other.getAABB())) return false;
    return checkCollisionSAT(other, event);
}

AABB SphereCollider::getAABB() const {
    AABB result;

    glm::vec3 position = r_transform->getPosition();

    result.m_minX = position.x - m_radius;
    result.m_maxX = position.x + m_radius;
    result.m_minY = position.y - m_radius;
    result.m_maxY = position.y + m_radius;
    result.m_minZ = position.z - m_radius;
    result.m_maxZ = position.z + m_radius;

    return result;
}

glm::vec3 SphereCollider::getSupportPoint(const glm::vec3& direction) const {
    return r_transform->getPosition() + direction * m_radius;
}

glm::vec3 SphereCollider::getClosestPoint(const glm::vec3& position) const {
    return getSupportPoint(glm::normalize(position - r_transform->getPosition()));
}

void SphereCollider::addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const {
    glm::vec3 position = r_transform->getPosition();
    glm::vec3 otherClosestPoint = other.getClosestPoint(position);
    normals.push_back(glm::normalize(otherClosestPoint - position));
}

AABB CapsuleCollider::getAABB() const {
    AABB result;
    glm::vec3 position = r_transform->getPosition();

    result.m_minX = result.m_maxX = position.x;
    result.m_minY = result.m_maxY = position.y;
    result.m_minZ = result.m_maxZ = position.z;

    result.m_minX = std::min(result.m_minX, getSupportPoint(glm::vec3 { -1,  0,  0 }).x);
    result.m_maxX = std::max(result.m_maxX, getSupportPoint(glm::vec3 {  1,  0,  0 }).x);
    result.m_minY = std::min(result.m_minY, getSupportPoint(glm::vec3 {  0, -1,  0 }).y);
    result.m_maxY = std::max(result.m_maxY, getSupportPoint(glm::vec3 {  0,  1,  0 }).y);
    result.m_minZ = std::min(result.m_minZ, getSupportPoint(glm::vec3 {  0,  0, -1 }).z);
    result.m_maxZ = std::max(result.m_maxZ, getSupportPoint(glm::vec3 {  0,  0,  1 }).z);
}

glm::vec3 CapsuleCollider::getSupportPoint(const glm::vec3& direction) const {
    glm::vec3 centre = r_transform->getPosition();
    glm::vec3 topPoint = centre + r_transform->getUp() * m_halfHeight;
    glm::vec3 bottomPoint = centre - r_transform->getUp() * m_halfHeight;

    float topValue = glm::dot(direction, topPoint);
    float bottomValue = glm::dot(direction, bottomPoint);

    glm::vec3 point = topValue > bottomValue ? topPoint : bottomPoint;

    return point + m_radius * direction;
}

glm::vec3 CapsuleCollider::getClosestPoint(const glm::vec3& position) const {
    glm::vec3 centre = r_transform->getPosition();
    glm::vec3 topPoint = centre + r_transform->getUp() * m_halfHeight;
    glm::vec3 bottomPoint = centre - r_transform->getUp() * m_halfHeight;

    float topDistance = glm::distance2(topPoint, position);
    float bottomDistance = glm::distance2(bottomPoint, position);

    glm::vec3 point = topDistance < bottomDistance ? topPoint : bottomPoint;

    glm::vec3 direction = position - point;
    return point + glm::normalize(direction) * m_radius;
}

void CapsuleCollider::addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const {
    glm::vec3 centre = r_transform->getPosition();
    glm::vec3 topPoint = centre + r_transform->getUp() * m_halfHeight;
    glm::vec3 bottomPoint = centre - r_transform->getUp() * m_halfHeight;

    glm::vec3 topClosestPoint = other.getClosestPoint(topPoint);
    glm::vec3 bottomClosestPoint = other.getClosestPoint(bottomPoint);

    float topDistance = glm::distance2(topPoint, topClosestPoint);
    float bottomDistance = glm::distance2(bottomPoint, bottomClosestPoint);

    if (topDistance < bottomDistance) {
        normals.push_back(topClosestPoint - topPoint);
    } else {
        normals.push_back(bottomClosestPoint - bottomPoint);
    }
}

std::array<glm::vec3, 8> OBBCollider::getModelSpaceCorners() const {
    float halfWidth = m_width / 2.f;
    float halfHeight = m_height / 2.f;
    float halfDepth = m_depth / 2.f;

    return std::array<glm::vec3, 8> {
        glm::vec3 { -halfWidth, -halfHeight, -halfDepth },
        glm::vec3 {  halfWidth, -halfHeight, -halfDepth },
        glm::vec3 { -halfWidth,  halfHeight, -halfDepth },
        glm::vec3 {  halfWidth,  halfHeight, -halfDepth },
        glm::vec3 { -halfWidth, -halfHeight,  halfDepth },
        glm::vec3 {  halfWidth, -halfHeight,  halfDepth },
        glm::vec3 { -halfWidth,  halfHeight,  halfDepth },
        glm::vec3 {  halfWidth,  halfHeight,  halfDepth },
    };
}

std::array<glm::vec3, 8> OBBCollider::getWorldSpaceCorners() const {
    std::array<glm::vec3, 8> modelSpaceCorners = getModelSpaceCorners();
    std::array<glm::vec3, 8> worldSpaceCorners;

    glm::mat4 transform = r_transform->getMat4();
    for (int i = 0; i < 8; i++) {
        glm::vec4 modelSpaceCorner { modelSpaceCorners[i], 1.f };
        glm::vec4 worldSpaceCorner = transform * modelSpaceCorner;
        worldSpaceCorner /= worldSpaceCorner.w;
        worldSpaceCorners[i] = glm::vec3 { worldSpaceCorner };
    }
    
    return worldSpaceCorners;
}

AABB OBBCollider::getAABB() const {
    AABB result;
    bool firstCorner = true;

    for (auto corner : getWorldSpaceCorners()) {
        if (firstCorner) {
            result.m_minX = result.m_maxX = corner.x;
            result.m_minY = result.m_maxY = corner.y;
            result.m_minZ = result.m_maxZ = corner.z;
        } else {
            result.m_minX = std::min(result.m_minX, corner.x);
            result.m_minY = std::min(result.m_minY, corner.y);
            result.m_minZ = std::min(result.m_minZ, corner.z);
            result.m_maxX = std::max(result.m_maxX, corner.x);
            result.m_maxY = std::max(result.m_maxY, corner.y);
            result.m_maxZ = std::max(result.m_maxZ, corner.z);
        }

        firstCorner = false;
    }

    return result;
}

glm::vec3 OBBCollider::getSupportPoint(const glm::vec3& direction) const {
    auto worldSpaceCorners = getWorldSpaceCorners();

    glm::vec3 result = worldSpaceCorners[0];
    float maxValue = glm::dot(result, direction);

    for (int i = 1; i < 8; i++) {
        glm::vec3 corner = worldSpaceCorners[i];
        float value = glm::dot(corner, direction);

        if (value > maxValue) {
            maxValue = value;
            result = corner;
        }
    }

    return result;
}

glm::vec3 OBBCollider::getClosestPoint(const glm::vec3& position) const {
    auto worldSpaceCorners = getWorldSpaceCorners();

    glm::vec3 result = worldSpaceCorners[0];
    float minValue = glm::distance2(result, position);

    for (int i = 1; i < 8; i++) {
        glm::vec3 corner = worldSpaceCorners[i];
        float value = glm::distance2(corner, position);

        if (value < minValue) {
            minValue = value;
            result = corner;
        }
    }

    return result;
}

void OBBCollider::addNormalsToVector(std::vector<glm::vec3>& normals, const Collider& other) const {
    normals.push_back(r_transform->getForward());
    normals.push_back(r_transform->getUp());
    normals.push_back(r_transform->getRight());
}

void CollisionSystem::BSPT::findSplitPlane(CollisionSystem::BSPT::Axis& axis, float& distance) {
    glm::vec3 variance, meanPosition;

    for (const auto& child : m_children)
        meanPosition += child->m_collider->r_transform->getPosition();
    meanPosition /= static_cast<float>(m_children.size());

    for (const auto& child : m_children) {
        glm::vec3 diff = child->m_collider->r_transform->getPosition() - meanPosition;
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

void CollisionSystem::BSPT::split(int depth) {
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

void CollisionSystem::BSPT::generateCollisionEvents(std::vector<CollisionEvent>& collisionEvents) {
    if (isLeaf()) {
        CollisionEvent event;
        for (auto& collider1 : m_children)
        for (auto& collider2 : m_children)
        if (collider1->m_entity != collider2->m_entity)
        if (collider1->checkCollision(*collider2, event)) {
            event.m_thisEntity = collider1->m_entity;
            event.m_otherEntity = collider2->m_entity;
            collisionEvents.push_back(event);
        }
    } else {
        if (m_left) m_left->generateCollisionEvents(collisionEvents);
        if (m_right) m_right->generateCollisionEvents(collisionEvents);
    }
}

std::vector<CollisionEvent> CollisionSystem::getCollisionEvents() {
    auto transformSystem = r_ecsManager->getSystem<TransformComponent>("Transform");

    BSPT bspt;
    for (auto& [ entity, comp ] : m_components) {
        comp.m_collider->r_transform = transformSystem->getComponent(entity);
        bspt.m_children.push_back(&comp);
    }

    bspt.split();

    std::vector<CollisionEvent> events;

    bspt.generateCollisionEvents(events);

    return events;
}

}