"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rgen -o raygen.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rmiss -o miss.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rchit -o closesthit.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 light.rchit -o light.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 denoiser.rchit -o denoiser.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 image_to_buffer.comp -o image_to_buffer.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 buffer_to_image.comp -o buffer_to_image.spv
