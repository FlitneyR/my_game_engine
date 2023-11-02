#ifndef UNIFORM_HPP
#define UNIFORM_HPP

#include <engine.hpp>

namespace mge {

class UniformBase {};

template<typename UniformData>
class Uniform : public UniformBase {
public:
    typedef UniformData BufferData;

    Engine* r_engine;

    std::vector<vk::Buffer> m_buffers;
    std::vector<vk::DeviceMemory> m_bufferMemories;
    vk::DescriptorPool m_descriptorPool;
    std::vector<vk::DescriptorSet> m_descriptorSets;
    static vk::DescriptorSetLayout s_descriptorSetLayout;

    virtual UniformData getUniformData() = 0;

    virtual void setup() {
        createBuffers();
        allocateBuffers();

        for (int i = 0; i < r_engine->getMaxFramesInFlight(); i++)
            updateBuffer(i);

        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSets();
    }

    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout, int index);

    void cleanup() {
        r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);

        for (auto& buffer : m_buffers)
            r_engine->m_device.destroyBuffer(buffer);

        for (auto& bufferMemory : m_bufferMemories)
            r_engine->m_device.freeMemory(bufferMemory);

        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
    }

    void createBuffers();
    void allocateBuffers();
    void updateBuffer();
    virtual void updateBuffer(int index);
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSets();
};

template<typename UniformData>
vk::DescriptorSetLayout Uniform<UniformData>::s_descriptorSetLayout = VK_NULL_HANDLE;

template<typename UniformData>
void Uniform<UniformData>::createBuffers() {
    m_buffers.resize(r_engine->getMaxFramesInFlight());

    auto createInfo = vk::BufferCreateInfo {}
        .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        .setSize(sizeof(UniformData))
        .setSharingMode(vk::SharingMode::eExclusive)
        ;

    for (auto& buffer : m_buffers)
        buffer = r_engine->m_device.createBuffer(createInfo);
}

template<typename UniformData>
void Uniform<UniformData>::allocateBuffers() {
    m_bufferMemories.resize(r_engine->getMaxFramesInFlight());

    vk::MemoryRequirements memReqs = r_engine->m_device.getBufferMemoryRequirements(m_buffers[0]);
    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    for (int i = 0; i < r_engine->getMaxFramesInFlight(); i++) {
        auto allocInfo = vk::MemoryAllocateInfo {}
            .setAllocationSize(memReqs.size)
            .setMemoryTypeIndex(r_engine->findMemoryType(memReqs.memoryTypeBits, properties))
            ;

        m_bufferMemories[i] = r_engine->m_device.allocateMemory(allocInfo);
        uint32_t offset = 0;
        r_engine->m_device.bindBufferMemory(m_buffers[i], m_bufferMemories[i], offset);
    }
}

template<typename UniformData>
void Uniform<UniformData>::updateBuffer() {
    updateBuffer(r_engine->m_currentInFlightFrame);
}

template<typename UniformData>
void Uniform<UniformData>::updateBuffer(int index) {
    void* mappedMemory = r_engine->m_device.mapMemory(m_bufferMemories[index], 0, sizeof(UniformData));
    UniformData uniformData = getUniformData();
    memcpy(mappedMemory, &uniformData, sizeof(UniformData));
    r_engine->m_device.unmapMemory(m_bufferMemories[index]);
}

template<typename UniformData>
void Uniform<UniformData>::createDescriptorSetLayout() {
    if (s_descriptorSetLayout != VK_NULL_HANDLE) return;

    auto layoutBindings = UniformData::getBindings();
    auto layoutCreateInfo = vk::DescriptorSetLayoutCreateInfo {}.setBindings(layoutBindings);
    s_descriptorSetLayout = r_engine->m_device.createDescriptorSetLayout(layoutCreateInfo);
}

template<typename UniformData>
void Uniform<UniformData>::createDescriptorPool() {
    auto poolSize = vk::DescriptorPoolSize {}
        .setDescriptorCount(r_engine->getMaxFramesInFlight())
        .setType(vk::DescriptorType::eUniformBuffer)
        ;

    auto createInfo = vk::DescriptorPoolCreateInfo {}
        .setPoolSizes(poolSize)
        .setMaxSets(r_engine->getMaxFramesInFlight())
        ;
    
    m_descriptorPool = r_engine->m_device.createDescriptorPool(createInfo);
}

template<typename UniformData>
void Uniform<UniformData>::createDescriptorSets() {
    std::vector<vk::DescriptorSetLayout> layouts(r_engine->getMaxFramesInFlight(), s_descriptorSetLayout);

    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setSetLayouts(layouts)
        .setDescriptorPool(m_descriptorPool)
        .setDescriptorSetCount(r_engine->getMaxFramesInFlight())
        ;

    m_descriptorSets = r_engine->m_device.allocateDescriptorSets(allocInfo);

    // std::vector<vk::DescriptorBufferInfo> bufferInfo(r_engine->getMaxFramesInFlight());
    // std::vector<vk::WriteDescriptorSet> descriptorWrite(r_engine->getMaxFramesInFlight());

    for (int i = 0; i < r_engine->getMaxFramesInFlight(); i++) {
        auto bufferInfo = vk::DescriptorBufferInfo {}
            .setBuffer(m_buffers[i])
            .setOffset(0)
            .setRange(sizeof(UniformData))
            ;

        auto descriptorWrite = vk::WriteDescriptorSet {}
            .setDstSet(m_descriptorSets[i])
            .setDstBinding(0)
            .setDstArrayElement(0)
            .setDescriptorType(vk::DescriptorType::eUniformBuffer)
            .setDescriptorCount(1)
            .setBufferInfo(bufferInfo)
            ;
        
        r_engine->m_device.updateDescriptorSets(descriptorWrite, nullptr);
    }
}

template<typename UniformData>
void Uniform<UniformData>::bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout, int index) {
    std::vector<uint32_t> offsets;
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, m_descriptorSets[index], offsets);
}

}

#endif