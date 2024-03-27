#pragma once

#include <glm/glm.hpp>

#include "setup.hpp"
#include "scene.hpp"
#include "command_pool.hpp"
#include "buffer.hpp"

class AccelerationStructureLevel : private IHasSetup {
public:
	vk::AccelerationStructureKHR handle;
	std::shared_ptr<Buffer> buffer;

	AccelerationStructureLevel(std::shared_ptr<Setup> setup);

	~AccelerationStructureLevel();
};

class BottomLevelStructure : public AccelerationStructureLevel {
public:
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;

	BottomLevelStructure(std::shared_ptr<Setup> setup);
};

class TopLevelStructure : public AccelerationStructureLevel {
public:
	std::shared_ptr<Buffer> instanceBuffer;

	TopLevelStructure(std::shared_ptr<Setup> setup);
};

class AccelerationStructure{
public:
	std::shared_ptr<TopLevelStructure> topLevel;
	std::vector<std::shared_ptr<BottomLevelStructure>> bottomLevel;
	std::shared_ptr<Buffer> vertexBuffer;
	std::shared_ptr<Buffer> indexBuffer;

	AccelerationStructure();
};

class AccelerationStructureBuilder : public Builder<std::shared_ptr<AccelerationStructure>> , private IHasSetup {
public:
	std::shared_ptr<CommandBuffer> commandBuffer;

	AccelerationStructureBuilder(std::shared_ptr<Setup> setup, std::shared_ptr<CommandBuffer> commandBuffer);

	std::shared_ptr<AccelerationStructure> build();

	//AccelerationStructureBuilder addModel3D(Model3D model);

	//AccelerationStructureBuilder addInstance(Instance instance, uint32_t modelIndex);

	AccelerationStructureBuilder setScene(std::shared_ptr<Scene> scene);

	static Requirements getRequirements();

	~AccelerationStructureBuilder();

private:
	vk::DeviceSize size;
	std::vector<std::shared_ptr<BottomLevelStructure>> bottomLevelStructures;

	std::shared_ptr<Scene> scene;

	std::shared_ptr<BottomLevelStructure> createBottomLevel(Model3D model);

	std::shared_ptr<TopLevelStructure> createTopLevel();
};