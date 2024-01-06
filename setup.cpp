#define VULKAN_HPP_NO_CONSTRUCTORS

#include "setup.hpp"

IHasSetup::IHasSetup(std::shared_ptr<Setup> setup) : setup(setup) {};

SetupBuilder SetupBuilder::addExtensions(RequiredExtensions extensions)
{
	for (auto& extension : extensions.instanceExtensions)
		this->extensions.instanceExtensions.push_back(extension);

	for (auto& extension : extensions.deviceExtensions)
		this->extensions.deviceExtensions.push_back(extension);

	return *this;
}

std::shared_ptr<Setup> SetupBuilder::build()
{
    this->setup = std::make_shared<Setup>();
    this->setup->instance = this->createInstance();
    this->setup->physicalDevice = this->getPhysicalDevice();
    this->setup->graphicsQueue.familyIndex = this->getQueueFamilyIndex(vk::QueueFlagBits::eGraphics);
    this->setup->device = this->createDevice();
    this->setup->graphicsQueue.handle = this->getQueue(this->setup->graphicsQueue.familyIndex);
    return this->setup;
}

vk::Instance SetupBuilder::createInstance()
{
    vk::ApplicationInfo appInfo{
        .pApplicationName = "Ray Tracing Renderer",
        .applicationVersion = VK_MAKE_VERSION(0,1,0),
        .pEngineName = "No engine",
        .engineVersion = VK_MAKE_VERSION(0,1,0),
        .apiVersion = VK_API_VERSION_1_3
    };
    vk::InstanceCreateInfo instanceInfo{
        .pApplicationInfo = &appInfo,
    };
    instanceInfo.setPEnabledExtensionNames(this->extensions.instanceExtensions);

    return vk::createInstance(instanceInfo);
}

vk::PhysicalDevice SetupBuilder::getPhysicalDevice()
{
    std::vector<vk::PhysicalDevice> physicalDevices = this->setup->instance.enumeratePhysicalDevices();
    return physicalDevices[0];
}

vk::Device SetupBuilder::createDevice()
{
    std::vector<vk::ExtensionProperties> availableExtensions = this->setup->physicalDevice.enumerateDeviceExtensionProperties();

    vk::PhysicalDeviceProperties deviceProperties = this->setup->physicalDevice.getProperties();
    vk::PhysicalDeviceFeatures deviceFeatures = this->setup->physicalDevice.getFeatures();

    size_t count = 0;
    for (auto& required : this->extensions.deviceExtensions)
        for (auto& available : availableExtensions)
            if (strcmp(required, available.extensionName) == 0) {
                count++;
                break;
            }

    if (count < this->extensions.deviceExtensions.size())
        throw std::runtime_error("One or more device extensions are not available!");

    float priorities[] = { 0.0f };
    vk::DeviceQueueCreateInfo queueInfo{
        .queueFamilyIndex = this->setup->graphicsQueue.familyIndex,
        .queueCount = 1,
        .pQueuePriorities = priorities
    };

    vk::PhysicalDeviceRayTracingPipelineFeaturesKHR rtPipelineFeatures{
        .rayTracingPipeline = vk::True
    };
    vk::PhysicalDeviceBufferDeviceAddressFeatures bufferAddressFeatures{
        .pNext = &rtPipelineFeatures,
        .bufferDeviceAddress = vk::True
    };
    vk::PhysicalDeviceAccelerationStructureFeaturesKHR accelerationFeatures{
        .pNext = &bufferAddressFeatures,
        .accelerationStructure = vk::True
    };

    vk::DeviceCreateInfo deviceInfo{
    .pNext = &accelerationFeatures,
    .queueCreateInfoCount = 1,
    .pQueueCreateInfos = &queueInfo,
    };
    deviceInfo.setPEnabledExtensionNames(this->extensions.deviceExtensions);

    return this->setup->physicalDevice.createDevice(deviceInfo);
	return vk::Device();
}

uint32_t SetupBuilder::getQueueFamilyIndex(vk::QueueFlags queueFamily)
{
    auto queueFamilyProperties = this->setup->physicalDevice.getQueueFamilyProperties();

    for (int i = 0; i < queueFamilyProperties.size(); i++)
        if (queueFamilyProperties[i].queueFlags & queueFamily) {
            return i;
        }
}

vk::Queue SetupBuilder::getQueue(uint32_t familyIndex)
{
    return this->setup->device.getQueue(familyIndex, 0);
}

Setup::~Setup() {
    this->device.destroy();
    this->instance.destroy();
}
