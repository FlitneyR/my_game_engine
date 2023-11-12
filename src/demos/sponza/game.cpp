#include <engine.hpp>
#include <ecsManager.hpp>
#include <modelInstance.hpp>
#include <lightInstance.hpp>
#include <camera.hpp>
#include <objloader.hpp>

#include <vector>
#include <string>

class Game : public mge::Engine {

    mge::ecs::ECSManager m_ecsManager;

    mge::ecs::ModelSystem m_modelSystem;
    mge::ecs::LightSystem m_lightSystem;
    mge::ecs::System<mge::ecs::TransformComponent> m_transformSystem;
    
    typedef mge::Model<
        mge::ModelVertex,
        mge::ModelTransformMeshInstance,
        mge::NTextureMaterialInstance<3>,
        mge::Camera
    >   Model;

    std::unique_ptr<Model::Material> m_modelMaterial;

    std::vector<std::unique_ptr<Model>> m_models;
    std::vector<std::unique_ptr<Model::Mesh>> m_meshes;
    std::vector<std::unique_ptr<Model::Material::Instance>> m_materialInstances;

    std::unique_ptr<mge::Light::Mesh> m_lightMesh;

    std::unique_ptr<mge::Light> m_light;
    std::unique_ptr<mge::LightMaterial> m_lightMaterial;
    std::unique_ptr<mge::Light::Material::Instance> m_lightMaterialInstance;

    std::vector<mge::LightInstance*> m_shadowMappedLightInstances;
    std::vector<std::unique_ptr<mge::ShadowMappedLight>> m_shadowMappedLights;
    std::unique_ptr<mge::ShadowMappedLightMaterial> m_shadowMappedLightMaterial;
    std::vector<std::unique_ptr<mge::ShadowMappedLight::Material::Instance>> m_shadowMappedLightMaterialInstances;

    std::vector<std::string> m_modelNames {
        "Arches",
        "BirdBaths",
        "BlueArchCloth",
        "BlueCloth",
        "Bowls",
        "Ceilings",
        "Chains",
        "CurtainRods",
        "ExteriorWalls",
        "Floors",
        "GreenArchCloth",
        "GreenCloth",
        "Ivy",
        "LionHeads",
        "Lions",
        "MiscDecorations",
        "Pillars",
        "Pillars2",
        "Plants",
        "Pots",
        "RedArchCloth",
        "RedCloth",
        "Windows",
    };

    std::unique_ptr<mge::Camera> m_camera;
    mge::ecs::Entity m_cameraEntity;
    
    void start() override {
        m_ecsManager.addSystem("Model", &m_modelSystem);
        m_ecsManager.addSystem("Transform", &m_transformSystem);
        m_ecsManager.addSystem("Light", &m_lightSystem);

        m_modelMaterial = std::make_unique<Model::Material>(*this,
            loadShaderModule("build/mvp.vert.spv"),
            loadShaderModule("build/pbr.frag.spv"));

        m_models.reserve(m_modelNames.size());

        for (const auto& modelName : m_modelNames) {
            auto mesh = std::make_unique<Model::Mesh>(mge::loadObjMesh(*this, ("assets/sponza/" + modelName + ".obj").c_str()));
            auto materialInstance = std::make_unique<Model::Material::Instance>(m_modelMaterial->makeInstance());

            mge::Texture
                albedoTexture(("assets/sponza/" + modelName + "Albedo.png").c_str()),
                armTexture(("assets/sponza/" + modelName + "ARM.png").c_str(), vk::Format::eR8G8B8A8Unorm),
                normalTexture(("assets/sponza/" + modelName + "Normal.png").c_str(), vk::Format::eR8G8B8A8Unorm);

            materialInstance->setup({ albedoTexture, armTexture, normalTexture });

            m_meshes.push_back(std::move(mesh));

            m_materialInstances.push_back(std::move(materialInstance));

            m_models.push_back(std::make_unique<Model>(*this, *m_meshes.back(), *m_modelMaterial, *m_materialInstances.back()));

            m_modelSystem.addModel(modelName, m_models.back().get());
            
            m_meshes.back()->setup();
            m_models.back()->setup();

            auto entity = m_ecsManager.makeEntity();
            m_modelSystem.addComponent(entity, modelName);
        }

        m_lightMesh = std::make_unique<mge::Light::Mesh>(mge::makeFullScreenQuad(*this));

        m_lightMaterial = std::make_unique<mge::LightMaterial>(*this,
            loadShaderModule("build/fullscreenLight.vert.spv"),
            loadShaderModule("build/light.frag.spv"));
        m_lightMaterialInstance = std::make_unique<mge::Light::Material::Instance>(m_lightMaterial->makeInstance());
        m_lightMaterialInstance->setup();

        m_light = std::make_unique<mge::Light>(*this, *m_lightMesh, *m_lightMaterial, *m_lightMaterialInstance);

        m_shadowMappedLightMaterial = std::make_unique<mge::ShadowMappedLightMaterial>(*this,
            loadShaderModule("build/fullscreenLight.vert.spv"),
            loadShaderModule("build/shadowMapLight.frag.spv"));
        
        m_shadowMappedLightMaterialInstances.push_back(std::make_unique<mge::ShadowMappedLightMaterialInstance>(m_shadowMappedLightMaterial->makeInstance()));
        m_shadowMappedLightMaterialInstances.back()->setup(1024, 1024);
        m_shadowMappedLights.push_back(std::make_unique<mge::ShadowMappedLight>(*this, *m_lightMesh, *m_shadowMappedLightMaterial, *m_shadowMappedLightMaterialInstances.back()));

        m_shadowMappedLightMaterialInstances.push_back(std::make_unique<mge::ShadowMappedLightMaterialInstance>(m_shadowMappedLightMaterial->makeInstance()));
        m_shadowMappedLightMaterialInstances.back()->setup(2048, 2048);
        m_shadowMappedLights.push_back(std::make_unique<mge::ShadowMappedLight>(*this, *m_lightMesh, *m_shadowMappedLightMaterial, *m_shadowMappedLightMaterialInstances.back()));

        m_lightSystem.r_light = m_light.get();

        {   // ambient light entity
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponent(entity);
            auto ambientLight = m_lightSystem.getInstance(entity);
            ambientLight->m_type = ambientLight->e_ambient;
            ambientLight->m_colour = glm::vec3 { 0.1f };
        }

        m_shadowMappedLightInstances.push_back(&m_shadowMappedLights[0]->getInstance(m_shadowMappedLights[0]->makeInstance()));
        m_shadowMappedLightInstances.back()->m_type = mge::LightInstance::e_spot;
        m_shadowMappedLightInstances.back()->m_position = glm::vec3 { 8.f, 5.f, 2.f };
        m_shadowMappedLightInstances.back()->m_angle = glm::radians(60.f);
        m_shadowMappedLightInstances.back()->m_near = 0.01f;
        m_shadowMappedLightInstances.back()->m_far = 500.f;
        m_shadowMappedLightInstances.back()->m_colour = glm::vec3 { 2.f, 1.f, 0.5f } * 50.f;
        m_shadowMappedLightInstances.back()->m_direction = glm::vec3 { 0.f, -1.f, -1.f };

        m_shadowMappedLightInstances.push_back(&m_shadowMappedLights[1]->getInstance(m_shadowMappedLights[1]->makeInstance()));
        m_shadowMappedLightInstances.back()->m_type = mge::LightInstance::e_directional;
        m_shadowMappedLightInstances.back()->m_position = glm::vec3 { 0.f, 0.f, 0.f };
        m_shadowMappedLightInstances.back()->m_direction = glm::vec3 { -2.f, 1.f, -5.f };
        m_shadowMappedLightInstances.back()->m_colour = glm::vec3 { 2.f, 1.5f, 1.f } * 15.f;
        m_shadowMappedLightInstances.back()->m_angle = 30.f;
        m_shadowMappedLightInstances.back()->m_near = -1000.f;
        m_shadowMappedLightInstances.back()->m_far = 500.f;

        std::vector<glm::vec3> colours { { 1.f, 0.1f, 0.1f }, { 0.1f, 1.f, 0.1f }, { 0.1f, 0.1f, 1.f } };
        int index = 0;

        for (int i = -1; i <= 1; i++) { // point lights
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponent(entity);
            auto pointLight = m_lightSystem.getInstance(entity);
            pointLight->m_type = pointLight->e_point;
            pointLight->m_colour = colours[index++] * 10.f;

            m_transformSystem.getComponent(entity)
                ->setPosition(glm::vec3 { 5.f * i, 0.f, 2.f });
        }

        m_cameraEntity = m_ecsManager.makeEntity();
        m_transformSystem.addComponent(m_cameraEntity)->setPosition({ 0.f, 0.f, 2.f });

        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_position = glm::vec3 { 0.f, 0.f, 2.f };
        m_camera->m_forward = glm::vec3 { 1.f, 0.f, 0.f };
        m_camera->m_up = glm::vec3 { 0.f, 0.f, 1.f };

        m_camera->setup();

        m_lightMaterial->setup();
        m_shadowMappedLightMaterial->setup();

        m_lightMesh->setup();

        m_light->setup();

        for (auto& light : m_shadowMappedLights)
            light->setup();

        m_modelMaterial->setup();
    }

    void update(double deltaTime) override {
        updateCameraPosition(deltaTime);

        m_camera->updateBuffer();
        m_modelSystem.updateTransforms();
        m_lightSystem.updateTransforms();
        m_light->updateInstanceBuffer();

        for (auto& light : m_shadowMappedLights) light->updateInstanceBuffer();
        for (auto& model : m_models) model->updateInstanceBuffer();
    }

    void updateCameraPosition(float deltaTime) {
        bool moveLeft = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
        bool moveRight = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
        bool moveForward = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
        bool moveBackward = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;
        bool moveUp = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
        bool moveDown = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;

        bool turnLeft = glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS;
        bool turnRight = glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool turnUp = glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS;
        bool turnDown = glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS;

        static float forwardInput, upInput, rightInput, panInput, pitchInput;
        static float pan = glm::radians(90.f), pitch;

        forwardInput = glm::mix(forwardInput, (float)(moveForward - moveBackward), 5.f * deltaTime);
        upInput = glm::mix(upInput, (float)(moveUp - moveDown), 5.f * deltaTime);
        rightInput = glm::mix(rightInput, (float)(moveRight - moveLeft), 5.f * deltaTime);

        panInput = glm::mix(panInput, (float)(turnLeft - turnRight), 5.f * deltaTime);
        pitchInput = glm::mix(pitchInput, (float)(turnUp - turnDown), 5.f * deltaTime);

        pan += 2.f * deltaTime * panInput;
        pitch += 2.f * deltaTime * pitchInput;

        pitch = glm::clamp(pitch, -0.95f * glm::half_pi<float>(), 0.95f * glm::half_pi<float>());

        auto cameraTransform = m_transformSystem.getComponent(m_cameraEntity);

        auto deltaPosition = forwardInput * cameraTransform->getForward()
                           + upInput * cameraTransform->getUp()
                           + rightInput * cameraTransform->getRight();

        cameraTransform->setPosition(cameraTransform->getPosition() + deltaTime * 3.f * deltaPosition);
        
        cameraTransform->setRotation(
            glm::angleAxis(pan, glm::vec3 { 0.f, 0.f, 1.f }) *
            glm::angleAxis(pitch, glm::vec3 { 1.f, 0.f, 0.f })
        );

        m_camera->m_position = cameraTransform->getPosition();
        m_camera->m_forward = cameraTransform->getForward();
        m_camera->m_up = cameraTransform->getUp();

        // static float time = 0.0;
        // time += deltaTime;
        // m_shadowMappedLightInstances[1]->m_direction.x = glm::cos(time);
        // m_shadowMappedLightInstances[1]->m_direction.y = glm::sin(time);
    }

    void renderShadowMaps(vk::CommandBuffer cmd) override {
        static bool hasRendered = false;

        if (!hasRendered) {
            for (int i = 0; i < m_shadowMappedLights.size(); i++) {
                m_shadowMappedLightMaterialInstances[i]->beginRenderPass(cmd);
                m_shadowMappedLightMaterialInstances[i]->updateViewBuffer(*m_shadowMappedLightInstances[i]);
                recordShadowMapDrawCommands(cmd, m_shadowMappedLightMaterialInstances[i]->m_shadowMapView);
                cmd.endRenderPass();
            }

            // hasRendered = true;
        }
    }

    void recordShadowMapDrawCommands(vk::CommandBuffer cmd, mge::Camera& shadowMapView) override {
        m_modelMaterial->bindShadowMapPipeline(cmd);
        m_modelMaterial->bindUniform(cmd, shadowMapView);

        for (int i = 0; i < m_models.size(); i++) {
            m_modelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }
    }

    void recordGBufferDrawCommands(vk::CommandBuffer cmd) override {
        m_modelMaterial->bindPipeline(cmd);
        m_modelMaterial->bindUniform(cmd, *m_camera);

        for (int i = 0; i < m_models.size(); i++) {
            m_modelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }
    }

    void recordLightingDrawCommands(vk::CommandBuffer cmd) override {
        m_lightMaterial->bindPipeline(cmd);
        m_light->drawInstances(cmd);
        
        m_shadowMappedLightMaterial->bindPipeline(cmd);

        for (auto& light : m_shadowMappedLights) light->drawInstances(cmd);
    }

    void end() override {
        m_camera->cleanup();

        for (auto& mesh : m_meshes) mesh->cleanup();
        for (auto& model : m_models) model->cleanup();
        for (auto& materialInstance : m_materialInstances) materialInstance->cleanup();
        m_modelMaterial->cleanup();

        m_lightMesh->cleanup();

        m_light->cleanup();
        for (auto& light : m_shadowMappedLights) light->cleanup();

        m_lightMaterialInstance->cleanup();
        for (auto& lightInstance : m_shadowMappedLightMaterialInstances) lightInstance->cleanup();

        m_lightMaterial->cleanup();
        m_shadowMappedLightMaterial->cleanup();
    }
};