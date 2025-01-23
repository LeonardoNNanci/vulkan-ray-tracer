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

void CommandBuffer::addWaitSemaphore(std::shared_ptr<Semaphore> semaphore, vk::PipelineStageFlags stage, uint64_t waitValue) {
    SemaphoreInfo info{
        .semaphore = semaphore,
        .value = waitValue,
        .stage = stage
    };
    this->waitSemaphores.push_back(info);
}

void CommandBuffer::addSignalSemaphore(std::shared_ptr<Semaphore> semaphore, vk::PipelineStageFlags stage, uint64_t signalValue) {
    SemaphoreInfo info{
    .semaphore = semaphore,
    .value = signalValue,
    .stage = stage
    };
    this->signalSemaphores.push_back(info);
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
    else if (this->signalSemaphores.size() > 0) {
        std::vector<vk::Semaphore> signalTimelineSemaphores;
        std::vector<uint64_t> signalValues;
        for (auto& info : this->signalSemaphores)
            if (info.semaphore->type == vk::SemaphoreType::eTimeline) {
                signalTimelineSemaphores.push_back(info.semaphore->handle);
                signalValues.push_back(info.value);
            }

        vk::SemaphoreWaitInfo waitInfo{};
        waitInfo.setSemaphores(signalTimelineSemaphores);
        waitInfo.setValues(signalValues);

        this->setup->device.waitSemaphores(waitInfo, UINT64_MAX);
    }
    else
        this->queue.handle.waitIdle();
}

void CommandBuffer::destroyFence() {
    if (this->fence != VK_NULL_HANDLE)
        this->setup->device.destroyFence(this->fence);
}

void CommandBuffer::clearSync() {
    this->waitSemaphores.clear();
    this->signalSemaphores.clear();
}

void CommandBuffer::submit() {
    this->handle.end();
    auto waitCount = this->waitSemaphores.size();
    auto signalCount = this->signalSemaphores.size();

    //for (int i = 0; i < waitCount; i++) {
    //    waitHandles[i] = this->waitSemaphores[i]->handle;
    //    waitStages[i] = this->waitSemaphores[i]->dstStages;
    //}

    //std::vector<vk::Semaphore> signalHandles(signalCount);
    //for (int i = 0; i < signalCount; i++) {
    //    signalHandles[i] = this->signalSemaphores[i]->handle;
    //}

    std::vector<uint64_t> waitValues(waitCount);
    std::vector<vk::Semaphore> waitHandles(waitCount);
    std::vector<vk::PipelineStageFlags> waitStages(waitCount);
    for (int i = 0; i < this->waitSemaphores.size(); i++) {
        auto info = this->waitSemaphores[i];
        waitValues[i] = info.value;
        waitHandles[i] = info.semaphore->handle;
        waitStages[i] = info.stage;
    }

    std::vector<uint64_t> signalValues(signalCount);
    std::vector<vk::Semaphore> signalHandles(signalCount);
    std::vector<vk::PipelineStageFlags> signalStages(signalCount);
    for (int i = 0; i < this->signalSemaphores.size(); i++) {
        auto info = this->signalSemaphores[i];
        signalValues[i] = info.value;
        signalHandles[i] = info.semaphore->handle;
        signalStages[i] = info.stage;
    }

    vk::TimelineSemaphoreSubmitInfo timelineInfo{};
    timelineInfo.setWaitSemaphoreValues(waitValues);
    timelineInfo.setSignalSemaphoreValues(signalValues);

    vk::SubmitInfo submitInfo{
        .pNext = &timelineInfo,
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
