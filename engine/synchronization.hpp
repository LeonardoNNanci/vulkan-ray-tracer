#pragma once
#define VK_USE_PLATFORM_WIN32_KHR

#include <vulkan/vulkan.hpp>

#include <cuda.h>
#include <cuda_runtime_api.h>
#include <optix.h>

#include "setup.hpp"

class Semaphore : IHasSetup {
public:
	vk::Semaphore handle;
	vk::SemaphoreType type;
	cudaExternalSemaphore_t cuda;

	Semaphore(std::shared_ptr<Setup> setup, uint64_t initialValue=UINT64_MAX);

	void waitSignaled(uint64_t value);

	~Semaphore();

	static Requirements getRequirements();

private:
	void importCudaExternalSemaphore(
		cudaExternalSemaphore_t& cudaSem, VkSemaphore& vkSem,
		VkExternalSemaphoreHandleTypeFlagBits handleType);

	void* getSemaphoreHandle(
		VkSemaphore semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType);

	void createExternalSemaphore(
		VkSemaphore& semaphore, VkExternalSemaphoreHandleTypeFlagBits handleType, uint64_t initialValue, vk::SemaphoreType type_);
};