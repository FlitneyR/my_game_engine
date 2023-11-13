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
    float m_angle, m_near, m_far;
    uint8_t m_type;

    enum Type {
        e_ambient = 0,
        e_directional,
        e_point,
        e_spot,
    };

    LightInstance(glm::vec3 colour = {}) : m_colour(colour) {}

    void updateShadowMapView(Camera& shadowMapView);

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

    void bindPipeline(vk::CommandBuffer cmd) override;

protected:
    bool shouldCreateShadowMappingPipeline() override { return false; }

    vk::PipelineRasterizationStateCreateInfo getRasterizationState() override;
    vk::PipelineColorBlendStateCreateInfo getColorBlendState() override;

    void createDescriptorSetLayouts() override;

    void modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) override;
};

class ShadowMappedLightMaterialInstance : public MaterialInstanceBase {
    Engine* r_engine;

    Texture m_texture;

    vk::Framebuffer m_framebuffer;
    
    vk::Image m_image;
    vk::ImageView m_imageView;
    vk::DeviceMemory m_imageMemory;

    vk::Sampler m_sampler;

    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;

public:
    Camera m_shadowMapView;

    ShadowMappedLightMaterialInstance(Engine& engine) :
        r_engine(&engine), m_shadowMapView(engine)
    {}

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup(uint32_t width, uint32_t height);
    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout);
    void cleanup();
    void updateViewBuffer(LightInstance& lightInstance);
    void beginShadowMapRenderPass(vk::CommandBuffer cmd, LightInstance &lightInstance);

private:
    void setupImage();
    void allocateImageMemory();
    void setupImageView();
    void setupFramebuffer();
    void createDescriptorSetLayout();
    void setupDescriptorPool();
    void setupSampler();
    void setupDescriptorSet();
};

class Light : public Model<PointVertex, LightInstance, SimpleMaterialInstance, Camera> {
public:
    Light(Engine& engine, Mesh& mesh, Material& material, SimpleMaterialInstance m_materialInstance) :
        Model(engine, mesh, material, m_materialInstance)
    {}
};

class ShadowMappedLight : public Model<PointVertex, LightInstance, ShadowMappedLightMaterialInstance, Camera> {
public:
    ShadowMappedLight(Engine& engine, Mesh& mesh, Material& material, ShadowMappedLightMaterialInstance m_materialInstance) :
        Model(engine, mesh, material, m_materialInstance)
    {}
};

class ShadowMappedLightMaterial : public ShadowMappedLight::Material {
public:
    ShadowMappedLightMaterial(Engine& engine, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) :
        Material(engine, vertexShader, fragmentShader)
    {}

    void bindPipeline(vk::CommandBuffer cmd) override;

protected:
    bool shouldCreateShadowMappingPipeline() override { return false; }
    vk::PipelineRasterizationStateCreateInfo getRasterizationState() override;
    vk::PipelineColorBlendStateCreateInfo getColorBlendState() override;
    void createDescriptorSetLayouts() override;
    void modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) override;
};

}

#endif