#ifndef ECSMANAGER_HPP
#define ECSMANAGER_HPP

#include <system.hpp>
#include <entity.hpp>

#include <unordered_set>
#include <unordered_map>
#include <functional>
#include <vector>
#include <any>

namespace mge::ecs {

class ECSManager {
    Entity nextEntityID = 0;

public:
    std::unordered_set<Entity> m_entities;
    std::unordered_map<std::string, SystemBase*> m_systems;

    std::unordered_map<std::string, std::function<Entity(ECSManager&)>> m_templateEntities;

    Entity makeEntity() { return *m_entities.insert(nextEntityID++).first; }

    void addSystem(const std::string& name, SystemBase* system) {
        system->r_ecsManager = this;
        m_systems[name] = system;
    }

    template<typename Component>
    System<Component>* getSystem(const std::string& name) {
        return static_cast<System<Component>*>(m_systems.at(name));
    }

    void destroyEntity(const Entity& entity) {
        for (auto& [ _, system ] : m_systems)
            system->removeComponent(entity);
        m_entities.erase(entity);
    }

    void addTemplate(const std::string& name, std::function<Entity(ECSManager&)> func) {
        m_templateEntities[name] = func;
    }

    Entity makeEntityFromTemplate(const std::string& name) {
        auto func = m_templateEntities.at(name);

        return func(*this);
    }
};

}

#endif