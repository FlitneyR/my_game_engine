#ifndef MODELINSTANCE_HPP
#define MODELINSTANCE_HPP

#include <component.hpp>
#include <system.hpp>
#include <transform.hpp>
#include <model.hpp>
#include <instance.hpp>

namespace mge::ecs {

class ModelComponent : public Component {
public:
    unsigned long m_instanceID;
};

class ModelSystem : public System<ModelComponent> {
public:
    ModelBase* r_model;
    System<TransformComponent>* r_transformSystem;

    ModelComponent* addComponent(const Entity& entity) override {
        auto* comp = System<ModelComponent>::addComponent(entity);
        comp->m_instanceID = r_model->makeInstance();
        return comp;
    }

    void updateTransforms() {
        for (auto& [ entity, comp ] : m_components)
        if (const auto transform = r_transformSystem->getComponent(entity)) {
            auto instance = static_cast<ModelTransformMeshInstance*>(&r_model->getInstance(comp.m_instanceID));
            instance->m_modelTransform = transform->getMat4();
        }

        r_model->updateInstanceBuffer();
    }

    void removeComponent(const Entity& entity) override {
        if (auto comp = getComponent(entity))
            r_model->destroyInstance(comp->m_instanceID);
        
        System<ModelComponent>::removeComponent(entity);
    }
};

}

#endif