#pragma once

#include<memory>

#include<vulkan/vulkan.hpp>

#include "builder.hpp"
#include "setup.hpp"
#include "synchronization.hpp"

class CommandBuffer : private IHasSetup {
public:
	vk::CommandBuffer handle;
	Queue queue;
	vk::CommandBufferUsageFlags usage;

	CommandBuffer(std::shared_ptr<Setup> setup);

	void begin();

	void submit();

	void addWaitSemaphore(std::shared_ptr<Semaphore> semaphore, vk::PipelineStageFlags dstStage, uint64_t waitValue = NULL);

	void addSignalSemaphore(std::shared_ptr<Semaphore> semaphore, vk::PipelineStageFlags srcStage, uint64_t signalValue = NULL);

	void setFence(bool signaled=false);

	void resetFence();

	void waitFinished();

	void clearSync();

	~CommandBuffer();

private:
	struct SemaphoreInfo {
		std::shared_ptr<Semaphore> semaphore;
		uint64_t value;
		vk::PipelineStageFlags stage;
	};
	std::vector<SemaphoreInfo> waitSemaphores;
	std::vector<SemaphoreInfo> signalSemaphores;
	vk::Fence fence = VK_NULL_HANDLE;

	void destroyFence();

};

class CommandPool : private IHasSetup {
public:
	vk::CommandPool handle;
	Queue queue;

	CommandPool(std::shared_ptr<Setup> setup);

	std::shared_ptr<CommandBuffer> createCommandBuffer(vk::CommandBufferUsageFlags usage = {});

	~CommandPool();	
};

class CommandPoolBuilder : public Builder<std::shared_ptr<CommandPool>>, private IHasSetup {
public:
	CommandPoolBuilder(std::shared_ptr<Setup> setup);

	std::shared_ptr<CommandPool> build();

private:
	std::shared_ptr<CommandPool> commandPool;

};