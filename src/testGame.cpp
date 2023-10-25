#include <engine.hpp>
#include <model.hpp>
#include <camera.hpp>
#include <objloader.hpp>

#include <memory>

class TestGame : public mge::Engine {
    typedef mge::Model<
        mge::ModelVertex,
        mge::ModelTransformMeshInstance,
        // mge::SimpleMaterialInstance,
        mge::SingleTextureMaterialInstance,
        mge::Camera
        > Model;

    std::unique_ptr<Model> m_model;
    std::unique_ptr<Model::Mesh> m_mesh;
    std::unique_ptr<mge::Camera> m_camera;
    std::unique_ptr<Model::Material> m_material;
    std::unique_ptr<Model::Material::Instance> m_materialInstance;
    mge::Texture m_texture;
    
    void start() override {
        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_far = 10'000.f;

        // m_mesh = std::make_unique<Model::Mesh>(*this,
        //     std::vector<mge::PointColorVertex> {
        //         {{ -1.f, -1.f, -1.f }, { 0.f, 0.f, 0.f, 1.f }},
        //         {{  1.f, -1.f, -1.f }, { 1.f, 0.f, 0.f, 1.f }},
        //         {{ -1.f,  1.f, -1.f }, { 0.f, 1.f, 0.f, 1.f }},
        //         {{  1.f,  1.f, -1.f }, { 1.f, 1.f, 0.f, 1.f }},
        //         {{ -1.f, -1.f,  1.f }, { 0.f, 0.f, 1.f, 1.f }},
        //         {{  1.f, -1.f,  1.f }, { 1.f, 0.f, 1.f, 1.f }},
        //         {{ -1.f,  1.f,  1.f }, { 0.f, 1.f, 1.f, 1.f }},
        //         {{  1.f,  1.f,  1.f }, { 1.f, 1.f, 1.f, 1.f }},
        //         },
        //     std::vector<uint16_t>{
        //         0, 2, 1,    2, 3, 1,    // front face
        //         4, 5, 6,    7, 6, 5,    // back face
        //         2, 6, 3,    6, 7, 3,    // top face
        //         0, 1, 4,    1, 5, 4,    // bottom face
        //         6, 2, 0,    0, 4, 6,    // left face
        //         1, 3, 7,    7, 5, 1,    // right face
        //         }
        //     );

        m_mesh = std::make_unique<mge::Mesh<mge::ModelVertex>>(mge::loadObjMesh(*this, "assets/monkey.obj"));

        m_texture = mge::Texture("assets/texture_09.png");

        m_material = std::make_unique<Model::Material>(*this,
            loadShaderModule("build/test.vert.spv"),
            loadShaderModule("build/test.frag.spv")
            );

        m_materialInstance = std::make_unique<Model::Material::Instance>(m_material->makeInstance());
        m_materialInstance->setup(m_texture);

        m_model = std::make_unique<Model>(
            *this,
            *m_mesh,
            *m_material,
            *m_materialInstance
            );

        for (float r = 0.1f; r < 30.f; r += 3.f) {
            float circumference = r * 6.28f;
            float count = glm::floor(circumference / 3.f);
            float stepSize = 6.28f / count;
            for (float theta = 0; theta < 6.28f; theta += stepSize) {
                auto index = m_model->makeInstance();
                auto& inst = m_model->getInstance(index);
                auto& mat = inst.m_modelTransform;

                mat = glm::rotate(mat, theta, glm::vec3(0.f, 1.f, 0.f));
                mat = glm::translate(mat, glm::vec3(r, 0.f, 0.f));
                mat = glm::rotate(mat, -glm::half_pi<float>(), glm::vec3(0.f, 1.f, 0.f));
            }
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

    void update(double time, double deltaTime) override {
        for (auto& [ID, inst] : m_model->m_meshInstances) {
            auto& transform = inst.m_modelTransform;

            glm::vec3 position = transform[3];

            double theta = glm::atan(position.x, position.z);
            double radius = glm::length(glm::vec2(position.x, position.z));

            transform[3][1] = 0.25f * radius * glm::sin(theta * 0.f + time * 1.f + radius * 1.f);
        }

        m_camera->m_position = glm::vec3(
            glm::sin(time * 0.25f),
            0.5f * glm::cos(time * 0.3f),
            glm::cos(time * 0.25f));

        m_camera->m_position = glm::normalize(m_camera->m_position);

        float radius = 7.f * (1.2f + glm::sin(time * 0.1f));
        m_camera->m_position *= glm::vec3(1.f, 1.f, 1.f) * radius;
        m_camera->m_position = glm::normalize(m_camera->m_position) * radius;

        m_camera->m_forward = m_camera->m_position * -glm::vec3(0.5f, 0.5f, 0.5f);

        m_model->updateInstanceBuffer();
        m_camera->updateBuffer();

        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
    }

    void end() override {
        m_camera->cleanup();
        m_model->cleanup();
        m_mesh->cleanup();
        m_materialInstance->cleanup();
        m_material->cleanup();
    }
};
