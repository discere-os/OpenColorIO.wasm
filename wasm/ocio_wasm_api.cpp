/**
 * OpenColorIO.wasm - Core API Implementation
 * Processor management and registry for WASM module
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#include "ocio_wasm_api.h"
#include <unordered_map>
#include <memory>
#include <atomic>

namespace OCIO = OPENCOLORIO_NAMESPACE;

// Global processor registry
static std::unordered_map<int, OCIO::ConstProcessorRcPtr> g_processors;
static std::atomic<int> g_next_processor_id{1};

/**
 * Register a processor and return a handle
 * @param processor - OCIO processor to register
 * @return Processor handle (0 if failed)
 */
int ocio_wasm_api_register_processor(OCIO::ConstProcessorRcPtr processor) {
    if (!processor) {
        return 0;
    }
    
    int id = g_next_processor_id.fetch_add(1);
    g_processors[id] = processor;
    
    return id;
}

/**
 * Get processor by handle
 * @param processor_id - Processor handle
 * @return OCIO processor (nullptr if not found)
 */
OCIO::ConstProcessorRcPtr ocio_wasm_api_get_processor(int processor_id) {
    auto it = g_processors.find(processor_id);
    if (it != g_processors.end()) {
        return it->second;
    }
    return nullptr;
}

/**
 * Release processor by handle
 * @param processor_id - Processor handle to release
 */
void ocio_wasm_api_release_processor(int processor_id) {
    g_processors.erase(processor_id);
}

/**
 * Cleanup all processors
 */
void ocio_wasm_api_cleanup() {
    g_processors.clear();
}