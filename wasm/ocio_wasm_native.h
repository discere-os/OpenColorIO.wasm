/**
 * OpenColorIO.wasm - WASM-Native Features Header
 * IDBFS persistent storage, async resource loading, and CDN integration
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#pragma once

/**
 * Initialize WASM-native filesystem patterns
 * @return true if initialization successful
 */
bool ocio_wasm_native_init();

/**
 * Check if WASM-native features are available
 * @return true if WASM-native filesystem patterns are functional
 */
bool ocio_wasm_native_available();

/**
 * Mount IDBFS for persistent browser storage
 * @param mount_point - Virtual path to mount IDBFS
 * @return true if mounting successful
 */
bool ocio_wasm_native_mount_idbfs(const char* mount_point);

/**
 * Load OCIO configuration from URL with caching
 * @param config_url - URL to load configuration from
 * @param config_id - Unique identifier for caching
 * @return true if successful
 */
bool ocio_wasm_native_load_config_from_url(const char* config_url, const char* config_id);

/**
 * Get cached OCIO configuration data
 * @param config_id - Configuration identifier
 * @param result - Output buffer for configuration data
 * @param max_len - Maximum length of output buffer
 * @return Length of configuration data (0 if not found)
 */
int ocio_wasm_native_get_cached_config(const char* config_id, char* result, int max_len);

/**
 * Load color space collection package
 * @param package_url - URL to ZIP package containing color spaces
 * @param package_id - Unique identifier for the package
 * @return true if successful
 */
bool ocio_wasm_native_load_package(const char* package_url, const char* package_id);

/**
 * Clear persistent cache
 * @return true if successful
 */
bool ocio_wasm_native_clear_cache();

/**
 * Get cache statistics
 * @param stats_json - Output buffer for JSON statistics
 * @param max_len - Maximum length of output buffer
 * @return true if successful
 */
bool ocio_wasm_native_get_cache_stats(char* stats_json, int max_len);

/**
 * Cleanup WASM-native resources
 */
void ocio_wasm_native_shutdown();