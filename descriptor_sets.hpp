#pragma once
#include <vulkan/vulkan.hpp>
#include "builder.hpp"
#include "acceleration_structure.hpp"
#include "image.hpp"

class DescriptorPool : IHasSetup {
public:
	vk::DescriptorPool handle;

	DescriptorPool(std::shared_ptr<Setup> setup);

	~DescriptorPool();
};

class Descriptor {
public:
	uint32_t set;
	uint32_t binding;
	vk::DescriptorType type;
	vk::ShaderStageFlags stagesUsed;
};

class DescriptorSet : IHasSetup{
public:
	vk::DescriptorSet handle;
	vk::DescriptorSetLayout layout;
	std::shared_ptr<DescriptorPool> descriptorPool;

	DescriptorSet(std::shared_ptr<Setup> setup, std::shared_ptr<DescriptorPool> descriptorPool);

	void updateDescriptor(Descriptor descriptor, std::shared_ptr<Buffer> buffer);

	void updateDescriptor(Descriptor descriptor, std::shared_ptr<Image> image);

	void updateDescriptor(Descriptor descriptor, std::shared_ptr<AccelerationStructure> accelerationStructure);

	//void submitUpdates();

	~DescriptorSet();

private:
	std::vector<vk::WriteDescriptorSet> writes;
};

class DescriptorSetBuilder : public Builder<std::shared_ptr<DescriptorSet>>, IHasSetup {
public:
	static Requirements getRequirements();
	
	DescriptorSetBuilder(std::shared_ptr<Setup> setup);

	std::shared_ptr<DescriptorSet> build();

	DescriptorSetBuilder addBinding(Descriptor binding);

private:
	uint32_t index;

	std::vector<Descriptor> descriptors;

	std::shared_ptr<DescriptorPool> createDescriptorPool();
};