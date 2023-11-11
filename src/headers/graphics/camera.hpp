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

    std::optional<glm::vec2> m_viewport;

    enum ProjectionType {
        e_perspective, e_orthographic
    } m_projectionType;

    Camera(Engine& engine) {
        r_engine = &engine;
        m_forward = glm::vec3(0.f, 1.f, 0.f);
        m_up = glm::vec3(0.f, 0.f, 1.f);

        m_fov = glm::radians(70.f);

        m_near = 0.01f;
        m_far = 1000.f;
    }

    CameraUniformData getUniformData() override {
        CameraUniformData result;

        glm::vec2 viewport;

        if (m_viewport) {
            viewport = *m_viewport;
        } else {
            viewport.x = r_engine->m_swapchainExtent.width;
            viewport.y = r_engine->m_swapchainExtent.height;
        }

        m_aspect = viewport.x / viewport.y;

        result.m_view = glm::lookAt(m_position, m_position + m_forward, m_up);

        switch (m_projectionType) {
        case e_perspective:
            result.m_projection = glm::perspective(m_fov, m_aspect, m_near, m_far);
            break;
        case e_orthographic:
            result.m_projection = glm::ortho(-viewport.x / 2.f, viewport.x / 2.f, -viewport.y / 2.f, viewport.y / 2.f, m_near, m_far);
            break;
        }

        result.m_projection = glm::scale(result.m_projection, glm::vec3 { 1.f, -1.f, 1.f });

        return result;
    }

    glm::vec3 getRight() {
        return glm::normalize(glm::cross(m_forward, m_up));
    }
};

}

#endif