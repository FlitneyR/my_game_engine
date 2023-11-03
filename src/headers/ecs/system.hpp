#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <entity.hpp>
#include <unordered_map>
#include <unordered_set>

namespace mge::ecs {

class ECSManager;

class SystemBase {
public:
    virtual void removeComponent(const Entity& entity) = 0;
};

template<typename Component>
class System : public SystemBase {
public:
    ECSManager* r_ecsManager;

    std::unordered_map<Entity, Component> m_components;

// public:
    virtual Component* addComponent(const Entity& entity) {
        auto result = &m_components[entity];
        result->m_entity = entity;
        return result;
    }

    virtual Component* getComponent(const Entity& entity) {
        if (!m_components.contains(entity)) return nullptr;
        return &m_components[entity];
    }

    virtual void removeComponent(const Entity& entity) {
        m_components.erase(entity);
    }
};

}

#endif