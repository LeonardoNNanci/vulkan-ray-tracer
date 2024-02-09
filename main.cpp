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

Model3D cubeModel {
	.vertices = {
		{ { -.5, .5, -.5, 1. }},
		{ { .5,  .5, -.5, 1. } },
		{ { -.5, -.5, -.5, 1. } },
		{ { .5, -.5, -.5, 1. } },
		{ { -.5,  .5,  .5, 1. } },
		{ { .5,  .5,  .5, 1. } },
		{ { -.5, -.5,  .5, 1. } },
		{ { .5, -.5,  .5, 1. } }},
	.indices = {
		0, 1, 2, // Side 0
		2, 1, 3,
		4, 0, 6, // Side 1
		6, 0, 2,
		7, 5, 6, // Side 2
		6, 5, 4,
		3, 1, 7, // Side 3 
		7, 1, 5,
		4, 5, 0, // Side 4 
		0, 5, 1,
		3, 7, 2, // Side 5 
		2, 7, 6,}
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
			//.addModel(cubeModel)
			.addModel(dragonModel)
			//.addModel(bunnyModel)
			.build();

		// TODO: change argument to command pool, instead of commandBuffer
	auto BVH = AccelerationStructureBuilder(setup, commandPool->createCommandBuffer())
			.addModel3D(squareModel)
			.addModel3D(dragonModel)
			//.addModel3D(bunnyModel)
			//.addModel3D(cubeModel)
			.addInstance(light, 0)
			.addInstance(ground, 0)
			//.addInstance(ceiling, 0)
			.addInstance(left, 0)
			.addInstance(right, 0)
			.addInstance(back, 0)
			.addInstance(front, 0)
			.addInstance(dragon, 1)
			//.addInstance(bunny, 2)
			//.addInstance(cube, 1)
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
	Descriptor albedoImageDescriptor{
		.set = 0,
		.binding = 2,
		.type = vk::DescriptorType::eStorageImage,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor normalImageDescriptor{
		.set = 0,
		.binding = 3,
		.type = vk::DescriptorType::eStorageImage,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eCompute | vk::ShaderStageFlagBits::eClosestHitKHR
	};
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
		.stagesUsed = vk::ShaderStageFlagBits::eCompute
	};
	Descriptor albedoDescriptor{
		.set = 0,
		.binding = 5,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eCompute
	};
	Descriptor normalDescriptor{
		.set = 0,
		.binding = 6,
		.type = vk::DescriptorType::eStorageBuffer,
		.stagesUsed = vk::ShaderStageFlagBits::eCompute
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
		.addDescriptor(albedoImageDescriptor)
		.addDescriptor(normalImageDescriptor)
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
	auto imageToBuffer = createComputePipeline(setup, "./shaders/image_to_buffer.spv", { rayTracingSet->layout }, {});

	auto commandBuffer = commandPool->createCommandBuffer();
	commandBuffer->setFence();

	auto imageReadySemaphore = std::make_shared<Semaphore>(setup);
	imageReadySemaphore->addSrcStage(vk::PipelineStageFlagBits::eTransfer);
	imageReadySemaphore->addDstStage(vk::PipelineStageFlagBits::eRayTracingShaderKHR);

	auto renderFinishedSemaphore = std::make_shared<Semaphore>(setup);
	renderFinishedSemaphore->addSrcStage(vk::PipelineStageFlagBits::eRayTracingShaderKHR);
	renderFinishedSemaphore->addSrcStage(vk::PipelineStageFlagBits::eAllCommands);

	commandBuffer->addWaitSemaphore(imageReadySemaphore);
	commandBuffer->addSignalSemaphore(renderFinishedSemaphore);
	auto copy_buffer = commandPool->createCommandBuffer();
	copy_buffer->setFence();
	auto other_buffer = commandPool->createCommandBuffer();
	other_buffer->setFence();
	auto previousTime = std::chrono::high_resolution_clock::now();
	float angle = 0;
	while (presentation->windowIsOpen()) {
		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();
		angle += 45. * deltaTime;
		float time = std::chrono::duration<float, std::chrono::seconds::period>(deltaTime).count();
		auto cameraPosition = glm::vec4(3.0f, 3.0f, 1.0f, 1.0f);
		pc.data.proj = glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f);
		pc.data.projInv = glm::inverse(pc.data.proj);
		pc.data.view = glm::rotate(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 1.f), glm::vec3(0.0f, 0.0f, -1.0f)), glm::radians(angle), glm::vec3(0., 0., 1.));
		pc.data.viewInv = glm::inverse(pc.data.view);
		pc.data = pc.data;

		//printf("\r%.2f", 1 / deltaTime);
		previousTime = currentTime;

		int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, { imageReadySemaphore->handle }, {}).value;
		auto currentImage = presentation->swapchain.images[imageIndex];
		auto albedoImage = presentation->albedoImages[imageIndex];
		auto normalImage = presentation->normalImages[imageIndex];

		commandBuffer->begin();
		currentImage->pipelineBarrier(commandBuffer, vk::ImageLayout::eGeneral);
		commandBuffer->submit();
		commandBuffer->waitFinished();
		commandBuffer->resetFence();

		rayTracingSet->updateDescriptor(rgbaImageDescriptor, currentImage);
		rayTracingSet->updateDescriptor(albedoImageDescriptor, albedoImage);
		rayTracingSet->updateDescriptor(normalImageDescriptor, normalImage);

		commandBuffer->begin();
		currentImage->clear(commandBuffer);
		currentImage->renderBarrier(commandBuffer);
		rayTracingPipeline->run(commandBuffer, presentation->swapchain, { pc });
		//presentation->swapchain.images[imageIndex]->presentBarrier(commandBuffer);
		commandBuffer->submit();
		commandBuffer->waitFinished();
		commandBuffer->resetFence();


		copy_buffer->begin();
		copy_buffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, imageToBuffer->handle);
		copy_buffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, imageToBuffer->layout, 0, { rayTracingSet->handle }, {0});
		copy_buffer->handle.dispatch(ceil((float)WIDTH / 16), ceil((float)HEIGHT / 16), 1);
		copy_buffer->submit();
		copy_buffer->waitFinished();
		copy_buffer->resetFence();

		denoiser->run(inputBuffer->optixBuffer, albedoBuffer->optixBuffer, normalBuffer->optixBuffer, resultBuffer->optixBuffer);

		other_buffer->begin();
		other_buffer->handle.bindPipeline(vk::PipelineBindPoint::eCompute, bufferToImage->handle);
		other_buffer->handle.bindDescriptorSets(vk::PipelineBindPoint::eCompute, bufferToImage->layout, 0, { rayTracingSet->handle }, { 0 });
		other_buffer->handle.dispatch(ceil((float)WIDTH / 16), ceil((float)HEIGHT / 16), 1);
		currentImage->presentBarrier(other_buffer);
		other_buffer->submit();
		other_buffer->waitFinished();
		other_buffer->resetFence();
		
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
