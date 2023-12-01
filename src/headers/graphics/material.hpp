#ifndef MATERIAL_HPP
#define MATERIAL_HPP

#include <libraries.hpp>
#include <engine.hpp>

#include <iostream>

namespace mge {

#define MATERIAL_TEMPLATE template<typename Vertex, typename MeshInstance, typename MaterialInstance, typename UniformType>
#define MATERIAL Material<Vertex, MeshInstance, MaterialInstance, UniformType>

class MaterialBase {};

MATERIAL_TEMPLATE
class Material : public MaterialBase {
protected:
    Engine& r_engine;

    vk::ShaderModule m_vertexShader, m_fragmentShader;
    std::vector<vk::DescriptorSetLayout> m_descriptorSetLayouts;
    vk::PipelineLayout m_pipelineLayout;
    vk::Pipeline m_pipeline, m_shadowMappingPipeline;

public:
    typedef MaterialInstance Instance;

    bool m_usesAlphaClipping = false;

    Material(Engine& engine, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) :
        r_engine(engine),
        m_vertexShader(vertexShader),
        m_fragmentShader(fragmentShader)
    {}

    virtual void setup();

    void bindUniform(vk::CommandBuffer cmd, UniformType& uniform);
    virtual void bindUniform(vk::CommandBuffer cmd, UniformType& uniform, int index);
    virtual void bindPipeline(vk::CommandBuffer cmd);
    virtual void bindShadowMapPipeline(vk::CommandBuffer cmd);
    void bindInstance(vk::CommandBuffer cmd, MaterialInstance& instance);

    virtual void cleanup();

    MaterialInstance makeInstance() {
        MaterialInstance result { r_engine };
        return result;
    }

protected:
    virtual bool shouldCreateShadowMappingPipeline() { return true; }

    virtual void createDescriptorSetLayouts();
    virtual void createPipelineLayout();

    virtual vk::PipelineDynamicStateCreateInfo getDynamicState();
    virtual vk::PipelineVertexInputStateCreateInfo makeVertexInputState();
    virtual vk::PipelineInputAssemblyStateCreateInfo getInputAssemblyState();
    virtual vk::PipelineRasterizationStateCreateInfo getRasterizationState();
    virtual vk::PipelineDepthStencilStateCreateInfo getDepthStencilState();
    virtual vk::PipelineColorBlendStateCreateInfo getColorBlendState();
    virtual void modifyPipelineCreateInfo(vk::GraphicsPipelineCreateInfo& createInfo) {}

    virtual void createPipeline();
};

MATERIAL_TEMPLATE
void MATERIAL::setup() {
    createDescriptorSetLayouts();
    createPipelineLayout();
    createPipeline();
}

MATERIAL_TEMPLATE
void MATERIAL::createDescriptorSetLayouts() {
    m_descriptorSetLayouts.clear();

    if (UniformType::s_descriptorSetLayout != VK_NULL_HANDLE)
        m_descriptorSetLayouts.push_back(UniformType::s_descriptorSetLayout);

    if (MaterialInstance::s_descriptorSetLayout != VK_NULL_HANDLE)
        m_descriptorSetLayouts.push_back(MaterialInstance::s_descriptorSetLayout);
}

MATERIAL_TEMPLATE
vk::PipelineDynamicStateCreateInfo MATERIAL::getDynamicState() {
    static std::vector<vk::DynamicState> dynamicStates {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor
    };

    return vk::PipelineDynamicStateCreateInfo {}
        .setDynamicStates(dynamicStates)
        ;
}

MATERIAL_TEMPLATE
vk::PipelineVertexInputStateCreateInfo MATERIAL::makeVertexInputState() {
    static std::vector<vk::VertexInputAttributeDescription> attribDescs {};
    static std::vector<vk::VertexInputBindingDescription> bindingDescs {};

    attribDescs.clear();
    bindingDescs.clear();

    auto vertexAttribs = Vertex::getAttributes();

    uint32_t firstAvailableLocation = 0;
    for (auto& attribute : vertexAttribs)
        firstAvailableLocation = std::max(firstAvailableLocation, attribute.location + 1);

    auto instanceAttribs = MeshInstance::getAttributes(firstAvailableLocation);
    auto vertexBindings = Vertex::getBindings();
    auto instanceBindings = MeshInstance::getBindings();

    attribDescs.insert(attribDescs.end(), vertexAttribs.begin(), vertexAttribs.end());
    attribDescs.insert(attribDescs.end(), instanceAttribs.begin(), instanceAttribs.end());
    bindingDescs.insert(bindingDescs.end(), vertexBindings.begin(), vertexBindings.end());
    bindingDescs.insert(bindingDescs.end(), instanceBindings.begin(), instanceBindings.end());

    return vk::PipelineVertexInputStateCreateInfo {}
        .setVertexAttributeDescriptions(attribDescs)
        .setVertexBindingDescriptions(bindingDescs)
        ;
}

MATERIAL_TEMPLATE
void MATERIAL::createPipelineLayout() {
    m_pipelineLayout = r_engine.m_device.createPipelineLayout(
        vk::PipelineLayoutCreateInfo {}
            .setSetLayouts(m_descriptorSetLayouts)
    );
}

MATERIAL_TEMPLATE
vk::PipelineInputAssemblyStateCreateInfo MATERIAL::getInputAssemblyState() {
    return vk::PipelineInputAssemblyStateCreateInfo {}
        .setTopology(vk::PrimitiveTopology::eTriangleList)
        ;
}

MATERIAL_TEMPLATE
vk::PipelineRasterizationStateCreateInfo MATERIAL::getRasterizationState() {
    return vk::PipelineRasterizationStateCreateInfo {}
        .setCullMode(vk::CullModeFlagBits::eBack)
        .setFrontFace(vk::FrontFace::eCounterClockwise)
        .setLineWidth(1.f)
        .setDepthClampEnable(false)
        .setRasterizerDiscardEnable(false)
        .setPolygonMode(vk::PolygonMode::eFill)
        ;
}

MATERIAL_TEMPLATE
vk::PipelineDepthStencilStateCreateInfo MATERIAL::getDepthStencilState() {
    return vk::PipelineDepthStencilStateCreateInfo {}
        .setDepthTestEnable(true)
        .setDepthWriteEnable(true)
        .setDepthCompareOp(vk::CompareOp::eLess)
        ;
}

MATERIAL_TEMPLATE
vk::PipelineColorBlendStateCreateInfo MATERIAL::getColorBlendState() {
    static auto attachment = vk::PipelineColorBlendAttachmentState {}
        .setColorWriteMask( vk::ColorComponentFlagBits::eR
                        |   vk::ColorComponentFlagBits::eG
                        |   vk::ColorComponentFlagBits::eB
                        |   vk::ColorComponentFlagBits::eA)
        .setBlendEnable(true)
        .setColorBlendOp(vk::BlendOp::eAdd)
        .setSrcColorBlendFactor(vk::BlendFactor::eSrcAlpha)
        .setDstColorBlendFactor(vk::BlendFactor::eOneMinusSrcAlpha)
        .setAlphaBlendOp(vk::BlendOp::eAdd)
        .setSrcAlphaBlendFactor(vk::BlendFactor::eOne)
        .setDstAlphaBlendFactor(vk::BlendFactor::eZero)
        ;

    static auto attachments = std::vector<vk::PipelineColorBlendAttachmentState> {
        attachment, attachment, attachment, attachment, attachment
    };

    return vk::PipelineColorBlendStateCreateInfo {}
        .setAttachments(attachments)
        ;
}

MATERIAL_TEMPLATE
void MATERIAL::createPipeline() {
    auto dynamicState = getDynamicState();
    auto vertexInputState = makeVertexInputState();
    auto inputAssemblyState = getInputAssemblyState();
    auto rasterizationState = getRasterizationState();
    auto depthStencilState = getDepthStencilState();
    auto colorBlendState = getColorBlendState();

    auto viewportState = vk::PipelineViewportStateCreateInfo {}
        .setViewports(vk::Viewport {}
            .setX(0.f)
            .setY(0.f)
            .setWidth(r_engine.m_swapchainExtent.width)
            .setHeight(r_engine.m_swapchainExtent.height)
            .setMinDepth(0.f)
            .setMaxDepth(1.f)
            )
        .setScissors(vk::Rect2D {}
            .setOffset({ 0, 0 })
            .setExtent(r_engine.m_swapchainExtent)
            )
        ;
    
    vk::PipelineMultisampleStateCreateInfo multisampleState;
    
    std::vector<vk::PipelineShaderStageCreateInfo> stages {
        vk::PipelineShaderStageCreateInfo {}
            .setStage(vk::ShaderStageFlagBits::eVertex)
            .setModule(m_vertexShader)
            .setPName("main")
            ,
        vk::PipelineShaderStageCreateInfo {}
            .setStage(vk::ShaderStageFlagBits::eFragment)
            .setModule(m_fragmentShader)
            .setPName("main")
            ,
    };

    auto createInfo = vk::GraphicsPipelineCreateInfo {}
        .setLayout(m_pipelineLayout)
        .setPDynamicState(&dynamicState)
        .setPVertexInputState(&vertexInputState)
        .setPInputAssemblyState(&inputAssemblyState)
        .setPRasterizationState(&rasterizationState)
        .setPDepthStencilState(&depthStencilState)
        .setPMultisampleState(&multisampleState)
        .setPColorBlendState(&colorBlendState)
        .setPViewportState(&viewportState)
        .setRenderPass(r_engine.m_renderPass)
        .setStages(stages)
        .setSubpass(0)
        ;
    
    modifyPipelineCreateInfo(createInfo);

    auto pipelineResultValue = r_engine.m_device.createGraphicsPipelines(VK_NULL_HANDLE, createInfo);
    vk::resultCheck(pipelineResultValue.result, "Failed to create pipeline");
    m_pipeline = pipelineResultValue.value[0];

    if (shouldCreateShadowMappingPipeline()) {
        colorBlendState.setAttachments({});

        createInfo
            .setRenderPass(r_engine.m_shadowMappingRenderPass)
            .setSubpass(0)
            ;
        
        rasterizationState
            .setDepthBiasEnable(true)
            .setDepthBiasConstantFactor(5.f)
            .setCullMode(vk::CullModeFlagBits::eFront)
            .setDepthBiasSlopeFactor(5.f)
            ;
        
        if (!m_usesAlphaClipping) createInfo.setStages(stages[0]);

        pipelineResultValue = r_engine.m_device.createGraphicsPipelines(VK_NULL_HANDLE, createInfo);
        vk::resultCheck(pipelineResultValue.result, "Failed to create shadow mapping pipeline");
        m_shadowMappingPipeline = pipelineResultValue.value[0];
    }
}

MATERIAL_TEMPLATE
void MATERIAL::bindUniform(vk::CommandBuffer cmd, UniformType& uniform) {
    bindUniform(cmd, uniform, r_engine.m_currentInFlightFrame);
}

MATERIAL_TEMPLATE
void MATERIAL::bindUniform(vk::CommandBuffer cmd, UniformType& uniform, int index) {
    uniform.bind(cmd, m_pipelineLayout, index);
}

MATERIAL_TEMPLATE
void MATERIAL::bindShadowMapPipeline(vk::CommandBuffer cmd) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_shadowMappingPipeline);
}

MATERIAL_TEMPLATE
void MATERIAL::bindPipeline(vk::CommandBuffer cmd) {
    cmd.bindPipeline(vk::PipelineBindPoint::eGraphics, m_pipeline);
}

MATERIAL_TEMPLATE
void MATERIAL::bindInstance(vk::CommandBuffer cmd, MaterialInstance& instance) {
    instance.bind(cmd, m_pipelineLayout);
}

MATERIAL_TEMPLATE
void MATERIAL::cleanup() {
    // for (auto& descSetLayout : m_descriptorSetLayouts)
    //     r_engine.m_device.destroyDescriptorSetLayout(descSetLayout);
    // m_descriptorSetLayouts.clear();

    r_engine.m_device.destroyShaderModule(m_vertexShader);
    r_engine.m_device.destroyShaderModule(m_fragmentShader);
    r_engine.m_device.destroyPipeline(m_pipeline);
    r_engine.m_device.destroyPipelineLayout(m_pipelineLayout);

    if (shouldCreateShadowMappingPipeline()) r_engine.m_device.destroyPipeline(m_shadowMappingPipeline);
}

#undef MATERIAL_TEMPLATE
#undef MATERIAL

}

#endif