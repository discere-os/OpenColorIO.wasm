/**
 * Playwright Configuration for OpenColorIO.wasm Browser Tests
 * Cross-browser testing with SIMD and WebGPU feature detection
 * 
 * Copyright 2025 Superstruct Ltd, New Zealand
 * Licensed under BSD 3-Clause License (same as OpenColorIO)
 */

const { defineConfig, devices } = require('@playwright/test');

module.exports = defineConfig({
  // Test configuration
  testDir: './test/browser',
  fullyParallel: true,
  forbidOnly: !!process.env.CI,
  retries: process.env.CI ? 2 : 0,
  workers: process.env.CI ? 1 : undefined,
  reporter: 'html',
  
  // Global test settings
  timeout: 60000,
  expect: {
    timeout: 10000
  },
  
  // Global browser configuration
  use: {
    baseURL: 'http://localhost:8080',
    
    // Required headers for SharedArrayBuffer and advanced WASM features
    extraHTTPHeaders: {
      'Cross-Origin-Opener-Policy': 'same-origin',
      'Cross-Origin-Embedder-Policy': 'require-corp',
      'Cross-Origin-Resource-Policy': 'cross-origin'
    },
    
    // Test settings
    trace: 'on-first-retry',
    screenshot: 'only-on-failure',
    video: 'retain-on-failure'
  },

  // Browser-specific test configurations
  projects: [
    // Chromium with SIMD and WebGPU support
    {
      name: 'chromium-modern',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            '--enable-features=WebAssemblySimd',
            '--enable-unsafe-webgpu',
            '--enable-features=SharedArrayBuffer',
            '--disable-web-security', // For testing only
            '--disable-features=VizDisplayCompositor'
          ]
        }
      }
    },

    // Firefox with SIMD support
    {
      name: 'firefox-modern',
      use: {
        ...devices['Desktop Firefox'],
        launchOptions: {
          firefoxUserPrefs: {
            'javascript.options.wasm_simd': true,
            'dom.postMessage.sharedArrayBuffer.bypassCOOP_COEP.insecure.enabled': true,
            'javascript.options.shared_memory': true
          }
        }
      }
    },

    // Safari/WebKit (limited SIMD support)
    {
      name: 'webkit-modern',
      use: {
        ...devices['Desktop Safari'],
        // Note: WebKit has limited support for advanced WASM features
      }
    },

    // Mobile Chrome (limited features)
    {
      name: 'mobile-chrome',
      use: {
        ...devices['Pixel 5'],
        launchOptions: {
          args: [
            '--enable-features=WebAssemblySimd'
          ]
        }
      }
    },

    // Legacy browser support (fallback testing)
    {
      name: 'chromium-legacy',
      use: {
        ...devices['Desktop Chrome'],
        launchOptions: {
          args: [
            '--disable-features=WebAssemblySimd',
            '--disable-features=SharedArrayBuffer'
          ]
        }
      }
    }
  ],

  // Development server configuration
  webServer: {
    command: 'npx http-server -p 8080 -c-1 --cors',
    port: 8080,
    reuseExistingServer: !process.env.CI,
    timeout: 30000
  }
});