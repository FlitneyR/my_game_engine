#ifndef MESH_HPP
#define MESH_HPP

#include <libraries.hpp>
#include <engine.hpp>

#include <vector>

namespace mge {

template<typename Vertex>
class Mesh {
    Engine& r_engine;

public:
    Mesh(
        Engine& engine,
        std::vector<Vertex> vertices,
        std::vector<uint16_t> indices
    ) :
        r_engine(engine)
    {
        m_vertices = vertices;
        m_indices = indices;
    }
    
    void setup();

    void bindBuffers(vk::CommandBuffer cmd);
    void drawInstances(vk::CommandBuffer cmd, uint32_t instanceCount);

    void cleanup();

private:
    std::vector<Vertex> m_vertices;
    std::vector<uint16_t> m_indices;

    vk::Buffer m_vertexBuffer, m_indexBuffer;
    vk::DeviceMemory m_vertexBufferMemory, m_indexBufferMemory;

    void setupVertexBuffer();
    void setupIndexBuffer();

    void allocateVertexBuffer();
    void allocateIndexBuffer();

    void fillVertexBuffer();
    void fillIndexBuffer();
};

template<typename Vertex>
void Mesh<Vertex>::setup() {
    setupVertexBuffer();
    setupIndexBuffer();

    fillVertexBuffer();
    fillIndexBuffer();
}

template<typename Vertex>
void Mesh<Vertex>::setupVertexBuffer() {
    vk::BufferCreateInfo createInfo; createInfo
        .setQueueFamilyIndices(*r_engine.m_queueFamilies.graphicsFamily)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSize(m_vertices.size() * sizeof(Vertex))
        .setUsage(vk::BufferUsageFlagBits::eVertexBuffer)
        ;

    m_vertexBuffer = r_engine.m_device.createBuffer(createInfo);

    allocateVertexBuffer();

    r_engine.m_device.bindBufferMemory(m_vertexBuffer, m_vertexBufferMemory, 0);
}

template<typename Vertex>
void Mesh<Vertex>::setupIndexBuffer() {
    vk::BufferCreateInfo createInfo; createInfo
        .setQueueFamilyIndices(*r_engine.m_queueFamilies.graphicsFamily)
        .setSharingMode(vk::SharingMode::eExclusive)
        .setSize(m_indices.size() * sizeof(uint16_t))
        .setUsage(vk::BufferUsageFlagBits::eIndexBuffer)
        ;

    m_indexBuffer = r_engine.m_device.createBuffer(createInfo);

    allocateIndexBuffer();

    r_engine.m_device.bindBufferMemory(m_indexBuffer, m_indexBufferMemory, 0);
}

template<typename Vertex>
void Mesh<Vertex>::allocateVertexBuffer() {
    auto reqs = r_engine.m_device.getBufferMemoryRequirements(m_vertexBuffer);
    auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo allocInfo; allocInfo
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(r_engine.findMemoryType(reqs.memoryTypeBits, properties));
        ;

    m_vertexBufferMemory = r_engine.m_device.allocateMemory(allocInfo);
}

template<typename Vertex>
void Mesh<Vertex>::allocateIndexBuffer() {
    auto reqs = r_engine.m_device.getBufferMemoryRequirements(m_indexBuffer);
    auto properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    vk::MemoryAllocateInfo allocInfo; allocInfo
        .setAllocationSize(reqs.size)
        .setMemoryTypeIndex(r_engine.findMemoryType(reqs.memoryTypeBits, properties));
        ;

    m_indexBufferMemory = r_engine.m_device.allocateMemory(allocInfo);
}

template<typename Vertex>
void Mesh<Vertex>::fillVertexBuffer() {
    uint32_t bufferSize = m_vertices.size() * sizeof(Vertex);
    void* mappedMemory = r_engine.m_device.mapMemory(m_vertexBufferMemory, 0, bufferSize);
    memcpy(mappedMemory, m_vertices.data(), bufferSize);
    r_engine.m_device.unmapMemory(m_vertexBufferMemory);
}

template<typename Vertex>
void Mesh<Vertex>::fillIndexBuffer() {
    uint32_t bufferSize = m_indices.size() * sizeof(uint16_t);
    void* mappedMemory = r_engine.m_device.mapMemory(m_indexBufferMemory, 0, bufferSize);
    memcpy(mappedMemory, m_indices.data(), bufferSize);
    r_engine.m_device.unmapMemory(m_indexBufferMemory);
}

template<typename Vertex>
void Mesh<Vertex>::bindBuffers(vk::CommandBuffer cmd) {
    uint32_t offset = 0;
    cmd.bindVertexBuffers(0, m_vertexBuffer, offset);
    cmd.bindIndexBuffer(m_indexBuffer, 0, vk::IndexType::eUint16);
}

template<typename Vertex>
void Mesh<Vertex>::drawInstances(vk::CommandBuffer cmd, uint32_t instanceCount) {
    cmd.drawIndexed(m_indices.size(), instanceCount, 0, 0, 0);
}

template<typename Vertex>
void Mesh<Vertex>::cleanup() {
    r_engine.m_device.freeMemory(m_vertexBufferMemory);
    r_engine.m_device.freeMemory(m_indexBufferMemory);

    r_engine.m_device.destroyBuffer(m_vertexBuffer);
    r_engine.m_device.destroyBuffer(m_indexBuffer);
}

}

#endif