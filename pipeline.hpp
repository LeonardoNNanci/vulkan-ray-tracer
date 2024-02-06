#pragma once

#include "builder.hpp"
#include "setup.hpp"
#include "buffer.hpp"
#include "file_reader.hpp"
#include "descriptor_sets.hpp"
#include "presentation.hpp"


#include <memory>

struct PushConstantData {
	alignas(16) glm::mat4 proj;
	alignas(16) glm::mat4 projInv;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 viewInv;
};

class PushConstant {
public:
	PushConstantData data;
	vk::ShaderStageFlags stagesUsed;
	uint32_t size();

	void* pointer();
};

class ShaderBindingTable {
public:
	std::shared_ptr<Buffer> buffer;
	vk::StridedDeviceAddressRegionKHR rayGenRegion;
	vk::StridedDeviceAddressRegionKHR missRegion;
	vk::StridedDeviceAddressRegionKHR hitRegion;
	vk::StridedDeviceAddressRegionKHR callRegion;
	vk::StridedDeviceAddressRegionKHR anyRegion;
};

class Pipeline : IHasSetup {
public:
	vk::Pipeline handle;
	vk::PipelineLayout layout;
	ShaderBindingTable SBT;
	std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;


	Pipeline(std::shared_ptr<Setup> setup);

	void run(std::shared_ptr<CommandBuffer> commandBuffer, Swapchain swapchain, std::vector<PushConstant> pushConstants);

	~Pipeline();
};

class PipelineBuilder : public Builder<std::shared_ptr<Pipeline>>, private IHasSetup {
public:
	static Requirements getRequirements();

	PipelineBuilder(std::shared_ptr<Setup> setup);

	std::shared_ptr<Pipeline> build();

	PipelineBuilder addShader(const std::string& shaderFileName, vk::ShaderStageFlagBits stage);

	PipelineBuilder addDescriptorSet(std::shared_ptr<DescriptorSet> descriptorSet);

	PipelineBuilder addPushconstant(PushConstant pushConstant);

	PipelineBuilder setMaxRecursionDepth(uint32_t depth);

private:
	std::vector<vk::PipelineShaderStageCreateInfo> stages;
	std::vector<vk::ShaderModule> shaderModules;
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
	std::vector<vk::PushConstantRange> pushConstantRanges;
	std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
	uint32_t maxRecursionDepth = 2;

	uint32_t hitCount = 0;
	uint32_t missCount = 0;
	uint32_t callCount = 0;
	uint32_t anyCount = 0;

	ShaderBindingTable createShaderBindingTable(vk::Pipeline pipeline);

	vk::ShaderModule createShaderModule(std::vector<char>& code);

	vk::PipelineShaderStageCreateInfo createStage(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits stage);
};

