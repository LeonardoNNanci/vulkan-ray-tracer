"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rgen -o raygen.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rmiss -o miss.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 light.rchit -o light.spv
"C:/Users/leoga/Documents/Visual Studio 2022/Libraries/VulkanSDK/Bin/glslc.exe" --target-env=vulkan1.3 raytrace.rchit -o closesthit.spv
