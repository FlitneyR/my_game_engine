#include <engine.hpp>
#include <ecsManager.hpp>
#include <modelInstance.hpp>
#include <lightInstance.hpp>
#include <camera.hpp>
#include <objloader.hpp>
#include <postProcessing.hpp>
#include <taa.hpp>
#include <bloom.hpp>

#include <vector>
#include <string>

class Sponza : public mge::Engine {

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
    std::unique_ptr<Model::Material> m_alphaClippedModelMaterial;

    std::vector<std::unique_ptr<Model>> m_models;
    std::vector<std::unique_ptr<Model::Mesh>> m_meshes;
    std::vector<std::unique_ptr<Model::Material::Instance>> m_materialInstances;

    std::unique_ptr<mge::Light::Mesh> m_lightMesh;

    std::unique_ptr<mge::Light> m_light;
    std::unique_ptr<mge::LightMaterial> m_lightMaterial;
    std::unique_ptr<mge::Light::Material::Instance> m_lightMaterialInstance;

    std::unique_ptr<mge::ShadowMappedLight> m_shadowMappedLightPrototype;
    std::unique_ptr<mge::ShadowMappedLightMaterial> m_shadowMappedLightMaterial;
    std::unique_ptr<mge::ShadowMappedLight::Material::Instance> m_shadowMappedLightMaterialInstance;

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
    mge::ecs::Entity m_cameraEntity, m_spotLightEntity;

    mge::HDRColourCorrection m_hdrColourCorrection;
    mge::TAA m_taa;
    mge::Bloom m_bloom;

    void start() override {
        m_ecsManager.r_engine = this;
        m_ecsManager.addSystem("Model", &m_modelSystem);
        m_ecsManager.addSystem("Transform", &m_transformSystem);
        m_ecsManager.addSystem("Light", &m_lightSystem);

        m_hdrColourCorrection = mge::HDRColourCorrection(*this);
        m_taa = mge::TAA(*this);
        m_bloom = mge::Bloom(*this);
        m_bloom.m_maxMipLevel = 5;

        m_modelMaterial = std::make_unique<Model::Material>(*this,
            loadShaderModule("mvp.vert.spv"),
            loadShaderModule("pbr.frag.spv"));

        m_alphaClippedModelMaterial = std::make_unique<Model::Material>(*this,
            loadShaderModule("mvp.vert.spv"),
            loadShaderModule("pbr.frag.spv"));
        m_alphaClippedModelMaterial->m_usesAlphaClipping = true;

        m_models.reserve(m_modelNames.size());

        for (const auto& modelName : m_modelNames) {
            auto mesh = std::make_unique<Model::Mesh>(mge::loadObjMesh(*this, ("assets/sponza/" + modelName + ".obj").c_str()));
            Model::Material* material;

            if (modelName == "Chains" || modelName == "Ivy" || modelName == "Plants") {
                material = m_alphaClippedModelMaterial.get();
            } else {
                material = m_modelMaterial.get();
            }

            auto materialInstance = std::make_unique<Model::Material::Instance>(material->makeInstance());

            mge::Texture
                albedoTexture(("assets/sponza/" + modelName + "Albedo.png").c_str()),
                armTexture(("assets/sponza/" + modelName + "ARM.png").c_str(), vk::Format::eR8G8B8A8Unorm),
                normalTexture(("assets/sponza/" + modelName + "Normal.png").c_str(), vk::Format::eR8G8B8A8Unorm);

            materialInstance->setup({ albedoTexture, armTexture, normalTexture });

            m_meshes.push_back(std::move(mesh));

            m_materialInstances.push_back(std::move(materialInstance));

            m_models.push_back(std::make_unique<Model>(*this, *m_meshes.back(), *material, *m_materialInstances.back()));

            m_modelSystem.addModel(modelName, m_models.back().get());
            
            m_meshes.back()->setup();
            m_models.back()->setup();

            auto entity = m_ecsManager.makeEntity();
            m_modelSystem.addComponent(entity, modelName);
        }

        m_lightMesh = std::make_unique<mge::Light::Mesh>(mge::Mesh<mge::PointVertex>(*this, {
            {{ -1.f, -1.f, 0.f }},
            {{ -1.f,  3.f, 0.f }},
            {{  3.f, -1.f, 0.f }},
        }, { 0, 1, 2, }));

        m_lightMaterial = std::make_unique<mge::LightMaterial>(*this,
            loadShaderModule("fullscreenLight.vert.spv"),
            loadShaderModule("light.frag.spv"));
        m_lightMaterialInstance = std::make_unique<mge::Light::Material::Instance>(m_lightMaterial->makeInstance());
        m_lightMaterialInstance->setup();

        m_light = std::make_unique<mge::Light>(*this, *m_lightMesh, *m_lightMaterial, *m_lightMaterialInstance);

        m_shadowMappedLightMaterial = std::make_unique<mge::ShadowMappedLightMaterial>(*this,
            loadShaderModule("fullscreenLight.vert.spv"),
            loadShaderModule("shadowMapLight.frag.spv"));
        
        m_shadowMappedLightMaterialInstance = std::make_unique<mge::ShadowMappedLightMaterialInstance>(m_shadowMappedLightMaterial->makeInstance());
        m_shadowMappedLightMaterialInstance->setup(1, 1);
        m_shadowMappedLightPrototype = std::make_unique<mge::ShadowMappedLight>(*this, *m_lightMesh, *m_shadowMappedLightMaterial, *m_shadowMappedLightMaterialInstance);

        m_lightSystem.r_shadowlessLight = m_light.get();
        m_lightSystem.r_shadowMappedLightPrototype = m_shadowMappedLightPrototype.get();

        {   // ambient light entity
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponent(entity);
            auto ambientLight = m_lightSystem.getInstance(entity);
            ambientLight->m_type = ambientLight->e_ambient;
            ambientLight->m_colour = glm::vec3 { 0.07f };
        }

        {   // sun light
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponentShadowMapped(entity, 2048);
            
            auto sunLight = m_lightSystem.getInstance(entity);
            auto transform = m_transformSystem.getComponent(entity);

            transform->setRotation(
                glm::angleAxis(glm::radians(45.f), glm::vec3 { 0.f, 0.f, 1.f }) *
                glm::angleAxis(glm::radians(-80.f), glm::vec3 { 1.f, 0.f, 0.f })
            );

            sunLight->m_type = sunLight->e_directional;
            sunLight->m_colour = glm::vec3 { 2.f, 1.5f, 1.f } * 7.5f;
            sunLight->m_shadowRange = 30.f;
            sunLight->m_near = -1000.f;
            sunLight->m_far = 500.f;
        }

        {   // spot light
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponentShadowMapped(entity, 2048);
            
            auto spotLight = m_lightSystem.getInstance(entity);
            auto transform = m_transformSystem.getComponent(entity);

            transform->setPosition({ -10.5f, -4.5f, 2.5f });
            transform->setRotation(
                glm::angleAxis(glm::radians(-20.f), glm::vec3 { 0.f, 0.f, 1.f }) *
                glm::angleAxis(glm::radians(-30.f), glm::vec3 { 1.f, 0.f, 0.f })
            );

            spotLight->m_type = mge::LightInstance::e_spot;
            spotLight->m_colour = glm::vec3 { 2.f, 1.f, 0.5f } * 75.f;
            spotLight->m_angle = glm::radians(45.f);
            spotLight->m_near = 0.01f;
            spotLight->m_far = 500.f;

            m_spotLightEntity = entity;
        }

        {   // spot light
            auto entity = m_ecsManager.makeEntity();
            m_lightSystem.addComponentShadowMapped(entity, 2048);
            
            auto spotLight = m_lightSystem.getInstance(entity);
            auto transform = m_transformSystem.getComponent(entity);

            transform->setPosition({ -10.5f, 4.5f, 2.5f });
            transform->setRotation(
                glm::angleAxis(glm::radians(210.f), glm::vec3 { 0.f, 0.f, 1.f }) *
                glm::angleAxis(glm::radians(-30.f), glm::vec3 { 1.f, 0.f, 0.f })
            );

            spotLight->m_type = mge::LightInstance::e_spot;
            spotLight->m_colour = glm::vec3 { 1.f, 2.f, 0.5f } * 75.f;
            spotLight->m_angle = glm::radians(45.f);
            spotLight->m_near = 0.01f;
            spotLight->m_far = 500.f;

            m_spotLightEntity = entity;
        }

        // std::vector<glm::vec3> colours { { 1.f, 0.1f, 0.1f }, { 0.1f, 1.f, 0.1f }, { 0.1f, 0.1f, 1.f } };
        // int index = 0;

        // for (int i = -1; i <= 1; i++) { // point lights
        //     auto entity = m_ecsManager.makeEntity();
        //     m_lightSystem.addComponent(entity);
        //     auto pointLight = m_lightSystem.getInstance(entity);
        //     pointLight->m_type = pointLight->e_point;
        //     pointLight->m_colour = colours[index++] * 10.f;

        //     m_transformSystem.getComponent(entity)
        //         ->setPosition(glm::vec3 { 5.f * i, 0.5f, 0.125f });
        // }

        m_cameraEntity = m_ecsManager.makeEntity();
        m_transformSystem.addComponent(m_cameraEntity)->setPosition({ 0.f, 0.f, 2.f });

        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_position = glm::vec3 { 0.f, 0.f, 2.f };
        m_camera->m_forward = glm::vec3 { 1.f, 0.f, 0.f };
        m_camera->m_up = glm::vec3 { 0.f, 0.f, 1.f };
        m_camera->m_taaJitter = true;

        m_camera->setup();

        m_lightMaterial->setup();
        m_shadowMappedLightMaterial->setup();

        m_lightMesh->setup();

        m_light->setup();

        m_modelMaterial->setup();
        m_alphaClippedModelMaterial->setup();

        m_hdrColourCorrection.setup();
        m_taa.setup();
        m_bloom.setup();
    }

    void update(double deltaTime) override {
        updateCameraPosition(deltaTime);
    }

    void keyCallback(int key, int scancode, int action, int mods) override {
        if (action == GLFW_PRESS)
        switch (key) {
        case GLFW_KEY_ESCAPE: {
            switch (glfwGetInputMode(m_window, GLFW_CURSOR)) {
            case GLFW_CURSOR_HIDDEN:
                glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
                break;
            default:
                glfwSetWindowShouldClose(m_window, true);
                break;
            }
        }
        }
    }

    void mouseButtonCallback(int button, int action, int mods) override {
        if (button == GLFW_MOUSE_BUTTON_1 && action == GLFW_PRESS) {
            reCentreMouse();
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
        }
    }

    void reCentreMouse() {
        glm::vec<2, int> windowSize;
        glfwGetWindowSize(m_window, &windowSize.x, &windowSize.y);

        glm::vec<2, double> center = glm::vec<2, double>(windowSize) / 2.0;
        glfwSetCursorPos(m_window, center.x, center.y);
    }

    void updateCameraPosition(float deltaTime) {
        bool moveUp = false;
        bool moveDown = false;
        bool moveLeft = false;
        bool moveRight = false;
        bool moveForward = false;
        bool moveBackward = false;

        if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_HIDDEN) {
            moveUp = glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS;
            moveDown = glfwGetKey(m_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
            moveLeft = glfwGetKey(m_window, GLFW_KEY_A) == GLFW_PRESS;
            moveRight = glfwGetKey(m_window, GLFW_KEY_D) == GLFW_PRESS;
            moveForward = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
            moveBackward = glfwGetKey(m_window, GLFW_KEY_S) == GLFW_PRESS;
        }

        static constexpr float MOUSE_SENSITIVITY = 0.005f;
        glm::vec2 mouseDelta { 0.f, 0.f };

        if (glfwGetInputMode(m_window, GLFW_CURSOR) == GLFW_CURSOR_HIDDEN) {
            glm::vec<2, double> mousePos;
            glfwGetCursorPos(m_window, &mousePos.x, &mousePos.y);

            glm::vec<2, int> windowSize;
            glfwGetWindowSize(m_window, &windowSize.x, &windowSize.y);
            glm::vec<2, double> windowCentre = windowSize; windowCentre /= 2.0;
            glfwSetCursorPos(m_window, windowCentre.x, windowCentre.y);

            mouseDelta = mousePos - windowCentre;
        }

        static float forwardInput, upInput, rightInput, panInput, pitchInput;
        static float pan = glm::radians(90.f), pitch;

        forwardInput = glm::mix(forwardInput, (float)(moveForward - moveBackward), 5.f * deltaTime);
        upInput = glm::mix(upInput, (float)(moveUp - moveDown), 5.f * deltaTime);
        rightInput = glm::mix(rightInput, (float)(moveRight - moveLeft), 5.f * deltaTime);
        
        pan -= mouseDelta.x * MOUSE_SENSITIVITY;
        pitch -= mouseDelta.y * MOUSE_SENSITIVITY;

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

        // auto spotLightTransform = m_transformSystem.getComponent(m_spotLightEntity);
        // spotLightTransform->setPosition(cameraTransform->getPosition() + cameraTransform->getForward() * 0.25f);
        // spotLightTransform->setRotation(cameraTransform->getRotation());
    }

    void updateBuffers() override {
        m_camera->updateBuffer();
        m_modelSystem.updateTransforms();
        m_lightSystem.update();
    }

    void recordShadowMapDrawCommands(vk::CommandBuffer cmd) override {
        for (auto& [ _, light ] : m_lightSystem.m_shadowMappedLights) {
            light->m_materialInstance.beginShadowMapRenderPass(cmd, light->getInstance(0));
            recordShadowMapGeometryDrawCommands(cmd, light->m_materialInstance.m_shadowMapView);
            cmd.endRenderPass();
        }
    }

    void recordShadowMapGeometryDrawCommands(vk::CommandBuffer cmd, mge::Camera& shadowMapView) override {
        m_modelMaterial->bindShadowMapPipeline(cmd);
        m_modelMaterial->bindUniform(cmd, shadowMapView);

        for (int i = 0; i < m_models.size(); i++)
        if (m_models[i]->m_material == m_modelMaterial.get()) {
            m_modelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }

        m_alphaClippedModelMaterial->bindShadowMapPipeline(cmd);
        m_alphaClippedModelMaterial->bindUniform(cmd, shadowMapView);

        for (int i = 0; i < m_models.size(); i++)
        if (m_models[i]->m_material == m_alphaClippedModelMaterial.get()) {
            m_alphaClippedModelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }
    }

    void recordGBufferDrawCommands(vk::CommandBuffer cmd) override {
        m_modelMaterial->bindPipeline(cmd);
        m_modelMaterial->bindUniform(cmd, *m_camera);

        for (int i = 0; i < m_models.size(); i++)
        if (m_models[i]->m_material == m_modelMaterial.get()) {
            m_modelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }

        m_alphaClippedModelMaterial->bindPipeline(cmd);
        m_alphaClippedModelMaterial->bindUniform(cmd, *m_camera);

        for (int i = 0; i < m_models.size(); i++)
        if (m_models[i]->m_material == m_alphaClippedModelMaterial.get()) {
            m_alphaClippedModelMaterial->bindInstance(cmd, *m_materialInstances[i]);
            m_models[i]->drawInstances(cmd);
        }
    }

    void recordLightingDrawCommands(vk::CommandBuffer cmd) override {
        m_lightMaterial->bindPipeline(cmd);
        m_lightMaterial->bindUniform(cmd, *m_camera);
        m_light->drawInstances(cmd);

        m_shadowMappedLightMaterial->bindPipeline(cmd);
        m_shadowMappedLightMaterial->bindUniform(cmd, *m_camera);
        for (auto& [ _, light ] : m_lightSystem.m_shadowMappedLights) light->drawInstances(cmd);
    }

    void recordPostProcessingDrawCommands(vk::CommandBuffer cmd) override {
        m_hdrColourCorrection.draw(cmd);
        m_taa.draw(cmd);
        m_bloom.draw(cmd);
    }

    void rebuildSwapchain() override {
        mge::Engine::rebuildSwapchain();

        m_hdrColourCorrection.cleanup();
        m_taa.cleanup();
        m_bloom.cleanup();

        m_hdrColourCorrection.setup();
        m_taa.setup();
        m_bloom.setup();
    }

    void end() override {
        m_camera->cleanup();

        for (auto& mesh : m_meshes) mesh->cleanup();
        for (auto& model : m_models) model->cleanup();
        for (auto& materialInstance : m_materialInstances) materialInstance->cleanup();
        
        m_modelMaterial->cleanup();
        m_alphaClippedModelMaterial->cleanup();

        m_lightMesh->cleanup();

        m_light->cleanup();
        m_lightMaterialInstance->cleanup();

        for (auto& [ _, light ] : m_lightSystem.m_shadowMappedLights) {
            light->cleanup();
            light->m_materialInstance.cleanup();
        }

        m_shadowMappedLightPrototype->cleanup();
        m_shadowMappedLightMaterialInstance->cleanup();

        m_lightMaterial->cleanup();
        m_shadowMappedLightMaterial->cleanup();

        m_hdrColourCorrection.cleanup();
        m_taa.cleanup();
        m_bloom.cleanup();
    }
};

int main() {
    Sponza game;

    game.init(1280, 720);
    game.main();
    game.cleanup();

    return 0;
}
