#pragma once
#include <vulkan/vulkan.hpp>

#include "setup.hpp"

class Semaphore : IHasSetup {
public:
	vk::Semaphore handle;
	vk::SemaphoreType type;

	Semaphore(std::shared_ptr<Setup> setup, uint64_t initialValue=UINT64_MAX);

	void waitSignaled(uint64_t value);

	~Semaphore();
};