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
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class Instance {
public:
	glm::mat4 transform;
	uint32_t hitShaderOffset;
	uint32_t modelId;

	Instance(glm::mat4 transform, uint32_t hitShaderOffset);
};

class Scene {
public:
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;
	std::shared_ptr<Buffer> objectDescriptionBuffer;
};

struct ModelDescription {
	alignas(4) uint32_t vertexStride;
	alignas(4) uint32_t indexStride;
};

class SceneBuilder : public Builder<std::shared_ptr<Scene>>, IHasSetup {
public:
	SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer);

	SceneBuilder addModel(Model3D model);

	std::shared_ptr<Scene> build();

private:
	std::vector<Model3D> models;
	std::shared_ptr<CommandBuffer> commandBuffer;
};