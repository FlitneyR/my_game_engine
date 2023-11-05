#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <component.hpp>
#include <entity.hpp>
#include <unordered_map>
#include <unordered_set>

namespace mge::ecs {

class ECSManager;

class SystemBase {
public:
    ECSManager* r_ecsManager;

    virtual void removeComponent(const Entity& entity) = 0;
    virtual Component* getAnonymousComponent(const Entity& entity) = 0;
    virtual Component* addAnonymousComponent(const Entity& entity) = 0;
};

template<typename Component>
class System : public SystemBase {
public:
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

    virtual void removeComponent(const Entity& entity) override { m_components.erase(entity); }

    mge::ecs::Component* addAnonymousComponent(const Entity& entity) override { return addComponent(entity); }
    mge::ecs::Component* getAnonymousComponent(const Entity& entity) override { return getComponent(entity); }
};

}

#endif