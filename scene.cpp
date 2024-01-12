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

Instance::Instance(glm::mat3x4 transform, uint32_t hitShaderOffset) : transform(transform), hitShaderOffset(hitShaderOffset) {}
