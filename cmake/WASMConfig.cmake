# SPDX-License-Identifier: BSD-3-Clause  
# Copyright Contributors to the OpenColorIO Project.

# WebAssembly-specific build configuration for OpenColorIO

message(STATUS "Configuring OpenColorIO for WebAssembly build")

# Force static library build
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build static libraries for WASM" FORCE)

# Disable components not needed for WASM
set(OCIO_BUILD_APPS OFF CACHE BOOL "Disable command-line apps for WASM" FORCE)
set(OCIO_BUILD_OPENFX OFF CACHE BOOL "Disable OpenFX plugins for WASM" FORCE)
set(OCIO_BUILD_NUKE OFF CACHE BOOL "Disable Nuke plugins for WASM" FORCE) 
set(OCIO_BUILD_TESTS OFF CACHE BOOL "Disable tests for WASM" FORCE)
set(OCIO_BUILD_GPU_TESTS OFF CACHE BOOL "Disable GPU tests for WASM" FORCE)
set(OCIO_BUILD_DOCS OFF CACHE BOOL "Disable documentation for WASM" FORCE)
set(OCIO_BUILD_PYTHON OFF CACHE BOOL "Disable Python bindings for WASM" FORCE)
set(OCIO_BUILD_JAVA OFF CACHE BOOL "Disable Java bindings for WASM" FORCE)

# Enable headless GPU rendering
set(OCIO_USE_HEADLESS ON CACHE BOOL "Enable headless GPU rendering for WASM" FORCE)

# SIMD configuration - enable WebAssembly SIMD compatible optimizations
# WebAssembly SIMD supports 128-bit vectors compatible with SSE-style operations
set(OCIO_USE_SIMD ON CACHE BOOL "Enable SIMD optimizations" FORCE)

# Enable SSE optimizations compatible with WebAssembly SIMD (-msimd128)
# These will be translated to WebAssembly SIMD instructions
set(OCIO_USE_SSE2 ON CACHE BOOL "Enable SSE2 for WASM SIMD" FORCE)
set(OCIO_USE_SSE3 ON CACHE BOOL "Enable SSE3 for WASM SIMD" FORCE) 
set(OCIO_USE_SSSE3 ON CACHE BOOL "Enable SSSE3 for WASM SIMD" FORCE)
set(OCIO_USE_SSE4 ON CACHE BOOL "Enable SSE4 for WASM SIMD" FORCE)
set(OCIO_USE_SSE42 ON CACHE BOOL "Enable SSE4.2 for WASM SIMD" FORCE)

# Disable AVX and higher - not supported by WebAssembly SIMD
set(OCIO_USE_AVX OFF CACHE BOOL "Disable AVX for WASM build" FORCE)
set(OCIO_USE_AVX2 OFF CACHE BOOL "Disable AVX2 for WASM build" FORCE)
set(OCIO_USE_AVX512 OFF CACHE BOOL "Disable AVX512 for WASM build" FORCE)
set(OCIO_USE_F16C OFF CACHE BOOL "Disable F16C for WASM build" FORCE)

# SSE2NEON not needed for WebAssembly SIMD
set(OCIO_USE_SSE2NEON OFF CACHE BOOL "Disable SSE2NEON for WASM build" FORCE)

# External packages - use MISSING to allow system packages where available
set(OCIO_INSTALL_EXT_PACKAGES "MISSING" CACHE STRING "Install missing external packages" FORCE)

# Warning configuration
set(OCIO_WARNING_AS_ERROR OFF CACHE BOOL "Disable warnings as errors for WASM" FORCE)

# Disable sanitizers for WASM
set(OCIO_ENABLE_SANITIZER OFF CACHE BOOL "Disable sanitizers for WASM" FORCE)

# Optimization settings
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Use Release build for WASM" FORCE)

message(STATUS "WASM Configuration applied:")
message(STATUS "  Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "  Static Libraries: ${BUILD_SHARED_LIBS}")
message(STATUS "  SIMD Enabled: ${OCIO_USE_SIMD}")
message(STATUS "  WebAssembly SIMD: SSE2=${OCIO_USE_SSE2} SSE3=${OCIO_USE_SSE3} SSE4=${OCIO_USE_SSE4}")
message(STATUS "  Apps/Tests Disabled: ${OCIO_BUILD_APPS}/${OCIO_BUILD_TESTS}")
message(STATUS "  Headless GPU: ${OCIO_USE_HEADLESS}")