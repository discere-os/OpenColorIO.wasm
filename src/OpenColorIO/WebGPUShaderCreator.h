// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_WEBGPUSHADERCREATOR_H
#define INCLUDED_OCIO_WEBGPUSHADERCREATOR_H

#include <memory>
#include <vector>

#include <OpenColorIO/OpenColorIO.h>
#include "GpuShaderDesc.h"
#include "WebGPUTypes.h"

namespace OCIO_NAMESPACE
{

/// WebGPU compute shader creator class
/// Extends the existing GPU shader system to generate WGSL compute shaders
/// for parallel color processing operations
class WebGPUShaderCreator : public GpuShaderCreator
{
public:
    static std::shared_ptr<WebGPUShaderCreator> Create();

    WebGPUShaderCreator();
    ~WebGPUShaderCreator() override;

    // Override base class methods for WGSL generation
    GpuShaderCreatorRcPtr clone() const override;
    void setLanguage(GpuLanguage lang) override;
    GpuLanguage getLanguage() const noexcept override;

    // WebGPU-specific configuration
    void setWorkgroupSize(unsigned int x, unsigned int y = 1, unsigned int z = 1);
    WebGPUWorkgroupConfig getWorkgroupSize() const noexcept;

    void setDispatchSize(unsigned int x, unsigned int y = 1, unsigned int z = 1);
    void getDispatchSize(unsigned int& x, unsigned int& y, unsigned int& z) const noexcept;

    // Buffer management
    void addInputBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding);
    void addOutputBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding);
    void addUniformBuffer(const std::string& name, size_t sizeInBytes, unsigned int binding);
    void addLUT1DBuffer(const std::string& name, size_t lutSize, unsigned int binding);
    void addLUT3DBuffer(const std::string& name, size_t lutSize, unsigned int binding);

    // Get buffer configuration
    const std::vector<WebGPUBufferDescriptor>& getBufferDescriptors() const noexcept;

    // Generate complete WGSL compute shader
    std::string generateComputeShader() const;

    // Generate compute pipeline configuration
    WebGPUComputeConfig generateComputeConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> m_impl;
};

typedef std::shared_ptr<WebGPUShaderCreator> WebGPUShaderCreatorRcPtr;

} // namespace OCIO_NAMESPACE

#endif