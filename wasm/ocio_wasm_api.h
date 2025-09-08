/**
 * OpenColorIO.wasm - Core API Header
 * Processor management and registry for WASM module
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#pragma once

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO = OPENCOLORIO_NAMESPACE;

/**
 * Register a processor and return a handle
 * @param processor - OCIO processor to register
 * @return Processor handle (0 if failed)
 */
int ocio_wasm_api_register_processor(OCIO::ConstProcessorRcPtr processor);

/**
 * Get processor by handle
 * @param processor_id - Processor handle
 * @return OCIO processor (nullptr if not found)
 */
OCIO::ConstProcessorRcPtr ocio_wasm_api_get_processor(int processor_id);

/**
 * Release processor by handle
 * @param processor_id - Processor handle to release
 */
void ocio_wasm_api_release_processor(int processor_id);

/**
 * Cleanup all processors
 */
void ocio_wasm_api_cleanup();