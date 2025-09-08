/**
 * OpenColorIO.wasm - WebGPU Integration Header
 * Hardware-accelerated color transforms using WebGPU compute shaders
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#pragma once

#include <OpenColorIO/OpenColorIO.h>

#ifdef OCIO_WASM_WEBGPU_ENABLED
#include <emscripten/val.h>
#endif

namespace OCIO = OPENCOLORIO_NAMESPACE;

/**
 * Initialize WebGPU support
 * @return true if WebGPU is available and functional
 */
bool ocio_wasm_webgpu_init();

/**
 * Check if WebGPU is available
 * @return true if WebGPU compute shaders are functional
 */
bool ocio_wasm_webgpu_available();

/**
 * Apply color transform using WebGPU compute shader
 * @param processor - OCIO processor (used to extract transform parameters)
 * @param pixel_data - Input/output pixel array (RGBA float)
 * @param num_pixels - Number of pixels to process
 * @return true if successful
 */
bool ocio_wasm_webgpu_apply_transform(OCIO::ConstProcessorRcPtr processor,
                                     float* pixel_data,
                                     int num_pixels);

/**
 * WebGPU performance benchmark
 * @param pixel_data - Test pixel array (RGBA float)
 * @param num_pixels - Number of pixels to benchmark
 * @param iterations - Number of benchmark iterations
 * @return Operations per second (or 0 if WebGPU unavailable)
 */
double ocio_wasm_webgpu_benchmark(float* pixel_data, int num_pixels, int iterations);

/**
 * Check WebGPU device limits and capabilities
 * @param limits_json - Output buffer for JSON limits information
 * @param max_len - Maximum length of output buffer
 * @return true if successful
 */
bool ocio_wasm_webgpu_get_limits(char* limits_json, int max_len);

/**
 * Cleanup WebGPU resources
 */
void ocio_wasm_webgpu_shutdown();

#ifdef OCIO_WASM_WEBGPU_ENABLED

/**
 * Create matrix transform compute pipeline
 * @param matrix - 4x4 transformation matrix
 * @return WebGPU compute pipeline for matrix transforms
 */
emscripten::val ocio_wasm_webgpu_create_matrix_pipeline(const float* matrix);

/**
 * Create 3D LUT transform compute pipeline
 * @param lut_data - 3D LUT data array
 * @param lut_size - Size of each LUT dimension
 * @return WebGPU compute pipeline for LUT transforms
 */
emscripten::val ocio_wasm_webgpu_create_lut3d_pipeline(const float* lut_data, int lut_size);

#endif // OCIO_WASM_WEBGPU_ENABLED