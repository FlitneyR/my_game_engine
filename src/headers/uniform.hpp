#ifndef UNIFORM_HPP
#define UNIFORM_HPP

#include <engine.hpp>

namespace mge {

template<typename UniformData>
class Uniform {
public:
    typedef UniformData BufferData;

    Engine* r_engine;

    vk::Buffer m_buffer;
    vk::DeviceMemory m_bufferMemory;
    vk::DescriptorPool m_descriptorPool;
    vk::DescriptorSet m_descriptorSet;

    static vk::DescriptorSetLayout s_descriptorSetLayout;

    virtual UniformData getUniformData() = 0;

    virtual void setup() {
        createBuffer();
        allocateBuffer();
        updateBuffer();
        createDescriptorSetLayout();
        createDescriptorPool();
        createDescriptorSet();
    }

    void bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout);

    void cleanup() {
        r_engine->m_device.destroyDescriptorSetLayout(s_descriptorSetLayout);
        r_engine->m_device.destroyBuffer(m_buffer);
        r_engine->m_device.freeMemory(m_bufferMemory);
        r_engine->m_device.destroyDescriptorPool(m_descriptorPool);
    }

    void createBuffer();
    void allocateBuffer();
    virtual void updateBuffer();
    void createDescriptorSetLayout();
    void createDescriptorPool();
    void createDescriptorSet();
};

template<typename UniformData>
vk::DescriptorSetLayout Uniform<UniformData>::s_descriptorSetLayout = VK_NULL_HANDLE;

template<typename UniformData>
void Uniform<UniformData>::createBuffer() {
    auto createInfo = vk::BufferCreateInfo {}
        .setQueueFamilyIndices(*r_engine->m_queueFamilies.graphicsFamily)
        .setUsage(vk::BufferUsageFlagBits::eUniformBuffer)
        .setSize(sizeof(UniformData))
        .setSharingMode(vk::SharingMode::eExclusive)
        ;

    m_buffer = r_engine->m_device.createBuffer(createInfo);
}

template<typename UniformData>
void Uniform<UniformData>::allocateBuffer() {
    vk::MemoryRequirements memReqs = r_engine->m_device.getBufferMemoryRequirements(m_buffer);
    vk::MemoryPropertyFlags properties = vk::MemoryPropertyFlagBits::eDeviceLocal | vk::MemoryPropertyFlagBits::eHostCoherent;

    auto allocInfo = vk::MemoryAllocateInfo {}
        .setAllocationSize(memReqs.size)
        .setMemoryTypeIndex(r_engine->findMemoryType(memReqs.memoryTypeBits, properties))
        ;

    m_bufferMemory = r_engine->m_device.allocateMemory(allocInfo);
    uint32_t offset = 0;
    r_engine->m_device.bindBufferMemory(m_buffer, m_bufferMemory, offset);
}

template<typename UniformData>
void Uniform<UniformData>::updateBuffer() {
    void* mappedMemory = r_engine->m_device.mapMemory(m_bufferMemory, 0, sizeof(UniformData));
    UniformData uniformData = getUniformData();
    memcpy(mappedMemory, &uniformData, sizeof(UniformData));
    r_engine->m_device.unmapMemory(m_bufferMemory);
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
        .setDescriptorCount(1)
        .setType(vk::DescriptorType::eUniformBuffer)
        ;

    auto createInfo = vk::DescriptorPoolCreateInfo {}
        .setPoolSizes(poolSize)
        .setMaxSets(1)
        ;
    
    m_descriptorPool = r_engine->m_device.createDescriptorPool(createInfo);
}

template<typename UniformData>
void Uniform<UniformData>::createDescriptorSet() {
    auto allocInfo = vk::DescriptorSetAllocateInfo {}
        .setSetLayouts(s_descriptorSetLayout)
        .setDescriptorPool(m_descriptorPool)
        ;

    m_descriptorSet = r_engine->m_device.allocateDescriptorSets(allocInfo)[0];

    // void updateDescriptorSets<Dispatch>(const vk::ArrayProxy<const vk::WriteDescriptorSet> &descriptorWrites, const vk::ArrayProxy<const vk::CopyDescriptorSet> &descriptorCopies, const Dispatch &d = ::vk::getDispatchLoaderStatic()) const

    auto bufferInfo = vk::DescriptorBufferInfo {}
        .setBuffer(m_buffer)
        .setOffset(0)
        .setRange(sizeof(UniformData))
        ;

    auto descriptorWrite = vk::WriteDescriptorSet {}
        .setDstSet(m_descriptorSet)
        .setDstBinding(0)
        .setDstArrayElement(0)
        .setDescriptorType(vk::DescriptorType::eUniformBuffer)
        .setDescriptorCount(1)
        .setBufferInfo(bufferInfo)
        ;

    // std::vector<vk::WriteDescriptorSet> copies;

    r_engine->m_device.updateDescriptorSets(descriptorWrite, nullptr);
}

template<typename UniformData>
void Uniform<UniformData>::bind(vk::CommandBuffer cmd, vk::PipelineLayout pipelineLayout) {
    std::vector<uint32_t> offsets;
    cmd.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, pipelineLayout, 0, m_descriptorSet, offsets);
}

}

#endif