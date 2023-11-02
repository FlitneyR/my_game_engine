#ifndef SYSTEM_HPP
#define SYSTEM_HPP

#include <entity.hpp>
#include <unordered_map>

namespace mge::ecs {

class SystemBase {};

template<typename Component>
class System {
protected:
    std::unordered_map<Entity, Component> m_components;

public:
    virtual Component* addComponent(const Entity& entity) {
        return &m_components[entity];
    }

    virtual Component* getComponent(const Entity& entity) {
        if (!m_components.contains(entity)) return nullptr;
        return &m_components[entity];
    }
};

}

#endif