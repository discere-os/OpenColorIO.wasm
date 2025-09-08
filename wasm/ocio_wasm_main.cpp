/**
 * OpenColorIO.wasm - Main WASM Module
 * Production-quality color management library for WebAssembly
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#include <emscripten.h>
#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <emscripten/threading.h>

#include <OpenColorIO/OpenColorIO.h>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>

#include "ocio_wasm_api.h"
#include "ocio_wasm_simd.h"
#include "ocio_wasm_webgpu.h"
#include "ocio_wasm_native.h"

namespace OCIO = OPENCOLORIO_NAMESPACE;

/**
 * Global OCIO configuration instance
 * Thread-safe access via Emscripten's main thread model
 */
static std::shared_ptr<OCIO::Config> g_config;
static bool g_initialized = false;

/**
 * Initialize OpenColorIO.wasm module
 * @param configData - Optional OCIO config data (empty = use default)
 * @return true if initialization successful
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_initialize(const char* configData) {
    try {
        if (configData && strlen(configData) > 0) {
            // Load config from provided data
            g_config = OCIO::Config::CreateFromStream(configData);
        } else {
            // Use default config
            g_config = OCIO::Config::CreateFromEnv();
            if (!g_config) {
                g_config = OCIO::Config::CreateRaw();
            }
        }
        
        if (!g_config) {
            return false;
        }
        
#ifdef OCIO_WASM_SIMD_ENABLED
        // Initialize SIMD optimizations
        if (!ocio_wasm_simd_init()) {
            printf("Warning: SIMD initialization failed, using scalar fallback\n");
        }
#endif

#ifdef OCIO_WASM_WEBGPU_ENABLED
        // Initialize WebGPU support
        if (!ocio_wasm_webgpu_init()) {
            printf("Warning: WebGPU initialization failed, using CPU fallback\n");
        }
#endif

#ifdef OCIO_WASM_NATIVE_ENABLED
        // Initialize WASM-native filesystem patterns
        if (!ocio_wasm_native_init()) {
            printf("Warning: WASM-native features initialization failed\n");
        }
#endif
        
        g_initialized = true;
        return true;
        
    } catch (const std::exception& e) {
        printf("OCIO initialization error: %s\n", e.what());
        return false;
    }
}

/**
 * Get OpenColorIO version information
 * @return Version string
 */
extern "C" EMSCRIPTEN_KEEPALIVE
const char* ocio_wasm_get_version() {
    return OCIO::GetVersion();
}

/**
 * Check if SIMD optimizations are available
 * @return true if SIMD is enabled and functional
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_has_simd() {
#ifdef OCIO_WASM_SIMD_ENABLED
    return ocio_wasm_simd_available();
#else
    return false;
#endif
}

/**
 * Check if WebGPU compute support is available
 * @return true if WebGPU is enabled and functional
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_has_webgpu() {
#ifdef OCIO_WASM_WEBGPU_ENABLED
    return ocio_wasm_webgpu_available();
#else
    return false;
#endif
}

/**
 * Get list of available color spaces
 * @param result - Output buffer for color space names (JSON array)
 * @param max_len - Maximum length of result buffer
 * @return Number of color spaces found
 */
extern "C" EMSCRIPTEN_KEEPALIVE
int ocio_wasm_get_color_spaces(char* result, int max_len) {
    if (!g_initialized || !g_config) {
        return 0;
    }
    
    try {
        int count = g_config->getNumColorSpaces();
        std::string json = "[";
        
        for (int i = 0; i < count; ++i) {
            if (i > 0) json += ",";
            json += "\"" + std::string(g_config->getColorSpaceNameByIndex(i)) + "\"";
        }
        
        json += "]";
        
        if (json.length() < static_cast<size_t>(max_len)) {
            strcpy(result, json.c_str());
            return count;
        }
        
        return -1; // Buffer too small
        
    } catch (const std::exception& e) {
        printf("Error getting color spaces: %s\n", e.what());
        return -1;
    }
}

/**
 * Create color space processor for transforming pixels
 * @param src_space - Source color space name
 * @param dst_space - Destination color space name
 * @param direction - Transform direction (forward=0, inverse=1)
 * @return Processor handle (0 if failed)
 */
extern "C" EMSCRIPTEN_KEEPALIVE
int ocio_wasm_create_processor(const char* src_space, const char* dst_space, int direction) {
    if (!g_initialized || !g_config) {
        return 0;
    }
    
    try {
        OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
        transform->setSrc(src_space);
        transform->setDst(dst_space);
        transform->setDirection(direction == 0 ? OCIO::TRANSFORM_DIR_FORWARD : OCIO::TRANSFORM_DIR_INVERSE);
        
        OCIO::ConstProcessorRcPtr processor = g_config->getProcessor(transform);
        if (!processor) {
            return 0;
        }
        
        // Store processor in global registry (simplified approach)
        return ocio_wasm_api_register_processor(processor);
        
    } catch (const std::exception& e) {
        printf("Error creating processor: %s\n", e.what());
        return 0;
    }
}

/**
 * Apply color transform to pixel data
 * @param processor_id - Processor handle from create_processor
 * @param pixel_data - Input/output pixel array (RGBA float)
 * @param num_pixels - Number of pixels to process
 * @return true if successful
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_apply_transform(int processor_id, float* pixel_data, int num_pixels) {
    if (!pixel_data || num_pixels <= 0) {
        return false;
    }
    
    try {
        auto processor = ocio_wasm_api_get_processor(processor_id);
        if (!processor) {
            return false;
        }
        
        // Get CPU processor for pixel operations
        OCIO::ConstCPUProcessorRcPtr cpu_processor = processor->getDefaultCPUProcessor();
        if (!cpu_processor) {
            return false;
        }
        
#ifdef OCIO_WASM_SIMD_ENABLED
        // Use SIMD-optimized processing if available
        if (ocio_wasm_simd_available() && num_pixels >= 4) {
            return ocio_wasm_simd_apply_transform(cpu_processor, pixel_data, num_pixels);
        }
#endif

        // Fallback to standard CPU processing
        OCIO::PackedImageDesc img(pixel_data, num_pixels, 1, 4);
        cpu_processor->apply(img);
        
        return true;
        
    } catch (const std::exception& e) {
        printf("Error applying transform: %s\n", e.what());
        return false;
    }
}

/**
 * Apply color transform using WebGPU compute shader (if available)
 * @param processor_id - Processor handle
 * @param pixel_data - Input/output pixel array (RGBA float)
 * @param num_pixels - Number of pixels to process
 * @return true if successful, false if not available or error
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_apply_transform_gpu(int processor_id, float* pixel_data, int num_pixels) {
#ifdef OCIO_WASM_WEBGPU_ENABLED
    if (!pixel_data || num_pixels <= 0) {
        return false;
    }
    
    try {
        auto processor = ocio_wasm_api_get_processor(processor_id);
        if (!processor) {
            return false;
        }
        
        return ocio_wasm_webgpu_apply_transform(processor, pixel_data, num_pixels);
        
    } catch (const std::exception& e) {
        printf("Error applying GPU transform: %s\n", e.what());
        return false;
    }
#else
    return false; // WebGPU not compiled in
#endif
}

/**
 * Release processor resources
 * @param processor_id - Processor handle to release
 */
extern "C" EMSCRIPTEN_KEEPALIVE
void ocio_wasm_release_processor(int processor_id) {
    ocio_wasm_api_release_processor(processor_id);
}

/**
 * Get performance statistics
 * @param result - Output buffer for JSON stats
 * @param max_len - Maximum length of result buffer
 * @return true if successful
 */
extern "C" EMSCRIPTEN_KEEPALIVE
bool ocio_wasm_get_stats(char* result, int max_len) {
    if (!result || max_len <= 0) {
        return false;
    }
    
    std::string stats = "{";
    stats += "\"version\":\"" + std::string(OCIO::GetVersion()) + "\",";
    stats += "\"initialized\":" + std::string(g_initialized ? "true" : "false") + ",";
    
#ifdef OCIO_WASM_SIMD_ENABLED
    stats += "\"simd\":" + std::string(ocio_wasm_simd_available() ? "true" : "false") + ",";
#else
    stats += "\"simd\":false,";
#endif

#ifdef OCIO_WASM_WEBGPU_ENABLED
    stats += "\"webgpu\":" + std::string(ocio_wasm_webgpu_available() ? "true" : "false") + ",";
#else
    stats += "\"webgpu\":false,";
#endif

#ifdef OCIO_WASM_NATIVE_ENABLED
    stats += "\"native\":true,";
#else
    stats += "\"native\":false,";
#endif

    // Memory usage (Emscripten specific)
    stats += "\"memory\":{";
    stats += "\"used\":" + std::to_string(emscripten_get_heap_size()) + ",";
    stats += "\"total\":" + std::to_string(emscripten_get_heap_size()) + "";
    stats += "}";
    
    stats += "}";
    
    if (stats.length() < static_cast<size_t>(max_len)) {
        strcpy(result, stats.c_str());
        return true;
    }
    
    return false;
}

/**
 * Cleanup and shutdown OCIO.wasm
 */
extern "C" EMSCRIPTEN_KEEPALIVE
void ocio_wasm_shutdown() {
    if (g_initialized) {
#ifdef OCIO_WASM_WEBGPU_ENABLED
        ocio_wasm_webgpu_shutdown();
#endif

#ifdef OCIO_WASM_SIMD_ENABLED
        ocio_wasm_simd_shutdown();
#endif

#ifdef OCIO_WASM_NATIVE_ENABLED
        ocio_wasm_native_shutdown();
#endif
        
        ocio_wasm_api_cleanup();
        g_config.reset();
        g_initialized = false;
    }
}

/**
 * Module initialization - called by Emscripten
 */
extern "C" void EMSCRIPTEN_KEEPALIVE ocio_wasm_module_init() {
    // Set up error handling
    std::set_terminate([]() {
        printf("OpenColorIO.wasm: Unhandled exception, terminating\n");
        abort();
    });
    
    printf("OpenColorIO.wasm v%s initialized\n", OCIO::GetVersion());
    printf("SIMD: %s, WebGPU: %s, Native: %s\n",
#ifdef OCIO_WASM_SIMD_ENABLED
           "enabled",
#else
           "disabled",
#endif
#ifdef OCIO_WASM_WEBGPU_ENABLED
           "enabled",
#else
           "disabled",
#endif
#ifdef OCIO_WASM_NATIVE_ENABLED
           "enabled"
#else
           "disabled"
#endif
    );
}