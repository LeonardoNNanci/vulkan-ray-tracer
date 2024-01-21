#pragma once
#include <stdexcept>
#include <optix.h>
#include <cuda_runtime_api.h>

OptixDeviceContext createContext() {
	OptixDeviceContext context = nullptr;
	cudaFree(0);
	CUcontext cuCtx = 0;
	if(optixDeviceContextCreate(cuCtx, 0, &context) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Context!\n");
	return context;
}

OptixDenoiser createDenoiser() {
	auto context = nullptr; // to be created
	auto kind = OptixDenoiserModelKind::OPTIX_DENOISER_MODEL_KIND_LDR;
	OptixDenoiserOptions options{
		.guideAlbedo = true,
		.guideNormal = true,
		.denoiseAlpha = OptixDenoiserAlphaMode::OPTIX_DENOISER_ALPHA_MODE_COPY,
	};
	OptixDenoiser denoiser = nullptr;
	if (optixDenoiserCreate(context, kind, &options, &denoiser) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Denoiser!");
	return denoiser;
}

void setupDenoiser() {
	OptixDenoiser denoiser = nullptr;
	OptixDenoiserSizes sizes;
	optixDenoiserComputeMemoryResources(denoiser, 800, 600, &sizes);

	CUdeviceptr denoiserBuffer;
	CUdeviceptr scratchBuffer;
	cudaMalloc((void**)&denoiserBuffer, sizes.stateSizeInBytes);
	cudaMalloc((void**)&scratchBuffer, sizes.withoutOverlapScratchSizeInBytes);

	// using default stream 0
	optixDenoiserSetup(denoiser, 0, 800, 600, denoiserBuffer, sizes.stateSizeInBytes, scratchBuffer, sizes.withoutOverlapScratchSizeInBytes);
}

void denoise() {
	OptixDenoiser denoiser;
	int stream = 0; // default stream
	OptixDenoiserParams params{
		.blendFactor = 0.,
	};
	CUdeviceptr denoiserBuffer;
	CUdeviceptr scratchBuffer;
	OptixDenoiserGuideLayer guideLayer{
		.albedo = {
			.data = nullptr,
			.width = 0,
			.height = 0,
			.rowStrideInBytes = sizeof_pixel * m_imageSize.width,
			.pixelStrideInBytes = sizeof_pixel,
			.format = pixel_format
		},
		.normal = {
			.data = nullptr,
			.width = 0,
			.height = 0,
			.rowStrideInBytes = sizeof_pixel * m_imageSize.width,
			.pixelStrideInBytes = sizeof_pixel,
			.format = pixel_format
		},
		.outputInternalGuideLayer = result
	};

	// define 1 layer per input
	OptixDenoiserLayer layers{
		.input = {
			.data = nullptr,
			.width = 0,
			.height = 0,
			.rowStrideInBytes = sizeof_pixel * m_imageSize.width,
			.pixelStrideInBytes = sizeof_pixel,
			.format = pixel_format
		},
		.output = {
			.data = nullptr,
			.width = 0,
			.height = 0,
			.rowStrideInBytes = sizeof_pixel * m_imageSize.width,
			.pixelStrideInBytes = sizeof_pixel,
			.format = pixel_format
		}
	};
	int num_layers = 3; // rgb, albedo and normal
	optixDenoiserInvoke(denoiser, stream, params, denoiserBuffer, stateSize, &guideLayer, &layers, num_layers, 0, 0, scratchBuffer, scratchSize)
}

void cleanup() {
	// free buffers
	// ...

	optixDenoiserDestroy(denoiser);
	optixDeviceContextDestroy(context);
}