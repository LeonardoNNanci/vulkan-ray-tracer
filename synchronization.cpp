#include "synchronization.hpp"

Semaphore::Semaphore(std::shared_ptr<Setup> setup, uint64_t initialValue) : IHasSetup(setup) {
	this->type = initialValue < UINT64_MAX ? 
		vk::SemaphoreType::eTimeline : 
		vk::SemaphoreType::eBinary;

	initialValue = initialValue < UINT64_MAX ?
		initialValue :
		0;

	vk::SemaphoreTypeCreateInfo typeInfo{
		.semaphoreType = this->type,
		.initialValue = initialValue
	};
	vk::SemaphoreCreateInfo semaphoreInfo{
		.pNext = &typeInfo
	};
	this->handle = this->setup->device.createSemaphore(semaphoreInfo);
}

void Semaphore::waitSignaled(uint64_t value) {
	vk::SemaphoreWaitInfo waitInfo{
		.semaphoreCount = 1,
		.pSemaphores = &this->handle,
		.pValues = &value
	};
	this->setup->device.waitSemaphores(waitInfo, UINT64_MAX);
}

Semaphore::~Semaphore() {
	this->setup->device.destroySemaphore(this->handle);
}
