#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <uniform.hpp>

namespace mge {

struct CameraUniformData {
    alignas(16) glm::mat4 m_view;
    alignas(16) glm::mat4 m_projection;
    alignas(16) glm::vec2 m_jitter;

    alignas(16) glm::mat4 m_previousView;
    alignas(16) glm::mat4 m_previousProjection;
    alignas(16) glm::vec2 m_previousJitter;

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

    bool m_taaJitter = false;

    std::optional<glm::vec2> m_shadowMapRange;

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

    glm::vec3 getJitter() {
        static const std::vector<glm::vec2> offsets {
            { 0.500000, 0.333333 },
            { 0.250000, 0.666667 },
            { 0.750000, 0.111111 },
            { 0.125000, 0.444444 },
            { 0.625000, 0.777778 },
            { 0.375000, 0.222222 },
            { 0.875000, 0.555556 },
            { 0.062500, 0.888889 },
            { 0.562500, 0.037037 },
            { 0.312500, 0.370370 },
            { 0.812500, 0.703704 },
            { 0.187500, 0.148148 },
            { 0.687500, 0.481481 },
            { 0.437500, 0.814815 },
            { 0.937500, 0.259259 },
            { 0.031250, 0.592593 },
        };
        
        glm::vec2 offset = offsets[r_engine->m_framecount % offsets.size()];

        glm::vec2 texel {
            r_engine->m_swapchainExtent.width,
            r_engine->m_swapchainExtent.height
        };

        texel = 1.f / texel;

        return { offset * texel, 0.f };
    }

    void updateUniformData(CameraUniformData& uniform) override {
        uniform.m_previousProjection = uniform.m_projection;
        uniform.m_previousView = uniform.m_view;
        uniform.m_previousJitter = uniform.m_jitter;

        glm::vec2 viewport;

        if (m_shadowMapRange) {
            viewport = *m_shadowMapRange;
        } else {
            viewport.x = r_engine->m_swapchainExtent.width;
            viewport.y = r_engine->m_swapchainExtent.height;
        }

        m_aspect = viewport.x / viewport.y;

        uniform.m_view = glm::lookAt(m_position, m_position + m_forward, m_up);

        switch (m_projectionType) {
        case e_perspective:
            uniform.m_projection = glm::perspective(m_fov, m_aspect, m_near, m_far);
            break;
        case e_orthographic:
            uniform.m_projection = glm::ortho(-viewport.x / 2.f, viewport.x / 2.f, -viewport.y / 2.f, viewport.y / 2.f, m_near, m_far);
            break;
        }

        uniform.m_projection = glm::scale(uniform.m_projection, glm::vec3 { 1.f, -1.f, 1.f });

        if (m_taaJitter) uniform.m_jitter = getJitter();
        else uniform.m_jitter = glm::vec2 { 0.f, 0.f };
    }

    glm::vec3 getRight() {
        return glm::normalize(glm::cross(m_forward, m_up));
    }
};

}

#endif