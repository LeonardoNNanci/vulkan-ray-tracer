#include "command_pool.hpp"

CommandPoolBuilder::CommandPoolBuilder(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

std::shared_ptr<CommandPool> CommandPoolBuilder::build()
{
    this->commandPool = std::make_shared<CommandPool>(this->setup);

    vk::CommandPoolCreateInfo commandPoolInfo{
        .flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer,
        .queueFamilyIndex = this->setup->graphicsQueue.familyIndex
    };

    this->commandPool->handle = this->setup->device.createCommandPool(commandPoolInfo);
    this->commandPool->queue = this->setup->graphicsQueue;

    return this->commandPool;
}

CommandPool::CommandPool(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

std::shared_ptr<CommandBuffer> CommandPool::createCommandBuffer(vk::CommandBufferUsageFlags usage)
{
    auto commandBuffer = std::make_shared<CommandBuffer>(this->setup);

    vk::CommandBufferAllocateInfo allocInfo{
        .commandPool = this->handle,
        .level = vk::CommandBufferLevel::ePrimary,
        .commandBufferCount = 1
    };
    commandBuffer->handle = this->setup->device.allocateCommandBuffers(allocInfo)[0];
    commandBuffer->usage = usage;
    commandBuffer->queue = this->queue;

    return commandBuffer;
}

CommandPool::~CommandPool() {
    this->setup->device.destroyCommandPool(this->handle);
}

CommandBuffer::CommandBuffer(std::shared_ptr<Setup> setup) : IHasSetup(setup) {}

void CommandBuffer::begin() {
    vk::CommandBufferBeginInfo beginInfo{
        .flags = this->usage
    };
    this->handle.begin(beginInfo);
}

void CommandBuffer::addWaitSemaphore(std::shared_ptr<Semaphore> semaphore) {
    this->waitSemaphores.push_back(semaphore);
}

void CommandBuffer::addSignalSemaphore(std::shared_ptr<Semaphore> semaphore) {
    this->signalSemaphores.push_back(semaphore);
}

void CommandBuffer::setFence(bool signaled) {
    this->destroyFence();

    vk::FenceCreateInfo fenceInfo{};
    if (signaled)
        fenceInfo.setFlags(vk::FenceCreateFlagBits::eSignaled);
    this->fence = this->setup->device.createFence(fenceInfo);
}

void CommandBuffer::resetFence() {
    this->setup->device.resetFences({ this->fence });
}

void CommandBuffer::waitFinished()
{
    if (this->fence != VK_NULL_HANDLE)
        this->setup->device.waitForFences({ this->fence }, vk::True, UINT64_MAX);
    else
        this->queue.handle.waitIdle();
}

void CommandBuffer::destroyFence() {
    if (this->fence != VK_NULL_HANDLE)
        this->setup->device.destroyFence(this->fence);
}

void CommandBuffer::submit() {
    this->handle.end();
    auto waitCount = this->waitSemaphores.size();

    std::vector<vk::Semaphore> waitHandles(waitCount);
    std::vector<vk::PipelineStageFlags> waitStages(waitCount);
    for (int i = 0; i < waitCount; i++) {
        waitHandles[i] = this->waitSemaphores[i]->handle;
        waitStages[i] = this->waitSemaphores[i]->dstStages;
    }

    auto signalCount = this->signalSemaphores.size();
    std::vector<vk::Semaphore> signalHandles(signalCount);
    for (int i = 0; i < signalCount; i++) {
        signalHandles[i] = this->signalSemaphores[i]->handle;
    }

    vk::SubmitInfo submitInfo{
        .commandBufferCount = 1,
        .pCommandBuffers = &this->handle
    };
    submitInfo.setWaitSemaphores(waitHandles);
    submitInfo.setWaitDstStageMask(waitStages);
    submitInfo.setSignalSemaphores(signalHandles);

    this->queue.handle.submit(submitInfo, this->fence);
}

CommandBuffer::~CommandBuffer() {
    this->destroyFence();
}
