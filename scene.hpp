#pragma once
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <vector>

#include "setup.hpp"
#include "builder.hpp"
#include "buffer.hpp"
#include "image.hpp"
#include "texture_sampler.hpp"

struct Vertex {
	alignas(16) glm::vec4 position;

	alignas(16) glm::vec3 normal;

	alignas(8) glm::vec2 textureCoordinates;

	static vk::VertexInputBindingDescription getBindingDescription();

	static std::array<vk::VertexInputAttributeDescription, 1> getAttributeDescriptions();
};

class TextureIndices {
public:
	uint32_t imageIndex;
	uint32_t samplerIndex;
};

class TexturePointers {
public:
	std::shared_ptr<Image> image;
	std::shared_ptr<Sampler> sampler;
};

class Material {
public:
	float metalicFactor;
	float roughnessFactor;
	alignas(16) glm::vec4 baseColor;

	uint32_t colorTexture;
	uint32_t metalicRoughnessTexture;
	uint32_t normalTexture;
	uint32_t occlusionTexture;
	uint32_t emissiveTexture;

	bool doubleSided;
};

class Model3D {
public:
	uint32_t vertexOffset;
	uint32_t indexOffset;
	uint32_t materialIndex;
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

class SamplerInfo {
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

	std::shared_ptr<Buffer> materialBuffer;

	std::vector<TexturePointers> texturePointers;
};

struct ModelDescription {
	alignas(4) uint32_t vertexStride;
	alignas(4) uint32_t indexStride;
	alignas(4) uint32_t materialIndex;
};

class SceneBuilder : public Builder<std::shared_ptr<Scene>>, IHasSetup {
public:
	SceneBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer);

	SceneBuilder addModel(Model3D model);

	SceneBuilder addInstance(Instance instance);

	SceneBuilder addTexture(TextureIndices texture);

	SceneBuilder addMaterial(Material material);

	SceneBuilder addSampler(SamplerInfo sampler);

	SceneBuilder addImage(std::string fileName);

	std::shared_ptr<Scene> build();

private:
	std::vector<Model3D> models;
	std::vector <Instance> instances;
	std::vector<Material> materials;
	std::vector<SamplerInfo> samplerInfos;
	std::vector<std::string> imageFiles;
	std::vector<TextureIndices> textureIndices;
	std::shared_ptr<CommandBuffer> commandBuffer;
};