#pragma once

#include<memory>

#include<vulkan/vulkan.hpp>

#include "builder.hpp"
#include "setup.hpp"

class CommandBuffer {
public:
	void 
};

class CommandPool {
public:
	CommandBuffer createCommandBuffer();

private:

};

class CommandPoolBuilder : public Builder<std::shared_ptr<CommandPool>> {
public:
	std::shared_ptr<CommandPool> build();

private:
	std::shared_ptr<CommandPool> commandPool;

};