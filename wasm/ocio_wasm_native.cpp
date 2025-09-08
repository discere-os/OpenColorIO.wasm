/**
 * OpenColorIO.wasm - WASM-Native Filesystem Integration
 * IDBFS persistent storage, async resource loading, and CDN integration
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

#include "ocio_wasm_native.h"

#ifdef OCIO_WASM_NATIVE_ENABLED

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>
#include <emscripten/threading.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

using namespace emscripten;

static bool g_native_available = false;
static bool g_idbfs_mounted = false;
static std::unordered_map<std::string, std::string> g_cached_configs;

/**
 * WASM-native virtual directory structure for OpenColorIO
 */
const char* OCIO_VIRTUAL_DIRS[] = {
    "/color-configs",         // OCIO configuration files
    "/color-cache",           // IDBFS persistent color space cache  
    "/color-luts",           // 3D LUT files and collections
    "/color-profiles",       // ICC profiles and workspace settings
    "/color-temp",           // MEMFS temporary processing
    "/color-packages",       // Color space collection packages
    "/color-cdn",           // CDN-loaded resources
    nullptr
};

/**
 * Initialize WASM-native filesystem patterns
 * @return true if initialization successful
 */
bool ocio_wasm_native_init() {
    try {
        // Create virtual directory structure
        for (int i = 0; OCIO_VIRTUAL_DIRS[i] != nullptr; ++i) {
            const char* dir = OCIO_VIRTUAL_DIRS[i];
            
            try {
                FS.mkdir(dir);
                printf("Created virtual directory: %s\n", dir);
            } catch (...) {
                // Directory might already exist - this is okay
            }
        }
        
        // Try to mount IDBFS for persistent storage
        if (ocio_wasm_native_mount_idbfs("/color-cache")) {
            printf("✅ IDBFS persistent storage mounted at /color-cache\n");
            g_idbfs_mounted = true;
        } else {
            printf("⚠️  IDBFS mounting failed, using memory-only cache\n");
            g_idbfs_mounted = false;
        }
        
        // Preload default OCIO configuration if available
        const char* default_config_path = "/color-configs/default.ocio";
        if (FS.analyzePath(default_config_path).exists) {
            try {
                std::vector<char> config_data = FS.readFile(default_config_path);
                std::string config_str(config_data.begin(), config_data.end());
                g_cached_configs["default"] = config_str;
                printf("Loaded default OCIO configuration from bundle\n");
            } catch (...) {
                printf("Warning: Could not load bundled default configuration\n");
            }
        }
        
        g_native_available = true;
        printf("OpenColorIO.wasm: WASM-native features initialized\n");
        return true;
        
    } catch (const std::exception& e) {
        printf("WASM-native initialization error: %s\n", e.what());
        g_native_available = false;
        return false;
    }
}

/**
 * Check if WASM-native features are available
 * @return true if WASM-native filesystem patterns are functional
 */
bool ocio_wasm_native_available() {
    return g_native_available;
}

/**
 * Mount IDBFS for persistent browser storage
 * @param mount_point - Virtual path to mount IDBFS
 * @return true if mounting successful
 */
bool ocio_wasm_native_mount_idbfs(const char* mount_point) {
    if (!mount_point) {
        return false;
    }
    
    try {
        // Mount IDBFS at the specified path
        FS.mkdir(mount_point);
        FS.mount(FS.filesystems.IDBFS, {}, mount_point);
        
        // Synchronize with IndexedDB (populate virtual filesystem)
        EM_ASM({
            const mount_point = UTF8ToString($0);
            FS.syncfs(true, function (err) {
                if (err) {
                    console.warn('IDBFS sync error:', err);
                } else {
                    console.log('IDBFS synchronized for', mount_point);
                }
            });
        }, mount_point);
        
        return true;
        
    } catch (...) {
        return false;
    }
}

/**
 * Load OCIO configuration from URL with caching
 * @param config_url - URL to load configuration from
 * @param config_id - Unique identifier for caching
 * @return true if successful
 */
bool ocio_wasm_native_load_config_from_url(const char* config_url, const char* config_id) {
    if (!g_native_available || !config_url || !config_id) {
        return false;
    }
    
    try {
        std::string cache_path = "/color-cache/" + std::string(config_id) + ".ocio";
        
        // Check persistent cache first
        if (FS.analyzePath(cache_path.c_str()).exists) {
            std::vector<char> cached_data = FS.readFile(cache_path.c_str());
            std::string config_str(cached_data.begin(), cached_data.end());
            g_cached_configs[config_id] = config_str;
            printf("Loaded OCIO config '%s' from cache\n", config_id);
            return true;
        }
        
        // Download asynchronously using Emscripten's fetch API
        printf("Downloading OCIO config from: %s\n", config_url);
        
        EM_ASM({
            const url = UTF8ToString($0);
            const config_id = UTF8ToString($1);
            const cache_path = UTF8ToString($2);
            
            // Use emscripten_async_wget for async download
            const onload = function(response) {
                try {
                    // Save to virtual filesystem
                    FS.writeFile(cache_path, response);
                    
                    // Save to persistent cache if available
                    if (FS.analyzePath('/color-cache').exists) {
                        FS.syncfs(false, function(err) {
                            if (!err) {
                                console.log('Config cached persistently:', config_id);
                            }
                        });
                    }
                    
                    console.log('OCIO config loaded:', config_id);
                    Module.ccall('ocio_wasm_native_config_loaded', null, 
                               ['string', 'number'], [config_id, 1]);
                } catch (e) {
                    console.error('Error saving config:', e);
                    Module.ccall('ocio_wasm_native_config_loaded', null, 
                               ['string', 'number'], [config_id, 0]);
                }
            };
            
            const onerror = function() {
                console.error('Failed to download config:', url);
                Module.ccall('ocio_wasm_native_config_loaded', null, 
                           ['string', 'number'], [config_id, 0]);
            };
            
            Module.ccall('emscripten_async_wget_data', null,
                       ['string', 'number', 'number'],
                       [url, onload, onerror]);
                       
        }, config_url, config_id, cache_path.c_str());
        
        return true;
        
    } catch (const std::exception& e) {
        printf("Error loading config from URL: %s\n", e.what());
        return false;
    }
}

/**
 * Callback for async config loading completion
 * @param config_id - Configuration identifier
 * @param success - 1 if successful, 0 if failed
 */
extern "C" EMSCRIPTEN_KEEPALIVE
void ocio_wasm_native_config_loaded(const char* config_id, int success) {
    if (!config_id) return;
    
    if (success) {
        try {
            std::string cache_path = "/color-cache/" + std::string(config_id) + ".ocio";
            std::vector<char> config_data = FS.readFile(cache_path.c_str());
            std::string config_str(config_data.begin(), config_data.end());
            g_cached_configs[config_id] = config_str;
            printf("✅ OCIO config '%s' loaded and cached\n", config_id);
        } catch (...) {
            printf("❌ Error caching loaded config '%s'\n", config_id);
        }
    } else {
        printf("❌ Failed to load OCIO config '%s'\n", config_id);
    }
}

/**
 * Get cached OCIO configuration data
 * @param config_id - Configuration identifier
 * @param result - Output buffer for configuration data
 * @param max_len - Maximum length of output buffer
 * @return Length of configuration data (0 if not found)
 */
int ocio_wasm_native_get_cached_config(const char* config_id, char* result, int max_len) {
    if (!g_native_available || !config_id || !result || max_len <= 0) {
        return 0;
    }
    
    auto it = g_cached_configs.find(config_id);
    if (it != g_cached_configs.end()) {
        const std::string& config_data = it->second;
        if (config_data.length() < static_cast<size_t>(max_len)) {
            strcpy(result, config_data.c_str());
            return static_cast<int>(config_data.length());
        }
    }
    
    return 0;
}

/**
 * Load color space collection package
 * @param package_url - URL to ZIP package containing color spaces
 * @param package_id - Unique identifier for the package
 * @return true if successful
 */
bool ocio_wasm_native_load_package(const char* package_url, const char* package_id) {
    if (!g_native_available || !package_url || !package_id) {
        return false;
    }
    
    try {
        std::string package_dir = "/color-packages/" + std::string(package_id);
        std::string package_path = package_dir + "/package.zip";
        
        // Create package directory
        FS.mkdir(package_dir.c_str());
        
        printf("Loading color space package: %s\n", package_url);
        
        // Download package asynchronously
        EM_ASM({
            const url = UTF8ToString($0);
            const package_id = UTF8ToString($1);
            const package_path = UTF8ToString($2);
            const package_dir = UTF8ToString($3);
            
            const onload = function(response) {
                try {
                    // Save ZIP package
                    FS.writeFile(package_path, response);
                    
                    // Extract package contents (would require minizip-ng integration)
                    console.log('Color space package downloaded:', package_id);
                    
                    // For now, just notify completion
                    Module.ccall('ocio_wasm_native_package_loaded', null,
                               ['string', 'number'], [package_id, 1]);
                               
                } catch (e) {
                    console.error('Error processing package:', e);
                    Module.ccall('ocio_wasm_native_package_loaded', null,
                               ['string', 'number'], [package_id, 0]);
                }
            };
            
            const onerror = function() {
                console.error('Failed to download package:', url);
                Module.ccall('ocio_wasm_native_package_loaded', null,
                           ['string', 'number'], [package_id, 0]);
            };
            
            Module.ccall('emscripten_async_wget_data', null,
                       ['string', 'number', 'number'],
                       [url, onload, onerror]);
                       
        }, package_url, package_id, package_path.c_str(), package_dir.c_str());
        
        return true;
        
    } catch (const std::exception& e) {
        printf("Error loading package: %s\n", e.what());
        return false;
    }
}

/**
 * Callback for async package loading completion
 * @param package_id - Package identifier
 * @param success - 1 if successful, 0 if failed
 */
extern "C" EMSCRIPTEN_KEEPALIVE
void ocio_wasm_native_package_loaded(const char* package_id, int success) {
    if (!package_id) return;
    
    if (success) {
        printf("✅ Color space package '%s' loaded successfully\n", package_id);
    } else {
        printf("❌ Failed to load color space package '%s'\n", package_id);
    }
}

/**
 * Clear persistent cache
 * @return true if successful
 */
bool ocio_wasm_native_clear_cache() {
    if (!g_native_available) {
        return false;
    }
    
    try {
        // Clear memory cache
        g_cached_configs.clear();
        
        // Clear persistent cache if available
        if (g_idbfs_mounted) {
            // Remove cached files
            std::vector<std::string> cache_files = FS.readdir("/color-cache");
            for (const std::string& file : cache_files) {
                if (file != "." && file != "..") {
                    std::string file_path = "/color-cache/" + file;
                    try {
                        FS.unlink(file_path.c_str());
                    } catch (...) {
                        // File might not exist or be protected
                    }
                }
            }
            
            // Sync changes to IndexedDB
            EM_ASM({
                FS.syncfs(false, function(err) {
                    if (err) {
                        console.warn('Cache clear sync error:', err);
                    } else {
                        console.log('Persistent cache cleared');
                    }
                });
            });
        }
        
        printf("Color cache cleared\n");
        return true;
        
    } catch (const std::exception& e) {
        printf("Error clearing cache: %s\n", e.what());
        return false;
    }
}

/**
 * Get cache statistics
 * @param stats_json - Output buffer for JSON statistics
 * @param max_len - Maximum length of output buffer
 * @return true if successful
 */
bool ocio_wasm_native_get_cache_stats(char* stats_json, int max_len) {
    if (!g_native_available || !stats_json || max_len <= 0) {
        return false;
    }
    
    try {
        std::string stats = "{";
        stats += "\"memory_cached\":" + std::to_string(g_cached_configs.size()) + ",";
        stats += "\"persistent_cache\":" + std::string(g_idbfs_mounted ? "true" : "false") + ",";
        
        // Count cached files
        int cached_files = 0;
        if (FS.analyzePath("/color-cache").exists) {
            std::vector<std::string> cache_files = FS.readdir("/color-cache");
            for (const std::string& file : cache_files) {
                if (file != "." && file != "..") {
                    cached_files++;
                }
            }
        }
        stats += "\"cached_files\":" + std::to_string(cached_files) + ",";
        
        // Estimate cache size
        size_t memory_size = 0;
        for (const auto& pair : g_cached_configs) {
            memory_size += pair.second.length();
        }
        stats += "\"memory_size\":" + std::to_string(memory_size);
        
        stats += "}";
        
        if (stats.length() < static_cast<size_t>(max_len)) {
            strcpy(stats_json, stats.c_str());
            return true;
        }
        
        return false;
        
    } catch (const std::exception& e) {
        printf("Error getting cache stats: %s\n", e.what());
        return false;
    }
}

/**
 * Cleanup WASM-native resources
 */
void ocio_wasm_native_shutdown() {
    if (g_native_available) {
        // Sync persistent cache one final time
        if (g_idbfs_mounted) {
            EM_ASM({
                FS.syncfs(false, function(err) {
                    if (!err) {
                        console.log('Final cache sync completed');
                    }
                });
            });
        }
        
        g_cached_configs.clear();
        g_idbfs_mounted = false;
        g_native_available = false;
        
        printf("WASM-native resources cleaned up\n");
    }
}

// Export JavaScript integration functions
EMSCRIPTEN_BINDINGS(native_integration) {
    function("loadConfigFromUrl", &ocio_wasm_native_load_config_from_url);
    function("loadPackage", &ocio_wasm_native_load_package);
    function("clearCache", &ocio_wasm_native_clear_cache);
    function("getCacheStats", &ocio_wasm_native_get_cache_stats);
}

#else // OCIO_WASM_NATIVE_ENABLED

// WASM-native disabled - provide stub implementations
bool ocio_wasm_native_init() { return false; }
bool ocio_wasm_native_available() { return false; }
bool ocio_wasm_native_mount_idbfs(const char*) { return false; }
bool ocio_wasm_native_load_config_from_url(const char*, const char*) { return false; }
int ocio_wasm_native_get_cached_config(const char*, char*, int) { return 0; }
bool ocio_wasm_native_load_package(const char*, const char*) { return false; }
bool ocio_wasm_native_clear_cache() { return false; }
bool ocio_wasm_native_get_cache_stats(char*, int) { return false; }
void ocio_wasm_native_shutdown() {}

// Callback stubs
extern "C" EMSCRIPTEN_KEEPALIVE
void ocio_wasm_native_config_loaded(const char*, int) {}

extern "C" EMSCRIPTEN_KEEPALIVE  
void ocio_wasm_native_package_loaded(const char*, int) {}

#endif // OCIO_WASM_NATIVE_ENABLED