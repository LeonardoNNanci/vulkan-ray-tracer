#include "setup.hpp"
#include "presentation.hpp"
#include "command_pool.hpp"
#include "buffer.hpp"
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
int main() {
	auto setup = SetupBuilder()
		.addExtensions(PresentationBuilder::getRequirements())
		.addExtensions(AccelerationStructureBuilder::getRequirements())
		.addExtensions(DescriptorSetsBuilder::getRequirements())
		.addExtensions(PipelineBuilder::getRequirements())
		.build();
	auto presentation = PresentationBuilder(setup).build();
	auto commandPool = CommandPoolBuilder(setup).build();

	auto dragonModel = FileReader().readPLY("./models/dragon_vrip.ply");
	//auto bunnyModel = FileReader().readPLY("./models/bunny.ply");
	Instance ground(glm::scale(glm::rotate(glm::mat4(1.), glm::pi<glm::float32>(), glm::vec3(0., 1., 0.)), glm::vec3(10.)), 0);
	Instance dragon(glm::translate(glm::rotate(glm::rotate(glm::scale(glm::mat4(1.), glm::vec3(10.)), glm::pi<glm::float32>() / 2,glm::vec3(1., 0., 0.)), glm::float32{-0.75}, glm::vec3(0., 1., 0.)), glm::vec3(0., -.054, 0.)), 0);
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

	Descriptor bvhDescriptor{
		.set = 0,
		.binding = 0,
		.type = vk::DescriptorType::eAccelerationStructureKHR,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitKHR
	};
	Descriptor imageDescriptor{
		.set = 0,
		.binding = 1,
		.type = vk::DescriptorType::eStorageImage,
		.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR
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

	auto sets = DescriptorSetsBuilder(setup)
		.addDescriptor(bvhDescriptor)
		.addDescriptor(imageDescriptor)
		.addDescriptor(vertexBufferDescriptor)
		.addDescriptor(indexBufferDescriptor)
		.addDescriptor(objectDescDescriptor)
		.build();

	auto rayTracingSet = sets[0];
	auto sceneSet = sets[1];
	rayTracingSet->updateDescriptor(bvhDescriptor, BVH);
	sceneSet->updateDescriptor(vertexBufferDescriptor, scene->vertexBuffer);
	sceneSet->updateDescriptor(indexBufferDescriptor, scene->indexBuffer);
	sceneSet->updateDescriptor(objectDescDescriptor, scene->objectDescriptionBuffer);

	PushConstant pc;
	pc.stagesUsed = vk::ShaderStageFlagBits::eRaygenKHR | vk::ShaderStageFlagBits::eClosestHitNV;

	auto pipeline = PipelineBuilder(setup)
		.addShader("./shaders/raygen.spv", vk::ShaderStageFlagBits::eRaygenKHR)
		.addShader("./shaders/miss.spv", vk::ShaderStageFlagBits::eMissKHR)
		//.addShader("./shaders/shadow.spv", vk::ShaderStageFlagBits::eMissKHR)
		.addShader("./shaders/closesthit.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addShader("./shaders/light.spv", vk::ShaderStageFlagBits::eClosestHitKHR)
		.addDescriptorSet(rayTracingSet)
		.addDescriptorSet(sceneSet)
		.addPushconstant(pc)
		.build();

	pc.data.clearColor = glm::vec4(0.);
	pc.data.lightPosition = glm::vec4(-2., 2., 2., 1.);
	pc.data.lightIntensity = .8;
	pc.data.lightType = 1;
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
	auto cameraPosition = glm::vec4(-5., 0., 1.5, 1.); // glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(2.0f, 2.0f, 2.0f, 1.0f);
	pc.data.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f));
	pc.data.viewInv = glm::inverse(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 1.5f), glm::vec3(0.0f, 0.0f, -1.0f)));
	pc.data = pc.data;
	auto commandBuffer = commandPool->createCommandBuffer();
	commandBuffer->setFence(true);

	auto imageReadySemaphore = std::make_shared<Semaphore>(setup);
	imageReadySemaphore->addSrcStage(vk::PipelineStageFlagBits::eTransfer);
	imageReadySemaphore->addDstStage(vk::PipelineStageFlagBits::eRayTracingShaderKHR);

	auto renderFinishedSemaphore = std::make_shared<Semaphore>(setup);
	renderFinishedSemaphore->addSrcStage(vk::PipelineStageFlagBits::eRayTracingShaderKHR);
	renderFinishedSemaphore->addSrcStage(vk::PipelineStageFlagBits::eAllCommands);

	commandBuffer->addWaitSemaphore(imageReadySemaphore);
	commandBuffer->addSignalSemaphore(renderFinishedSemaphore);

	auto previousTime = std::chrono::high_resolution_clock::now();

	while (presentation->windowIsOpen()) {
		commandBuffer->waitFinished();
		commandBuffer->resetFence();

		auto currentTime = std::chrono::high_resolution_clock::now();
		float deltaTime = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - previousTime).count();

		float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();
		auto cameraPosition =  glm::rotate(glm::mat4(1.0f), time * glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f)) * glm::vec4(3.0f, 3.0f, 2.0f, 1.0f);
		pc.data.projInv = glm::inverse(glm::perspective(glm::radians(45.0f), presentation->swapchain.extent.width / (float)presentation->swapchain.extent.height, 0.1f, 10.0f));
		pc.data.viewInv = glm::inverse(glm::lookAt(glm::vec3(cameraPosition), glm::vec3(0.f, 0.0f, 1.5f), glm::vec3(0.0f, 0.0f, -1.0f)));
		pc.data = pc.data;

		//printf("\r%.2f", 1 / deltaTime);
		previousTime = currentTime;

		int imageIndex = setup->device.acquireNextImageKHR(presentation->swapchain.handle, UINT64_MAX, { imageReadySemaphore->handle }, {}).value;
		rayTracingSet->updateDescriptor(imageDescriptor, presentation->swapchain.images[imageIndex]);

		commandBuffer->begin();
		presentation->swapchain.images[imageIndex]->clear(commandBuffer);
		presentation->swapchain.images[imageIndex]->renderBarrier(commandBuffer);
		pipeline->run(commandBuffer, presentation->swapchain, { pc });
		presentation->swapchain.images[imageIndex]->presentBarrier(commandBuffer);
		commandBuffer->submit();
		
		std::vector<vk::SwapchainKHR> swapchains = { presentation->swapchain.handle };
		std::vector<uint32_t> imageIndices = { static_cast<uint32_t>(imageIndex) };

		vk::PresentInfoKHR presentInfo{};
		presentInfo.setSwapchains(swapchains);
		presentInfo.setImageIndices(imageIndices);
		presentInfo.setWaitSemaphores(renderFinishedSemaphore->handle);
		setup->graphicsQueue.handle.presentKHR(presentInfo);
	}
	setup->device.waitIdle();
}
