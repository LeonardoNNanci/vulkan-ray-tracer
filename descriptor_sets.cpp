#include "acceleration_structure.hpp"
#include "descriptor_sets.hpp"
#include "buffer.hpp"

DescriptorSet::DescriptorSet(std::shared_ptr<Setup> setup, std::shared_ptr<DescriptorPool> descriptorPool)
    : IHasSetup(setup), descriptorPool(descriptorPool) {}

void DescriptorSet::updateDescriptor(Descriptor descriptor, std::shared_ptr<Buffer> buffer) {
    vk::DescriptorBufferInfo bufferInfo{
        .buffer = buffer->handle,
        .offset = buffer->offset,
        .range = buffer->size
    };
    vk::WriteDescriptorSet writeBuffer{
        .dstSet = this->handle,
        .dstBinding = descriptor.binding,
        .descriptorCount = 1,
        .descriptorType = descriptor.type,
        .pBufferInfo = &bufferInfo
    };
    this->setup->device.updateDescriptorSets({ writeBuffer }, {});
}

void DescriptorSet::updateDescriptor(Descriptor descriptor, std::shared_ptr<Image> image) {
    vk::DescriptorImageInfo imageInfo{
        .imageView = image->view,
        .imageLayout = image->layout,
    };
    vk::WriteDescriptorSet writeImage{
        .dstSet = this->handle,
        .dstBinding = descriptor.binding,
        .descriptorCount = 1,
        .descriptorType = descriptor.type,
        .pImageInfo = &imageInfo
    };
    this->setup->device.updateDescriptorSets({ writeImage }, {});
}

void DescriptorSet::updateDescriptor(Descriptor descriptor, std::shared_ptr<AccelerationStructure> accelerationStructure) {
    vk::WriteDescriptorSetAccelerationStructureKHR accelerationData{
        .accelerationStructureCount = 1,
        .pAccelerationStructures = &accelerationStructure->topLevel->handle,
    };
    vk::DescriptorBufferInfo accelerationBufferInfo{
        .buffer = accelerationStructure->topLevel->buffer->handle,
        .offset = accelerationStructure->topLevel->buffer->offset
    };
    vk::WriteDescriptorSet writeAccelerationStructure{
        .pNext = &accelerationData,
        .dstSet = this->handle,
        .dstBinding = descriptor.binding,
        .descriptorCount = 1,
        .descriptorType = descriptor.type,
        .pBufferInfo = &accelerationBufferInfo
    };
    this->setup->device.updateDescriptorSets({ writeAccelerationStructure }, {});
}


DescriptorSet::~DescriptorSet() {
    this->setup->device.destroyDescriptorSetLayout(this->layout);
    this->setup->device.freeDescriptorSets(this->descriptorPool->handle, { this->handle });
}

DescriptorPool::DescriptorPool(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

DescriptorPool::~DescriptorPool() {
    this->setup->device.destroyDescriptorPool(this->handle);
}

Requirements DescriptorSetBuilder::getRequirements()
{
    Requirements extensions;
    extensions.deviceExtensions = {
    };
    return extensions;
}

DescriptorSetBuilder::DescriptorSetBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup), index(index) {}

std::shared_ptr<DescriptorSet> DescriptorSetBuilder::build() {
    auto pool = this->createDescriptorPool();
    vk::DescriptorSetLayout layout;
    
    // declare bindings
    std::vector<vk::DescriptorSetLayoutBinding> bindings;
    bindings.reserve(this->descriptors.size());
    for (auto descriptor : this->descriptors){
        vk::DescriptorSetLayoutBinding binding{
            .binding = descriptor.binding,
            .descriptorType = descriptor.type,
            .descriptorCount = 1,
            .stageFlags = descriptor.stagesUsed
        };
        bindings.emplace_back(binding);
    }

    // create layout
    vk::DescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.setBindings(bindings);
    layout = this->setup->device.createDescriptorSetLayout(layoutInfo);

    // allocate sets
    vk::DescriptorSetAllocateInfo setInfo{
        .descriptorPool = pool->handle
    };
    setInfo.setSetLayouts(layout);
    auto handle = this->setup->device.allocateDescriptorSets(setInfo)[0];

    // create objects
    auto set = std::make_shared<DescriptorSet>(this->setup, pool);
    set->handle = handle;
    set->layout = layout;

    return set;
}

DescriptorSetBuilder DescriptorSetBuilder::addBinding(Descriptor descriptor) {
    this->descriptors.push_back(descriptor);
    return *this;
}

std::shared_ptr<DescriptorPool> DescriptorSetBuilder::createDescriptorPool()
{
    std::vector<vk::DescriptorPoolSize> poolSizes({
        {
            .type = vk::DescriptorType::eAccelerationStructureKHR,
            .descriptorCount = 10
        },
        {
            .type = vk::DescriptorType::eStorageImage,
            .descriptorCount = 10
        },
        {
            .type = vk::DescriptorType::eStorageBuffer,
            .descriptorCount = 10
        }
        });
    vk::DescriptorPoolCreateInfo poolInfo{
        .flags = vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet,
        .maxSets = 1,
        .poolSizeCount = static_cast<uint32_t>(poolSizes.size()),
        .pPoolSizes = poolSizes.data()
    };
    auto handle = this->setup->device.createDescriptorPool(poolInfo);

    auto pool = std::make_shared<DescriptorPool>(this->setup);
    pool->handle = handle;
    return pool;
}
