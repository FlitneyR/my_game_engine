#ifndef LIGHTINSTANCE_HPP
#define LIGHTINSTANCE_HPP

#include <component.hpp>
#include <ecsManager.hpp>
#include <transform.hpp>
#include <model.hpp>
#include <light.hpp>

namespace mge::ecs {

class LightComponent : public Component {
public:
    unsigned long m_instanceID;
};

class LightSystem : public System<LightComponent> {
public:
    mge::Light* r_light;

    LightComponent* addComponent(const Entity& entity) override {
        auto* comp = System<LightComponent>::addComponent(entity);
        comp->m_instanceID = r_light->makeInstance();
        return comp;
    }

    LightInstance* getInstance(const Entity& entity) {
        return &r_light->getInstance(getComponent(entity)->m_instanceID);
    }

    void updateTransforms() {
        auto transformSystem = r_ecsManager->getSystem<TransformComponent>("Transform");

        for (auto& [ entity, comp ] : m_components)
        if (const auto transform = transformSystem->getComponent(entity)) {
            auto instance = static_cast<LightInstance*>(&r_light->getInstance(comp.m_instanceID));
            instance->m_position = transform->getPosition();
            instance->m_direction = transform->getForward();
        }

        r_light->updateInstanceBuffer();
    }

    void removeComponent(const Entity& entity) override {
        if (auto comp = getComponent(entity))
            r_light->destroyInstance(comp->m_instanceID);
        
        System<LightComponent>::removeComponent(entity);
    }
};

}

#endif