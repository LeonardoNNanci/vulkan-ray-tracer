#pragma once

#include<memory>

#include <vulkan/vulkan.hpp>

#include "builder.hpp"

class Queue {
public:
	vk::Queue handle;
	uint32_t familyIndex;
};

class Setup {
public:
	vk::Instance instance;
	vk::PhysicalDevice physicalDevice;
	vk::Device device;

	Queue graphicsQueue;

	~Setup();
};

class IHasSetup {
protected:
	std::shared_ptr<Setup> setup;

	IHasSetup(std::shared_ptr<Setup> setup);
};

struct RequiredExtensions {
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> instanceExtensions;
};

class SetupBuilder : public Builder<std::shared_ptr<Setup>> {
public:
	SetupBuilder() = default;

	SetupBuilder addExtensions(RequiredExtensions extensions);

	std::shared_ptr<Setup> build();

private:
	std::shared_ptr<Setup> setup;

	RequiredExtensions extensions;

	vk::Instance createInstance();

	vk::PhysicalDevice getPhysicalDevice();

	vk::Device createDevice();

	uint32_t getQueueFamilyIndex(vk::QueueFlags queueFamily);

	vk::Queue getQueue(uint32_t familyIndex);
};