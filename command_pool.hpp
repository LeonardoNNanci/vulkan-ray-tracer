#pragma once

#include<memory>

#include<vulkan/vulkan.hpp>

#include "builder.hpp"
#include "setup.hpp"

class CommandBuffer : private IHasSetup {
public:
	vk::CommandBuffer handle;
	Queue queue;
	vk::CommandBufferUsageFlags usage;

	CommandBuffer(std::shared_ptr<Setup> setup);

	void begin();

	void submit();

	void setWaitSemaphore(vk::Semaphore semaphore);

	void setSignalSemaphore(vk::Semaphore semaphore);

	void setFence();

	void resetFence();

	void waitFinished();

	~CommandBuffer();

private:
	std::vector<vk::Semaphore> waitSemaphores;
	std::vector<vk::Semaphore> signalSemaphores;
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