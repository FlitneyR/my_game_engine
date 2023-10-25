#ifndef MODEL_HPP
#define MODEL_HPP

#include <iostream>

#include <libraries.hpp>
#include <mesh.hpp>
#include <vertex.hpp>
#include <material.hpp>
#include <instance.hpp>
#include <camera.hpp>

namespace mge {

#define MODEL_TEMPLATE template<typename Vertex, typename MeshInstance, typename MaterialInstance, typename UniformType>
#define MODEL Model<Vertex, MeshInstance, MaterialInstance, UniformType>

MODEL_TEMPLATE
class Model {
    Engine& r_engine;

public:
    typedef Mesh<Vertex> Mesh;
    typedef Material<Vertex, MeshInstance, MaterialInstance, UniformType> Material;

    Mesh* m_mesh;
    Material* m_material;
    MaterialInstance m_materialInstance;
    std::unordered_map<uint32_t, MeshInstance> m_meshInstances;
    uint32_t nextInstanceID = 0;

    vk::Buffer m_instanceBuffer;
    vk::DeviceMemory m_instanceBufferMemory;

    void setupInstanceBuffer();
    void allocateInstanceBuffer();

public:
    Model(Engine& engine, Mesh& mesh, Material& material, MaterialInstance m_materialInstance) :
        r_engine(engine),
        m_mesh(&mesh),
        m_material(&material),
        m_materialInstance(m_materialInstance)
    {}

    void setup();
    void updateInstanceBuffer();
    void cleanup();

    uint32_t makeInstance() {
        uint32_t id = nextInstanceID++;
        m_meshInstances[id] = MeshInstance {};
        // m_instances[id] = Instance { this, id };
        return id;
    }

    MeshInstance& getInstance(uint32_t instanceID) {
        return m_meshInstances[instanceID];
    }

    void drawInstances(vk::CommandBuffer cmd);
};

MODEL_TEMPLATE
void MODEL::setup() {
    setupInstanceBuffer();
    updateInstanceBuffer();
}

MODEL_TEMPLATE
void MODEL::setupInstanceBuffer() {
    vk::BufferCreateInfo createInfo; createInfo
        .setQueueFamilyIndices(*r_engine.m_queueFamilies.graphicsFamily)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSize(m_meshInstances.size() * sizeof(MeshInstance))
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        ;

    m_instanceBuffer = r_engine.m_device.createBuffer(createInfo);
    allocateInstanceBuffer();
}

MODEL_TEMPLATE
void MODEL::allocateInstanceBuffer() {
    auto reqs = r_engine.m_device.getBufferMemoryRequirements(m_instanceBuffer);
    auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo allocInfo; allocInfo
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(r_engine.findMemoryType(reqs.memoryTypeBits, properties));
        ;

    m_instanceBufferMemory = r_engine.m_device.allocateMemory(allocInfo);

    r_engine.m_device.bindBufferMemory(m_instanceBuffer, m_instanceBufferMemory, 0);
}

MODEL_TEMPLATE
void MODEL::updateInstanceBuffer() {
    uint32_t bufferSize = m_meshInstances.size() * sizeof(MeshInstance);
    void* mappedMemory = r_engine.m_device.mapMemory(m_instanceBufferMemory, 0, bufferSize);

    std::vector<MeshInstance> meshInstances;
    meshInstances.reserve(m_meshInstances.size());
    for (auto& [ID, inst] : m_meshInstances)
        meshInstances.push_back(inst);
    
    memcpy(mappedMemory, meshInstances.data(), bufferSize);

    r_engine.m_device.unmapMemory(m_instanceBufferMemory);
}

MODEL_TEMPLATE
void MODEL::cleanup() {
    r_engine.m_device.freeMemory(m_instanceBufferMemory);
    r_engine.m_device.destroyBuffer(m_instanceBuffer);
}

MODEL_TEMPLATE
void MODEL::drawInstances(vk::CommandBuffer cmd) {
    m_material->bindPipeline(cmd);
    m_material->bindInstance(cmd, m_materialInstance);
    m_mesh->bindBuffers(cmd);
    uint32_t offset = 0;
    cmd.bindVertexBuffers(1, m_instanceBuffer, offset);
    m_mesh->drawInstances(cmd, m_meshInstances.size());
}

}

#endif