#include <engine.hpp>
#include <model.hpp>
#include <camera.hpp>
#include <objloader.hpp>

#include "asteroid.hpp"
#include "skybox.hpp"
#include "bullet.hpp"
#include "spaceship.hpp"

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

    std::vector<Asteroid> m_asteroids;
    std::vector<Bullet> m_bullets;
    Spaceship m_spaceship;

    std::unique_ptr<mge::Camera> m_camera;

    std::string getGameName() override { return "Asteroids"; }
    
    void start() override {
        std::srand(std::chrono::system_clock::now().time_since_epoch().count());

        m_camera = std::make_unique<mge::Camera>(*this);
        m_camera->m_far = 10'000.f;

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
        
        for (int i = 0; i < 2000; i++) {
            Asteroid asteroid;

            asteroid.m_modelInstanceId = m_asteroidModel->makeInstance();
            asteroid.start();

            m_asteroids.push_back(asteroid);
        }

        m_spaceship.m_modelInstanceId = m_spaceshipModel->makeInstance();
        m_spaceship.m_position = glm::vec3(0.f);
        m_spaceship.m_velocity = glm::vec3(0.f);
        m_spaceship.m_yaw = 0.f;
        m_spaceship.m_roll = 0.f;

        m_camera->m_up = glm::vec3(0.f, 0.f, 1.f);

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

    void recordDrawCommands(vk::CommandBuffer cmd) override {
        m_skyboxMaterial->bindUniform(cmd, *m_camera);
        m_skyboxModel->drawInstances(cmd);

        m_objectMaterial->bindUniform(cmd, *m_camera);

        m_spaceshipModel->drawInstances(cmd);
        
        m_asteroidModel->drawInstances(cmd);

        m_bulletMaterial->bindUniform(cmd, *m_camera);
        m_bulletModel->drawInstances(cmd);
    }

    void update(double deltaTime) override {
        for (int i = m_bullets.size() - 1; i >= 0; i--) {
            Bullet& bullet = m_bullets[i];
            bullet.update(deltaTime);
            m_bulletModel->getInstance(bullet.m_meshInstanceId).m_modelTransform = bullet.getTransform();

            bool shouldDestroyBullet = glm::distance(bullet.m_position, m_camera->m_position) > 200.f;

            if (!shouldDestroyBullet)
            for (int j = m_asteroids.size() - 1; j >= 0; j--) {
                Asteroid& asteroid = m_asteroids[j];

                if (bullet.isColliding(asteroid)) {
                    glm::vec3 breakDirection = randomUnitVector() * glm::vec3(2.f, 2.f, 0.f) * asteroid.m_radius;
                    float ratio = randomRangeFloat(0.1f, 0.9f);

                    if (asteroid.m_radius > 2.f)
                    for (int i = -1; i <= 1; i += 2) {
                        Asteroid newAsteroid;
                        newAsteroid.start();
                        newAsteroid.m_radius = asteroid.m_radius * glm::lerp(ratio, 1.f - ratio, 0.5f + 0.5f * (float)i);
                        newAsteroid.m_linearVelocity = (float)i * breakDirection;
                        newAsteroid.m_position = asteroid.m_position + newAsteroid.m_linearVelocity * 0.25f;
                        newAsteroid.m_angularVelocity *= 3.f;

                        if (newAsteroid.m_radius < 1.f) continue;

                        newAsteroid.m_modelInstanceId = m_asteroidModel->makeInstance();
                        m_asteroids.push_back(newAsteroid);
                    }

                    m_asteroidModel->destroyInstance(asteroid.m_modelInstanceId);

                    std::swap(asteroid, m_asteroids.back());
                    m_asteroids.pop_back();

                    shouldDestroyBullet = true;
                }
            }

            if (shouldDestroyBullet) {
                m_bulletModel->destroyInstance(bullet.m_meshInstanceId);
                bullet = m_bullets.back();
                m_bullets.pop_back();
            }
        }

        for (auto& asteroid : m_asteroids) {
            if (m_spaceship.isColliding(asteroid))
                m_spaceship.die();

            for (auto& other : m_asteroids)
            if (asteroid.m_modelInstanceId != other.m_modelInstanceId)
            if (asteroid.isColliding(other))
                asteroid.resolveCollision(other);
            
            asteroid.update(deltaTime, m_camera->m_position);
            m_asteroidModel->getInstance(asteroid.m_modelInstanceId).m_modelTransform = asteroid.getTransform();
        }

        m_spaceship.update(
            glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS,
            glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS,
            glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS,
            deltaTime
            );
        
        m_spaceshipModel->getInstance(m_spaceship.m_modelInstanceId).m_modelTransform = m_spaceship.getTransform();

        if (m_spaceship.m_alive) {
            m_camera->m_position = glm::lerp(m_camera->m_position, m_spaceship.getCameraPosition(), (float)deltaTime * 5.f);
            m_camera->m_forward = m_spaceship.getCameraTarget() - m_camera->m_position;
        } else {
            m_camera->m_fov = glm::lerp(m_camera->m_fov, glm::radians(30.f), (float)deltaTime);
            m_camera->m_forward = glm::lerp(m_camera->m_forward, m_spaceship.m_position - m_camera->m_position, (float)deltaTime);
        }

        m_camera->updateBuffer();

        m_skyboxModel->getInstance(0).m_modelTransform = glm::translate(glm::mat4(1.f), m_camera->m_position);

        m_asteroidModel->updateInstanceBuffer();
        m_spaceshipModel->updateInstanceBuffer();
        m_bulletModel->updateInstanceBuffer();
        m_skyboxModel->updateInstanceBuffer();

        if (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(m_window, GLFW_TRUE);
        
        static int previousShootState = GLFW_RELEASE;

        if (m_spaceship.m_alive && glfwGetKey(m_window, GLFW_KEY_Z) == GLFW_PRESS) {
            if (previousShootState == GLFW_RELEASE) {
                Bullet bullet {};

                bullet.m_velocity = m_spaceship.getForward() * 100.f;

                bullet.m_position = m_spaceship.m_position + m_spaceship.getForward() * 0.5f + m_spaceship.getRight();
                bullet.m_meshInstanceId = m_bulletModel->makeInstance();
                m_bullets.push_back(bullet);

                bullet.m_position = m_spaceship.m_position + m_spaceship.getForward() * 0.5f - m_spaceship.getRight();
                bullet.m_meshInstanceId = m_bulletModel->makeInstance();
                m_bullets.push_back(bullet);

                previousShootState = GLFW_PRESS;
            }
        } else previousShootState = GLFW_RELEASE;
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