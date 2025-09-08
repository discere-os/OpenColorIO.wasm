/**
 * OpenColorIO.wasm TypeScript Definitions
 * Complete type coverage for OpenColorIO WebAssembly module
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

declare module 'opencolorio.wasm' {
  /**
   * OpenColorIO.wasm initialization options
   */
  export interface OCIOInitOptions {
    /** Enable WebAssembly SIMD optimizations */
    simdOptimizations?: boolean;
    /** Enable WebGPU compute shader acceleration */
    webgpuAcceleration?: boolean;
    /** Enable WASM-native filesystem features (IDBFS, async loading) */
    wasmNativeFeatures?: boolean;
    /** Maximum size for persistent cache in bytes */
    maxCacheSize?: number;
  }

  /**
   * Color transform processor options
   */
  export interface ProcessorOptions {
    /** Transform direction: "forward" or "inverse" */
    direction?: 'forward' | 'inverse';
    /** Try to use WebGPU acceleration if available */
    useGPU?: boolean;
  }

  /**
   * Performance statistics
   */
  export interface OCIOStats {
    /** OpenColorIO version string */
    version: string;
    /** Whether module is initialized */
    initialized: boolean;
    /** Whether SIMD optimizations are available */
    simd: boolean;
    /** Whether WebGPU compute shaders are available */
    webgpu: boolean;
    /** Whether WASM-native features are available */
    native: boolean;
    /** Number of available color spaces */
    numColorSpaces?: number;
    /** Current memory usage in bytes */
    memoryUsed?: number;
    /** Total memory available in bytes */
    memoryTotal?: number;
  }

  /**
   * Performance benchmark results
   */
  export interface BenchmarkResults {
    /** SIMD operations per second (if available) */
    simdOpsPerSecond?: number;
    /** WebGPU operations per second (if available) */
    webgpuOpsPerSecond?: number;
    /** Number of test pixels used */
    testPixels: number;
    /** Number of iterations performed */
    iterations: number;
    /** Total benchmark duration in milliseconds */
    duration?: number;
  }

  /**
   * Cache statistics for WASM-native features
   */
  export interface CacheStats {
    /** Number of configurations cached in memory */
    memoryCached: number;
    /** Whether persistent cache (IDBFS) is available */
    persistentCache: boolean;
    /** Number of files cached persistently */
    cachedFiles: number;
    /** Total memory cache size in bytes */
    memorySize: number;
  }

  /**
   * Main OpenColorIO.wasm class
   */
  export class OpenColorIOWasm {
    constructor();

    /**
     * Initialize OpenColorIO with optional configuration
     * @param configData - OCIO configuration string (optional)
     * @returns Promise resolving to true if successful
     */
    initialize(configData?: string): boolean;

    /**
     * Get OpenColorIO version
     * @returns Version string
     */
    getVersion(): string;

    /**
     * Check if SIMD optimizations are available
     * @returns true if SIMD is functional
     */
    hasSIMD(): boolean;

    /**
     * Check if WebGPU compute shaders are available
     * @returns true if WebGPU is functional
     */
    hasWebGPU(): boolean;

    /**
     * Check if WASM-native features are available
     * @returns true if native features are functional
     */
    hasNative(): boolean;

    /**
     * Get list of available color spaces
     * @returns Array of color space names
     */
    getColorSpaces(): string[];

    /**
     * Create color space processor for transforming pixels
     * @param srcSpace - Source color space name
     * @param dstSpace - Destination color space name
     * @param direction - Transform direction ("forward" or "inverse")
     * @returns Processor handle (0 if failed)
     */
    createProcessor(
      srcSpace: string,
      dstSpace: string,
      direction?: 'forward' | 'inverse'
    ): number;

    /**
     * Apply color transform to pixel data
     * @param processorId - Processor handle from createProcessor
     * @param pixels - Float32Array of pixel data (RGBA interleaved)
     * @param useGPU - Try to use WebGPU if available
     * @returns true if successful
     */
    applyTransform(
      processorId: number,
      pixels: Float32Array,
      useGPU?: boolean
    ): boolean;

    /**
     * Release processor resources
     * @param processorId - Processor handle to release
     */
    releaseProcessor(processorId: number): void;

    /**
     * Load configuration from URL with caching (WASM-native feature)
     * @param configUrl - URL to load configuration from
     * @param configId - Unique identifier for caching
     * @returns true if load initiated successfully
     */
    loadConfigFromUrl(configUrl: string, configId: string): boolean;

    /**
     * Get cached configuration data (WASM-native feature)
     * @param configId - Configuration identifier
     * @returns Configuration data string (empty if not found)
     */
    getCachedConfig(configId: string): string;

    /**
     * Clear persistent cache (WASM-native feature)
     * @returns true if successful
     */
    clearCache(): boolean;

    /**
     * Get performance and feature statistics
     * @returns Statistics object
     */
    getStats(): OCIOStats;

    /**
     * Run performance benchmark
     * @param numPixels - Number of test pixels
     * @param iterations - Number of iterations
     * @returns Benchmark results
     */
    benchmark(numPixels?: number, iterations?: number): BenchmarkResults;
  }

  /**
   * WebGPU device limits (if WebGPU is available)
   */
  export interface WebGPULimits {
    maxStorageBufferBindingSize: number;
    maxUniformBufferBindingSize: number;
    maxComputeWorkgroupSizeX: number;
    maxComputeWorkgroupSizeY: number;
    maxComputeWorkgroupSizeZ: number;
    maxComputeInvocationsPerWorkgroup: number;
    maxComputeWorkgroupsPerDimension: number;
  }

  /**
   * Module factory function for async initialization
   */
  export interface OpenColorIOModule {
    /**
     * Create OpenColorIO.wasm instance
     * @param options - Initialization options
     * @returns Promise resolving to OpenColorIOWasm instance
     */
    (options?: OCIOInitOptions): Promise<{
      OpenColorIOWasm: typeof OpenColorIOWasm;
    }>;
  }

  /**
   * Default export - module factory
   */
  const OpenColorIOWasmModule: OpenColorIOModule;
  export default OpenColorIOWasmModule;

  /**
   * Utility functions for advanced usage
   */
  export namespace Utils {
    /**
     * Get WebGPU device limits (if available)
     * @returns WebGPU limits object or null
     */
    export function getWebGPULimits(): WebGPULimits | null;

    /**
     * Get cache statistics (WASM-native feature)
     * @returns Cache statistics or null
     */
    export function getCacheStats(): CacheStats | null;

    /**
     * Check browser compatibility for advanced features
     * @returns Compatibility information
     */
    export function checkCompatibility(): {
      simd: boolean;
      webgpu: boolean;
      sharedArrayBuffer: boolean;
      crossOriginIsolated: boolean;
    };
  }
}

/**
 * Global type declarations for browser environment
 */
declare global {
  interface Window {
    OpenColorIOWasmModule?: typeof import('opencolorio.wasm').default;
  }
}

export {};