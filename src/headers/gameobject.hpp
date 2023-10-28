#ifndef GAMEOBJECT_HPP
#define GAMEOBJECT_HPP

#include <libraries.hpp>

namespace mge {

// class Component;
class GameObject;

class Component {
    friend GameObject;

protected:
    GameObject* r_gameObject;
    // std::vector<std::unique_ptr<Component>> m_children;

public:
    virtual void start() {
        // for (auto& child : m_children)
        //     child->start();
    }

    virtual void update(float deltaTime) {
        // for (auto& child : m_children)
        //     child->update(deltaTime);
    }

    virtual void physicsUpdate(float deltaTime) {
        // for (auto& child : m_children)
        //     child->physicsUpdate(deltaTime);
    }
};

class GameObject {
protected:
    std::vector<std::unique_ptr<Component>> m_children;

public:
    glm::vec3 m_position;
    glm::quat m_rotation;
    glm::vec3 m_scale;

    bool m_shouldDestroy = false;
    bool m_hasStarted = false;

    glm::vec3 getForward() {
        return glm::mat3(getTransform()) * glm::vec3(0, 1, 0);
    }
    
    glm::vec3 getRight() {
        return glm::mat3(getTransform()) * glm::vec3(1, 0, 0);
    }

    glm::vec3 getUp() {
        return glm::mat3(getTransform()) * glm::vec3(0, 0, 1);
    }

    glm::mat4 getTransform() {
        glm::mat4 result(1.f);

        result = glm::translate(result, m_position);
        result = glm::scale(result, m_scale);
        result = result * glm::toMat4(m_rotation);

        return result;
    }

    void scheduleDestruction() { m_shouldDestroy = true; }

    template<typename T>
    T* addComponent() {
        m_children.push_back(std::make_unique<T>(*this));
        return dynamic_cast<T*>(m_children.back().get());
    }

    template<typename T>
    T* getComponent() {
        for (auto& child : m_children)
        if (T* t = dynamic_cast<T*>(child.get())) return t;
        return nullptr;
    }

    virtual void start() {
        for (auto& child : m_children)
            child->start();
        m_hasStarted = true;
    }

    virtual void update(float deltaTime) {
        for (auto& child : m_children)
            child->update(deltaTime);
    }

    virtual void physicsUpdate(float deltaTime) {
        for (auto& child : m_children)
            child->physicsUpdate(deltaTime);
    }
};

template<typename Model>
class ModelInstance : public Component {
public:
    Model* r_model;
    uint32_t m_id;

    ModelInstance(GameObject& gameObject) {
        r_gameObject = &gameObject;
    }

    void update(float deltaTime) override {
        Component::update(deltaTime);
        r_model->getInstance(m_id).m_modelTransform = r_gameObject->getTransform();
    }
};

}

#endif