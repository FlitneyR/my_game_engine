#include <engine.hpp>
#include <model.hpp>
#include <camera.hpp>

#include <memory>

class TestGame : public mge::Engine {
    typedef mge::Model<
        mge::PointColorVertex,
        mge::ModelTransformMeshInstance,
        mge::SimpleMaterialInstance,
        mge::Camera
        > Model;

    std::unique_ptr<mge::Camera> m_camera;

    std::unique_ptr<Model> m_model;
    std::unique_ptr<Model::Mesh> m_mesh;
    std::unique_ptr<Model::Material> m_material;
    
    void start() override {
        m_camera = std::make_unique<mge::Camera>(*this);

        m_mesh = std::make_unique<Model::Mesh>(*this,
            std::vector<mge::PointColorVertex> {
                {{ -1.f, -1.f, -1.f }, { 0.f, 0.f, 0.f, 1.f }},
                {{  1.f, -1.f, -1.f }, { 1.f, 0.f, 0.f, 1.f }},
                {{ -1.f,  1.f, -1.f }, { 0.f, 1.f, 0.f, 1.f }},
                {{  1.f,  1.f, -1.f }, { 1.f, 1.f, 0.f, 1.f }},
                {{ -1.f, -1.f,  1.f }, { 0.f, 0.f, 1.f, 1.f }},
                {{  1.f, -1.f,  1.f }, { 1.f, 0.f, 1.f, 1.f }},
                {{ -1.f,  1.f,  1.f }, { 0.f, 1.f, 1.f, 1.f }},
                {{  1.f,  1.f,  1.f }, { 1.f, 1.f, 1.f, 1.f }},
                },
            std::vector<uint16_t>{
                0, 2, 1,    2, 3, 1,    // front face
                4, 5, 6,    7, 6, 5,    // back face
                2, 6, 3,    6, 7, 3,    // top face
                0, 1, 4,    1, 5, 4,    // bottom face
                6, 2, 0,    0, 4, 6,    // left face
                1, 3, 7,    7, 5, 1,    // right face
                }
            );

        m_material = std::make_unique<Model::Material>(*this,
            loadShaderModule("build/test.vert.spv"),
            loadShaderModule("build/test.frag.spv")
            );

        m_model = std::make_unique<Model>(
            *this,
            *m_mesh,
            *m_material,
            m_material->makeInstance()
            );

        for (int j = -4; j <= 4; j++)
        for (int i = -4; i <= 4; i++) {
            auto index = m_model->makeInstance();
            auto& inst = m_model->getInstance(index);
            auto& transform = inst.m_modelTransform;
            
            transform = glm::translate(transform, glm::vec3(3.f * i, 0.f, 3.f * j));
        }

        m_camera->setup();
        m_mesh->setup();
        m_material->setup();
        m_model->setup();
    }

    void recordDrawCommands(vk::CommandBuffer cmd) override {
        m_material->bindUniform(cmd, *m_camera);
        m_model->drawInstances(cmd);
    }

    void update(float time, float deltaTime) override {
        for (auto& [ID, inst] : m_model->m_meshInstances) {
            auto& transform = inst.m_modelTransform;
            transform[3][1] = glm::sin(time * 0.33f + (float)ID * 0.5f);
        }

        m_camera->m_position = glm::vec3(20.f * glm::sin(time * 0.5f), 7.f, 20.f * glm::cos(time * 0.5f));
        m_camera->m_forward = -m_camera->m_position + glm::vec3(0.f, -5.f, 0.f);

        m_model->updateInstanceBuffer();
        m_camera->updateBuffer();

        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    void end() override {
        m_camera->cleanup();
        m_model->cleanup();
        m_mesh->cleanup();
        m_material->cleanup();
    }
};
