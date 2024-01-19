#include "synchronization.hpp"

Semaphore::Semaphore(std::shared_ptr<Setup> setup) : IHasSetup(setup) {
	this->handle = this->setup->device.createSemaphore({});
}

void Semaphore::addSrcStage(vk::PipelineStageFlags stage) {
	this->srcStages |= stage;
}

void Semaphore::addDstStage(vk::PipelineStageFlags stage) {
	this->dstStages |= stage;
}

Semaphore::~Semaphore() {
	this->setup->device.destroySemaphore(this->handle);
}
