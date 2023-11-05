#include <engine.hpp>
#include <model.hpp>
#include <camera.hpp>
#include <objloader.hpp>

// #include "asteroid.hpp"
#include "skybox.hpp"
// #include "bullet.hpp"
// #include "spaceship.hpp"
#include "logic.hpp"
#include <modelInstance.hpp>

#include <iostream>
#include <memory>

class Game : public mge::Engine {
    typedef mge::Model<
        mge::ModelVertex,
        mge::ModelTransformMeshInstance,
        mge::NTextureMaterialInstance<2>,
        mge::Camera
        > ObjectModel;
    
    typedef mge::Model<
        mge::ModelVertex,
        mge::ModelTransformMeshInstance,
        mge::NTextureMaterialInstance<1>,
        mge::Camera
        > SkyboxModel;
    
    typedef mge::Model<
        mge::PointColorVertex,
        mge::ModelTransformMeshInstance,
        mge::SimpleMaterialInstance,
        mge::Camera
        > BulletModel;

    std::unique_ptr<ObjectModel::Material> m_objectMaterial;

    std::unique_ptr<ObjectModel> m_asteroidModel;
    std::unique_ptr<ObjectModel::Mesh> m_asteroidMesh;
    std::unique_ptr<ObjectModel::Material::Instance> m_asteroidMaterialInstance;
    mge::Texture m_asteroidAlbedo, m_asteroidARM;

    std::unique_ptr<ObjectModel> m_spaceshipModel;
    std::unique_ptr<ObjectModel::Mesh> m_spaceshipMesh;
    std::unique_ptr<ObjectModel::Material::Instance> m_spaceshipMaterialInstance;
    mge::Texture m_spaceshipAlbedo, m_spaceshipARM;

    std::unique_ptr<BulletModel> m_bulletModel;
    std::unique_ptr<BulletModel::Mesh> m_bulletMesh;
    std::unique_ptr<BulletModel::Material> m_bulletMaterial;
    std::unique_ptr<BulletModel::Material::Instance> m_bulletMaterialInstance;

    std::unique_ptr<SkyboxModel> m_skyboxModel;
    std::unique_ptr<SkyboxModel::Mesh> m_skyboxMesh;
    std::unique_ptr<SkyboxMaterial> m_skyboxMaterial;
    std::unique_ptr<SkyboxModel::Material::Instance> m_skyboxMaterialInstance;
    mge::Texture m_skyboxTexture;

    std::unique_ptr<mge::Camera> m_camera;

    mge::ecs::ECSManager m_ecsManager;

    AsteroidSystem m_asteroidSystem;
    BulletSystem m_bulletSystem;
    SpaceshipSystem m_spaceshipSystem;
    mge::ecs::RigidbodySystem m_rigidbodySystem;
    mge::ecs::System<mge::ecs::TransformComponent> m_transformSystem;
    mge::ecs::CollisionSystem m_collisionSystem;
    mge::ecs::ModelSystem m_modelSystem;

    std::string getGameName() override { return "Asteroids"; }
    
    void start() override {
        std::srand(std::chrono::system_clock::now().time_since_epoch().count());

        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_near = 0.1f;
        m_camera->m_far = 10'000.f;
        m_camera->m_fov = glm::radians(70.f);
        m_camera->m_position = glm::vec3 { 0.f, 0.f, 0.f };
        m_camera->m_forward = glm::vec3 { 0.f, 1.f, 0.f };
        m_camera->m_up = glm::vec3 { 0.f, 0.f, 1.f };

        m_asteroidMesh = std::make_unique<ObjectModel::Mesh>(mge::loadObjMesh(*this, "assets/asteroid.obj"));
        m_spaceshipMesh = std::make_unique<ObjectModel::Mesh>(mge::loadObjMesh(*this, "assets/spaceship.obj"));
        m_skyboxMesh = std::make_unique<SkyboxModel::Mesh>(mge::loadObjMesh(*this, "assets/skybox.obj"));
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
                0, 4, 3,
                0, 2, 4,
                0, 5, 2,
                0, 3, 5,
                1, 3, 4,
                1, 4, 2,
                1, 2, 5,
                1, 5, 3,
            });

        m_asteroidAlbedo = mge::Texture("assets/asteroid_albedo.png");
        m_asteroidARM = mge::Texture("assets/asteroid_arm.png");
        m_spaceshipAlbedo = mge::Texture("assets/spaceship_albedo.png");
        m_spaceshipARM = mge::Texture("assets/spaceship_arm.png");
        m_skyboxTexture = mge::Texture("assets/skybox.png");

        m_objectMaterial = std::make_unique<ObjectModel::Material>(*this,
            loadShaderModule("build/mvp.vert.spv"),
            loadShaderModule("build/pbr.frag.spv")
            );

        m_skyboxMaterial = std::make_unique<SkyboxMaterial>(*this,
            loadShaderModule("build/skybox.vert.spv"),
            loadShaderModule("build/skybox.frag.spv")
            );
        
        m_bulletMaterial = std::make_unique<BulletModel::Material>(*this,
            loadShaderModule("build/bullet.vert.spv"),
            loadShaderModule("build/bullet.frag.spv")
            );

        m_asteroidMaterialInstance = std::make_unique<ObjectModel::Material::Instance>(m_objectMaterial->makeInstance());
        m_asteroidMaterialInstance->setup({ m_asteroidAlbedo, m_asteroidARM });

        m_spaceshipMaterialInstance = std::make_unique<ObjectModel::Material::Instance>(m_objectMaterial->makeInstance());
        m_spaceshipMaterialInstance->setup({ m_spaceshipAlbedo, m_spaceshipARM });

        m_skyboxMaterialInstance = std::make_unique<SkyboxModel::Material::Instance>(m_skyboxMaterial->makeInstance());
        m_skyboxMaterialInstance->setup({ m_skyboxTexture });
    
        m_bulletMaterialInstance = std::make_unique<BulletModel::Material::Instance>(m_bulletMaterial->makeInstance());
        m_bulletMaterialInstance->setup();

        m_asteroidModel = std::make_unique<ObjectModel>(
            *this,
            *m_asteroidMesh,
            *m_objectMaterial,
            *m_asteroidMaterialInstance
            );

        m_spaceshipModel = std::make_unique<ObjectModel>(
            *this,
            *m_spaceshipMesh,
            *m_objectMaterial,
            *m_spaceshipMaterialInstance
            );
        
        m_skyboxModel = std::make_unique<SkyboxModel>(
            *this,
            *m_skyboxMesh,
            *m_skyboxMaterial,
            *m_skyboxMaterialInstance
            );
        
        m_bulletModel = std::make_unique<BulletModel>(
            *this,
            *m_bulletMesh,
            *m_bulletMaterial,
            *m_bulletMaterialInstance
            );
        
        m_skyboxModel->makeInstance();

        m_modelSystem.addModel("Asteroid", m_asteroidModel.get());
        m_modelSystem.addModel("Spaceship", m_spaceshipModel.get());
        m_modelSystem.addModel("Bullet", m_bulletModel.get());

        m_ecsManager.addSystem("Asteroid", &m_asteroidSystem);
        m_ecsManager.addSystem("Bullet", &m_bulletSystem);
        m_ecsManager.addSystem("Spaceship", &m_spaceshipSystem);
        m_ecsManager.addSystem("Rigidbody", &m_rigidbodySystem);
        m_ecsManager.addSystem("Transform", &m_transformSystem);
        m_ecsManager.addSystem("Model", &m_modelSystem);
        m_ecsManager.addSystem("Collision", &m_collisionSystem);
        
        makeTemplates();

        for (int i = 0; i < 8'000; i++)
            auto entity = m_ecsManager.makeEntityFromTemplate("Asteroid");

        auto spaceshipEntity = m_ecsManager.makeEntityFromTemplate("Spaceship");
        auto spaceshipTransform = m_transformSystem.getComponent(spaceshipEntity);

        m_camera->m_position = spaceshipTransform->getPosition() + spaceshipTransform->getUp();
        m_camera->m_up = spaceshipTransform->getUp();

        m_camera->setup();

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
    }

    void makeTemplates() {
        m_ecsManager.addTemplate("Asteroid", [&](mge::ecs::ECSManager& ecs) {
            auto entity = ecs.makeEntity();

            auto transform = m_transformSystem.addComponent(entity);
            auto rigidbody = m_rigidbodySystem.addComponent(entity);
            auto model = m_modelSystem.addComponent(entity, "Asteroid");
            auto collider = m_collisionSystem.addComponent(entity);
            auto asteroid = m_asteroidSystem.addComponent(entity);

            float radius = glm::mix(5.f, 40.f, glm::pow(randomRangeFloat(0.f, 1.f), 40.f));
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

            bulletRigidbody->m_physicsType = bulletRigidbody->e_dynamic;
            bulletRigidbody->m_mass = 0.001f;

            bulletCollision->setupSphere(1.f);

            return entity;
        });

        m_ecsManager.addTemplate("Spaceship", [&](mge::ecs::ECSManager& ecs){
            auto entity = ecs.makeEntity();

            m_modelSystem.addComponent(entity, "Spaceship");
            m_spaceshipSystem.addComponent(entity);
            m_transformSystem.addComponent(entity);
            auto collision = m_collisionSystem.addComponent(entity);
            auto rigidbody = m_rigidbodySystem.addComponent(entity);

            collision->setupSphere(2.f);

            rigidbody->m_physicsType = rigidbody->e_dynamic;
            rigidbody->m_mass = 50.f;

            return entity;
        });
    }

    void recordDrawCommands(vk::CommandBuffer cmd) override {
        m_camera->updateBuffer();

        m_skyboxMaterial->bindUniform(cmd, *m_camera);
        m_skyboxModel->drawInstances(cmd);

        m_objectMaterial->bindUniform(cmd, *m_camera);

        m_spaceshipModel->drawInstances(cmd);
        
        m_asteroidModel->drawInstances(cmd);

        m_bulletMaterial->bindUniform(cmd, *m_camera);
        m_bulletModel->drawInstances(cmd);
    }

    void update(double deltaTime) override {
        auto collisionEvents = m_collisionSystem.getCollisionEvents();

        m_bulletSystem.handleCollisions(collisionEvents);
        m_spaceshipSystem.checkForAsteroidCollision(collisionEvents);
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
            cameraFocus += spaceshipTransform->getUp() * 2.5f;

            cameraUp = spaceshipTransform->getUp();

            targetFov = glm::radians(70.f);
        } else {
            cameraTargetPosition = m_camera->m_position;
            cameraFocus = spaceshipTransform->getPosition();

            cameraUp = m_camera->m_up;

            targetFov = glm::radians(10.f);
        }

        m_camera->m_position = glm::mix(m_camera->m_position, cameraTargetPosition, 5.f * deltaTime);
        m_camera->m_forward = cameraFocus - m_camera->m_position;
        m_camera->m_up = glm::mix(m_camera->m_up, cameraUp, deltaTime);
        m_camera->m_fov = glm::mix(m_camera->m_fov, targetFov, deltaTime * 0.3f);

        // m_camera->m_position = glm::vec3 { 0.f, -30.f, 20.f };
        // m_camera->m_forward = glm::vec3 { 0.f, 3.f, -2.f };

        m_skyboxModel->getInstance(0).m_modelTransform = glm::translate(glm::mat4 { 1.f }, m_camera->m_position);
        m_modelSystem.updateTransforms();

        m_spaceshipModel->updateInstanceBuffer();
        m_skyboxModel->updateInstanceBuffer();
        m_bulletModel->updateInstanceBuffer();
        m_asteroidModel->updateInstanceBuffer();
    }

    void end() override {
        m_camera->cleanup();

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
