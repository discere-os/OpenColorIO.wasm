// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "WebGPUShaderCreator.h"
#include "WGSLTranslator.h"
#include <iostream>
#include <sstream>

namespace OCIO_NAMESPACE
{

/// Example class demonstrating WebGPU compute pipeline usage with OpenColorIO
class WebGPUExample
{
public:
    /// Create a simple gamma correction compute pipeline example
    static std::string createGammaCorrectionExample()
    {
        auto webgpuCreator = WebGPUShaderCreator::Create();
        
        // Configure workgroup size for parallel processing
        webgpuCreator->setWorkgroupSize(64, 1, 1);  // 64 threads per workgroup
        
        // Add buffers for input/output image data
        const size_t imageSize = 1920 * 1080 * 4 * sizeof(float);  // 1080p RGBA
        webgpuCreator->addInputBuffer("inputBuffer", imageSize, 0);
        webgpuCreator->addOutputBuffer("outputBuffer", imageSize, 1);
        
        // Add uniform buffer for gamma value
        webgpuCreator->addUniformBuffer("gammaParams", sizeof(float), 2);
        
        // Set dispatch size (number of pixels / workgroup size)
        const unsigned int numPixels = 1920 * 1080;
        const unsigned int numWorkgroups = (numPixels + 63) / 64;  // Round up
        webgpuCreator->setDispatchSize(numWorkgroups, 1, 1);
        
        return webgpuCreator->generateComputeShader();
    }
    
    /// Create a 1D LUT lookup compute pipeline example
    static std::string createLUT1DExample()
    {
        auto webgpuCreator = WebGPUShaderCreator::Create();
        
        // Configure for 1D LUT processing
        webgpuCreator->setWorkgroupSize(64, 1, 1);
        
        // Add buffers
        const size_t imageSize = 1920 * 1080 * 4 * sizeof(float);
        const size_t lutSize = 1024;  // 1024-point LUT
        
        webgpuCreator->addInputBuffer("inputBuffer", imageSize, 0);
        webgpuCreator->addOutputBuffer("outputBuffer", imageSize, 1);
        webgpuCreator->addLUT1DBuffer("lut1D", lutSize, 2);
        
        // Dispatch configuration
        const unsigned int numPixels = 1920 * 1080;
        webgpuCreator->setDispatchSize((numPixels + 63) / 64, 1, 1);
        
        return webgpuCreator->generateComputeShader();
    }
    
    /// Create a complete color transformation pipeline example
    static std::string createColorTransformExample()
    {
        auto webgpuCreator = WebGPUShaderCreator::Create();
        
        // Configure for complex color pipeline
        webgpuCreator->setWorkgroupSize(64, 1, 1);
        
        // Add all necessary buffers
        const size_t imageSize = 1920 * 1080 * 4 * sizeof(float);
        const size_t lut3DSize = 64;  // 64^3 LUT
        
        webgpuCreator->addInputBuffer("inputBuffer", imageSize, 0);
        webgpuCreator->addOutputBuffer("outputBuffer", imageSize, 1);
        webgpuCreator->addUniformBuffer("colorMatrix", 16 * sizeof(float), 2);  // 4x4 matrix
        webgpuCreator->addLUT3DBuffer("lut3D", lut3DSize, 3);
        webgpuCreator->addUniformBuffer("gammaParams", 4 * sizeof(float), 4);  // RGB gamma + spare
        
        // Dispatch for full image
        const unsigned int numPixels = 1920 * 1080;
        webgpuCreator->setDispatchSize((numPixels + 63) / 64, 1, 1);
        
        return webgpuCreator->generateComputeShader();
    }
    
    /// Generate a complete WGSL shader with helper functions
    static std::string generateCompleteShader()
    {
        std::ostringstream shader;
        
        // Add helper functions
        shader << WGSLTranslator::generateWGSLHelperFunctions();
        
        // Add example compute pipeline
        shader << createColorTransformExample();
        
        return shader.str();
    }
    
    /// Generate JavaScript code for WebGPU integration
    static std::string generateJavaScriptExample()
    {
        std::ostringstream js;
        
        js << "// OpenColorIO WebGPU Integration Example\n";
        js << "class OCIOWebGPUProcessor {\n";
        js << "    constructor() {\n";
        js << "        this.device = null;\n";
        js << "        this.computePipeline = null;\n";
        js << "        this.bindGroupLayout = null;\n";
        js << "    }\n\n";
        
        js << "    async initialize() {\n";
        js << "        // Get WebGPU adapter and device\n";
        js << "        if (!navigator.gpu) {\n";
        js << "            throw new Error('WebGPU not supported');\n";
        js << "        }\n\n";
        
        js << "        const adapter = await navigator.gpu.requestAdapter();\n";
        js << "        this.device = await adapter.requestDevice();\n\n";
        
        js << "        // Create compute pipeline\n";
        js << "        const shaderModule = this.device.createShaderModule({\n";
        js << "            code: OCIO_WGSL_SHADER_SOURCE  // Generated WGSL\n";
        js << "        });\n\n";
        
        js << "        this.computePipeline = this.device.createComputePipeline({\n";
        js << "            compute: {\n";
        js << "                module: shaderModule,\n";
        js << "                entryPoint: 'colorTransform'\n";
        js << "            }\n";
        js << "        });\n\n";
        
        js << "        this.bindGroupLayout = this.computePipeline.getBindGroupLayout(0);\n";
        js << "    }\n\n";
        
        js << "    async processImage(inputImageData, config) {\n";
        js << "        const width = inputImageData.width;\n";
        js << "        const height = inputImageData.height;\n";
        js << "        const pixelCount = width * height;\n\n";
        
        js << "        // Create input/output buffers\n";
        js << "        const inputBuffer = this.device.createBuffer({\n";
        js << "            size: pixelCount * 4 * 4,  // RGBA float32\n";
        js << "            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_DST\n";
        js << "        });\n\n";
        
        js << "        const outputBuffer = this.device.createBuffer({\n";
        js << "            size: pixelCount * 4 * 4,  // RGBA float32\n";
        js << "            usage: GPUBufferUsage.STORAGE | GPUBufferUsage.COPY_SRC\n";
        js << "        });\n\n";
        
        js << "        // Upload image data\n";
        js << "        const imageFloat32 = new Float32Array(inputImageData.data.buffer);\n";
        js << "        this.device.queue.writeBuffer(inputBuffer, 0, imageFloat32);\n\n";
        
        js << "        // Create bind group\n";
        js << "        const bindGroup = this.device.createBindGroup({\n";
        js << "            layout: this.bindGroupLayout,\n";
        js << "            entries: [\n";
        js << "                { binding: 0, resource: { buffer: inputBuffer } },\n";
        js << "                { binding: 1, resource: { buffer: outputBuffer } },\n";
        js << "                // Add other buffers (uniforms, LUTs) as needed\n";
        js << "            ]\n";
        js << "        });\n\n";
        
        js << "        // Dispatch compute shader\n";
        js << "        const commandEncoder = this.device.createCommandEncoder();\n";
        js << "        const passEncoder = commandEncoder.beginComputePass();\n";
        js << "        passEncoder.setPipeline(this.computePipeline);\n";
        js << "        passEncoder.setBindGroup(0, bindGroup);\n";
        js << "        passEncoder.dispatchWorkgroups(Math.ceil(pixelCount / 64));\n";
        js << "        passEncoder.end();\n\n";
        
        js << "        // Submit and wait for completion\n";
        js << "        this.device.queue.submit([commandEncoder.finish()]);\n";
        js << "        await this.device.queue.onSubmittedWorkDone();\n\n";
        
        js << "        // Read back results\n";
        js << "        const readBuffer = this.device.createBuffer({\n";
        js << "            size: pixelCount * 4 * 4,\n";
        js << "            usage: GPUBufferUsage.COPY_DST | GPUBufferUsage.MAP_READ\n";
        js << "        });\n\n";
        
        js << "        const copyEncoder = this.device.createCommandEncoder();\n";
        js << "        copyEncoder.copyBufferToBuffer(outputBuffer, 0, readBuffer, 0, pixelCount * 4 * 4);\n";
        js << "        this.device.queue.submit([copyEncoder.finish()]);\n\n";
        
        js << "        await readBuffer.mapAsync(GPUMapMode.READ);\n";
        js << "        const resultData = new Float32Array(readBuffer.getMappedRange());\n\n";
        
        js << "        // Create output ImageData\n";
        js << "        const outputImageData = new ImageData(width, height);\n";
        js << "        const uint8Data = new Uint8ClampedArray(resultData.buffer);\n";
        js << "        outputImageData.data.set(uint8Data);\n\n";
        
        js << "        // Cleanup\n";
        js << "        readBuffer.unmap();\n";
        js << "        inputBuffer.destroy();\n";
        js << "        outputBuffer.destroy();\n";
        js << "        readBuffer.destroy();\n\n";
        
        js << "        return outputImageData;\n";
        js << "    }\n";
        js << "}\n\n";
        
        js << "// Usage example:\n";
        js << "// const processor = new OCIOWebGPUProcessor();\n";
        js << "// await processor.initialize();\n";
        js << "// const result = await processor.processImage(inputImageData, config);\n";
        
        return js.str();
    }
};

} // namespace OCIO_NAMESPACE