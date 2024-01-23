#include "denoiser.hpp"

#pragma once
#include <stdexcept>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include <cuda_runtime_api.h>

uint32_t sizeof_light_pixel = static_cast<uint32_t>(4 * sizeof(float));
uint32_t sizeof_albedo_pixel = static_cast<uint32_t>(3 * sizeof(float));
uint32_t sizeof_normal_pixel = static_cast<uint32_t>(3 * sizeof(float));

DenoiserBuilder::DenoiserBuilder(uint width, uint height) : width(width), height(height) {}

std::shared_ptr<Denoiser> DenoiserBuilder::build()
{
	this->context = this->createContext();
	this->stream = this->createStream();
	this->handle = this->createDenoiser();
	this->setupDenoiser();

	auto denoiser = std::make_shared<Denoiser>(
		this->context,
		this->stream,
		this->handle,
		this->width,
		this->height,
		this->denoiserBuffer,
		this->scratchBuffer,
		this->sizes
	);

	return denoiser;
}

OptixDeviceContext DenoiserBuilder::createContext() {
	cudaFree(0);
	CUcontext cuCtx = 0;

	if (optixInit() != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to initialize OptiX!\n");

	OptixDeviceContext context = nullptr;
	if (optixDeviceContextCreate(cuCtx, 0, &context) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Context!\n");
	return context;
}

CUstream DenoiserBuilder::createStream()
{
	CUstream stream;
	cuStreamCreate(&stream, 0);
	return stream;
}

OptixDenoiser DenoiserBuilder::createDenoiser() {
	auto kind = OptixDenoiserModelKind::OPTIX_DENOISER_MODEL_KIND_LDR;
	OptixDenoiserOptions options{
		.guideAlbedo = true,
		.guideNormal = true,
		.denoiseAlpha = OptixDenoiserAlphaMode::OPTIX_DENOISER_ALPHA_MODE_COPY,
	};
	OptixDenoiser denoiser = nullptr;
	if (optixDenoiserCreate(this->context, kind, &options, &denoiser) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Denoiser!");
	return denoiser;
}

void DenoiserBuilder::setupDenoiser() {
	OptixDenoiser denoiser = nullptr;
	optixDenoiserComputeMemoryResources(denoiser, this->width, this->height, &this->sizes);

	cudaMalloc((void**)&this->denoiserBuffer, this->sizes.stateSizeInBytes);
	cudaMalloc((void**)&this->scratchBuffer, this->sizes.withoutOverlapScratchSizeInBytes);

	// using default stream 0
	optixDenoiserSetup(denoiser, this->stream, this->width, this->height, this->denoiserBuffer, this->sizes.stateSizeInBytes, this->scratchBuffer, this->sizes.withoutOverlapScratchSizeInBytes);
	cudaStreamSynchronize(this->stream);
}

Denoiser::Denoiser(OptixDeviceContext context, CUstream stream, OptixDenoiser handle, uint width, uint heigth, CUdeviceptr denoiserBuffer, CUdeviceptr scratchBuffer, OptixDenoiserSizes sizes) :
	context(context),
	stream(stream),
	handle(handle),
	width(width),
	height(heigth),
	denoiserBuffer(denoiserBuffer),
	scratchBuffer(scratchBuffer),
	sizes(sizes)
{}

CUdeviceptr Denoiser::run(CUdeviceptr lightBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer)
{
	OptixDenoiser denoiser;
	OptixDenoiserParams params{
		.blendFactor = 0.,
	};
	CUdeviceptr denoiserBuffer;
	CUdeviceptr scratchBuffer;
	OptixDenoiserGuideLayer guideLayer{
		.albedo = {
			.data = albedoBuffer,
			.width = this->width,
			.height = this->height,
			.rowStrideInBytes = this->width * sizeof_albedo_pixel,
			.pixelStrideInBytes = sizeof_albedo_pixel,
			.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
		},
		.normal = {
			.data = normalBuffer,
			.width = this->width,
			.height = this->height,
			.rowStrideInBytes = this->width * sizeof_normal_pixel,
			.pixelStrideInBytes = sizeof_normal_pixel,
			.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
		}
	};

	CUdeviceptr resultBuffer;
	OptixDenoiserLayer layers{
		.input = {
			.data = lightBuffer,
			.width = this->width,
			.height = this->height,
			.rowStrideInBytes = sizeof_light_pixel * this->width,
			.pixelStrideInBytes = sizeof_light_pixel,
			.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT4
		},
		.output = {
			.data = resultBuffer,
			.width = this->width,
			.height = this->height,
			.rowStrideInBytes = sizeof_light_pixel * this->width,
			.pixelStrideInBytes = sizeof_light_pixel,
			.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT4
		}
	};
	int num_layers = 3; // rgb, albedo and normal
	optixDenoiserInvoke(denoiser, stream, &params, denoiserBuffer, this->sizes.stateSizeInBytes, &guideLayer, &layers, num_layers, 0, 0, scratchBuffer, this->sizes.withoutOverlapScratchSizeInBytes);
	cudaStreamSynchronize(this->stream);

	return resultBuffer;
}

Denoiser::~Denoiser()
{	
	cudaFree((void*)&this->scratchBuffer);
	cudaFree((void*)&this->denoiserBuffer);
	optixDenoiserDestroy(this->handle);
	optixDeviceContextDestroy(this->context);
}
