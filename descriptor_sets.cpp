#include "acceleration_structure.hpp"
#include "descriptor_sets.hpp"
#include "buffer.hpp"

DescriptorSet::DescriptorSet(std::shared_ptr<Setup> setup, std::shared_ptr<DescriptorPool> descriptorPool)
    : IHasSetup(setup), descriptorPool(descriptorPool) {}

void DescriptorSet::updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<Buffer> buffer) {
    if (buffer == nullptr)
        return;

    vk::DescriptorBufferInfo bufferInfo{
        .buffer = buffer->handle,
        .offset = buffer->offset,
        .range = buffer->size
    };
    vk::WriteDescriptorSet writeBuffer{
        .dstSet = this->handle,
        .dstBinding = binding.binding,
        .descriptorCount = 1,
        .descriptorType = binding.descriptorType,
        .pBufferInfo = &bufferInfo
    };
    this->setup->device.updateDescriptorSets({ writeBuffer }, {});
}

void DescriptorSet::updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<Image> image) {
    vk::DescriptorImageInfo imageInfo{
        .imageView = image->view,
        .imageLayout = image->layout,
    };
    vk::WriteDescriptorSet writeImage{
        .dstSet = this->handle,
        .dstBinding = binding.binding,
        .descriptorCount = 1,
        .descriptorType = binding.descriptorType,
        .pImageInfo = &imageInfo
    };
    this->setup->device.updateDescriptorSets({ writeImage }, {});
}

void DescriptorSet::updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<AccelerationStructure> accelerationStructure) {
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
        .dstBinding = binding.binding,
        .descriptorCount = 1,
        .descriptorType = binding.descriptorType,
        .pBufferInfo = &accelerationBufferInfo
    };
    this->setup->device.updateDescriptorSets({ writeAccelerationStructure }, {});
}

void DescriptorSet::updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::vector<TexturePointers> textures) {
    if (textures.empty())
        return;

    std::vector<vk::DescriptorImageInfo> imageInfos;
    for (auto texture : textures) {
        vk::DescriptorImageInfo imageInfo{
            .sampler = texture.sampler->handle,
            .imageView = texture.image->view,
            .imageLayout = texture.image->layout,
        };
        imageInfos.push_back(imageInfo);
    }
    vk::WriteDescriptorSet writeTextures{
        .dstSet = this->handle,
        .dstBinding = binding.binding,
        .descriptorType = binding.descriptorType,
    };
    writeTextures.setImageInfo(imageInfos);
    
    this->setup->device.updateDescriptorSets({ writeTextures }, {});
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

    vk::DescriptorSetLayoutBindingFlagsCreateInfo flagsInfo{
        .bindingCount = static_cast<uint32_t>(this->bindingFlags.size())
    };
    flagsInfo.setBindingFlags(this->bindingFlags);

    // create layout
    vk::DescriptorSetLayoutCreateInfo layoutInfo{
        .pNext = &flagsInfo
    };
    layoutInfo.setBindings(bindings);
    auto layout = this->setup->device.createDescriptorSetLayout(layoutInfo);

    // allocate sets
    vk::DescriptorSetAllocateInfo setInfo{
        .descriptorPool = pool->handle
    };
    setInfo.setSetLayouts(layout);

    // if has variable descriptor count
    // variables here for correct scoping
    uint32_t maxDescriptors = 0;
    vk::DescriptorSetVariableDescriptorCountAllocateInfo setCounts{
        .descriptorSetCount = 1,
        .pDescriptorCounts = &maxDescriptors 
    };
    if (hasVariableDescriptorCount) {
        for (auto binding : this->bindings)
            maxDescriptors = std::max<uint32_t>(maxDescriptors, binding.descriptorCount);
        setInfo.setPNext(&setCounts);
    }

    auto handle = this->setup->device.allocateDescriptorSets(setInfo)[0];

    // create objects
    auto set = std::make_shared<DescriptorSet>(this->setup, pool);
    set->handle = handle;
    set->layout = layout;
    set->index = this->index;

    return set;
}

DescriptorSetBuilder DescriptorSetBuilder::addBinding(vk::DescriptorSetLayoutBinding binding) {
    this->bindings.push_back(binding);

    vk::DescriptorBindingFlags flags;
    if (binding.descriptorCount > 1) {
        flags = vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound;
        this->hasVariableDescriptorCount = true;
    }
    this->bindingFlags.push_back(flags);

    return *this;
}

DescriptorSetBuilder DescriptorSetBuilder::setIndex(uint32_t index)
{
    this->index = index;
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
