#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <uniform.hpp>

namespace mge {

struct CameraUniformData {
    glm::mat4 m_view;
    glm::mat4 m_projection;

    static std::vector<vk::DescriptorSetLayoutBinding> getBindings() {
        return std::vector<vk::DescriptorSetLayoutBinding> {
            vk::DescriptorSetLayoutBinding {}
                .setBinding(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eAllGraphics)
                ,
        };
    }
};

struct Camera : public Uniform<CameraUniformData> {
    glm::vec3 m_position;
    glm::vec3 m_forward;
    glm::vec3 m_up;

    float m_fov;
    float m_aspect;
    float m_near;
    float m_far;

    Camera(Engine& engine) {
        r_engine = &engine;
        m_forward = glm::vec3(0.f, 0.f, 1.f);
        m_up = glm::vec3(0.f, 1.f, 0.f);

        m_fov = glm::radians(70.f);

        m_near = 0.01f;
        m_far = 1000.f;
    }

    CameraUniformData getUniformData() override {
        CameraUniformData result;

        m_aspect = static_cast<float>(r_engine->m_swapchainExtent.width);
        m_aspect /= static_cast<float>(r_engine->m_swapchainExtent.height);

        result.m_view = glm::lookAt(m_position, m_position + m_forward, m_up);
        result.m_projection = glm::perspective(m_fov, m_aspect, m_near, m_far);
        result.m_projection = glm::rotate(result.m_projection, glm::radians(180.f), glm::vec3(0.f, 0.f, 1.f));

        return result;
    }
};

}

#endif