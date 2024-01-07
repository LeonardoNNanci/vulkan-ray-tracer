#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>

struct Vertex {
	alignas(32) glm::vec4 pos;

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions();
};

class Model3D {
public:
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class Instance {
public:
	glm::mat3x4 transform;
	uint32_t hitShaderOffset;

	Instance(glm::mat3x4 transform, uint32_t hitShaderOffset);
};
