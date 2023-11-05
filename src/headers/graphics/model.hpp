#ifndef MODEL_HPP
#define MODEL_HPP

#include <libraries.hpp>
#include <mesh.hpp>
#include <vertex.hpp>
#include <material.hpp>
#include <instance.hpp>
#include <camera.hpp>

namespace mge {

struct ModelBase {
    virtual void setup() = 0;
    virtual void updateInstanceBuffer() = 0;
    virtual void drawInstances(vk::CommandBuffer cmd) = 0;
    virtual void cleanup() = 0;

    virtual uint32_t makeInstance() = 0;
    virtual MeshInstanceBase& getInstance(uint32_t instanceID) = 0;
    virtual void destroyInstance(uint32_t instanceID) = 0;
};

#define MODEL_TEMPLATE template<typename Vertex, typename MeshInstance, typename MaterialInstance, typename UniformType>
#define MODEL Model<Vertex, MeshInstance, MaterialInstance, UniformType>

MODEL_TEMPLATE
class Model : public ModelBase {
    Engine& r_engine;

public:
    typedef Mesh<Vertex> Mesh;
    typedef Material<Vertex, MeshInstance, MaterialInstance, UniformType> Material;

    Mesh* m_mesh;
    Material* m_material;
    MaterialInstance m_materialInstance;
    std::unordered_map<uint32_t, MeshInstance> m_meshInstances;
    uint32_t nextInstanceID = 0;

    vk::DeviceSize m_instanceBufferSize = 0;
    std::vector<vk::Buffer> m_instanceBuffers;
    std::vector<vk::DeviceMemory> m_instanceBufferMemories;

    void setupInstanceBuffers();
    void allocateInstanceBuffer(int index);

    Model(Engine& engine, Mesh& mesh, Material& material, MaterialInstance m_materialInstance) :
        r_engine(engine),
        m_mesh(&mesh),
        m_material(&material),
        m_materialInstance(m_materialInstance)
    {}

    void setup() override;
    void updateInstanceBuffer() override;
    void updateInstanceBuffer(int index);
    void cleanup() override;

    uint32_t makeInstance() override {
        uint32_t id = nextInstanceID++;
        m_meshInstances[id] = MeshInstance {};
        // m_instances[id] = Instance { this, id };
        return id;
    }

    MeshInstance& getInstance(uint32_t instanceID) override {
        return m_meshInstances[instanceID];
    }

    void destroyInstance(uint32_t instanceID) override {
        m_meshInstances.erase(instanceID);
    }

    uint32_t getInstanceCount() { return m_meshInstances.size(); }

    void drawInstances(vk::CommandBuffer cmd) override;
    void drawInstances(vk::CommandBuffer cmd, int index);
};

MODEL_TEMPLATE
void MODEL::setup() {
    setupInstanceBuffers();

    for (int i = 0; i < r_engine.getMaxFramesInFlight(); i++)
        updateInstanceBuffer(i);
}

MODEL_TEMPLATE
void MODEL::setupInstanceBuffers() {
    // grow buffer in the same way you grow a vector's bufer
    size_t numberToAllocate = glm::pow(2.f, glm::max(0.f, glm::ceil(glm::log2<float>(m_meshInstances.size()))));
    m_instanceBufferSize = numberToAllocate * sizeof(MeshInstance);
    vk::DeviceSize minimumBufferSize = 1 * sizeof(MeshInstance);
    m_instanceBufferSize = std::max(m_instanceBufferSize, minimumBufferSize);

    vk::BufferCreateInfo createInfo; createInfo
        .setQueueFamilyIndices(*r_engine.m_queueFamilies.graphicsFamily)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSize(m_instanceBufferSize)
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        ;

    m_instanceBuffers.resize(r_engine.getMaxFramesInFlight());

    for (auto& instanceBuffer : m_instanceBuffers)
        instanceBuffer = r_engine.m_device.createBuffer(createInfo);

    m_instanceBufferMemories.resize(r_engine.getMaxFramesInFlight());

    for (int i = 0; i < r_engine.getMaxFramesInFlight(); i++)
        allocateInstanceBuffer(i);
}

MODEL_TEMPLATE
void MODEL::allocateInstanceBuffer(int index) {
    auto reqs = r_engine.m_device.getBufferMemoryRequirements(m_instanceBuffers[index]);
    auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo allocInfo; allocInfo
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(r_engine.findMemoryType(reqs.memoryTypeBits, properties));
        ;

    m_instanceBufferMemories[index] = r_engine.m_device.allocateMemory(allocInfo);

    r_engine.m_device.bindBufferMemory(m_instanceBuffers[index], m_instanceBufferMemories[index], 0);
}

MODEL_TEMPLATE
void MODEL::updateInstanceBuffer() {
    updateInstanceBuffer(r_engine.m_currentInFlightFrame);
}

MODEL_TEMPLATE
void MODEL::updateInstanceBuffer(int index) {
    uint32_t requiredBufferSize = m_meshInstances.size() * sizeof(MeshInstance);

    if (requiredBufferSize > m_instanceBufferSize) {
        r_engine.m_device.waitIdle();
        cleanup();
        setup();
        return;
    }

    if (requiredBufferSize == 0) return;

    void* mappedMemory = r_engine.m_device.mapMemory(m_instanceBufferMemories[index], 0, requiredBufferSize);

    std::vector<MeshInstance> meshInstances;
    meshInstances.reserve(m_meshInstances.size());
    for (auto& [ID, inst] : m_meshInstances)
        meshInstances.push_back(inst);
    
    memcpy(mappedMemory, meshInstances.data(), requiredBufferSize);

    r_engine.m_device.unmapMemory(m_instanceBufferMemories[index]);
}

MODEL_TEMPLATE
void MODEL::cleanup() {
    for (auto& instanceBufferMemory : m_instanceBufferMemories)
        r_engine.m_device.freeMemory(instanceBufferMemory);
    
    for (auto& instanceBuffer : m_instanceBuffers)
        r_engine.m_device.destroyBuffer(instanceBuffer);
}

MODEL_TEMPLATE
void MODEL::drawInstances(vk::CommandBuffer cmd) {
    drawInstances(cmd, r_engine.m_currentInFlightFrame);
}

MODEL_TEMPLATE
void MODEL::drawInstances(vk::CommandBuffer cmd, int index) {
    m_material->bindPipeline(cmd);
    m_material->bindInstance(cmd, m_materialInstance);
    m_mesh->bindBuffers(cmd);
    uint32_t offset = 0;
    cmd.bindVertexBuffers(1, m_instanceBuffers[index], offset);
    m_mesh->drawInstances(cmd, m_meshInstances.size());
}

}

#endif