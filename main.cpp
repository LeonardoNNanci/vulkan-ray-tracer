#include "setup.hpp"
#include "presentation.hpp"
#include "command_pool.hpp"
#include "buffer.hpp"
#include "buffer_external.hpp"
#include "denoiser.hpp"
#include "acceleration_structure.hpp"
#include "file_reader.hpp"
#include "descriptor_sets.hpp"
#include "pipeline.hpp"
#include<glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <chrono>
#include<iostream>
#include <cstdlib>


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

	//vk::QueryPoolCreateInfo queryPoolInfo{
	//	.queryType = vk::QueryType::eTimestamp,
	//	.queryCount = 12
	//};
	//auto queryPool = setup->device.createQueryPool(queryPoolInfo);


	auto scene = FileReader().readGLTF("models\\Sponza\\", "Sponza.gltf", SceneBuilder(setup, commandPool->createCommandBuffer())).build();

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
	vk::DescriptorSetLayoutBinding resultDescriptor{
		.binding = 7,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eCompute
	};

	vk::DescriptorSetLayoutBinding vertexBufferDescriptor{
		.binding = 0,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	vk::DescriptorSetLayoutBinding indexBufferDescriptor{
		.binding = 1,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	vk::DescriptorSetLayoutBinding objectDescDescriptor{
		.binding = 2,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
		.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
	};
	vk::DescriptorSetLayoutBinding materialBufferDescriptor{
		.binding = 3,
		.descriptorType = vk::DescriptorType::eStorageBuffer,
	.descriptorCount = 1,
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	vk::DescriptorSetLayoutBinding textureDescriptor{
		.binding = 4,
		.descriptorType = vk::DescriptorType::eCombinedImageSampler,
		.descriptorCount = static_cast<uint32_t>(scene->texturePointers.size()),
		.stageFlags = vk::ShaderStageFlagBits::eClosestHitKHR,
	};

	auto sceneSet = DescriptorSetBuilder(setup)
		.addBinding(vertexBufferDescriptor)
		.addBinding(indexBufferDescriptor)
		.addBinding(objectDescDescriptor)
		.addBinding(materialBufferDescriptor)
		.addBinding(textureDescriptor)
		.build();
	sceneSet->updateDescriptor(vertexBufferDescriptor, scene->vertexBuffer);
	sceneSet->updateDescriptor(indexBufferDescriptor, scene->indexBuffer);
	sceneSet->updateDescriptor(objectDescDescriptor, scene->objectDescriptionBuffer);
	sceneSet->updateDescriptor(materialBufferDescriptor, scene->materialBuffer);
	sceneSet->updateDescriptor(textureDescriptor, scene->texturePointers);

	auto imageArrayBuilder = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer);

	std::shared_ptr<DescriptorSet> rayTracingSets[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> inputBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> albedoBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> normalBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<BufferExternal> resultBuffers[FRAMES_IN_FLIGHT];
	auto rayTracingSetBuilder = DescriptorSetBuilder(setup)
		.addBinding(bvhDescriptor)
		.addBinding(rgbaImageDescriptor)
		.addBinding(rgbDescriptor)
		.addBinding(albedoDescriptor)
		.addBinding(normalDescriptor)
		.addBinding(resultDescriptor);

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		inputBuffers[i] = imageArrayBuilder.buildExternal();
		albedoBuffers[i] = imageArrayBuilder.buildExternal();
		normalBuffers[i] = imageArrayBuilder.buildExternal();
		//partialResultBuffers[i] = imageArrayBuilder.buildExternal();
		resultBuffers[i] = imageArrayBuilder.buildExternal();

		rayTracingSets[i] = rayTracingSetBuilder.build();

		rayTracingSets[i]->updateDescriptor(bvhDescriptor, BVH);
		rayTracingSets[i]->updateDescriptor(rgbDescriptor, inputBuffers[i]);
		rayTracingSets[i]->updateDescriptor(albedoDescriptor, albedoBuffers[i]);
		rayTracingSets[i]->updateDescriptor(resultDescriptor, resultBuffers[i]);
		//rayTracingSets[i]->updateDescriptor(partialResultDescriptor, partialResultBuffers[i]);
		rayTracingSets[i]->updateDescriptor(normalDescriptor, normalBuffers[i]);
		//rayTracingSets[i]->updateDescriptor(foveatedRangesDescriptor, foveatedRangeBuffer);
	}
	

	PushConstant pc;
	pc.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

	auto rayTracingPipeline = PipelineBuilder(setup)
		.addShader("./shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
		.addShader("./shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addShader("./shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addShader("./shaders/light.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addDescriptorSet(rayTracingSets[0])
		.addDescriptorSet(sceneSet)
		.addPushconstant(pc)
		.setMaxRecursionDepth(31)
		.build();

	auto bufferToImage = createComputePipeline(setup, "./shaders/buffer_to_image.spv", { rayTracingSets[0]->layout}, {});
	//auto imageBlend = createComputePipeline(setup, "./shaders/image_blend.spv", { rayTracingSets[0]->layout }, {});

	std::shared_ptr<Semaphore> imageReadySemaphores[FRAMES_IN_FLIGHT];
	std::shared_ptr<Semaphore> renderFinishedSemaphores[FRAMES_IN_FLIGHT];
	std::shared_ptr<Semaphore> timelineSemaphores[FRAMES_IN_FLIGHT];

	std::shared_ptr<CommandBuffer> layoutChangeBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<CommandBuffer> rayTracingBuffers[FRAMES_IN_FLIGHT];
	//std::shared_ptr<CommandBuffer> imgToArrayBuffers[FRAMES_IN_FLIGHT];
	std::shared_ptr<CommandBuffer> arrayToImgBuffers[FRAMES_IN_FLIGHT];
	//std::shared_ptr<CommandBuffer> blendImageBuffers[FRAMES_IN_FLIGHT];

	for (int i = 0; i < FRAMES_IN_FLIGHT; i++) {
		imageReadySemaphores[i] = std::make_shared<Semaphore>(setup);
		renderFinishedSemaphores[i] = std::make_shared<Semaphore>(setup);
		timelineSemaphores[i] = std::make_shared<Semaphore>(setup, timelineTrackers[i]);

		layoutChangeBuffers[i] = commandPool->createCommandBuffer();
		rayTracingBuffers[i] = commandPool->createCommandBuffer();
		//imgToArrayBuffers[i] = commandPool->createCommandBuffer();
		arrayToImgBuffers[i] = commandPool->createCommandBuffer();
		//blendImageBuffers[i] = commandPool->createCommandBuffer();
	}

		auto fullDenoiser = DenoiserBuilder(WIDTH, HEIGHT)
		.setGuideAlbedo()
		.setGuideNormal()
		.build();
	//auto partialDenoiser = DenoiserBuilder(WIDTH, HEIGHT)
	//	.setGuideAlbedo()
	//	.build();

	auto previousTime = std::chrono::high_resolution_clock::now();
	float angle = 0;
	//printf("LC\t\tRT\t\tA2I\t\tDenoisers\t\tFPS\n");

for (int i = 0; presentation->windowIsOpen(); i++) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
		angle = 360./1000. * i;
		float time = std::chrono::duration<float, std::chrono::seconds::period>(deltaTime).count();
		auto cameraPosition = glm::vec4(1, 1, 1., 1.0f);
		pc.data.proj = glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f);
		pc.data.projInv = glm::inverse(pc.data.proj);
		pc.data.view = glm::rotate(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 1.f), glm::vec3(0.0f, 0.0f, -1.0f)), glm::radians(angle), glm::vec3(0., 0., 1.));
		pc.data.viewInv = glm::inverse(pc.data.view);
		pc.data = pc.data;

		previousTime = currentTime;

		auto& timelineTracker = timelineTrackers[iterationTracker];
		auto& imageReadySemaphore = imageReadySemaphores[iterationTracker];
		auto& renderFinishedSemaphore = renderFinishedSemaphores[iterationTracker];
		auto& timelineSemaphore = timelineSemaphores[iterationTracker];
		auto& prevSemaphore = timelineSemaphores[prevIteration()];

		auto& layoutChangeBuffer = layoutChangeBuffers[iterationTracker];
		auto& rayTracingBuffer = rayTracingBuffers[iterationTracker];
		auto& arrayToImgBuffer = arrayToImgBuffers[iterationTracker];
		//auto& blendImageBuffer = blendImageBuffers[iterationTracker];

		auto& rayTracingSet = rayTracingSets[iterationTracker];
		auto& inputBuffer = inputBuffers[iterationTracker];
		auto& albedoBuffer = albedoBuffers[iterationTracker];
		auto& normalBuffer = normalBuffers[iterationTracker];
		//auto& partialResultBuffer = partialResultBuffers[iterationTracker];
		auto& resultBuffer = resultBuffers[iterationTracker];

		iterationTracker = (iterationTracker + 1) % FRAMES_IN_FLIGHT;

		int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, { imageReadySemaphore->handle }, {}).value;
		auto currentImage = presentation->swapchain.images[imageIndex];

		timelineSemaphore->waitSignaled(timelineTracker);
		layoutChangeBuffer->clearSync();
		rayTracingBuffer->clearSync();
		arrayToImgBuffer->clearSync();
		//blendImageBuffer->clearSync();

		//int queryTracker = 0;

		layoutChangeBuffer->addWaitSemaphore(imageReadySemaphore, vk::PipelineStageFlagBits::eAllCommands);
		layoutChangeBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		layoutChangeBuffer->begin();
		//layoutChangeBuffer->handle.resetQueryPool(queryPool, 0, 10);
		//layoutChangeBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, queryTracker++);
		currentImage->pipelineBarrier(layoutChangeBuffer, vk::ImageLayout::eGeneral);
		//layoutChangeBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, queryTracker++);
		layoutChangeBuffer->submit();
		layoutChangeBuffer->waitFinished();

		rayTracingSet->updateDescriptor(rgbaImageDescriptor, currentImage);

		rayTracingBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eRayTracingShaderKHR, timelineTracker);
		rayTracingBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		rayTracingBuffer->begin();
		//rayTracingBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, queryTracker++);
		rayTracingPipeline->run(rayTracingBuffer, presentation->swapchain.extent, {rayTracingSet, sceneSet}, { pc }, ranges);
		//rayTracingBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, queryTracker++);
		rayTracingBuffer->submit();

		//auto fullImage = calcTile(std::max(WIDTH, HEIGHT) / 2, true);
		//partialDenoiser->setSync(timelineSemaphore->cuda, timelineTracker++, timelineTracker+1);
		//partialDenoiser->run(0., inputBuffer->optixBuffer, albedoBuffer->optixBuffer, resultBuffer->optixBuffer, fullImage);
		
		//blendImageBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eComputeShader, timelineTracker);
		//blendImageBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		//blendImageBuffer->begin();
		////blendImageBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, queryTracker++);
		//blendImageBuffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, imageBlend->handle);
		//blendImageBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, imageBlend->layout, 0, { rayTracingSet->handle }, { 0 });
		//blendImageBuffer->handle.dispatch(ceil((float)WIDTH / 16.), ceil((float)HEIGHT / 16.), 1);
		////blendImageBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, queryTracker++);
		//blendImageBuffer->submit();

		auto fullImage = calcTile(std::max(WIDTH, HEIGHT) / 2, true);
		//auto centerTile = calcTile(OUTER_RADIUS, true);
		fullDenoiser->setSync(timelineSemaphore->cuda, timelineTracker++, timelineTracker+1);
		fullDenoiser->run(1., normalBuffer->optixBuffer, albedoBuffer->optixBuffer, normalBuffer->optixBuffer, resultBuffer->optixBuffer, fullImage);

		arrayToImgBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eComputeShader, timelineTracker);
		arrayToImgBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		arrayToImgBuffer->addSignalSemaphore(renderFinishedSemaphore, vk::PipelineStageFlagBits::eAllCommands);
		arrayToImgBuffer->begin();
		//arrayToImgBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eTopOfPipe, queryPool, queryTracker++);
		arrayToImgBuffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, bufferToImage->handle);
		arrayToImgBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, bufferToImage->layout, 0, { rayTracingSet->handle }, { 0 });
		arrayToImgBuffer->handle.dispatch(ceil((float)WIDTH / 16.), ceil((float)HEIGHT / 16.), 1);
		currentImage->presentBarrier(arrayToImgBuffer);
		//arrayToImgBuffer->handle.writeTimestamp(vk::PipelineStageFlagBits::eBottomOfPipe, queryPool, queryTracker++);
		arrayToImgBuffer->submit();
		
		std::vector<vk::SwapchainKHR> swapchains = { presentation->swapchain.handle };
		std::vector<uint32_t> imageIndices = { static_cast<uint32_t>(imageIndex) };
		vk::PresentInfoKHR presentInfo{};
		presentInfo.setSwapchains(swapchains);
		presentInfo.setImageIndices(imageIndices);
		presentInfo.setWaitSemaphores(renderFinishedSemaphore->handle);
		setup->graphicsQueue.handle.presentKHR(presentInfo);

		//auto timestamps = setup->device.getQueryPoolResults<uint64_t>(queryPool, 0, queryTracker, queryTracker*sizeof(uint64_t), sizeof(uint64_t), vk::QueryResultFlagBits::eWait | vk::QueryResultFlagBits::e64).value;
		//for (int i = 0; i < timestamps.size(); i += 2) {
		//	float rtTime = float(timestamps[i+1] - timestamps[i]) * deviceLimits.timestampPeriod / 1000000.0f;
		//	printf("%f\t", rtTime);
		//}
		/*{
			float denoiseTime = float(timestamps[4] - timestamps[3]) * deviceLimits.timestampPeriod / 1000000.0f;
			printf("%f\t", denoiseTime);
			printf("%f\n", 1 / deltaTime);
		}*/
	}
	printf("\n");
	setup->device.waitIdle();
	presentation->closeWindow();

	//setup->device.destroyQueryPool(queryPool);
}

int main(int argc, char* argv[]) {
	//if(argc < 6)
	//{
	//	std::cerr << "Not enough command line arguments. Expected: WIDTH HEIGHT INNER_RADUIS OUTER_RADIUS P" << std::endl;
	//	return -1;
	//}

	WIDTH = 900;
	HEIGHT = 900;
	INNER_RADIUS = 1500;
	OUTER_RADIUS = 1900;
	//P = std::atof(argv[5]);

	ranges = { {1., 0., (float)INNER_RADIUS}, {(float)1. / P, (float)OUTER_RADIUS, (float)WIDTH} };

	gazePoint = { WIDTH / 2, HEIGHT / 2 };

	try {
		run();
	}
	catch (const std::exception& e) {
		std::cerr << e.what() << std::endl;
	}
}