/**
 * OpenColorIO.wasm - SIMD Optimizations Header
 * WebAssembly SIMD128 optimizations for color processing
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#pragma once

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OPENCOLORIO_NAMESPACE;

/**
 * Initialize WASM SIMD support
 * @return true if SIMD is available and functional
 */
bool ocio_wasm_simd_init();

/**
 * Check if SIMD is available and functional
 * @return true if SIMD optimizations are functional
 */
bool ocio_wasm_simd_available();

/**
 * SIMD-optimized bulk color transform application
 * @param processor - OCIO CPU processor
 * @param pixel_data - Input/output pixel array (RGBA float)
 * @param num_pixels - Number of pixels to process
 * @return true if successful
 */
bool ocio_wasm_simd_apply_transform(OCIO::ConstCPUProcessorRcPtr processor, 
                                   float* pixel_data, 
                                   int num_pixels);

/**
 * SIMD performance benchmark for color transforms
 * @param pixel_data - Test pixel array (RGBA float)
 * @param num_pixels - Number of pixels to benchmark
 * @param iterations - Number of benchmark iterations
 * @return Operations per second
 */
double ocio_wasm_simd_benchmark(float* pixel_data, int num_pixels, int iterations);

/**
 * Cleanup SIMD resources
 */
void ocio_wasm_simd_shutdown();