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
    std::string m_modelName;
};

class ModelSystem : public System<ModelComponent> {
public:
    std::unordered_map<std::string, ModelBase*> r_models;

    void addModel(std::string name, ModelBase* model) {
        r_models[name] = model;
    }

    ModelComponent* addComponent(const Entity& entity, const std::string modelName) {
        auto* comp = System<ModelComponent>::addComponent(entity);
        comp->m_modelName = modelName;
        comp->m_instanceID = r_models.at(modelName)->makeInstance();
        return comp;
    }

    void updateTransforms() {
        auto transformSystem = r_ecsManager->getSystem<TransformComponent>("Transform");

        for (auto& [ entity, comp ] : m_components)
        if (const auto transform = transformSystem->getComponent(entity)) {
            auto instance = static_cast<ModelTransformMeshInstance*>(&r_models.at(comp.m_modelName)->getInstance(comp.m_instanceID));
            instance->m_modelTransform = transform->getMat4();
        }

        for (auto [ _, model ] : r_models)
            model->updateInstanceBuffer();
    }

    void removeComponent(const Entity& entity) override {
        if (auto comp = getComponent(entity))
            r_models.at(comp->m_modelName)->destroyInstance(comp->m_instanceID);
        
        System<ModelComponent>::removeComponent(entity);
    }
};

}

#endif