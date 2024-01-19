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

	void addWaitSemaphore(std::shared_ptr<Semaphore> semaphore);

	void addSignalSemaphore(std::shared_ptr<Semaphore> semaphore);

	void setFence(bool signaled=false);

	void resetFence();

	void waitFinished();

	~CommandBuffer();

private:
	std::vector<std::shared_ptr<Semaphore>> waitSemaphores;
	std::vector<std::shared_ptr<Semaphore>> signalSemaphores;
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