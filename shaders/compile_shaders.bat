%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/raytrace.rgen -o shaders/raygen.spv
%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/raytrace.rmiss -o shaders/miss.spv
%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/raytrace.rchit -o shaders/closesthit.spv
%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/light.rchit -o shaders/light.spv
%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/image_blend.comp -o shaders/image_blend.spv
%VULKAN_SDK%/Bin/glslc.exe --target-env=vulkan1.3 shaders/buffer_to_image.comp -o shaders/buffer_to_image.spv
