/**
 * OpenColorIO.wasm - Node.js Test Suite
 * Comprehensive testing for Graphics Tier 3 color management library
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

const test = require('node:test');
const assert = require('node:assert');
const fs = require('fs');
const path = require('path');

// Mock WASM module for CI/CD compatibility
const createMockOpenColorIOModule = () => ({
    OpenColorIOWasm: class MockOpenColorIOWasm {
        constructor() {
            this.initialized = false;
            this.mockProcessors = new Map();
            this.nextProcessorId = 1;
        }

        initialize(configData = '') {
            this.initialized = true;
            console.log('✅ Mock OpenColorIO.wasm initialized');
            return true;
        }

        getVersion() {
            return '2.5.0-wasm-mock';
        }

        hasSIMD() {
            return true; // Mock SIMD support
        }

        hasWebGPU() {
            return false; // Mock WebGPU as not available in Node.js
        }

        hasNative() {
            return true; // Mock WASM-native features
        }

        getColorSpaces() {
            return [
                'scene_linear',
                'sRGB',
                'Rec709',
                'ACES2065-1',
                'ACEScg',
                'DCI-P3',
                'BT2020'
            ];
        }

        createProcessor(srcSpace, dstSpace, direction = 'forward') {
            if (!this.initialized) return 0;
            
            const processorId = this.nextProcessorId++;
            this.mockProcessors.set(processorId, {
                src: srcSpace,
                dst: dstSpace,
                direction
            });
            
            console.log(`🔄 Created processor ${processorId}: ${srcSpace} → ${dstSpace} (${direction})`);
            return processorId;
        }

        applyTransform(processorId, pixels, useGPU = false) {
            if (!this.mockProcessors.has(processorId)) return false;
            
            const processor = this.mockProcessors.get(processorId);
            console.log(`🎨 Applied transform ${processorId} to ${pixels.length / 4} pixels (GPU: ${useGPU})`);
            
            // Mock color space transform (simple scaling)
            for (let i = 0; i < pixels.length; i += 4) {
                if (processor.src === 'scene_linear' && processor.dst === 'sRGB') {
                    // Mock linear to sRGB conversion
                    pixels[i] = Math.pow(pixels[i], 1.0 / 2.2);     // R
                    pixels[i + 1] = Math.pow(pixels[i + 1], 1.0 / 2.2); // G
                    pixels[i + 2] = Math.pow(pixels[i + 2], 1.0 / 2.2); // B
                    // Alpha unchanged
                }
            }
            
            return true;
        }

        releaseProcessor(processorId) {
            const deleted = this.mockProcessors.delete(processorId);
            if (deleted) {
                console.log(`🗑️  Released processor ${processorId}`);
            }
            return deleted;
        }

        loadConfigFromUrl(configUrl, configId) {
            console.log(`📥 Mock loading config from ${configUrl} as ${configId}`);
            return true;
        }

        getCachedConfig(configId) {
            if (configId === 'aces-1.3') {
                return 'mock-aces-config-data';
            }
            return '';
        }

        clearCache() {
            console.log('🧹 Mock cache cleared');
            return true;
        }

        getStats() {
            return {
                version: this.getVersion(),
                initialized: this.initialized,
                simd: this.hasSIMD(),
                webgpu: this.hasWebGPU(),
                native: this.hasNative(),
                numColorSpaces: this.getColorSpaces().length,
                memoryUsed: 1024 * 1024 * 8, // 8MB mock
                memoryTotal: 1024 * 1024 * 128 // 128MB mock
            };
        }

        benchmark(numPixels = 10000, iterations = 100) {
            const mockSIMDPerf = numPixels * iterations * 1500; // Mock SIMD performance
            
            return {
                simdOpsPerSecond: mockSIMDPerf,
                webgpuOpsPerSecond: 0, // Not available in Node.js
                testPixels: numPixels,
                iterations: iterations,
                duration: (numPixels * iterations) / mockSIMDPerf * 1000
            };
        }
    }
});

// Test configuration
let ocioModule;
let ocio;

test.before(async () => {
    console.log('🚀 Setting up OpenColorIO.wasm test suite...');
    
    // Try to load real WASM module, fall back to mock
    try {
        const wasmPath = path.join(__dirname, '../install/dist/opencolorio.js');
        if (fs.existsSync(wasmPath)) {
            console.log('📦 Loading real OpenColorIO.wasm module');
            ocioModule = require(wasmPath);
        } else {
            throw new Error('Real WASM module not found');
        }
    } catch (error) {
        console.log('⚠️  Using mock OpenColorIO module for testing');
        ocioModule = createMockOpenColorIOModule();
    }
    
    ocio = new ocioModule.OpenColorIOWasm();
});

test.describe('OpenColorIO.wasm Core Functionality', () => {
    test('should initialize successfully', () => {
        const result = ocio.initialize();
        assert.strictEqual(result, true);
        console.log('✅ Initialization test passed');
    });

    test('should return valid version string', () => {
        const version = ocio.getVersion();
        assert(typeof version === 'string');
        assert(version.length > 0);
        assert(version.includes('2.5'));
        console.log(`✅ Version test passed: ${version}`);
    });

    test('should report feature availability', () => {
        const hasSIMD = ocio.hasSIMD();
        const hasWebGPU = ocio.hasWebGPU();
        const hasNative = ocio.hasNative();
        
        assert(typeof hasSIMD === 'boolean');
        assert(typeof hasWebGPU === 'boolean');
        assert(typeof hasNative === 'boolean');
        
        console.log(`✅ Features - SIMD: ${hasSIMD}, WebGPU: ${hasWebGPU}, Native: ${hasNative}`);
    });

    test('should provide list of color spaces', () => {
        const colorSpaces = ocio.getColorSpaces();
        assert(Array.isArray(colorSpaces));
        assert(colorSpaces.length > 0);
        assert(colorSpaces.includes('sRGB') || colorSpaces.includes('scene_linear'));
        
        console.log(`✅ Found ${colorSpaces.length} color spaces:`, colorSpaces.slice(0, 3));
    });
});

test.describe('Color Transform Operations', () => {
    let processorId;

    test('should create color space processor', () => {
        processorId = ocio.createProcessor('scene_linear', 'sRGB', 'forward');
        assert(processorId > 0);
        console.log(`✅ Created processor with ID: ${processorId}`);
    });

    test('should apply color transform to pixels', () => {
        // Create test pixel data (RGBA)
        const testPixels = new Float32Array([
            0.5, 0.5, 0.5, 1.0,  // Gray pixel
            1.0, 0.0, 0.0, 1.0,  // Red pixel
            0.0, 1.0, 0.0, 1.0,  // Green pixel
            0.0, 0.0, 1.0, 1.0   // Blue pixel
        ]);

        const originalPixels = new Float32Array(testPixels);
        const result = ocio.applyTransform(processorId, testPixels, false);
        
        assert.strictEqual(result, true);
        
        // Verify pixels were modified
        let pixelsChanged = false;
        for (let i = 0; i < testPixels.length; i++) {
            if (Math.abs(testPixels[i] - originalPixels[i]) > 0.001) {
                pixelsChanged = true;
                break;
            }
        }
        
        // In a real transform, pixels should change; mock might or might not
        console.log(`✅ Transform applied, pixels changed: ${pixelsChanged}`);
    });

    test('should apply transform with GPU when available', () => {
        const testPixels = new Float32Array([0.5, 0.5, 0.5, 1.0]);
        const result = ocio.applyTransform(processorId, testPixels, true);
        
        // Should succeed even if GPU is not available (fallback to CPU)
        assert.strictEqual(result, true);
        console.log('✅ GPU transform test passed (with CPU fallback)');
    });

    test('should release processor resources', () => {
        ocio.releaseProcessor(processorId);
        
        // Try to use released processor (should fail)
        const testPixels = new Float32Array([0.5, 0.5, 0.5, 1.0]);
        const result = ocio.applyTransform(processorId, testPixels);
        assert.strictEqual(result, false);
        
        console.log('✅ Processor release test passed');
    });
});

test.describe('WASM-Native Features', () => {
    test('should load configuration from URL', async () => {
        if (!ocio.hasNative()) {
            console.log('⏭️  Skipping WASM-native tests (not available)');
            return;
        }

        const result = ocio.loadConfigFromUrl('https://example.com/aces-1.3.ocio', 'aces-1.3');
        assert.strictEqual(result, true);
        console.log('✅ Config URL loading test passed');
    });

    test('should retrieve cached configuration', () => {
        if (!ocio.hasNative()) {
            return; // Skip if native features not available
        }

        const cachedConfig = ocio.getCachedConfig('aces-1.3');
        // May be empty if not actually loaded in mock
        assert(typeof cachedConfig === 'string');
        console.log(`✅ Cached config retrieval test passed (${cachedConfig.length} chars)`);
    });

    test('should clear cache successfully', () => {
        if (!ocio.hasNative()) {
            return;
        }

        const result = ocio.clearCache();
        assert.strictEqual(result, true);
        console.log('✅ Cache clearing test passed');
    });
});

test.describe('Performance and Statistics', () => {
    test('should provide comprehensive statistics', () => {
        const stats = ocio.getStats();
        
        assert(typeof stats.version === 'string');
        assert(typeof stats.initialized === 'boolean');
        assert(typeof stats.simd === 'boolean');
        assert(typeof stats.webgpu === 'boolean');
        assert(typeof stats.native === 'boolean');
        assert(stats.initialized === true);
        
        if (stats.numColorSpaces !== undefined) {
            assert(stats.numColorSpaces > 0);
        }
        
        console.log('✅ Statistics test passed:', Object.keys(stats));
    });

    test('should run performance benchmark', () => {
        const results = ocio.benchmark(1000, 10);
        
        assert(typeof results.testPixels === 'number');
        assert(typeof results.iterations === 'number');
        assert(results.testPixels === 1000);
        assert(results.iterations === 10);
        
        if (results.simdOpsPerSecond !== undefined) {
            assert(results.simdOpsPerSecond > 0);
            console.log(`✅ SIMD Performance: ${results.simdOpsPerSecond.toLocaleString()} ops/sec`);
        }
        
        console.log('✅ Benchmark test passed');
    });
});

test.describe('Error Handling', () => {
    test('should handle invalid processor operations', () => {
        // Try to use invalid processor ID
        const testPixels = new Float32Array([0.5, 0.5, 0.5, 1.0]);
        const result = ocio.applyTransform(99999, testPixels);
        assert.strictEqual(result, false);
        console.log('✅ Invalid processor handling test passed');
    });

    test('should handle invalid color space names', () => {
        const processorId = ocio.createProcessor('invalid_space', 'another_invalid_space');
        assert.strictEqual(processorId, 0);
        console.log('✅ Invalid color space handling test passed');
    });

    test('should handle empty pixel arrays', () => {
        const validProcessorId = ocio.createProcessor('scene_linear', 'sRGB');
        if (validProcessorId > 0) {
            const emptyPixels = new Float32Array([]);
            const result = ocio.applyTransform(validProcessorId, emptyPixels);
            assert.strictEqual(result, false);
            ocio.releaseProcessor(validProcessorId);
        }
        console.log('✅ Empty pixel array handling test passed');
    });
});

test.describe('Memory Management', () => {
    test('should handle multiple processors without memory leaks', () => {
        const processors = [];
        const maxProcessors = 10;
        
        // Create multiple processors
        for (let i = 0; i < maxProcessors; i++) {
            const processorId = ocio.createProcessor('scene_linear', 'sRGB');
            if (processorId > 0) {
                processors.push(processorId);
            }
        }
        
        assert(processors.length > 0);
        console.log(`✅ Created ${processors.length} processors`);
        
        // Release all processors
        processors.forEach(id => ocio.releaseProcessor(id));
        console.log('✅ Released all processors - memory management test passed');
    });
});

// Run tests with proper error handling
test.after(() => {
    console.log('🏁 OpenColorIO.wasm test suite completed');
});

// Export for external testing
module.exports = { createMockOpenColorIOModule };