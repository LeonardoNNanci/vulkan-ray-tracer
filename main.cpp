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

uint64_t timelineTracker = 0;

Model3D squareModel {
	.vertices = {
		{{ -1., 1.,  0., 0.5 }},
		{{ 1., 1.,  0., 0.5 }},
		{{ -1., -1.,  0., 0.5 }},
		{{ 1., -1.,  0., 0.5 }}},
	.indices = {
		2, 0, 1, //floor
		3, 2, 1}
};

#include<iostream>

std::shared_ptr< Pipeline > createComputePipeline(std::shared_ptr<Setup> setup, const char* shaderFile,
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

int main() {
	auto setup = SetupBuilder()
		.addExtensions(PresentationBuilder::getRequirements())
		.addExtensions(BufferExternalBuilder::getRequirements())
		.addExtensions(AccelerationStructureBuilder::getRequirements())
		.addExtensions(DescriptorSetsBuilder::getRequirements())
		.addExtensions(PipelineBuilder::getRequirements())
		.build();
	auto presentation = PresentationBuilder(setup).build();
	auto commandPool = CommandPoolBuilder(setup).build();

	auto WIDTH = presentation->swapchain.extent.width;
	auto HEIGHT = presentation->swapchain.extent.height;
	auto dragonModel = FileReader().readPLY("./models/dragon_vrip.ply");
	//auto bunnyModel = FileReader().readPLY("./models/bunny.ply");
	Instance ground(glm::scale(glm::rotate(glm::mat4(1.), glm::pi<glm::float32>(), glm::vec3(0., 1., 0.)), glm::vec3(10.)), 0);
	Instance dragon(glm::translate(glm::rotate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), glm::float32{ -0.75 }, glm::vec3(0., 1., 0.)), glm::vec3(0., -.054, 0.)), 0);
	Instance light(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10)), -glm::pi<glm::float32>(), glm::vec3(1., 1., 0.)), glm::vec3(0., 0., -0.5)), 1);
	////Instance bunny(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(5.)), glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), glm::vec3(.04, -0.037, -.1)), 0);
	//Instance ceiling(glm::translate(glm::rotate(glm::mat4(1.), glm::pi<glm::float32>(), -glm::vec3(0., 0., 1.)), glm::vec3(0., 0., 3.)), 0);
	Instance left(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), -glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), glm::vec3(0., -0.5, 0.5)), 0);
	Instance right(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), glm::pi<glm::float32>() / 2, glm::vec3(1., 0., 0.)), glm::vec3(0., 0.5, 0.5)), 0);
	Instance front(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), -glm::pi<glm::float32>() / 2, glm::vec3(0., 1., 0.)), glm::vec3(0.5, 0., 0.5)), 0);
	Instance back(glm::translate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), glm::pi<glm::float32>() / 2, glm::vec3(0., 1., 0.)), glm::vec3(-0.5, 0., 0.5)), 0);
	//Instance cube(glm::rotate(glm::translate(glm::mat4(1.), glm::vec3(0., 0., 0.5)), glm::pi<glm::float32>() / 6, glm::vec3(0., 0., 1.)), 0);

	auto scene = SceneBuilder(setup, commandPool->createCommandBuffer())
			.addModel(squareModel)
			.addModel(dragonModel)
			.build();

	auto BVH = AccelerationStructureBuilder(setup, commandPool->createCommandBuffer())
			.addModel3D(squareModel)
			.addModel3D(dragonModel)
			.addInstance(light, 0)
			.addInstance(ground, 0)
			.addInstance(left, 0)
			.addInstance(right, 0)
			.addInstance(back, 0)
			.addInstance(front, 0)
			.addInstance(dragon, 1)
			.build();
	auto inputBuffer = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.buildExternal();
	inputBuffer->commandBuffer->setFence();
	auto albedoBuffer = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.buildExternal();
	albedoBuffer->commandBuffer->setFence();
	auto normalBuffer = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.buildExternal();
	normalBuffer->commandBuffer->setFence();
	auto resultBuffer = BufferExternalBuilder(setup)
		.setSize(WIDTH * HEIGHT * 3 * sizeof(float))
		.setMemoryProperties(vk::MemoryPropertyFlagBits::eDeviceLocal)
		.setCommandBuffer(commandPool->createCommandBuffer())
		.setUsage(vk::BufferUsageFlagBits::eStorageBuffer)
		.buildExternal();
	resultBuffer->commandBuffer->setFence();

	auto denoiser = DenoiserBuilder(WIDTH, HEIGHT).build();

	Descriptor bvhDescriptor{
		.set = 0,
		.binding = 0,
		.type = vk::DescriptorType::eAccelerationStructureKHR,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor rgbaImageDescriptor{
		.set = 0,
		.binding = 1,
		.type = vk::DescriptorType::eStorageImage,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute
	};
	//Descriptor albedoImageDescriptor{
	//	.set = 0,
	//	.binding = 2,
	//	.type = vk::DescriptorType::eStorageImage,
	//	.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eClosestHitKHR
	//};
	//Descriptor normalImageDescriptor{
	//	.set = 0,
	//	.binding = 3,
	//	.type = vk::DescriptorType::eStorageImage,
	//	.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eClosestHitKHR
	//};
	Descriptor vertexBufferDescriptor{
		.set = 1,
		.binding = 0,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor indexBufferDescriptor{
		.set = 1,
		.binding = 1,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor objectDescDescriptor{
		.set = 1,
		.binding = 2,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR,
	};
	Descriptor rgbDescriptor{
		.set = 0,
		.binding = 4,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR
	};
	Descriptor albedoDescriptor{
		.set = 0,
		.binding = 5,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor normalDescriptor{
		.set = 0,
		.binding = 6,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor resultDescriptor{
		.set = 0,
		.binding = 7,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eCompute
	};

	auto sets = DescriptorSetsBuilder(setup)
		.addDescriptor(bvhDescriptor)
		.addDescriptor(rgbaImageDescriptor)
		//.addDescriptor(albedoImageDescriptor)
		//.addDescriptor(normalImageDescriptor)
		.addDescriptor(vertexBufferDescriptor)
		.addDescriptor(indexBufferDescriptor)
		.addDescriptor(objectDescDescriptor)
		.addDescriptor(rgbDescriptor)
		.addDescriptor(albedoDescriptor)
		.addDescriptor(normalDescriptor)
		.addDescriptor(resultDescriptor)
		.build();

	auto rayTracingSet = sets[0];
	auto sceneSet = sets[1];
	//auto denoiserSet = sets[2];
	rayTracingSet->updateDescriptor(bvhDescriptor, BVH);
	sceneSet->updateDescriptor(vertexBufferDescriptor, scene->vertexBuffer);
	sceneSet->updateDescriptor(indexBufferDescriptor, scene->indexBuffer);
	sceneSet->updateDescriptor(objectDescDescriptor, scene->objectDescriptionBuffer);
	rayTracingSet->updateDescriptor(rgbDescriptor, inputBuffer);
	rayTracingSet->updateDescriptor(albedoDescriptor, albedoBuffer);
	rayTracingSet->updateDescriptor(resultDescriptor, resultBuffer);
	rayTracingSet->updateDescriptor(normalDescriptor, normalBuffer);

	PushConstant pc;
	pc.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR;

	auto rayTracingPipeline = PipelineBuilder(setup)
		.addShader("./shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
		.addShader("./shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR)
		//.addShader("./shaders/shadow.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addShader("./shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addShader("./shaders/light.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		//.addShader("./shaders/denoiser.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		//.addShader("./shaders/denoiser.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addDescriptorSet(rayTracingSet)
		.addDescriptorSet(sceneSet)
		.addPushconstant(pc)
		.build();

	auto bufferToImage = createComputePipeline(setup, "./shaders/buffer_to_image.spv", { rayTracingSet->layout }, {});
	//auto imageToBuffer = createComputePipeline(setup, "./shaders/image_to_buffer.spv", { rayTracingSet->layout }, {});

	//auto commandBuffer = commandPool->createCommandBuffer();
	//commandBuffer->setFence();

	auto imageReadySemaphore = std::make_shared<Semaphore>(setup);
	auto renderFinishedSemaphore = std::make_shared<Semaphore>(setup);
	auto timelineSemaphore = std::make_shared<Semaphore>(setup, timelineTracker);

	auto layoutChangeBuffer = commandPool->createCommandBuffer();
	auto rayTracingBuffer = commandPool->createCommandBuffer();
	auto imgToArrayBuffer = commandPool->createCommandBuffer();
	auto arrayToImgBuffer = commandPool->createCommandBuffer();

	//layoutChangeBuffer->begin();
	//for (auto& img : presentation->albedoImages)
	//	img->pipelineBarrier(layoutChangeBuffer, vk::ImageLayout::eGeneral);
	//layoutChangeBuffer->submit();
	//layoutChangeBuffer->waitFinished();

	auto previousTime = std::chrono::high_resolution_clock::now();
	float angle = 0;
	while (presentation->windowIsOpen()) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
		angle += 30. * deltaTime;
		float time = std::chrono::duration<float, std::chrono::seconds::period>(deltaTime).count();
		auto cameraPosition = glm::vec4(3.0f, 3.0f, 1.0f, 1.0f);
		pc.data.proj = glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f);
		pc.data.projInv = glm::inverse(pc.data.proj);
		pc.data.view = glm::rotate(glm::lookAt((glm::vec3(cameraPosition)), glm::vec3(0.f, 0.0f, 1.f), glm::vec3(0.0f, 0.0f, -1.0f)), glm::radians(angle), glm::vec3(0., 0., 1.));
		pc.data.viewInv = glm::inverse(pc.data.view);
		pc.data = pc.data;

		printf("\r%.2f", 1 / deltaTime);
		previousTime = currentTime;

		int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, { imageReadySemaphore->handle }, {}).value;
		auto currentImage = presentation->swapchain.images[imageIndex];
		auto albedoImage = presentation->albedoImages[imageIndex];
		auto normalImage = presentation->normalImages[imageIndex];

		timelineSemaphore->waitSignaled(timelineTracker);
		layoutChangeBuffer->clearSync();
		rayTracingBuffer->clearSync();
		arrayToImgBuffer->clearSync();

		layoutChangeBuffer->addWaitSemaphore(imageReadySemaphore, vk::PipelineStageFlagBits::eAllCommands);
		layoutChangeBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		layoutChangeBuffer->begin();
		currentImage->pipelineBarrier(layoutChangeBuffer, vk::ImageLayout::eGeneral);
		layoutChangeBuffer->submit();
		layoutChangeBuffer->waitFinished();

		rayTracingSet->updateDescriptor(rgbaImageDescriptor, currentImage);
		//rayTracingSet->updateDescriptor(albedoImageDescriptor, albedoImage);
		//rayTracingSet->updateDescriptor(normalImageDescriptor, normalImage);

		rayTracingBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eRayTracingShaderKHR, timelineTracker);
		rayTracingBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		rayTracingBuffer->begin();
		rayTracingPipeline->run(rayTracingBuffer, presentation->swapchain.extent, { pc });
		rayTracingBuffer->submit();
		rayTracingBuffer->waitFinished();

		//imgToArrayBuffer->addWaitSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, timelineTracker);
		//imgToArrayBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		//imgToArrayBuffer->begin();
		//imgToArrayBuffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, imageToBuffer->handle);
		//imgToArrayBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, imageToBuffer->layout, 0, { rayTracingSet->handle }, {0});
		//imgToArrayBuffer->handle.dispatch(ceil((float)WIDTH / 16), ceil((float)HEIGHT / 16), 1);
		//imgToArrayBuffer->submit();
		//imgToArrayBuffer->waitFinished();

		denoiser->run(inputBuffer->optixBuffer, albedoBuffer->optixBuffer, normalBuffer->optixBuffer, resultBuffer->optixBuffer);

		arrayToImgBuffer->addSignalSemaphore(timelineSemaphore, vk::PipelineStageFlagBits::eAllCommands, ++timelineTracker);
		arrayToImgBuffer->addSignalSemaphore(renderFinishedSemaphore, vk::PipelineStageFlagBits::eAllCommands);
		arrayToImgBuffer->begin();
		arrayToImgBuffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, bufferToImage->handle);
		arrayToImgBuffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, bufferToImage->layout, 0, { rayTracingSet->handle }, { 0 });
		arrayToImgBuffer->handle.dispatch(ceil((float)WIDTH / 16), ceil((float)HEIGHT / 16), 1);
		currentImage->presentBarrier(arrayToImgBuffer);
		arrayToImgBuffer->submit();
		
		std::vector<vk::SwapchainKHR> swapchains = { presentation->swapchain.handle };
		std::vector<uint32_t> imageIndices = { static_cast<uint32_t>(imageIndex) };
		vk::PresentInfoKHR presentInfo{};
		presentInfo.setSwapchains(swapchains);
		presentInfo.setImageIndices(imageIndices);
		presentInfo.setWaitSemaphores(renderFinishedSemaphore->handle);
		setup->graphicsQueue.handle.presentKHR(presentInfo);

	}
	printf("\n");
	setup->device.waitIdle();
}
