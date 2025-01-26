#pragma once

#include "builder.hpp"
#include "setup.hpp"
#include "buffer.hpp"
#include "file_reader.hpp"
#include "descriptor_sets.hpp"
#include "presentation.hpp"


#include <memory>

struct CameraData {
	alignas(16) glm::mat4 currProj;
	alignas(16) glm::mat4 currProjInv;
	alignas(16) glm::mat4 currView;
	alignas(16) glm::mat4 currViewInv;

	//alignas(16) glm::mat4 prevProj;
	//alignas(16) glm::mat4 prevProjInv;
	//alignas(16) glm::mat4 prevView;
	//alignas(16) glm::mat4 prevViewInv;

public:
	void setCurrMats(glm::mat4 proj, glm::mat4 view);
};

class PushConstant {

	class Contents {
	public:
		alignas(4) float time;
		alignas(4) int frame;
	};

public:
	Contents data;
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
	//std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;


	Pipeline(std::shared_ptr<Setup> setup);

	void run(std::shared_ptr<CommandBuffer> commandBuffer, vk::Extent2D extent, std::vector<std::shared_ptr<DescriptorSet>> descriptorSets, std::vector<PushConstant> pushConstants, std::vector<Range> ranges);

	~Pipeline();
};

class PipelineBuilder : public Builder<std::shared_ptr<Pipeline>>, private IHasSetup {
public:
	static Requirements getRequirements();

	PipelineBuilder(std::shared_ptr<Setup> setup);

	std::shared_ptr<Pipeline> build();

	PipelineBuilder addShader(const std::string& shaderFileName, vk::ShaderStageFlagBits stage);

	PipelineBuilder addHitGroup(const std::string& closestHitFileName, const std::string& anyHitFilename);;

	PipelineBuilder addDescriptorSet(std::shared_ptr<DescriptorSet> descriptorSet);

	PipelineBuilder addPushconstant(PushConstant pushConstant);

	PipelineBuilder setMaxRecursionDepth(uint32_t depth);

private:
	std::vector<vk::PipelineShaderStageCreateInfo> stages;
	std::map<std::string, std::pair<vk::ShaderModule, unsigned int>> shaderModules; // <fileName, <shaderModule, stageIndex>>
	std::vector<vk::RayTracingShaderGroupCreateInfoKHR> shaderGroups;
	std::vector<vk::PushConstantRange> pushConstantRanges;
	std::vector<std::shared_ptr<DescriptorSet>> descriptorSets;
	uint32_t maxRecursionDepth = 32;

	uint32_t hitCount = 0;
	uint32_t missCount = 0;
	uint32_t callCount = 0;
	uint32_t anyCount = 0;

	ShaderBindingTable createShaderBindingTable(vk::Pipeline pipeline);

	uint32_t resolveStage(const std::string& shaderFileName, vk::ShaderStageFlagBits stage);

	vk::ShaderModule createShaderModule(std::vector<char>& code);

	vk::PipelineShaderStageCreateInfo createStage(vk::ShaderModule shaderModule, vk::ShaderStageFlagBits stage);
};

