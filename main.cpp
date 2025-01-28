#include "engine/setup.hpp"
#include "engine/presentation.hpp"
#include "engine/command_pool.hpp"
#include "engine/buffer.hpp"
#include "engine/buffer_external.hpp"
#include "engine/denoiser.hpp"
#include "engine/acceleration_structure.hpp"
#include "engine/file_reader.hpp"
#include "engine/descriptor_sets.hpp"
#include "engine/pipeline.hpp"

#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <chrono>
#include<iostream>
#include <cstdlib>

#include "tiny_gltf.cc"

int WIDTH;
int HEIGHT;
int INNER_RADIUS;
int OUTER_RADIUS;
float P;

glm::ivec2 gazePoint;

#define FRAMES_IN_FLIGHT 1
uint64_t timelineTrackers[] = { 6, 12, 18 };
int iterationTracker = 0;

float calcHalfTileSize(float radius, bool circumscribed) {
	if (circumscribed)
		return radius;
	return radius / sqrt(2.);
}

std::vector<std::pair<glm::ivec2, glm::ivec2>> calcTile(float outerRadius, bool circumscribed) {
	auto halfSize = calcHalfTileSize(outerRadius, circumscribed);
	glm::ivec2 disp(halfSize);

	auto bottomLeft = gazePoint - disp;
	auto topRight = gazePoint + disp;

	bottomLeft.x = std::clamp(bottomLeft.x, 0, WIDTH);
	bottomLeft.y = std::clamp(bottomLeft.y, 0, HEIGHT);
	topRight.x = std::clamp(topRight.x, 0, WIDTH);
	topRight.y = std::clamp(topRight.y, 0, HEIGHT);

	return {
		{bottomLeft, topRight}
	};
}

std::vector<std::pair<glm::ivec2, glm::ivec2>> calcTiles(float innerRadius, float outerRadius) {
	auto centerTile = calcTile(innerRadius, false)[0];
	auto outerTile = calcTile(outerRadius, true)[0];

	return {
		{ {outerTile.first.x, outerTile.first.y}, {centerTile.first.x, outerTile.second.y}},
		{ {centerTile.second.x, outerTile.first.y}, {outerTile.second.x, outerTile.second.y} },
		{ {centerTile.first.x, outerTile.first.y}, {centerTile.second.x, centerTile.first.y} },
		{ {centerTile.first.x, centerTile.second.y}, {centerTile.second.x, outerTile.second.y} }
	};
}

Model3D squareModel {
	.vertices = {
		{{ -1., 1.,  0., 1 }},
		{{ 1., 1.,  0., 1 }},
		{{ -1., -1.,  0., 1 }},
		{{ 1., -1.,  0., 1 }}},
	.indices = {
		2, 0, 1, //floor
		3, 2, 1}
};

std::vector<Range> ranges;;

std::shared_ptr<Pipeline> createComputePipeline(std::shared_ptr<Setup> setup, const char* shaderFile,
	std::vector<vk::DescriptorSetLayout> pipelineDescriptorSetLayouts,
	std::vector<vk::PushConstantRange> pipelinePushConstantRanges) {

	vk::PipelineLayoutCreateInfo layoutInfo{};
	layoutInfo.setPushConstantRanges(pipelinePushConstantRanges);
	layoutInfo.setSetLayouts(pipelineDescriptorSetLayouts);
	auto pipelineLayout = setup->device.createPipelineLayout(layoutInfo);

	auto code = FileReader().readSPV(shaderFile);
	vk::ShaderModuleCreateInfo moduleInfo{
		.codeSize = static_cast<uint32_t>(code.size()),
		.pCode = reinterpret_cast<const uint32_t*>(code.data())
	};
	vk::ShaderModule shaderModule = setup->device.createShaderModule(moduleInfo);

	vk::ComputePipelineCreateInfo pipelineInfo{
		.stage = {
			.stage = vk::ShaderStageFlagBits::eCompute,
			.module = shaderModule,
			.pName = "main"
		},
		.layout = pipelineLayout
	};
	auto result = setup->device.createComputePipeline(nullptr, pipelineInfo);
	if (result.result != vk::Result::eSuccess)
		throw std::runtime_error("Failed to create pipeline!");
	setup->device.destroyShaderModule(shaderModule);

	auto pipeline = std::make_shared<Pipeline>(setup);
	pipeline->handle = result.value;
	pipeline->layout = pipelineLayout;

	return pipeline;

}

int prevIteration() {
	int prev = iterationTracker - 1;
	return prev >= 0 ? prev : FRAMES_IN_FLIGHT - 1;
}

void updateCamera(CameraData& camera, float angle, std::shared_ptr<Presentation> presentation) {
	auto cameraPosition = glm::vec4(-0.5, 7.5f, 0., 1.0f);
	auto proj = glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f);
	auto view = glm::rotate(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 7.5f, 0.f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::radians(angle), glm::vec3(0., 1., 0.));
	auto view2 = glm::rotate(glm::lookAt(glm::vec3(cameraPosition) + glm::vec3(.1, 0, 0), glm::vec3(0.f, 7.5f, 0.f), glm::vec3(0.0f, -1.0f, 0.0f)), glm::radians(angle), glm::vec3(0., 1., 0.));

	camera.setCurrMats(proj, view, proj, view2);
}

void run() {
	auto setup = SetupBuilder()
		.addExtensions(PresentationBuilder::getRequirements())
		.addExtensions(BufferExternalBuilder::getRequirements())
		.addExtensions(AccelerationStructureBuilder::getRequirements())
		.addExtensions(DescriptorSetBuilder::getRequirements())
		.addExtensions(PipelineBuilder::getRequirements())
		.addExtensions(Semaphore::getRequirements())
		.build();
	auto presentation = PresentationBuilder(setup, WIDTH, HEIGHT).build();
	auto commandPool = CommandPoolBuilder(setup).build();

	auto deviceLimits = setup->physicalDevice.getProperties().limits;

	std::shared_ptr<Scene> scene;
	glm::mat4 light;
	{
		std::string folder = "models\\Sponza\\";
		std::string file = "Sponza.gltf";

		auto gltfData = FileReader().readGLTF(folder, file);

		scene = SceneBuilder(setup, commandPool->createCommandBuffer())
			.loadGlTF(gltfData, folder)
			.build();
	}

	auto BVH = AccelerationStructureBuilder(setup, commandPool->createCommandBuffer())
			.setScene(scene)
			.build();

	vk::DescriptorSetLayoutBinding bvhDescriptor{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eAccelerationStructureKHR,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
	};
	vk::DescriptorSetLayoutBinding rgbaImageDescriptor{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eStorageImage,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute
	};
	vk::DescriptorSetLayoutBinding rgbDescriptor{
		.binding = 4,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute
	};
	vk::DescriptorSetLayoutBinding albedoDescriptor{
		.binding = 5,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
	};
	vk::DescriptorSetLayoutBinding normalDescriptor{
		.binding = 6,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR
	};
	vk::DescriptorSetLayoutBinding flowDescriptor{
		.binding = 7,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute
	};
	vk::DescriptorSetLayoutBinding resultDescriptor{
		.binding = 8,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute
	};

	vk::DescriptorSetLayoutBinding vertexBufferDescriptor{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR
	};
	vk::DescriptorSetLayoutBinding indexBufferDescriptor{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR
	};
	vk::DescriptorSetLayoutBinding objectDescDescriptor{
		.binding = 2,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR,
	};
	vk::DescriptorSetLayoutBinding materialBufferDescriptor{
		.binding = 3,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR
	};
	vk::DescriptorSetLayoutBinding lightDescriptor{
		.binding = 4,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
	};
	vk::DescriptorSetLayoutBinding textureDescriptor{
		.binding = 5,
		.descriptorType = vk::DescriptorType::eCombinedImageSampler,
		.descriptorCount = static_cast<uint32_t>(scene->texturePointers.size()),
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eAnyHitKHR
	};

	auto sceneSet = DescriptorSetBuilder(setup)
		.addBinding(vertexBufferDescriptor)
		.addBinding(indexBufferDescriptor)
		.addBinding(objectDescDescriptor)
		.addBinding(materialBufferDescriptor)
		.addBinding(lightDescriptor)
		.addBinding(textureDescriptor)
		.build();
	sceneSet->updateDescriptor(vertexBufferDescriptor, scene->vertexBuffer);
	sceneSet->updateDescriptor(indexBufferDescriptor, scene->indexBuffer);
	sceneSet->updateDescriptor(objectDescDescriptor, scene->objectDescriptionBuffer);
	sceneSet->updateDescriptor(materialBufferDescriptor, scene->materialBuffer);
	sceneSet->updateDescriptor(lightDescriptor, scene->lightBuffer);
	sceneSet->updateDescriptor(textureDescriptor, scene->texturePointers);

	auto imageArrayBuilder = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer);
	auto flowArrayBuilder = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 2 * sizeof(float))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

	std::shared_ptr<DescriptorSet> rayTracingSets[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> inputBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> albedoBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> normalBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> resultBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> flowBuffers[FRAMES_IN_FLIGHT];
	auto rayTracingSetBuilder = DescriptorSetBuilder(setup)
		.addBinding(bvhDescriptor)
		.addBinding(rgbaImageDescriptor)
		.addBinding(rgbDescriptor)
		.addBinding(albedoDescriptor)
		.addBinding(normalDescriptor)
		.addBinding(flowDescriptor)
		.addBinding(resultDescriptor);

	vk::DescriptorSetLayoutBinding cameraBinding{
			.binding = 0,
			.descriptorType = vk::DescriptorType::eUniformBuffer,
			.descriptorCount = 1,
			.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR | vk::ShaderStageFlagBits::eRaygenKHR,
	};
	std::shared_ptr<Buffer> cameraBuffers[FRAMES_IN_FLIGHT];
	auto cameraSetBuilder = DescriptorSetBuilder(setup)
		.addBinding(cameraBinding);
	std::shared_ptr<DescriptorSet> cameraSets[FRAMES_IN_FLIGHT];
	auto cameraBufferBuilder = BufferBuilder(setup)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setSize(sizeof(CameraData))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent)
		.setUsage(vk::BufferUsageFlagBits::eUniformBuffer);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		inputBuffers[i] = imageArrayBuilder.buildExternal();
		albedoBuffers[i] = imageArrayBuilder.buildExternal();
		normalBuffers[i] = imageArrayBuilder.buildExternal();
		resultBuffers[i] = imageArrayBuilder.buildExternal();
		flowBuffers[i] = flowArrayBuilder.buildExternal();

		rayTracingSets[i] = rayTracingSetBuilder.build();

		rayTracingSets[i]->updateDescriptor(bvhDescriptor, BVH);
		rayTracingSets[i]->updateDescriptor(rgbDescriptor, inputBuffers[i]);
		rayTracingSets[i]->updateDescriptor(albedoDescriptor, albedoBuffers[i]);
		rayTracingSets[i]->updateDescriptor(resultDescriptor, resultBuffers[i]);
		rayTracingSets[i]->updateDescriptor(normalDescriptor, normalBuffers[i]);
		rayTracingSets[i]->updateDescriptor(flowDescriptor, flowBuffers[i]);

		cameraBuffers[i] = cameraBufferBuilder.build();
		cameraSets[i] = cameraSetBuilder.build();
		cameraSets[i]->updateDescriptor(cameraBinding, cameraBuffers[i]);
	}
	
	PushConstant pc;
	pc.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR;

	auto rayTracingPipeline = PipelineBuilder(setup)
		.addShader("./shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
		.addShader("./shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addShader("./shaders/shadow.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addHitGroup("./shaders/closesthit.spv", "./shaders/anyhit.spv")
		.addDescriptorSet(rayTracingSets[0])
		.addDescriptorSet(sceneSet)
		.addDescriptorSet(cameraSets[0])
		.addPushconstant(pc)
		.setMaxRecursionDepth(31)
		.build();

	auto bufferToImage = createComputePipeline(setup, "./shaders/buffer_to_image.spv", { rayTracingSets[0]->layout}, {});

	std::shared_ptr<Semaphore> imageReadySemaphores[FRAMES_IN_FLIGHT];
	std::shared_ptr<Semaphore> renderFinishedSemaphores[FRAMES_IN_FLIGHT];
	std::shared_ptr<Semaphore> timelineSemaphores[FRAMES_IN_FLIGHT];

	std::shared_ptr<CommandBuffer> layoutChangeBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<CommandBuffer> rayTracingBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<CommandBuffer> arrayToImgBuffers[FRAMES_IN_FLIGHT];

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		imageReadySemaphores[i] = std::make_shared<Semaphore>(setup);
		renderFinishedSemaphores[i] = std::make_shared<Semaphore>(setup);
		timelineSemaphores[i] = std::make_shared<Semaphore>(setup, timelineTrackers[i]);

		layoutChangeBuffers[i] = commandPool->createCommandBuffer();
		rayTracingBuffers[i] = commandPool->createCommandBuffer();
		arrayToImgBuffers[i] = commandPool->createCommandBuffer();
	}

		auto fullDenoiser = DenoiserBuilder(WIDTH, HEIGHT)
		.setGuideAlbedo()
		.setGuideNormal()
		.build();

	auto previousTime = std::chrono::high_resolution_clock::now();
	float angle = 0;

	CameraData camera;
	updateCamera(camera, angle, presentation);

	for (int i = 0; presentation->windowIsOpen(); i++) {
		auto& timelineTracker = timelineTrackers[iterationTracker];
		auto& imageReadySemaphore = imageReadySemaphores[iterationTracker];
		auto& renderFinishedSemaphore = renderFinishedSemaphores[iterationTracker];
		auto& timelineSemaphore = timelineSemaphores[iterationTracker];

		auto& layoutChangeBuffer = layoutChangeBuffers[iterationTracker];
		auto& rayTracingBuffer = rayTracingBuffers[iterationTracker];
		auto& arrayToImgBuffer = arrayToImgBuffers[iterationTracker];

		auto& rayTracingSet = rayTracingSets[iterationTracker];
		auto& inputBuffer = inputBuffers[iterationTracker];
		auto& albedoBuffer = albedoBuffers[iterationTracker];
		auto& normalBuffer = normalBuffers[iterationTracker];
		auto& flowBuffer = flowBuffers[iterationTracker];
		auto& resultBuffer = resultBuffers[iterationTracker];

		auto& cameraSet = cameraSets[iterationTracker];
		auto& cameraBuffer = cameraBuffers[iterationTracker];

		angle += 360./1000.;
		updateCamera(camera, angle, presentation);
		cameraBuffer->fill<CameraData>({ camera });

		auto currentTime = std::chrono::high_resolution_clock::now();

		pc.data.frame = i;
		pc.data.time = std::chrono::duration<float>(currentTime.time_since_epoch()).count();

		iterationTracker = (iterationTracker + 1) % FRAMES_IN_FLIGHT;

		int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, { imageReadySemaphore->handle }, {}).value;
		auto currentImage = presentation->swapchain.images[imageIndex];

		timelineSemaphore->waitSignaled(timelineTracker);
		layoutChangeBuffer->clearSync();
		rayTracingBuffer->clearSync();
		arrayToImgBuffer->clearSync();

		layoutChangeBuffer->addWaitSemaphore(imageReadySemaphore, vk::PipelineStageFlagBits::eAllCommands);
		layoutChangeBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		layoutChangeBuffer->begin();
		currentImage->pipelineBarrier(layoutChangeBuffer, vk::ImageLayout::eGeneral);

		rayTracingSet->updateDescriptor(rgbaImageDescriptor, currentImage);

		rayTracingBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eRayTracingShaderKHR, timelineTracker);
		rayTracingBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		rayTracingBuffer->begin();
		rayTracingPipeline->run(rayTracingBuffer, presentation->swapchain.extent, {rayTracingSet, sceneSet, cameraSet}, { pc }, ranges);

		auto fullImage = calcTile(std::max(WIDTH, HEIGHT) / 2, true);
		fullDenoiser->setSync(timelineSemaphore->cuda, timelineTracker++, timelineTracker+1);

		arrayToImgBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eComputeShader, timelineTracker);
		arrayToImgBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		arrayToImgBuffer->addSignalSemaphore(renderFinishedSemaphore, vk::PipelineStageFlagBits::eAllCommands);
		arrayToImgBuffer->begin();
		arrayToImgBuffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, bufferToImage->handle);
		arrayToImgBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, bufferToImage->layout, 0, { rayTracingSet->handle }, { 0 });
		arrayToImgBuffer->handle.dispatch(ceil((float)WIDTH / 16.), ceil((float)HEIGHT / 16.), 1);
		currentImage->presentBarrier(arrayToImgBuffer);
		
		std::vector<vk::SwapchainKHR> swapchains = { presentation->swapchain.handle };
		std::vector<uint32_t> imageIndices = { static_cast<uint32_t>(imageIndex) };
		vk::PresentInfoKHR presentInfo{};
		presentInfo.setSwapchains(swapchains);
		presentInfo.setImageIndices(imageIndices);
		presentInfo.setWaitSemaphores(renderFinishedSemaphore->handle);

		layoutChangeBuffer->submit();
		layoutChangeBuffer->waitFinished();
		rayTracingBuffer->submit();
		fullDenoiser->run(0., inputBuffer->optixBuffer, albedoBuffer->optixBuffer, normalBuffer->optixBuffer, flowBuffer->optixBuffer, resultBuffer->optixBuffer, fullImage);
		arrayToImgBuffer->submit();
		setup->graphicsQueue.handle.presentKHR(presentInfo);
	}
	printf("\n");
	setup->device.waitIdle();
	presentation->closeWindow();

	//setup->device.destroyQueryPool(queryPool);
}

int main(int argc, char* argv[]) {
	WIDTH = 1920;
	HEIGHT = 1080;
	INNER_RADIUS = 1500;
	OUTER_RADIUS = 1900;

	ranges = { {1., 0., (float)INNER_RADIUS}, {(float)1. / P, (float)OUTER_RADIUS, (float)WIDTH} };

	gazePoint = { WIDTH / 2, HEIGHT / 2 };

	try {
		run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}
