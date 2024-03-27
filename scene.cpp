#include <vulkan/vulkan.hpp>

#include "scene.hpp"

vk::VertexInputBindingDescription Vertex::getBindingDescription() {
    return {
    .binding = 0,
    .stride = sizeof(Vertex),
    .inputRate = vk::VertexInputRate::eVertex
    };
}

std::array<vk::VertexInputAttributeDescription, 1> Vertex::getAttributeDescriptions() {
    vk::VertexInputAttributeDescription posDescription{
        .location = 0,
        .binding = 0,
        .format = vk::Format::eR32G32B32Sfloat,
        .offset = offsetof(Vertex, pos)
    };
    return { posDescription };
}

Instance::Instance(glm::mat4 transform, uint32_t hitShaderOffset) : transform(transform), hitShaderOffset(hitShaderOffset) {}

SceneBuilder::SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer) : IHasSetup(setup), commandBuffer(commandBuffer) {}

SceneBuilder SceneBuilder::addModel(Model3D model)
{
    this->models.push_back(model);
    return *this;
}

SceneBuilder SceneBuilder::addInstance(Instance instance)
{
    instance.modelId = this->models.size() - 1;
    this->instances.push_back(instance);
    return *this;
}

std::shared_ptr<Scene> SceneBuilder::build()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ModelDescription> objectDescriptions;

    for (auto& model : this->models) {
        objectDescriptions.push_back({
            .vertexStride = static_cast<uint32_t>(vertices.size()),
            .indexStride = static_cast<uint32_t>(indices.size())
        });
        model.vertexOffset = vertices.size();
        model.indexOffset = indices.size();
        vertices.insert(vertices.end(), model.vertices.begin(), model.vertices.end());
        indices.insert(indices.end(), model.indices.begin(), model.indices.end());
    }

    auto vertexBuffer = BufferBuilder(this->setup)
        .setSize(vertices.size() * sizeof(vertices[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    auto indexBuffer = BufferBuilder(this->setup)
        .setSize(indices.size() * sizeof(indices[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setUsage(vk::BufferUsageFlagBits::eShaderDeviceAddress)
        .setUsage(vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    auto descriptionBuffer = BufferBuilder(this->setup)
        .setSize(objectDescriptions.size() * sizeof(objectDescriptions[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();
    vertexBuffer->fill(vertices);
    indexBuffer->fill(indices);
    descriptionBuffer->fill(objectDescriptions);

    auto scene = std::make_shared<Scene>();
    scene->models = this->models;
    scene->instances = this->instances;
    scene->vertexBuffer = vertexBuffer;
    scene->indexBuffer = indexBuffer;
    scene->objectDescriptionBuffer = descriptionBuffer;

    return scene;
}
