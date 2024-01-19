#pragma once
#include <vulkan/vulkan.hpp>

#include "setup.hpp"

class Semaphore : IHasSetup {
public:
	vk::Semaphore handle;
	vk::PipelineStageFlags srcStages;
	vk::PipelineStageFlags dstStages;

	Semaphore(std::shared_ptr<Setup> setup);

	void addSrcStage(vk::PipelineStageFlags stage);
	
	void addDstStage(vk::PipelineStageFlags stage);

	~Semaphore();
};