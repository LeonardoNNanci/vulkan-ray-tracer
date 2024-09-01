#pragma once
#include "setup.hpp"

class Sampler : IHasSetup
{
public:
	vk::Sampler handle;
	
	Sampler(std::shared_ptr<Setup> setup, vk::Filter magFilter, vk::Filter minFilter);

	~Sampler();
};

