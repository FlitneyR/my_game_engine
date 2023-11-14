#ifndef LIGHTINSTANCE_HPP
#define LIGHTINSTANCE_HPP

#include <component.hpp>
#include <ecsManager.hpp>
#include <transform.hpp>
#include <model.hpp>
#include <light.hpp>

#include <optional>

namespace mge::ecs {

class LightComponent : public Component {
public:
    unsigned long m_instanceID;
    std::optional<int> m_shadowMapID;
};

class LightSystem : public System<LightComponent> {
    int m_nextShadowMapID = 0;

public:
    mge::Light* r_shadowlessLight;
    mge::ShadowMappedLight* r_shadowMappedLightPrototype;
    std::unordered_map<int, std::unique_ptr<mge::ShadowMappedLight>> m_shadowMappedLights;

    LightComponent* addComponent(const Entity& entity) override {
        r_ecsManager->getSystem<TransformComponent>("Transform")->addComponent(entity);

        auto* comp = System<LightComponent>::addComponent(entity);
        comp->m_instanceID = r_shadowlessLight->makeInstance();
        return comp;
    }

    LightComponent* addComponentShadowMapped(const Entity& entity, uint32_t shadowMapResolution) {
        r_ecsManager->getSystem<TransformComponent>("Transform")->addComponent(entity);

        auto* comp = System<LightComponent>::addComponent(entity);

        auto materialInstance = r_shadowMappedLightPrototype->m_material->makeInstance();
        materialInstance.setup(shadowMapResolution, shadowMapResolution);

        auto light = std::make_unique<mge::ShadowMappedLight>(
            *r_ecsManager->r_engine,
            *r_shadowMappedLightPrototype->m_mesh,
            *r_shadowMappedLightPrototype->m_material,
            materialInstance
        );

        comp->m_shadowMapID = m_nextShadowMapID;
        comp->m_instanceID = light->makeInstance();

        light->setup();

        std::swap(m_shadowMappedLights[m_nextShadowMapID++], light);

        return comp;
    }

    LightInstance* getInstance(const Entity& entity) {
        return getInstance(*getComponent(entity));
    }

    LightInstance* getInstance(const mge::ecs::LightComponent& component) {
        return static_cast<LightInstance*>(&getLight(component)->getInstance(component.m_instanceID));
    }

    mge::ModelBase* getLight(const LightComponent& component) {
        if (component.m_shadowMapID) {
            return m_shadowMappedLights.at(*component.m_shadowMapID).get();
        } else return r_shadowlessLight;
    }

    void updateTransforms() {
        auto transformSystem = r_ecsManager->getSystem<TransformComponent>("Transform");

        for (auto& [ entity, comp ] : m_components)
        if (const auto transform = transformSystem->getComponent(entity)) {
            auto instance = getInstance(comp);
            instance->m_position = transform->getPosition();
            instance->m_direction = transform->getForward();
        }

        r_shadowlessLight->updateInstanceBuffer();
    }

    void removeComponent(const Entity& entity) override {
        if (auto comp = getComponent(entity))
            r_shadowlessLight->destroyInstance(comp->m_instanceID);
        
        System<LightComponent>::removeComponent(entity);
    }
};

}

#endif