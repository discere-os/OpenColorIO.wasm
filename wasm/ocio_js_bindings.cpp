/**
 * OpenColorIO.wasm - JavaScript Bindings via Embind
 * High-level JavaScript API for OpenColorIO color management
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <OpenColorIO/OpenColorIO.h>
#include <string>
#include <vector>

#include "ocio_wasm_api.h"
#include "ocio_wasm_simd.h"
#include "ocio_wasm_webgpu.h"
#include "ocio_wasm_native.h"

using namespace emscripten;
namespace OCIO = OPENCOLORIO_NAMESPACE;

/**
 * JavaScript-friendly OpenColorIO wrapper class
 */
class OpenColorIOWasm {
private:
    OCIO::ConstConfigRcPtr config_;
    bool initialized_;
    
public:
    OpenColorIOWasm() : initialized_(false) {}
    
    /**
     * Initialize with optional configuration data
     * @param configData - OCIO configuration string (optional)
     * @return true if successful
     */
    bool initialize(const std::string& configData = "") {
        try {
            if (!configData.empty()) {
                config_ = OCIO::Config::CreateFromStream(configData.c_str());
            } else {
                config_ = OCIO::Config::CreateFromEnv();
                if (!config_) {
                    config_ = OCIO::Config::CreateRaw();
                }
            }
            
            initialized_ = (config_ != nullptr);
            return initialized_;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    /**
     * Get OpenColorIO version
     * @return Version string
     */
    std::string getVersion() const {
        return OCIO::GetVersion();
    }
    
    /**
     * Check if SIMD optimizations are available
     * @return true if SIMD is functional
     */
    bool hasSIMD() const {
        return ocio_wasm_simd_available();
    }
    
    /**
     * Check if WebGPU compute shaders are available
     * @return true if WebGPU is functional
     */
    bool hasWebGPU() const {
        return ocio_wasm_webgpu_available();
    }
    
    /**
     * Check if WASM-native features are available
     * @return true if native features are functional
     */
    bool hasNative() const {
        return ocio_wasm_native_available();
    }
    
    /**
     * Get list of available color spaces
     * @return Array of color space names
     */
    std::vector<std::string> getColorSpaces() const {
        std::vector<std::string> spaces;
        
        if (!initialized_ || !config_) {
            return spaces;
        }
        
        try {
            int count = config_->getNumColorSpaces();
            spaces.reserve(count);
            
            for (int i = 0; i < count; ++i) {
                spaces.emplace_back(config_->getColorSpaceNameByIndex(i));
            }
            
        } catch (const std::exception& e) {
            // Return empty vector on error
        }
        
        return spaces;
    }
    
    /**
     * Create color space processor
     * @param srcSpace - Source color space name
     * @param dstSpace - Destination color space name
     * @param direction - Transform direction ("forward" or "inverse")
     * @return Processor handle (0 if failed)
     */
    int createProcessor(const std::string& srcSpace, 
                       const std::string& dstSpace,
                       const std::string& direction = "forward") {
        if (!initialized_ || !config_) {
            return 0;
        }
        
        try {
            OCIO::ColorSpaceTransformRcPtr transform = OCIO::ColorSpaceTransform::Create();
            transform->setSrc(srcSpace.c_str());
            transform->setDst(dstSpace.c_str());
            
            OCIO::TransformDirection dir = (direction == "inverse") ? 
                OCIO::TRANSFORM_DIR_INVERSE : OCIO::TRANSFORM_DIR_FORWARD;
            transform->setDirection(dir);
            
            OCIO::ConstProcessorRcPtr processor = config_->getProcessor(transform);
            if (!processor) {
                return 0;
            }
            
            return ocio_wasm_api_register_processor(processor);
            
        } catch (const std::exception& e) {
            return 0;
        }
    }
    
    /**
     * Apply color transform to pixel data
     * @param processorId - Processor handle from createProcessor
     * @param pixels - Float32Array of pixel data (RGBA)
     * @param useGPU - Try to use WebGPU if available
     * @return true if successful
     */
    bool applyTransform(int processorId, val pixels, bool useGPU = false) {
        if (!initialized_ || processorId <= 0) {
            return false;
        }
        
        try {
            // Convert JavaScript Float32Array to C++ float array
            std::vector<float> pixelData = vecFromJSArray<float>(pixels);
            if (pixelData.empty() || pixelData.size() % 4 != 0) {
                return false;
            }
            
            int numPixels = pixelData.size() / 4;
            float* data = pixelData.data();
            
            bool success = false;
            
            // Try WebGPU first if requested and available
            if (useGPU && ocio_wasm_webgpu_available()) {
                success = ocio_wasm_webgpu_apply_transform(
                    ocio_wasm_api_get_processor(processorId), data, numPixels);
            }
            
            // Fall back to SIMD or CPU processing
            if (!success) {
                if (ocio_wasm_simd_available()) {
                    success = ocio_wasm_simd_apply_transform(
                        ocio_wasm_api_get_processor(processorId), data, numPixels);
                } else {
                    // Standard CPU processing
                    auto processor = ocio_wasm_api_get_processor(processorId);
                    if (processor) {
                        OCIO::ConstCPUProcessorRcPtr cpuProcessor = 
                            processor->getDefaultCPUProcessor();
                        if (cpuProcessor) {
                            OCIO::PackedImageDesc img(data, numPixels, 1, 4);
                            cpuProcessor->apply(img);
                            success = true;
                        }
                    }
                }
            }
            
            if (success) {
                // Copy processed data back to JavaScript array
                for (size_t i = 0; i < pixelData.size(); ++i) {
                    pixels.set(i, pixelData[i]);
                }
            }
            
            return success;
            
        } catch (const std::exception& e) {
            return false;
        }
    }
    
    /**
     * Release processor resources
     * @param processorId - Processor handle to release
     */
    void releaseProcessor(int processorId) {
        ocio_wasm_api_release_processor(processorId);
    }
    
    /**
     * Load configuration from URL (WASM-native feature)
     * @param configUrl - URL to load configuration from
     * @param configId - Unique identifier for caching
     * @return true if load initiated successfully
     */
    bool loadConfigFromUrl(const std::string& configUrl, const std::string& configId) {
        return ocio_wasm_native_load_config_from_url(configUrl.c_str(), configId.c_str());
    }
    
    /**
     * Get cached configuration data (WASM-native feature)
     * @param configId - Configuration identifier
     * @return Configuration data string (empty if not found)
     */
    std::string getCachedConfig(const std::string& configId) {
        char buffer[65536]; // 64KB buffer
        int len = ocio_wasm_native_get_cached_config(configId.c_str(), buffer, sizeof(buffer));
        
        if (len > 0) {
            return std::string(buffer, len);
        }
        
        return "";
    }
    
    /**
     * Clear persistent cache (WASM-native feature)
     * @return true if successful
     */
    bool clearCache() {
        return ocio_wasm_native_clear_cache();
    }
    
    /**
     * Get performance and feature statistics
     * @return JavaScript object with statistics
     */
    val getStats() {
        val stats = val::object();
        
        stats.set("version", getVersion());
        stats.set("initialized", initialized_);
        stats.set("simd", hasSIMD());
        stats.set("webgpu", hasWebGPU());
        stats.set("native", hasNative());
        
        if (initialized_ && config_) {
            stats.set("numColorSpaces", config_->getNumColorSpaces());
        }
        
        return stats;
    }
    
    /**
     * Run performance benchmark
     * @param numPixels - Number of test pixels
     * @param iterations - Number of iterations
     * @return JavaScript object with benchmark results
     */
    val benchmark(int numPixels = 10000, int iterations = 100) {
        val results = val::object();
        
        // Create test data
        std::vector<float> testPixels(numPixels * 4, 0.5f); // RGBA with 0.5 values
        
        // SIMD benchmark
        if (ocio_wasm_simd_available()) {
            double simdOps = ocio_wasm_simd_benchmark(testPixels.data(), numPixels, iterations);
            results.set("simdOpsPerSecond", simdOps);
        }
        
        // WebGPU benchmark
        if (ocio_wasm_webgpu_available()) {
            double gpuOps = ocio_wasm_webgpu_benchmark(testPixels.data(), numPixels, iterations);
            results.set("webgpuOpsPerSecond", gpuOps);
        }
        
        results.set("testPixels", numPixels);
        results.set("iterations", iterations);
        
        return results;
    }
};

// Emscripten bindings for JavaScript integration
EMSCRIPTEN_BINDINGS(opencolorio_wasm) {
    // Main OpenColorIO class
    class_<OpenColorIOWasm>("OpenColorIOWasm")
        .constructor<>()
        .function("initialize", &OpenColorIOWasm::initialize)
        .function("getVersion", &OpenColorIOWasm::getVersion)
        .function("hasSIMD", &OpenColorIOWasm::hasSIMD)
        .function("hasWebGPU", &OpenColorIOWasm::hasWebGPU)
        .function("hasNative", &OpenColorIOWasm::hasNative)
        .function("getColorSpaces", &OpenColorIOWasm::getColorSpaces)
        .function("createProcessor", &OpenColorIOWasm::createProcessor)
        .function("applyTransform", &OpenColorIOWasm::applyTransform)
        .function("releaseProcessor", &OpenColorIOWasm::releaseProcessor)
        .function("loadConfigFromUrl", &OpenColorIOWasm::loadConfigFromUrl)
        .function("getCachedConfig", &OpenColorIOWasm::getCachedConfig)
        .function("clearCache", &OpenColorIOWasm::clearCache)
        .function("getStats", &OpenColorIOWasm::getStats)
        .function("benchmark", &OpenColorIOWasm::benchmark);
    
    // Vector bindings for JavaScript arrays
    register_vector<std::string>("VectorString");
    register_vector<float>("VectorFloat");
}