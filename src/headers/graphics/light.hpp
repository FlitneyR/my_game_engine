#ifndef LIGHT_HPP
#define LIGHT_HPP

#include <material.hpp>
#include <vertex.hpp>
#include <instance.hpp>
#include <camera.hpp>
#include <model.hpp>
#include <mesh.hpp>

namespace mge {

class LightInstance : public MeshInstanceBase {
public:
    glm::vec3 m_colour;
    glm::vec3 m_position;
    glm::vec3 m_direction;
    float m_angle;
    uint8_t m_type;

    enum Type {
        e_ambient = 0,
        e_directional,
        e_point,
        e_spot,
    };

    LightInstance(glm::vec3 colour = {}) : m_colour(colour) {}

    static constexpr std::vector<vk::VertexInputAttributeDescription> getAttributes(uint32_t firstAvailableLocation) {
        return std::vector<vk::VertexInputAttributeDescription>{
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(LightInstance, m_colour))
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(LightInstance, m_position))
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setFormat(vk::Format::eR32G32B32Sfloat)
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(LightInstance, m_direction))
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setFormat(vk::Format::eR32Sfloat)
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(LightInstance, m_angle))
                ,
            vk::VertexInputAttributeDescription {}
                .setBinding(1)
                .setFormat(vk::Format::eR8Uint)
                .setLocation(firstAvailableLocation++)
                .setOffset(offsetof(LightInstance, m_type))
                ,
        };
    }

    static constexpr std::vector<vk::VertexInputBindingDescription> getBindings() {
        return std::vector<vk::VertexInputBindingDescription>{
            vk::VertexInputBindingDescription {}
                .setBinding(1)
                .setInputRate(vk::VertexInputRate::eInstance)
                .setStride(sizeof(LightInstance))
                ,
        };
    }
};

class LightMaterial : public Material<PointVertex, LightInstance, SimpleMaterialInstance, Camera> {
public:
    LightMaterial(Engine& engine, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) :
        Material(engine, vertexShader, fragmentShader)
    {}

    void bindPipeline(vk::CommandBuffer cmd) override {
        Material<PointVertex, LightInstance, SimpleMaterialInstance, Camera>::bindPipeline(cmd);
        std::vector<uint32_t> offsets;
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 1, r_engine.m_gbufferDescriptorSet, offsets);
    }

protected:

    vk::PipelineRasterizationStateCreateInfo getRasterizationState() override {
        return vk::PipelineRasterizationStateCreateInfo {}
            .setCullMode(vk::CullModeFlagBits::eFront)
            .setFrontFace(vk::FrontFace::eClockwise)
            .setLineWidth(1.f)
            .setDepthClampEnable(false)
            .setRasterizerDiscardEnable(false)
            .setPolygonMode(vk::PolygonMode::eFill)
            ;
    }

    vk::PipelineColorBlendStateCreateInfo getColorBlendState() override {
        static auto attachment = vk::PipelineColorBlendAttachmentState {}
            .setColorWriteMask( vk::ColorComponentFlagBits::eR
                            |   vk::ColorComponentFlagBits::eG
                            |   vk::ColorComponentFlagBits::eB
                            |   vk::ColorComponentFlagBits::eA)
            .setBlendEnable(true)
            .setColorBlendOp(vk::BlendOp::eAdd)
            .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
            .setDstColorBlendFactor(vk::BlendFactor::eOne)
            .setAlphaBlendOp(vk::BlendOp::eAdd)
            .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
            .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
            ;

        return vk::PipelineColorBlendStateCreateInfo {}
            .setAttachments(attachment)
            ;
    }

    void createDescriptorSetLayouts() override {
        Material<PointVertex, LightInstance, SimpleMaterialInstance, Camera>::createDescriptorSetLayouts();

        m_descriptorSetLayouts.push_back(r_engine.m_gbufferDescriptorSetLayout);
    }

    uint32_t getSubpassIndex() override { return 1; }
};

Mesh<PointVertex> makeFullScreenQuad(Engine& engine) {
    return Mesh<PointVertex>(engine, std::vector<PointVertex> {
        { glm::vec3 { -1.f, -1.f, 0.f } },
        { glm::vec3 {  1.f, -1.f, 0.f } },
        { glm::vec3 { -1.f,  1.f, 0.f } },
        { glm::vec3 {  1.f,  1.f, 0.f } },
    }, std::vector<uint16_t> {
        0, 2, 1,    3, 1, 2,
    });
}

class Light : public Model<PointVertex, LightInstance, SimpleMaterialInstance, Camera> {
public:
    Light(Engine& engine, Mesh& mesh, Material& material, SimpleMaterialInstance m_materialInstance) :
        Model(engine, mesh, material, m_materialInstance)
    {}
};

}

#endif