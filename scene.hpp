#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "setup.hpp"
#include "builder.hpp"
#include "buffer.hpp"

struct Vertex {
	alignas(16) glm::vec4 pos;

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions();
};

class Model3D {
public:
	//uint32_t index;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	//std::shared_ptr<Buffer> vertexBuffer;
	//std::shared_ptr<Buffer> indexBuffer;
};

class Instance {
public:
	glm::mat3x4 transform;
	uint32_t hitShaderOffset;

	Instance(glm::mat4 transform, uint32_t hitShaderOffset);
};

//class SceneBuffersBuilder : public Builder<std::tuple<std::shared_ptr<Buffer>, std::shared_ptr<Buffer>>>, IHasSetup {
//public:
//	SceneBuffersBuilder(std::shared_ptr<Setup> setup);
//
//	SceneBuffersBuilder addModel(Model3D& model);
//
//	std::tuple<std::shared_ptr<Buffer>, std::shared_ptr<Buffer>> build();
//
//private:
//	std::vector<Model3D&> models;
//};