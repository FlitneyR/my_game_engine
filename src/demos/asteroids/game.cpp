#include <engine.hpp>
#include <model.hpp>
#include <camera.hpp>
#include <objloader.hpp>
#include <skybox.hpp>
#include <lightInstance.hpp>
#include <modelInstance.hpp>
#include <postProcessing.hpp>
#include <taa.hpp>

#include "logic.hpp"

#include <iostream>
#include <memory>

class Game : public mge::Engine {
    typedef mge::Model<mge::ModelVertex, mge::ModelTransformMeshInstance, mge::NTextureMaterialInstance<3>, mge::Camera> ObjectModel;
    typedef mge::Model<mge::ModelVertex, mge::ModelTransformMeshInstance, mge::NTextureMaterialInstance<1>, mge::Camera> SkyboxModel;
    typedef mge::Model< mge::PointColorVertex, mge::ModelTransformMeshInstance, mge::SimpleMaterialInstance, mge::Camera> BulletModel;

    std::unique_ptr<ObjectModel::Material> m_objectMaterial;

    std::unique_ptr<ObjectModel> m_asteroidModel;
    std::unique_ptr<ObjectModel::Mesh> m_asteroidMesh;
    std::unique_ptr<ObjectModel::Material::Instance> m_asteroidMaterialInstance;
    mge::Texture m_asteroidAlbedo, m_asteroidARM, m_asteroidNormal;

    std::unique_ptr<ObjectModel> m_spaceshipModel;
    std::unique_ptr<ObjectModel::Mesh> m_spaceshipMesh;
    std::unique_ptr<ObjectModel::Material::Instance> m_spaceshipMaterialInstance;
    mge::Texture m_spaceshipAlbedo, m_spaceshipARM, m_spaceshipNormal;

    std::unique_ptr<BulletModel> m_bulletModel;
    std::unique_ptr<BulletModel::Mesh> m_bulletMesh;
    std::unique_ptr<BulletModel::Material> m_bulletMaterial;
    std::unique_ptr<BulletModel::Material::Instance> m_bulletMaterialInstance;

    std::unique_ptr<SkyboxModel> m_skyboxModel;
    std::unique_ptr<SkyboxModel::Mesh> m_skyboxMesh;
    std::unique_ptr<mge::SkyboxMaterial> m_skyboxMaterial;
    std::unique_ptr<SkyboxModel::Material::Instance> m_skyboxMaterialInstance;
    mge::Texture m_skyboxTexture;

    std::unique_ptr<mge::Light> m_light;
    std::unique_ptr<mge::Light::Mesh> m_lightQuad;
    std::unique_ptr<mge::LightMaterial> m_lightMaterial;
    std::unique_ptr<mge::LightMaterial::Instance> m_lightMaterialInstance;

    std::unique_ptr<mge::ShadowMappedLight> m_shadowMappedLightPrototype;
    std::unique_ptr<mge::ShadowMappedLightMaterial> m_shadowMappedLightMaterial;
    std::unique_ptr<mge::ShadowMappedLight::Material::Instance> m_shadowMappedLightMaterialInstance;

    std::unique_ptr<mge::Camera> m_camera;

    mge::ecs::ECSManager m_ecsManager;

    AsteroidSystem m_asteroidSystem;
    BulletSystem m_bulletSystem;
    SpaceshipSystem m_spaceshipSystem;
    mge::ecs::RigidbodySystem m_rigidbodySystem;
    mge::ecs::System<mge::ecs::TransformComponent> m_transformSystem;
    mge::ecs::CollisionSystem m_collisionSystem;
    mge::ecs::ModelSystem m_modelSystem;
    mge::ecs::LightSystem m_lightSystem;

    mge::HDRColourCorrection m_hdrColourCorrection;
    mge::TAA m_taa;

    std::string getGameName() override { return "Asteroids"; }
    
    void start() override {
        std::srand(std::chrono::system_clock::now().time_since_epoch().count());

        m_hdrColourCorrection = mge::HDRColourCorrection(*this);
        m_taa = mge::TAA(*this);

        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_near = 0.1f;
        m_camera->m_far = 10'000.f;
        m_camera->m_fov = glm::radians(70.f);
        m_camera->m_position = glm::vec3 { 0.f, 0.f, 0.f };
        m_camera->m_forward = glm::vec3 { 0.f, 1.f, 0.f };
        m_camera->m_up = glm::vec3 { 0.f, 0.f, 1.f };
        m_camera->m_taaJitter = true;

        m_asteroidMesh = std::make_unique<ObjectModel::Mesh>(mge::loadObjMesh(*this, "assets/asteroids/lowpoly_asteroid.obj"));
        m_spaceshipMesh = std::make_unique<ObjectModel::Mesh>(mge::loadObjMesh(*this, "assets/asteroids/smoother_spaceship.obj"));
        m_skyboxMesh = std::make_unique<SkyboxModel::Mesh>(mge::loadObjMesh(*this, "assets/asteroids/skybox.obj"));
        m_lightQuad = std::make_unique<mge::Light::Mesh>(mge::Mesh<mge::PointVertex>(*this, {
            {{ -1.f, -1.f, 0.f }},
            {{ -1.f,  3.f, 0.f }},
            {{  3.f, -1.f, 0.f }},
        }, { 0, 1, 2, }));

        m_bulletMesh = std::make_unique<BulletModel::Mesh>(
            *this,
            std::vector<mge::PointColorVertex> {
                { {   0.0f,  0.25f,   0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 0
                { {   0.0f, -0.25f,   0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 1
                { {  0.25f,   0.0f,   0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 2
                { { -0.25f,   0.0f,   0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 3
                { {   0.0f,   0.0f,  1.25f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 4
                { {   0.0f,   0.0f, -1.25f }, { 1.0f, 0.0f, 0.0f, 1.0f } }, // 5
            },
            std::vector<uint16_t> {
                0, 4, 3,    0, 2, 4,
                0, 5, 2,    0, 3, 5,
                1, 3, 4,    1, 4, 2,
                1, 2, 5,    1, 5, 3,
            });

        m_asteroidAlbedo = mge::Texture("assets/asteroids/asteroid_albedo.png");
        m_asteroidARM = mge::Texture("assets/asteroids/new_asteroid_arm.png");
        m_asteroidNormal = mge::Texture("assets/asteroids/asteroid_normal.png", vk::Format::eR8G8B8A8Unorm);
        
        m_spaceshipAlbedo = mge::Texture("assets/asteroids/spaceship_albedo.png");
        m_spaceshipARM = mge::Texture("assets/asteroids/spaceship_arm.png");
        m_spaceshipNormal = mge::Texture("assets/asteroids/spaceship_normal.png");

        m_skyboxTexture = mge::Texture("assets/asteroids/skybox.png");

        m_objectMaterial = std::make_unique<ObjectModel::Material>(*this,
            loadShaderModule("build/mvp.vert.spv"), loadShaderModule("build/pbr.frag.spv"));

        m_skyboxMaterial = std::make_unique<mge::SkyboxMaterial>(*this,
            loadShaderModule("build/skybox.vert.spv"), loadShaderModule("build/skybox.frag.spv"));
        
        m_bulletMaterial = std::make_unique<BulletModel::Material>(*this,
            loadShaderModule("build/bullet.vert.spv"), loadShaderModule("build/bullet.frag.spv"));
        
        m_lightMaterial = std::make_unique<mge::LightMaterial>(*this,
            loadShaderModule("build/fullscreenLight.vert.spv"), loadShaderModule("build/light.frag.spv"));

        m_shadowMappedLightMaterial = std::make_unique<mge::ShadowMappedLightMaterial>(*this,
            loadShaderModule("build/fullscreenLight.vert.spv"), loadShaderModule("build/shadowMapLight.frag.spv"));

        m_asteroidMaterialInstance = std::make_unique<ObjectModel::Material::Instance>(m_objectMaterial->makeInstance());
        m_asteroidMaterialInstance->setup({ m_asteroidAlbedo, m_asteroidARM, m_asteroidNormal });

        m_spaceshipMaterialInstance = std::make_unique<ObjectModel::Material::Instance>(m_objectMaterial->makeInstance());
        m_spaceshipMaterialInstance->setup({ m_spaceshipAlbedo, m_spaceshipARM, m_spaceshipNormal });

        m_skyboxMaterialInstance = std::make_unique<SkyboxModel::Material::Instance>(m_skyboxMaterial->makeInstance());
        m_skyboxMaterialInstance->setup({ m_skyboxTexture });
    
        m_bulletMaterialInstance = std::make_unique<BulletModel::Material::Instance>(m_bulletMaterial->makeInstance());
        m_bulletMaterialInstance->setup();

        m_lightMaterialInstance = std::make_unique<mge::Light::Material::Instance>(m_lightMaterial->makeInstance());
        m_lightMaterialInstance->setup();

        m_shadowMappedLightMaterialInstance = std::make_unique<mge::ShadowMappedLightMaterialInstance>(m_shadowMappedLightMaterial->makeInstance());
        m_shadowMappedLightMaterialInstance->setup(1, 1);

        m_asteroidModel = std::make_unique<ObjectModel>(*this, *m_asteroidMesh, *m_objectMaterial, *m_asteroidMaterialInstance);
        m_spaceshipModel = std::make_unique<ObjectModel>(*this, *m_spaceshipMesh, *m_objectMaterial, *m_spaceshipMaterialInstance);
        m_skyboxModel = std::make_unique<SkyboxModel>(*this, *m_skyboxMesh, *m_skyboxMaterial, *m_skyboxMaterialInstance);
        m_bulletModel = std::make_unique<BulletModel>(*this, *m_bulletMesh, *m_bulletMaterial, *m_bulletMaterialInstance);
        m_light = std::make_unique<mge::Light>(*this, *m_lightQuad, *m_lightMaterial, *m_lightMaterialInstance);
        m_shadowMappedLightPrototype = std::make_unique<mge::ShadowMappedLight>(*this, *m_lightQuad, *m_shadowMappedLightMaterial, *m_shadowMappedLightMaterialInstance);
        
        m_skyboxModel->makeInstance();

        m_lightSystem.r_shadowlessLight = m_light.get();
        m_lightSystem.r_shadowMappedLightPrototype = m_shadowMappedLightPrototype.get();

        m_modelSystem.addModel("Asteroid", m_asteroidModel.get());
        m_modelSystem.addModel("Spaceship", m_spaceshipModel.get());
        m_modelSystem.addModel("Bullet", m_bulletModel.get());

        m_ecsManager.r_engine = this;
        m_ecsManager.addSystem("Asteroid", &m_asteroidSystem);
        m_ecsManager.addSystem("Bullet", &m_bulletSystem);
        m_ecsManager.addSystem("Spaceship", &m_spaceshipSystem);
        m_ecsManager.addSystem("Rigidbody", &m_rigidbodySystem);
        m_ecsManager.addSystem("Transform", &m_transformSystem);
        m_ecsManager.addSystem("Model", &m_modelSystem);
        m_ecsManager.addSystem("Light", &m_lightSystem);
        m_ecsManager.addSystem("Collision", &m_collisionSystem);

        m_camera->setup();

        m_lightMaterial->setup();
        m_lightQuad->setup();
        m_light->setup();

        m_shadowMappedLightMaterial->setup();

        m_objectMaterial->setup();

        m_asteroidMesh->setup();
        m_asteroidModel->setup();

        m_spaceshipMesh->setup();
        m_spaceshipModel->setup();

        m_skyboxMaterial->setup();
        m_skyboxMesh->setup();
        m_skyboxModel->setup();

        m_bulletMaterial->setup();
        m_bulletMesh->setup();
        m_bulletModel->setup();

        m_hdrColourCorrection.setup();
        m_taa.setup();
        
        makeTemplates();

        for (int i = 0; i < 3; i++) {
            auto sunEntity = m_ecsManager.makeEntity();
            m_lightSystem.addComponent(sunEntity);
            auto sunInstance = m_lightSystem.getInstance(sunEntity);

            sunInstance->m_type = sunInstance->e_directional;

            sunInstance->m_colour = glm::vec3 {
                randomRangeFloat(0.125f, 0.25f),
                randomRangeFloat(0.0625f, 0.125f),
                randomRangeFloat(0.25f, 0.5f)
            };

            sunInstance->m_direction = randomUnitVector();
        }

        auto ambientLight = m_ecsManager.makeEntity();
        m_lightSystem.addComponent(ambientLight);
        auto ambientLightInstance = m_lightSystem.getInstance(ambientLight);
        ambientLightInstance->m_type = ambientLightInstance->e_ambient;
        ambientLightInstance->m_colour = glm::vec3 { 0.02f };

        for (int i = 0; i < 6'000; i++)
            auto entity = m_ecsManager.makeEntityFromTemplate("Asteroid");

        auto spaceshipEntity = m_ecsManager.makeEntityFromTemplate("Spaceship");
        auto spaceshipTransform = m_transformSystem.getComponent(spaceshipEntity);

        m_camera->m_position = spaceshipTransform->getPosition() + spaceshipTransform->getUp();
        m_camera->m_up = spaceshipTransform->getUp();
    }

    void makeTemplates() {
        m_ecsManager.addTemplate("Asteroid", [&](mge::ecs::ECSManager& ecs) {
            auto entity = ecs.makeEntity();

            auto transform = m_transformSystem.addComponent(entity);
            auto rigidbody = m_rigidbodySystem.addComponent(entity);
            auto model = m_modelSystem.addComponent(entity, "Asteroid");
            auto collider = m_collisionSystem.addComponent(entity);
            auto asteroid = m_asteroidSystem.addComponent(entity);

            float radius = glm::mix(5.f, 40.f, glm::pow(randomRangeFloat(0.f, 1.f), 10.f));
            collider->setupSphere(radius);

            transform->setPosition(glm::vec3 {
                randomRangeFloat(-1.f, 1.f),
                randomRangeFloat(-1.f, 1.f),
                randomRangeFloat(-1.f, 1.f)
            } * AsteroidSystem::MAX_DISTANCE);
            transform->setRotation(glm::angleAxis(randomRangeFloat(-glm::pi<float>(), glm::pi<float>()), randomUnitVector()));
            transform->setScale(glm::vec3 { radius });

            rigidbody->m_velocity = randomUnitVector() * randomRangeFloat(0.f, 15.f) * glm::vec3 { 1.f, 1.f, 1.f };
            rigidbody->m_physicsType = rigidbody->e_dynamic;
            rigidbody->m_mass = radius * radius * radius;
            rigidbody->m_angularVelocity = randomRangeFloat(0.f, 1.f) * randomUnitVector();

            return entity;
        });

        m_ecsManager.addTemplate("Bullet", [&](mge::ecs::ECSManager& ecs) {
            auto entity = ecs.makeEntity();

            m_bulletSystem.addComponent(entity);
            m_modelSystem.addComponent(entity, "Bullet");
            m_transformSystem.addComponent(entity);
            auto bulletRigidbody = m_rigidbodySystem.addComponent(entity);
            auto bulletCollision = m_collisionSystem.addComponent(entity);
            m_lightSystem.addComponent(entity);
            auto light = m_lightSystem.getInstance(entity);

            bulletRigidbody->m_physicsType = bulletRigidbody->e_dynamic;
            bulletRigidbody->m_mass = 0.001f;

            bulletCollision->setupSphere(1.f);

            light->m_type = light->e_point;
            light->m_colour = glm::vec3 { 100.f, 0.f, 0.f };

            return entity;
        });

        m_ecsManager.addTemplate("Spaceship", [&](mge::ecs::ECSManager& ecs){
            auto entity = ecs.makeEntity();

            m_modelSystem.addComponent(entity, "Spaceship");
            m_spaceshipSystem.addComponent(entity);
            m_transformSystem.addComponent(entity);
            auto collision = m_collisionSystem.addComponent(entity);
            auto rigidbody = m_rigidbodySystem.addComponent(entity);

            m_lightSystem.addComponentShadowMapped(entity, 2048);
            auto spaceshipLight = m_lightSystem.getInstance(entity);

            collision->setupSphere(2.f);

            rigidbody->m_physicsType = rigidbody->e_dynamic;
            rigidbody->m_mass = 50.f;

            spaceshipLight->m_colour = 10'000.f * glm::vec3 { 0.5f, 0.75f, 1.0f };
            spaceshipLight->m_type = spaceshipLight->e_spot;
            spaceshipLight->m_angle = glm::radians(30.f);
            spaceshipLight->m_near = 0.01f;
            spaceshipLight->m_far = 1'000.f;

            return entity;
        });
    }

    void recordShadowMapDrawCommands(vk::CommandBuffer cmd) override {
        for (auto& [ _, light ] : m_lightSystem.m_shadowMappedLights) {
            light->m_materialInstance.beginShadowMapRenderPass(cmd, light->getInstance(0));
            recordShadowMapGeometryDrawCommands(cmd, light->m_materialInstance.m_shadowMapView);
            cmd.endRenderPass();
        }
    }

    void recordShadowMapGeometryDrawCommands(vk::CommandBuffer cmd, mge::Camera& shadowMapView) override {
        m_objectMaterial->bindShadowMapPipeline(cmd);
        m_objectMaterial->bindUniform(cmd, shadowMapView);

        // m_spaceshipModel->drawInstances(cmd);
        
        m_asteroidModel->drawInstances(cmd);
    }

    void recordGBufferDrawCommands(vk::CommandBuffer cmd) override {
        m_skyboxMaterial->bindPipeline(cmd);
        m_skyboxMaterial->bindUniform(cmd, *m_camera);
        m_skyboxModel->drawInstances(cmd);

        m_objectMaterial->bindPipeline(cmd);
        m_objectMaterial->bindUniform(cmd, *m_camera);

        m_spaceshipModel->drawInstances(cmd);
        
        m_asteroidModel->drawInstances(cmd);

        m_bulletMaterial->bindPipeline(cmd);
        m_bulletMaterial->bindUniform(cmd, *m_camera);
        m_bulletModel->drawInstances(cmd);
    }

    void recordLightingDrawCommands(vk::CommandBuffer cmd) override {
        m_light->updateInstanceBuffer();

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
    }

    void rebuildSwapchain() override {
        mge::Engine::rebuildSwapchain();

        m_hdrColourCorrection.cleanup();
        m_taa.cleanup();

        m_hdrColourCorrection.setup();
        m_taa.setup();
    }

    void update(double deltaTime) override {
        auto collisionEvents = m_collisionSystem.getCollisionEvents();

        m_bulletSystem.handleCollisions(collisionEvents);
        // m_spaceshipSystem.checkForAsteroidCollision(collisionEvents);
        m_rigidbodySystem.resolveCollisions(collisionEvents);

        m_rigidbodySystem.update(deltaTime);

        m_bulletSystem.destroyOldBullets(deltaTime);

        bool accelerate = glfwGetKey(m_window, GLFW_KEY_W) == GLFW_PRESS;
        bool pitchUp = glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS;
        bool pitchDown = glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS;
        bool turnLeft = glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS;
        bool turnRight = glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
        bool fire = glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS;

        m_spaceshipSystem.update(deltaTime, accelerate, pitchUp, pitchDown, turnLeft, turnRight, fire);

        auto spaceshipEntity = m_spaceshipSystem.m_components.begin()->first;

        auto spaceship = m_spaceshipSystem.getComponent(spaceshipEntity);
        auto spaceshipTransform = m_transformSystem.getComponent(spaceshipEntity);

        if (spaceship->m_alive)
            m_asteroidSystem.wrapAsteroids();

        glm::vec3 cameraTargetPosition, cameraFocus, cameraUp;
        float targetFov;

        if (spaceship->m_alive) {
            cameraTargetPosition = spaceshipTransform->getPosition();
            cameraTargetPosition += spaceshipTransform->getForward() * -10.f;
            cameraTargetPosition += spaceshipTransform->getUp() * 5.f;

            cameraFocus = spaceshipTransform->getPosition();
            cameraFocus += spaceshipTransform->getForward() * 10.f;
            cameraFocus += spaceshipTransform->getUp() * 1.f;

            cameraUp = spaceshipTransform->getUp();

            targetFov = glm::radians(70.f);
        } else {
            cameraTargetPosition = m_camera->m_position;
            cameraFocus = spaceshipTransform->getPosition();

            cameraUp = m_camera->m_up;

            targetFov = glm::radians(10.f);
        }

        m_camera->m_position = glm::mix(m_camera->m_position, cameraTargetPosition, 10.f * deltaTime);
        m_camera->m_forward = cameraFocus - m_camera->m_position;
        m_camera->m_up = glm::mix(m_camera->m_up, cameraUp, deltaTime);
        m_camera->m_fov = glm::mix(m_camera->m_fov, targetFov, deltaTime * 0.1f);

        m_skyboxModel->getInstance(0).m_modelTransform = glm::translate(glm::mat4 { 1.f }, m_camera->m_position);

        m_modelSystem.updateTransforms();
        m_lightSystem.update();

        m_camera->updateBuffer();
        m_spaceshipModel->updateInstanceBuffer();
        m_skyboxModel->updateInstanceBuffer();
        m_bulletModel->updateInstanceBuffer();
        m_asteroidModel->updateInstanceBuffer();
    }

    void end() override {
        m_camera->cleanup();

        m_light->cleanup();
        m_lightQuad->cleanup();
        m_lightMaterialInstance->cleanup();
        m_lightMaterial->cleanup();

        for (auto& [ _, light ] : m_lightSystem.m_shadowMappedLights) {
            light->cleanup();
            light->m_materialInstance.cleanup();
        }

        m_shadowMappedLightMaterialInstance->cleanup();
        m_shadowMappedLightMaterial->cleanup();

        m_hdrColourCorrection.cleanup();
        m_taa.cleanup();

        m_asteroidModel->cleanup();
        m_asteroidMesh->cleanup();
        m_asteroidMaterialInstance->cleanup();

        m_spaceshipModel->cleanup();
        m_spaceshipMesh->cleanup();
        m_spaceshipMaterialInstance->cleanup();

        m_objectMaterial->cleanup();

        m_bulletModel->cleanup();
        m_bulletMesh->cleanup();
        m_bulletMaterialInstance->cleanup();
        m_bulletMaterial->cleanup();

        m_skyboxModel->cleanup();
        m_skyboxMesh->cleanup();
        m_skyboxMaterialInstance->cleanup();
        m_skyboxMaterial->cleanup();
    }
};
