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

    void updateShadowMapView(mge::Camera& shadowMapView) {
        shadowMapView.m_position = m_position;
        shadowMapView.m_forward = m_direction;
        shadowMapView.m_near = m_near;
        shadowMapView.m_far = m_far;

        switch (m_type) {
        case e_directional:
            shadowMapView.m_projectionType = shadowMapView.e_orthographic;
            shadowMapView.m_viewport = glm::vec2 { m_angle, m_angle };
            break;
        case e_spot:
            shadowMapView.m_projectionType = shadowMapView.e_perspective;
            shadowMapView.m_fov = m_angle * 2.f;
            shadowMapView.m_viewport = std::nullopt;
            break;
        default: throw std::runtime_error("Cannot get shadow map view for this type of light");
        }

        shadowMapView.updateBuffer();
    }

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
    bool shouldCreateShadowMappingPipeline() override { return false; }

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

    void modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) override {
        createInfo.setSubpass(1);
    }
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
    mge::Camera m_shadowMapView;

    ShadowMappedLightMaterialInstance(Engine& engine) :
        r_engine(&engine), m_shadowMapView(engine)
    {}

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    void setup(uint32_t width, uint32_t height) {
        m_shadowMapView.setup();

        m_texture = Texture(width, height,
            r_engine->m_depthImageFormat,
            vk::ImageUsageFlagBits::eDepthStencilAttachment,
            mge::Texture::e_depth
            );

        setupImage();
        allocateImageMemory();
        setupImageView();

        setupFramebuffer();

        createDescriptorSetLayout();
        setupDescriptorPool();
        
        setupSampler();
        setupLightViewBuffer();

        setupDescriptorSet();
    }

    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {
        cmd.bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics,
            pipelineLayout, 1,
            m_descriptorSets[r_engine->m_currentInFlightFrame],
            nullptr
        );
    }

    void cleanup() {
        m_shadowMapView.cleanup();

        r_engine->m_device.destroyFramebuffer(m_framebuffer);
        r_engine->m_device.destroyImage(m_image);
        r_engine->m_device.destroyImageView(m_imageView);
        r_engine->m_device.destroySampler(m_sampler);
        r_engine->m_device.freeMemory(m_imageMemory);
        // r_engine->m_device.destroyBuffer(m_lightViewBuffer);
        // r_engine->m_device.freeMemory(m_lightViewBufferMemory);
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);

        if (s_descriptorSetLayout != VK_NULL_HANDLE) {
            r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);
            s_descriptorSetLayout = VK_NULL_HANDLE;
        }
    }

    void updateViewBuffer(LightInstance& lightInstance) {
        lightInstance.updateShadowMapView(m_shadowMapView);
    }

    void beginShadowMapRenderPass(vk::CommandBuffer cmd, mge::LightInstance &lightInstance) {
        updateViewBuffer(lightInstance);

        auto clearValue = vk::ClearValue {}
            .setDepthStencil({ 1.0f, 0 })
            ;
        
        auto viewport = vk::Viewport {}
            .setX(0.f)
            .setY(0.f)
            .setWidth(m_texture.m_width)
            .setHeight(m_texture.m_height)
            .setMinDepth(0.f)
            .setMaxDepth(1.f)
            ;

        auto scissor = vk::Rect2D {}
            .setOffset({ 0, 0 })
            .setExtent({ m_texture.m_width, m_texture.m_height })
            ;

        cmd.beginRenderPass(vk::RenderPassBeginInfo {}
            .setClearValues(clearValue)
            .setFramebuffer(m_framebuffer)
            .setRenderPass(r_engine->m_shadowMappingRenderPass)
            .setRenderArea(scissor),
            vk::SubpassContents::eInline
            );
        
        cmd.setViewport(0, viewport);
        cmd.setScissor(0, scissor);
    }

private:
    void setupImage() {
        m_image = r_engine->m_device.createImage(vk::ImageCreateInfo {}
            .setArrayLayers(1)
            .setExtent({ m_texture.m_width, m_texture.m_height, 1 })
            .setFormat(m_texture.m_format)
            .setImageType(vk::ImageType::e2D)
            .setInitialLayout(vk::ImageLayout::eUndefined)
            .setMipLevels(1)
            .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
            .setSamples(vk::SampleCountFlagBits::e1)
            .setSharingMode(vk::SharingMode::eExclusive)
            .setTiling(vk::ImageTiling::eOptimal)
            .setUsage(vk::ImageUsageFlagBits::eDepthStencilAttachment
                    | vk::ImageUsageFlagBits::eSampled)
            );
    }

    void allocateImageMemory() {
        auto memreqs = r_engine->m_device.getImageMemoryRequirements(m_image);

        m_imageMemory = r_engine->m_device.allocateMemory(vk::MemoryAllocateInfo {}
            .setAllocationSize(memreqs.size)
            .setMemoryTypeIndex(r_engine->findMemoryType(memreqs.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal))
            );
        
        r_engine->m_device.bindImageMemory(m_image, m_imageMemory, {});
    }

    void setupImageView() {
        m_imageView = r_engine->m_device.createImageView(vk::ImageViewCreateInfo {}
            .setFormat(m_texture.m_format)
            .setImage(m_image)
            .setSubresourceRange(vk::ImageSubresourceRange {}
                .setAspectMask(vk::ImageAspectFlagBits::eDepth)
                .setBaseArrayLayer(0)
                .setBaseMipLevel(0)
                .setLayerCount(1)
                .setLevelCount(1)
                )
            .setViewType(vk::ImageViewType::e2D)
            );
    }

    void setupFramebuffer() {
        m_framebuffer = r_engine->m_device.createFramebuffer(vk::FramebufferCreateInfo {}
            .setRenderPass(r_engine->m_shadowMappingRenderPass)
            .setAttachments(m_imageView)
            .setWidth(m_texture.m_width)
            .setHeight(m_texture.m_height)
            .setLayers(1)
            );
    }

    void createDescriptorSetLayout() {
        if (s_descriptorSetLayout != VK_NULL_HANDLE) return;

        std::vector<vk::DescriptorSetLayoutBinding> bindings {
            vk::DescriptorSetLayoutBinding {}
                .setBinding(0)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
                ,
            vk::DescriptorSetLayoutBinding {}
                .setBinding(1)
                .setDescriptorCount(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setStageFlags(vk::ShaderStageFlagBits::eFragment)
            };

        s_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(vk::DescriptorSetLayoutCreateInfo {}
            .setBindings(bindings));
    }

    void setupDescriptorPool() {
        std::vector<vk::DescriptorPoolSize> poolSizes {
            vk::DescriptorPoolSize {}
                .setDescriptorCount(r_engine->getMaxFramesInFlight())
                .setType(vk::DescriptorType::eCombinedImageSampler)
                ,
            vk::DescriptorPoolSize {}
                .setDescriptorCount(r_engine->getMaxFramesInFlight())
                .setType(vk::DescriptorType::eUniformBuffer)
        };

        m_descriptorPool = r_engine->m_device.createDescriptorPool(vk::DescriptorPoolCreateInfo {}
            .setMaxSets(r_engine->getMaxFramesInFlight())
            .setPoolSizes(poolSizes)
            );
    }

    void setupSampler() {
        m_sampler = r_engine->m_device.createSampler(vk::SamplerCreateInfo {}
            .setAddressModeU(vk::SamplerAddressMode::eClampToBorder)
            .setAddressModeV(vk::SamplerAddressMode::eClampToBorder)
            .setMagFilter(vk::Filter::eLinear)
            .setMinFilter(vk::Filter::eLinear)
            .setMipmapMode(vk::SamplerMipmapMode::eLinear)
            .setBorderColor(vk::BorderColor::eFloatOpaqueWhite)
            );
    }

    void setupLightViewBuffer() {
        // m_lightViewBuffer = r_engine->m_device.createBuffer(vk::BufferCreateInfo {}
        //     .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        //     .setSharingMode(vk::SharingMode::eExclusive)
        //     .setSize(sizeof(CameraUniformData))
        //     .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        //     );
        
        // auto memreqs = r_engine->m_device.getBufferMemoryRequirements(m_lightViewBuffer);

        // m_lightViewBufferMemory = r_engine->m_device.allocateMemory(vk::MemoryAllocateInfo {}
        //     .setAllocationSize(memreqs.size)
        //     .setMemoryTypeIndex(r_engine->findMemoryType(memreqs.memoryTypeBits,
        //         vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent)
        //     ));
        
        // r_engine->m_device.bindBufferMemory(m_lightViewBuffer, m_lightViewBufferMemory, {});
    }

    void setupDescriptorSet() {
        std::vector<vk::DescriptorSetLayout> layouts(r_engine->getMaxFramesInFlight(), s_descriptorSetLayout);

        m_descriptorSets = r_engine->m_device.allocateDescriptorSets(vk::DescriptorSetAllocateInfo {}
            .setDescriptorPool(m_descriptorPool)
            .setDescriptorSetCount(r_engine->getMaxFramesInFlight())
            .setSetLayouts(layouts)
            );
        
        for (int i = 0; i < m_descriptorSets.size(); i++) {
            r_engine->m_device.updateDescriptorSets(vk::WriteDescriptorSet {}
                .setDstSet(m_descriptorSets[i])
                .setDstBinding(0)
                .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
                .setImageInfo(vk::DescriptorImageInfo {}
                    .setImageView(m_imageView)
                    .setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
                    .setSampler(m_sampler)
                    ),
                {});
            
            auto memreqs = r_engine->m_device.getBufferMemoryRequirements(m_shadowMapView.m_buffers[i]);
            
            r_engine->m_device.updateDescriptorSets(vk::WriteDescriptorSet {}
                .setDstSet(m_descriptorSets[i])
                .setDstBinding(1)
                .setDescriptorType(vk::DescriptorType::eUniformBuffer)
                .setBufferInfo(vk::DescriptorBufferInfo {}
                    .setBuffer(m_shadowMapView.m_buffers[i])
                    .setOffset({})
                    .setRange(memreqs.size)),
                {});
        }
    }
};

vk::DescriptorSetLayout ShadowMappedLightMaterialInstance::s_descriptorSetLayout = VK_NULL_HANDLE;

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

    void bindPipeline(vk::CommandBuffer cmd) override {
        ShadowMappedLight::Material::bindPipeline(cmd);
        std::vector<uint32_t> offsets;
        cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, m_pipelineLayout, 2, r_engine.m_gbufferDescriptorSet, offsets);
    }

protected:
    bool shouldCreateShadowMappingPipeline() override { return false; }

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
        ShadowMappedLight::Material::createDescriptorSetLayouts();

        m_descriptorSetLayouts.push_back(r_engine.m_gbufferDescriptorSetLayout);
    }

    void modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) override {
        createInfo.setSubpass(1);
    }
};

}

#endif