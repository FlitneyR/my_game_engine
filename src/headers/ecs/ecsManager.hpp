#ifndef ECSMANAGER_HPP
#define ECSMANAGER_HPP

#include <system.hpp>
#include <entity.hpp>

#include <unordered_set>
#include <vector>

namespace mge::ecs {

class ECSManager {
    Entity nextEntityID = 0;

public:
    std::unordered_set<Entity> m_entities;
    std::vector<SystemBase*> m_systems;

    Entity makeEntity() { return *m_entities.insert(nextEntityID++).first; }

    void destroyEntity(const Entity& entity) {
        for (auto& system : m_systems)
            system->removeComponent(entity);
        m_entities.erase(entity);
    }
};

}

#endif