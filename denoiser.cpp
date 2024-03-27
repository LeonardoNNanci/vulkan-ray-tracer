#include "denoiser.hpp"
#include <iostream>

#pragma once
#include <stdexcept>
#include <optix.h>
#include <optix_function_table_definition.h>
#include <optix_stubs.h>
#include <cuda_runtime_api.h>

uint32_t sizeof_light_pixel = static_cast<uint32_t>(3 * sizeof(float));
uint32_t sizeof_albedo_pixel = static_cast<uint32_t>(3 * sizeof(float));
uint32_t sizeof_normal_pixel = static_cast<uint32_t>(3 * sizeof(float));

void optixLogCallback(unsigned int level, const char* tag, const char* message, void* cbdata)
{
	printf("OptiX [%s]: %s\n", tag, message);
}

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

	OptixDeviceContextOptions options = {
		.logCallbackFunction = &optixLogCallback,
		.logCallbackLevel = 4
	};

	OptixDeviceContext context = nullptr;
	if (optixDeviceContextCreate(cuCtx, &options, &context) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Context!\n");
	return context;
}

CUstream DenoiserBuilder::createStream()
{
	CUstream stream;
	if (cuStreamCreate(&stream, CU_STREAM_DEFAULT) != CUDA_SUCCESS) {
		throw std::runtime_error("Failed to create stream!\n");
	}
	return stream;
}

OptixDenoiser DenoiserBuilder::createDenoiser() {
	auto kind = OPTIX_DENOISER_MODEL_KIND_LDR;
	
	OptixDenoiserOptions options{
		.guideAlbedo = true,
		.guideNormal = true,
		.denoiseAlpha = OptixDenoiserAlphaMode::OPTIX_DENOISER_ALPHA_MODE_DENOISE
	};
	OptixDenoiser denoiser = nullptr;
	
	if (optixDenoiserCreate(this->context, kind, &options, &denoiser) != OPTIX_SUCCESS)
		throw std::runtime_error("Failed to create OptiX Denoiser!");
	return denoiser;
}

void DenoiserBuilder::setupDenoiser() {

	optixDenoiserComputeMemoryResources(this->handle, (uint)this->width, (uint)this->height, &this->sizes);

	cudaMalloc(reinterpret_cast<void**>(&this->denoiserBuffer), this->sizes.stateSizeInBytes);
	cudaMalloc(reinterpret_cast<void**>(&this->scratchBuffer), this->sizes.withoutOverlapScratchSizeInBytes);

	cudaDeviceSynchronize();
	// using default stream 0
	optixDenoiserSetup(this->handle, 0, this->width, this->height, this->denoiserBuffer, this->sizes.stateSizeInBytes, this->scratchBuffer, this->sizes.withoutOverlapScratchSizeInBytes);
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

void Denoiser::run(CUdeviceptr inputBuffer, CUdeviceptr albedoBuffer, CUdeviceptr normalBuffer, CUdeviceptr outputBuffer)
{
	//cudaMemcpy((void*)outputBuffer, (void*)normalBuffer, width * height * 3 * sizeof(float), cudaMemcpyDeviceToDevice);
	//cudaDeviceSynchronize();
	try {
		OptixDenoiserParams params = {
			.blendFactor = 0.5
		};
	
		OptixDenoiserGuideLayer guideLayer{
			.albedo = {
				.data = albedoBuffer,
				.width = this->width,
				.height = this->height,
				.rowStrideInBytes = this->width * sizeof_light_pixel,
				.pixelStrideInBytes = sizeof_light_pixel,
				.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
			},
			.normal = {
				.data = normalBuffer,
				.width = this->width,
				.height = this->height,
				.rowStrideInBytes = this->width * sizeof_light_pixel,
				.pixelStrideInBytes = sizeof_light_pixel,
				.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
			},
		};
	
		OptixDenoiserLayer layers{
			.input = {
				.data = inputBuffer,
				.width = this->width,
				.height = this->height,
				.rowStrideInBytes = sizeof_light_pixel * this->width,
				.pixelStrideInBytes = sizeof_light_pixel,
				.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
			},
			.output = {
				.data = outputBuffer,
				.width = this->width,
				.height = this->height,
				.rowStrideInBytes = sizeof_light_pixel * this->width,
				.pixelStrideInBytes = sizeof_light_pixel,
				.format = OptixPixelFormat::OPTIX_PIXEL_FORMAT_FLOAT3
			},
		};
		int num_layers = 1;
		optixDenoiserInvoke(this->handle, 0, &params, this->denoiserBuffer, this->sizes.stateSizeInBytes, &guideLayer, &layers, num_layers, 0, 0, this->scratchBuffer, this->sizes.withoutOverlapScratchSizeInBytes);
		cudaDeviceSynchronize();
	}
	catch (const std::exception& e)
	{
		std::cout << e.what() << std::endl;
	}
}

Denoiser::~Denoiser()
{	
	cudaFree((void*)this->scratchBuffer);
	cudaFree((void*)this->denoiserBuffer);
	optixDenoiserDestroy(this->handle);
	optixDeviceContextDestroy(this->context);
}

