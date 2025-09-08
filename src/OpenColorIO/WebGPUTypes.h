// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_WEBGPUTYPES_H
#define INCLUDED_OCIO_WEBGPUTYPES_H

#include <OpenColorIO/OpenColorIO.h>

namespace OCIO_NAMESPACE
{

// Extend GpuLanguage enum for WebGPU support
// Note: This would normally be added to OpenColorTypes.h, but we'll keep it separate for now
enum WebGPUExtendedGpuLanguage
{
    GPU_LANGUAGE_WGSL_1_0 = 1000,  ///< WebGPU Shading Language (compute shaders)
};

/// WebGPU buffer types for different data usage patterns
enum WebGPUBufferType
{
    WEBGPU_BUFFER_UNKNOWN = 0,
    WEBGPU_BUFFER_INPUT,      ///< Input data buffer (read-only)
    WEBGPU_BUFFER_OUTPUT,     ///< Output data buffer (write-only)
    WEBGPU_BUFFER_UNIFORM,    ///< Uniform/constant data buffer
    WEBGPU_BUFFER_STORAGE,    ///< General-purpose storage buffer
    WEBGPU_BUFFER_LUT_1D,     ///< 1D LUT texture buffer
    WEBGPU_BUFFER_LUT_3D      ///< 3D LUT texture buffer
};

/// WebGPU workgroup configuration
struct WebGPUWorkgroupConfig
{
    unsigned int x = 64;   // Workgroup size in X dimension (typical: 64)
    unsigned int y = 1;    // Workgroup size in Y dimension
    unsigned int z = 1;    // Workgroup size in Z dimension
};

/// WebGPU buffer descriptor
struct WebGPUBufferDescriptor
{
    WebGPUBufferType type = WEBGPU_BUFFER_UNKNOWN;
    unsigned int bindingIndex = 0;        // Binding index in compute shader
    size_t sizeInBytes = 0;               // Buffer size in bytes
    const void* initialData = nullptr;    // Initial data (optional)
    std::string name;                     // Buffer name for debugging
};

/// WebGPU compute pipeline configuration
struct WebGPUComputeConfig
{
    std::string shaderName;               // Shader function entry point
    WebGPUWorkgroupConfig workgroup;      // Workgroup dimensions
    std::vector<WebGPUBufferDescriptor> buffers;  // Buffer descriptors
    unsigned int dispatchX = 1;          // Number of workgroups in X
    unsigned int dispatchY = 1;          // Number of workgroups in Y  
    unsigned int dispatchZ = 1;          // Number of workgroups in Z
};

} // namespace OCIO_NAMESPACE

#endif