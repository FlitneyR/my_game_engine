#include <engine.hpp>

class SkyboxMaterial : public mge::Material<
    mge::ModelVertex,
    mge::ModelTransformMeshInstance,
    mge::SingleTextureMaterialInstance,
    mge::Camera
> {
public:
    SkyboxMaterial(mge::Engine& engine, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) :
        Material(engine, vertexShader, fragmentShader)
    {}

    vk::PipelineDepthStencilStateCreateInfo getDepthStencilState() override {
        return vk::PipelineDepthStencilStateCreateInfo {}
            .setDepthTestEnable(false)
            .setDepthWriteEnable(false)
            .setDepthCompareOp(vk::CompareOp::eAlways)
            ;
    }

    vk::PipelineRasterizationStateCreateInfo getRasterizationState() override {
        return vk::PipelineRasterizationStateCreateInfo {}
            .setCullMode(vk::CullModeFlagBits::eBack)
            .setFrontFace(vk::FrontFace::eClockwise)
            .setLineWidth(1.f)
            .setDepthClampEnable(false)
            .setRasterizerDiscardEnable(false)
            .setPolygonMode(vk::PolygonMode::eFill)
            ;
    }
};