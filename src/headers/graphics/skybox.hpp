#ifndef SKYBOX_HPP
#define SKYBOX_HPP

#include <engine.hpp>
#include <material.hpp>
#include <vertex.hpp>
#include <instance.hpp>
#include <camera.hpp>

namespace mge {

class SkyboxMaterial : public Material<
    ModelVertex,
    ModelTransformMeshInstance,
    NTextureMaterialInstance<1>,
    Camera
> {
public:
    SkyboxMaterial(Engine& engine, vk::ShaderModule vertexShader, vk::ShaderModule fragmentShader) :
        Material(engine, vertexShader, fragmentShader)
    {}

    bool shouldCreateShadowMappingPipeline() override { return false; }

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
            .setFrontFace(vk::FrontFace::eCounterClockwise)
            .setLineWidth(1.f)
            .setDepthClampEnable(false)
            .setRasterizerDiscardEnable(false)
            .setPolygonMode(vk::PolygonMode::eFill)
            ;
    }
};

}

#endif
