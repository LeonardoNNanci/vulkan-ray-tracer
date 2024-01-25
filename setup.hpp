#pragma once

#include<memory>
#include<any>

#ifdef _WIN32
#define VK_USE_PLATFORM_WIN32_KHR
#endif
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

struct Requirements {
	std::vector<const char*> deviceExtensions;
	std::vector<const char*> instanceExtensions;
};

class SetupBuilder : public Builder<std::shared_ptr<Setup>> {
public:
	SetupBuilder() = default;

	SetupBuilder addExtensions(Requirements extensions);

	std::shared_ptr<Setup> build();

private:
	std::shared_ptr<Setup> setup;

	Requirements extensions;

	vk::Instance createInstance();

	vk::PhysicalDevice getPhysicalDevice();

	vk::Device createDevice();

	uint32_t getQueueFamilyIndex(vk::QueueFlags queueFamily);

	vk::Queue getQueue(uint32_t familyIndex);
};