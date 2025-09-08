/**
 * OpenColorIO.wasm - Browser Integration Tests (Playwright)
 * Cross-browser compatibility and performance testing
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

const { test, expect } = require('@playwright/test');

test.describe('OpenColorIO.wasm Browser Integration', () => {
  test.beforeEach(async ({ page }) => {
    // Set up COOP/COEP headers for SharedArrayBuffer support
    await page.setExtraHTTPHeaders({
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp'
    });
    
    // Navigate to test page
    await page.goto('/test/browser/test-page.html');
  });

  test('should load OpenColorIO.wasm module', async ({ page }) => {
    await page.waitForFunction(() => window.OpenColorIOLoaded === true, { timeout: 30000 });
    
    const version = await page.evaluate(() => window.ocioInstance?.getVersion());
    expect(version).toBeTruthy();
    expect(version).toMatch(/2\.\d+\.\d+/);
    
    console.log(`✅ OpenColorIO.wasm loaded successfully: ${version}`);
  });

  test('should detect SIMD support in modern browsers', async ({ page, browserName }) => {
    const hasSIMD = await page.evaluate(() => window.ocioInstance?.hasSIMD());
    
    if (browserName === 'chromium' || browserName === 'firefox') {
      // Modern browsers should support WASM SIMD
      expect(hasSIMD).toBe(true);
      console.log(`✅ SIMD support confirmed in ${browserName}`);
    } else {
      console.log(`ℹ️  SIMD support in ${browserName}: ${hasSIMD}`);
    }
  });

  test('should detect WebGPU availability', async ({ page, browserName }) => {
    const hasWebGPU = await page.evaluate(async () => {
      // Check browser WebGPU support first
      if (!navigator.gpu) {
        return false;
      }
      
      try {
        const adapter = await navigator.gpu.requestAdapter();
        return adapter !== null && window.ocioInstance?.hasWebGPU();
      } catch {
        return false;
      }
    });
    
    console.log(`ℹ️  WebGPU support in ${browserName}: ${hasWebGPU}`);
  });

  test('should initialize with default configuration', async ({ page }) => {
    const initialized = await page.evaluate(() => {
      return window.ocioInstance?.initialize();
    });
    
    expect(initialized).toBe(true);
    
    const colorSpaces = await page.evaluate(() => {
      return window.ocioInstance?.getColorSpaces();
    });
    
    expect(Array.isArray(colorSpaces)).toBe(true);
    expect(colorSpaces.length).toBeGreaterThan(0);
    
    console.log(`✅ Initialized with ${colorSpaces.length} color spaces`);
  });

  test('should perform color space transforms', async ({ page }) => {
    // Create test pixel data
    const result = await page.evaluate(() => {
      if (!window.ocioInstance) return false;
      
      const processorId = window.ocioInstance.createProcessor('scene_linear', 'sRGB');
      if (processorId <= 0) return false;
      
      // Test pixel array (RGBA)
      const pixels = new Float32Array([
        0.5, 0.5, 0.5, 1.0,  // Gray
        1.0, 0.0, 0.0, 1.0,  // Red
        0.0, 1.0, 0.0, 1.0,  // Green
        0.0, 0.0, 1.0, 1.0   // Blue
      ]);
      
      const transformResult = window.ocioInstance.applyTransform(processorId, pixels);
      window.ocioInstance.releaseProcessor(processorId);
      
      return {
        success: transformResult,
        pixelCount: pixels.length / 4,
        firstPixel: [pixels[0], pixels[1], pixels[2], pixels[3]]
      };
    });
    
    expect(result.success).toBe(true);
    expect(result.pixelCount).toBe(4);
    console.log(`✅ Transformed ${result.pixelCount} pixels successfully`);
  });

  test('should handle WASM-native features', async ({ page }) => {
    const nativeFeatures = await page.evaluate(async () => {
      if (!window.ocioInstance?.hasNative()) {
        return { available: false };
      }
      
      // Test cache operations
      const clearResult = window.ocioInstance.clearCache();
      
      // Test config loading (mock URL)
      const loadResult = window.ocioInstance.loadConfigFromUrl(
        'https://httpbin.org/json', // Mock endpoint
        'test-config'
      );
      
      return {
        available: true,
        clearCache: clearResult,
        loadConfig: loadResult
      };
    });
    
    if (nativeFeatures.available) {
      expect(nativeFeatures.clearCache).toBe(true);
      console.log('✅ WASM-native features working correctly');
    } else {
      console.log('ℹ️  WASM-native features not available (expected in some configurations)');
    }
  });

  test('should provide performance statistics', async ({ page }) => {
    const stats = await page.evaluate(() => {
      return window.ocioInstance?.getStats();
    });
    
    expect(stats).toBeTruthy();
    expect(typeof stats.version).toBe('string');
    expect(typeof stats.initialized).toBe('boolean');
    expect(typeof stats.simd).toBe('boolean');
    expect(typeof stats.webgpu).toBe('boolean');
    expect(typeof stats.native).toBe('boolean');
    
    console.log('✅ Statistics retrieved:', {
      version: stats.version,
      features: `SIMD:${stats.simd} WebGPU:${stats.webgpu} Native:${stats.native}`
    });
  });

  test('should run performance benchmark', async ({ page }) => {
    const benchmark = await page.evaluate(async () => {
      if (!window.ocioInstance) return null;
      
      // Run benchmark with smaller dataset for browser testing
      return window.ocioInstance.benchmark(1000, 10);
    });
    
    expect(benchmark).toBeTruthy();
    expect(benchmark.testPixels).toBe(1000);
    expect(benchmark.iterations).toBe(10);
    
    if (benchmark.simdOpsPerSecond) {
      expect(benchmark.simdOpsPerSecond).toBeGreaterThan(0);
      console.log(`✅ SIMD Performance: ${Math.round(benchmark.simdOpsPerSecond / 1000)}K ops/sec`);
    }
    
    if (benchmark.webgpuOpsPerSecond && benchmark.webgpuOpsPerSecond > 0) {
      console.log(`✅ WebGPU Performance: ${Math.round(benchmark.webgpuOpsPerSecond / 1000)}K ops/sec`);
    }
  });

  test('should handle memory management correctly', async ({ page }) => {
    const memoryTest = await page.evaluate(() => {
      if (!window.ocioInstance) return false;
      
      const processors = [];
      let succeeded = true;
      
      try {
        // Create multiple processors
        for (let i = 0; i < 20; i++) {
          const processorId = window.ocioInstance.createProcessor('scene_linear', 'sRGB');
          if (processorId > 0) {
            processors.push(processorId);
          }
        }
        
        // Test transforms with multiple processors
        const testPixels = new Float32Array([0.5, 0.5, 0.5, 1.0]);
        
        for (const processorId of processors) {
          const result = window.ocioInstance.applyTransform(processorId, testPixels);
          if (!result) {
            succeeded = false;
            break;
          }
        }
        
        // Clean up all processors
        for (const processorId of processors) {
          window.ocioInstance.releaseProcessor(processorId);
        }
        
      } catch (error) {
        console.error('Memory test error:', error);
        succeeded = false;
      }
      
      return {
        succeeded,
        processorsCreated: processors.length
      };
    });
    
    expect(memoryTest.succeeded).toBe(true);
    expect(memoryTest.processorsCreated).toBeGreaterThan(0);
    console.log(`✅ Memory management test passed with ${memoryTest.processorsCreated} processors`);
  });

  test('should handle errors gracefully', async ({ page }) => {
    const errorHandling = await page.evaluate(() => {
      if (!window.ocioInstance) return false;
      
      try {
        // Test invalid operations
        const invalidProcessor = window.ocioInstance.createProcessor('invalid_space', 'another_invalid');
        const invalidTransform = window.ocioInstance.applyTransform(99999, new Float32Array([1, 2, 3, 4]));
        const emptyTransform = window.ocioInstance.applyTransform(1, new Float32Array([]));
        
        return {
          invalidProcessor: invalidProcessor === 0,
          invalidTransform: invalidTransform === false,
          emptyTransform: emptyTransform === false
        };
        
      } catch (error) {
        console.error('Unexpected error in error handling test:', error);
        return false;
      }
    });
    
    expect(errorHandling.invalidProcessor).toBe(true);
    expect(errorHandling.invalidTransform).toBe(true);
    expect(errorHandling.emptyTransform).toBe(true);
    
    console.log('✅ Error handling tests passed');
  });
});

test.describe('Cross-Browser Compatibility', () => {
  test('should work consistently across browsers', async ({ page, browserName }) => {
    // Wait for module to load
    await page.waitForFunction(() => window.OpenColorIOLoaded === true, { timeout: 30000 });
    
    const compatibility = await page.evaluate(() => {
      const stats = window.ocioInstance?.getStats();
      
      return {
        version: stats?.version,
        initialized: stats?.initialized,
        simdSupport: stats?.simd,
        webgpuSupport: stats?.webgpu,
        nativeSupport: stats?.native,
        browserSupport: {
          wasmSupported: typeof WebAssembly !== 'undefined',
          simdDetected: typeof WebAssembly.SIMD !== 'undefined',
          webgpuDetected: typeof navigator !== 'undefined' && !!navigator.gpu
        }
      };
    });
    
    expect(compatibility.initialized).toBe(true);
    expect(compatibility.browserSupport.wasmSupported).toBe(true);
    
    console.log(`✅ ${browserName} compatibility:`, {
      version: compatibility.version,
      features: `SIMD:${compatibility.simdSupport} WebGPU:${compatibility.webgpuSupport}`,
      browser: `WASM:✅ SIMD:${compatibility.browserSupport.simdDetected ? '✅' : '❌'} WebGPU:${compatibility.browserSupport.webgpuDetected ? '✅' : '❌'}`
    });
  });
});

// Visual regression tests for color transforms
test.describe('Visual Color Transform Tests', () => {
  test('should produce consistent color transform results', async ({ page }) => {
    // This test would compare rendered colors against reference images
    // For now, we'll do basic consistency checks
    
    const colorTest = await page.evaluate(() => {
      if (!window.ocioInstance) return null;
      
      const processorId = window.ocioInstance.createProcessor('scene_linear', 'sRGB');
      if (processorId <= 0) return null;
      
      // Test known color values
      const testColors = [
        [0.0, 0.0, 0.0, 1.0],  // Black
        [0.5, 0.5, 0.5, 1.0],  // Gray
        [1.0, 1.0, 1.0, 1.0],  // White
        [1.0, 0.0, 0.0, 1.0],  // Red
        [0.0, 1.0, 0.0, 1.0],  // Green
        [0.0, 0.0, 1.0, 1.0]   // Blue
      ];
      
      const results = [];
      
      for (const color of testColors) {
        const pixels = new Float32Array(color);
        const transformResult = window.ocioInstance.applyTransform(processorId, pixels);
        
        if (transformResult) {
          results.push({
            input: [...color],
            output: [...pixels]
          });
        }
      }
      
      window.ocioInstance.releaseProcessor(processorId);
      
      return {
        success: results.length === testColors.length,
        results: results
      };
    });
    
    expect(colorTest?.success).toBe(true);
    expect(colorTest.results).toHaveLength(6);
    
    // Basic sanity checks for color transforms
    const whiteResult = colorTest.results[2]; // White should remain roughly white
    expect(whiteResult.output[0]).toBeGreaterThan(0.8); // R
    expect(whiteResult.output[1]).toBeGreaterThan(0.8); // G
    expect(whiteResult.output[2]).toBeGreaterThan(0.8); // B
    expect(whiteResult.output[3]).toBe(1.0); // A unchanged
    
    console.log('✅ Visual color transform consistency verified');
  });
});