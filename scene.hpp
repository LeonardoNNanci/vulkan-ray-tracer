#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "setup.hpp"
#include "builder.hpp"
#include "buffer.hpp"

struct Vertex {
	alignas(16) glm::vec4 pos;

	alignas(16) glm::vec3 normal;

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions();
};

class Material {
public:
	double metalicFactor;
	double roughnessFactor;
	glm::vec4 baseColor;

	int colorTexture;
	int metalicRoughnessTexture;
	int normalTexture;
	int occlusionTexture;
	int emissiveTexture;

	bool doubleSided;
};

class Model3D {
public:
	uint32_t vertexOffset;
	uint32_t indexOffset;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
};

class Instance {
public:
	glm::mat4 transform;
	uint32_t hitShaderOffset;
	uint32_t modelId;

	Instance(uint32_t modelId, glm::mat4 transform, uint32_t hitShaderOffset);
};

class Texture {
public:
	int image;
	int sampler;
};

class Sampler {
public:
	vk::Filter magFilter;
	vk::Filter minFilter;
};

class Scene {
public:
	std::vector<Model3D> models;
	std::vector<Instance> instances;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;
	std::shared_ptr<Buffer> objectDescriptionBuffer;

	std::vector<Material> materials;
	std::vector<Texture> textures;
	std::vector<Sampler> samplers;
};

struct ModelDescription {
	alignas(4) uint32_t vertexStride;
	alignas(4) uint32_t indexStride;
};

class SceneBuilder : public Builder<std::shared_ptr<Scene>>, IHasSetup {
public:
	SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer);

	SceneBuilder addModel(Model3D model);

	SceneBuilder addInstance(Instance instance);

	SceneBuilder addTexture(Texture texture);

	SceneBuilder addMaterial(Material material);

	SceneBuilder addSampler(Sampler sampler);

	SceneBuilder addImage(std::string fileName);

	std::shared_ptr<Scene> build();

private:
	std::vector<Model3D> models;
	std::vector <Instance> instances;
	std::vector<Material> materials;
	std::vector<Texture> textures;
	std::vector<Sampler> samplers;
	std::vector<std::string> imageFiles;
	std::shared_ptr<CommandBuffer> commandBuffer;
};