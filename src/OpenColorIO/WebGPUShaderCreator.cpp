// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include <sstream>
#include <algorithm>

#include "WebGPUShaderCreator.h"
#include "GpuShaderUtils.h"

namespace OCIO_NAMESPACE
{

class WebGPUShaderCreator::Impl
{
public:
    WebGPUWorkgroupConfig m_workgroupConfig;
    unsigned int m_dispatchX = 1;
    unsigned int m_dispatchY = 1;
    unsigned int m_dispatchZ = 1;
    
    std::vector<WebGPUBufferDescriptor> m_buffers;
    std::string m_computeShaderName = "colorTransform";
    
    // WebGPU always uses WGSL
    static constexpr GpuLanguage WEBGPU_LANGUAGE = static_cast<GpuLanguage>(GPU_LANGUAGE_WGSL_1_0);
    
    Impl() = default;
    ~Impl() = default;
    
    std::string generateWGSLBufferBindings() const;
    std::string generateWGSLComputeFunction(const std::string& functionBody) const;
    std::string generateWGSLWorkgroupSize() const;
};

std::shared_ptr<WebGPUShaderCreator> WebGPUShaderCreator::Create()
{
    return std::make_shared<WebGPUShaderCreator>();
}

WebGPUShaderCreator::WebGPUShaderCreator()
    : GpuShaderCreator()
    , m_impl(std::make_unique<Impl>())
{
    // Set language to our extended WGSL enum
    // Note: This will need integration with base class language handling
}

WebGPUShaderCreator::~WebGPUShaderCreator() = default;

GpuShaderCreatorRcPtr WebGPUShaderCreator::clone() const
{
    auto cloned = Create();
    
    // Copy WebGPU-specific data
    cloned->m_impl->m_workgroupConfig = m_impl->m_workgroupConfig;
    cloned->m_impl->m_dispatchX = m_impl->m_dispatchX;
    cloned->m_impl->m_dispatchY = m_impl->m_dispatchY;
    cloned->m_impl->m_dispatchZ = m_impl->m_dispatchZ;
    cloned->m_impl->m_buffers = m_impl->m_buffers;
    cloned->m_impl->m_computeShaderName = m_impl->m_computeShaderName;
    
    return cloned;
}

void WebGPUShaderCreator::setLanguage(GpuLanguage lang)
{
    // WebGPU always uses WGSL
    if (lang != Impl::WEBGPU_LANGUAGE)
    {
        // Log warning but continue with WGSL
    }
    GpuShaderCreator::setLanguage(Impl::WEBGPU_LANGUAGE);
}

GpuLanguage WebGPUShaderCreator::getLanguage() const noexcept
{
    return Impl::WEBGPU_LANGUAGE;
}

void WebGPUShaderCreator::setWorkgroupSize(unsigned int x, unsigned int y, unsigned int z)
{
    // WebGPU workgroup size limits: max 256 total, max 256 per dimension
    const unsigned int maxWorkgroupSize = 256;
    const unsigned int totalSize = x * y * z;
    
    if (totalSize > maxWorkgroupSize)
    {
        // Adjust to fit within limits
        x = std::min(x, maxWorkgroupSize);
        y = std::min(y, maxWorkgroupSize / x);
        z = std::min(z, maxWorkgroupSize / (x * y));
    }
    
    m_impl->m_workgroupConfig.x = x;
    m_impl->m_workgroupConfig.y = y;
    m_impl->m_workgroupConfig.z = z;
}

WebGPUWorkgroupConfig WebGPUShaderCreator::getWorkgroupSize() const noexcept
{
    return m_impl->m_workgroupConfig;
}

void WebGPUShaderCreator::setDispatchSize(unsigned int x, unsigned int y, unsigned int z)
{
    m_impl->m_dispatchX = x;
    m_impl->m_dispatchY = y;
    m_impl->m_dispatchZ = z;
}

void WebGPUShaderCreator::getDispatchSize(unsigned int& x, unsigned int& y, unsigned int& z) const noexcept
{
    x = m_impl->m_dispatchX;
    y = m_impl->m_dispatchY;
    z = m_impl->m_dispatchZ;
}

void WebGPUShaderCreator::addInputBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding)
{
    WebGPUBufferDescriptor desc;
    desc.type = WEBGPU_BUFFER_INPUT;
    desc.bindingIndex = binding;
    desc.sizeInBytes = sizeInBytes;
    desc.name = name;
    m_impl->m_buffers.push_back(desc);
}

void WebGPUShaderCreator::addOutputBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding)
{
    WebGPUBufferDescriptor desc;
    desc.type = WEBGPU_BUFFER_OUTPUT;
    desc.bindingIndex = binding;
    desc.sizeInBytes = sizeInBytes;
    desc.name = name;
    m_impl->m_buffers.push_back(desc);
}

void WebGPUShaderCreator::addUniformBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding)
{
    WebGPUBufferDescriptor desc;
    desc.type = WEBGPU_BUFFER_UNIFORM;
    desc.bindingIndex = binding;
    desc.sizeInBytes = sizeInBytes;
    desc.name = name;
    m_impl->m_buffers.push_back(desc);
}

void WebGPUShaderCreator::addLUT1DBuffer(const std::string& name, size_t lutSize, unsigned int binding)
{
    WebGPUBufferDescriptor desc;
    desc.type = WEBGPU_BUFFER_LUT_1D;
    desc.bindingIndex = binding;
    desc.sizeInBytes = lutSize * 3 * sizeof(float); // RGB values
    desc.name = name;
    m_impl->m_buffers.push_back(desc);
}

void WebGPUShaderCreator::addLUT3DBuffer(const std::string& name, size_t lutSize, unsigned int binding)
{
    WebGPUBufferDescriptor desc;
    desc.type = WEBGPU_BUFFER_LUT_3D;
    desc.bindingIndex = binding;
    desc.sizeInBytes = lutSize * lutSize * lutSize * 3 * sizeof(float); // RGB values
    desc.name = name;
    m_impl->m_buffers.push_back(desc);
}

const std::vector<WebGPUBufferDescriptor>& WebGPUShaderCreator::getBufferDescriptors() const noexcept
{
    return m_impl->m_buffers;
}

std::string WebGPUShaderCreator::generateComputeShader() const
{
    std::ostringstream shader;
    
    // Generate WGSL header
    shader << "// OpenColorIO WebGPU Compute Shader (WGSL)\n";
    shader << "// Auto-generated by OpenColorIO WebGPUShaderCreator\n\n";
    
    // Generate buffer bindings
    shader << m_impl->generateWGSLBufferBindings();
    
    // Generate workgroup size
    shader << m_impl->generateWGSLWorkgroupSize();
    
    // Generate compute function with OCIO body
    const std::string functionBody = getFunctionBody(); // From base class
    shader << m_impl->generateWGSLComputeFunction(functionBody);
    
    return shader.str();
}

WebGPUComputeConfig WebGPUShaderCreator::generateComputeConfig() const
{
    WebGPUComputeConfig config;
    config.shaderName = m_impl->m_computeShaderName;
    config.workgroup = m_impl->m_workgroupConfig;
    config.buffers = m_impl->m_buffers;
    config.dispatchX = m_impl->m_dispatchX;
    config.dispatchY = m_impl->m_dispatchY;
    config.dispatchZ = m_impl->m_dispatchZ;
    return config;
}

std::string WebGPUShaderCreator::Impl::generateWGSLBufferBindings() const
{
    std::ostringstream bindings;
    
    for (const auto& buffer : m_buffers)
    {
        bindings << "// Buffer: " << buffer.name << "\n";
        bindings << "@group(0) @binding(" << buffer.bindingIndex << ")\n";
        
        switch (buffer.type)
        {
            case WEBGPU_BUFFER_INPUT:
                bindings << "var<storage, read> " << buffer.name << ": array<vec4<f32>>;\n\n";
                break;
                
            case WEBGPU_BUFFER_OUTPUT:
                bindings << "var<storage, read_write> " << buffer.name << ": array<vec4<f32>>;\n\n";
                break;
                
            case WEBGPU_BUFFER_UNIFORM:
                bindings << "var<uniform> " << buffer.name << ": array<f32, " 
                         << (buffer.sizeInBytes / sizeof(float)) << ">;\n\n";
                break;
                
            case WEBGPU_BUFFER_LUT_1D:
                bindings << "var<storage, read> " << buffer.name << ": array<vec3<f32>>;\n\n";
                break;
                
            case WEBGPU_BUFFER_LUT_3D:
                bindings << "var<storage, read> " << buffer.name << ": array<vec3<f32>>;\n\n";
                break;
                
            default:
                bindings << "var<storage, read_write> " << buffer.name << ": array<f32>;\n\n";
                break;
        }
    }
    
    return bindings.str();
}

std::string WebGPUShaderCreator::Impl::generateWGSLComputeFunction(const std::string& functionBody) const
{
    std::ostringstream compute;
    
    compute << "@compute @workgroup_size(" << m_workgroupConfig.x;
    if (m_workgroupConfig.y > 1 || m_workgroupConfig.z > 1)
    {
        compute << ", " << m_workgroupConfig.y;
        if (m_workgroupConfig.z > 1)
        {
            compute << ", " << m_workgroupConfig.z;
        }
    }
    compute << ")\n";
    
    compute << "fn " << m_computeShaderName << "(@builtin(global_invocation_id) globalId: vec3<u32>) {\n";
    compute << "    let index = globalId.x;\n";
    compute << "    \n";
    compute << "    // Get input pixel\n";
    compute << "    if (index >= arrayLength(&inputBuffer)) {\n";
    compute << "        return;\n";
    compute << "    }\n";
    compute << "    \n";
    compute << "    let inColor = inputBuffer[index];\n";
    compute << "    var outColor = inColor;\n";
    compute << "    \n";
    
    // Convert OCIO function body to WGSL
    // This would need sophisticated translation from GLSL to WGSL
    compute << "    // OCIO color transformation (converted to WGSL)\n";
    compute << "    // TODO: Implement GLSL to WGSL translation\n";
    compute << "    // " << functionBody << "\n";
    compute << "    \n";
    
    compute << "    // Store result\n";
    compute << "    outputBuffer[index] = outColor;\n";
    compute << "}\n";
    
    return compute.str();
}

std::string WebGPUShaderCreator::Impl::generateWGSLWorkgroupSize() const
{
    std::ostringstream workgroup;
    workgroup << "// Workgroup configuration\n";
    workgroup << "// Workgroup size: " << m_workgroupConfig.x << "x" << m_workgroupConfig.y << "x" << m_workgroupConfig.z << "\n";
    workgroup << "// Total threads per workgroup: " << (m_workgroupConfig.x * m_workgroupConfig.y * m_workgroupConfig.z) << "\n\n";
    return workgroup.str();
}

} // namespace OCIO_NAMESPACE