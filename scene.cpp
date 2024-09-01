#include <vulkan/vulkan.hpp>

#include "scene.hpp"
#include "texture_sampler.hpp"

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
        .offset = offsetof(Vertex, position)
    };
    return { posDescription,  };
}

Instance::Instance(uint32_t modelId, glm::mat4 transform, uint32_t hitShaderOffset) : modelId(modelId), transform(transform), hitShaderOffset(hitShaderOffset) {}

SceneBuilder::SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer) : IHasSetup(setup), commandBuffer(commandBuffer) {}

SceneBuilder SceneBuilder::addModel(Model3D model)
{
    this->models.push_back(model);
    return *this;
}

SceneBuilder SceneBuilder::addInstance(Instance instance)
{
    this->instances.push_back(instance);
    return *this;
}

SceneBuilder SceneBuilder::addTexture(TextureIndices texture)
{
    this->textureIndices.push_back(texture);
    return *this;
}

SceneBuilder SceneBuilder::addMaterial(Material material)
{
    this->materials.push_back(material);
    return *this;
}

SceneBuilder SceneBuilder::addSampler(SamplerInfo sampler)
{
    this->samplerInfos.push_back(sampler);
    return *this;
}

SceneBuilder SceneBuilder::addImage(std::string fileName)
{
    this->imageFiles.push_back(fileName);
    return *this;
}

std::shared_ptr<Scene> SceneBuilder::build()
{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<ModelDescription> objectDescriptions;

    std::vector <std::shared_ptr<Image>> images(imageFiles.size());
    std::vector <std::shared_ptr<Sampler>> samplers(samplerInfos.size());

    std::vector <TexturePointers> texturePointers(textureIndices.size());

    for (int i = 0; i < images.size(); i++) {
        images[i] = std::make_shared<Image>(setup, commandBuffer, imageFiles[i]);
    }

    for (int i = 0; i < samplerInfos.size(); i++) {
        auto samplerInfo = samplerInfos[i];
        samplers[i] = std::make_shared<Sampler>(setup, samplerInfo.magFilter, samplerInfo.minFilter);
    }

    for (int i = 0; i < textureIndices.size(); i++) {
        texturePointers[i].image = images[textureIndices[i].imageIndex];
        texturePointers[i].sampler = samplers[textureIndices[i].samplerIndex];
    }

    for (auto& model : this->models) {
        objectDescriptions.push_back({
            .vertexStride = static_cast<uint32_t>(vertices.size()),
            .indexStride = static_cast<uint32_t>(indices.size()),
            .materialIndex = static_cast<uint32_t>(model.materialIndex)
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
    auto materialBuffer = BufferBuilder(this->setup)
        .setSize(this->materials.size() * sizeof(this->materials[0]))
        .setCommandBuffer(this->commandBuffer)
        .setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
        .setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
        .build();

    vertexBuffer->fill(vertices);
    indexBuffer->fill(indices);
    descriptionBuffer->fill(objectDescriptions);
    materialBuffer->fill(this->materials);

    auto scene = std::make_shared<Scene>();
    scene->models = this->models;
    scene->instances = this->instances;
    scene->vertexBuffer = vertexBuffer;
    scene->indexBuffer = indexBuffer;
    scene->objectDescriptionBuffer = descriptionBuffer;
    scene->materialBuffer = materialBuffer;
    scene->texturePointers = texturePointers;

    return scene;
}
