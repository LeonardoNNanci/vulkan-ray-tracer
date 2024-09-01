#include "texture_sampler.hpp"

Sampler::Sampler(std::shared_ptr<Setup> setup, vk::Filter magFilter, vk::Filter minFilter) : IHasSetup(setup)
{
	auto props = this->setup->physicalDevice.getProperties();

	vk::SamplerCreateInfo samplerInfo{
		.magFilter = magFilter,
		.minFilter = minFilter,
		.mipmapMode = vk::SamplerMipmapMode::eLinear,
		.addressModeU = vk::SamplerAddressMode::eRepeat,
		.addressModeV = vk::SamplerAddressMode::eRepeat,
		.addressModeW = vk::SamplerAddressMode::eRepeat,
		.anisotropyEnable = vk::True,
		.maxAnisotropy = props.limits.maxSamplerAnisotropy,
		.compareEnable = vk::False,
		.borderColor = vk::BorderColor::eFloatOpaqueBlack,
		.unnormalizedCoordinates = vk::False,
	};

	this->handle = this->setup->device.createSampler(samplerInfo);
}

Sampler::~Sampler()
{
	this->setup->device.destroySampler(this->handle);
}
