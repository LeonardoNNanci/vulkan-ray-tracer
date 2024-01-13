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

//SceneBuffersBuilder::SceneBuffersBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}
//
//SceneBuffersBuilder SceneBuffersBuilder::addModel(Model3D& model)
//{
//    model.index = this->models.size();
//    this->models.push_back(model);
//
//    return *this;
//}
//
//std::tuple<std::shared_ptr<Buffer>, std::shared_ptr<Buffer>> SceneBuffersBuilder::build()
//{
//    uint32_t vertexCount = 0;
//    uint32_t indexCount = 0;
//    for (auto& model : this->models) {
//        vertexCount += model.vertices.size();
//        indexCount += model.indices.size();
//    }
//}
