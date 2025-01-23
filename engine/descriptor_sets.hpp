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

class DescriptorSet : IHasSetup{
public:
	vk::DescriptorSet handle;
	uint32_t index;
	vk::DescriptorSetLayout layout;
	std::shared_ptr<DescriptorPool> descriptorPool;

	DescriptorSet(std::shared_ptr<Setup> setup, std::shared_ptr<DescriptorPool> descriptorPool);

	void updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<Buffer> buffer);

	void updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<Image> image);

	void updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::shared_ptr<AccelerationStructure> accelerationStructure);

	void updateDescriptor(vk::DescriptorSetLayoutBinding binding, std::vector<TexturePointers> textures);

	~DescriptorSet();

private:

	std::vector<vk::WriteDescriptorSet> writes;
};

class DescriptorSetBuilder : public Builder<std::shared_ptr<DescriptorSet>>, IHasSetup {
public:
	static Requirements getRequirements();
	
	DescriptorSetBuilder(std::shared_ptr<Setup> setup);

	std::shared_ptr<DescriptorSet> build();

	DescriptorSetBuilder addBinding(vk::DescriptorSetLayoutBinding binding);

	DescriptorSetBuilder setIndex(uint32_t index);

private:
	uint32_t index;

	std::vector<vk::DescriptorSetLayoutBinding> bindings;
	std::vector<vk::DescriptorBindingFlags> bindingFlags;

	bool hasVariableDescriptorCount = false;

	std::shared_ptr<DescriptorPool> createDescriptorPool();
};