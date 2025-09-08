// SPDX-License-Identifier: BSD-3-Clause  
// Copyright Contributors to the OpenColorIO Project.

#include <iostream>
#include <fstream>
#include "../src/OpenColorIO/WebGPUExample.cpp"

// Demonstration of OpenColorIO WebGPU compute shader generation
int main()
{
    std::cout << "OpenColorIO WebGPU Compute Shader Demo\n";
    std::cout << "======================================\n\n";
    
    try 
    {
        // Generate gamma correction example
        std::cout << "1. Gamma Correction Shader:\n";
        std::cout << "----------------------------\n";
        std::string gammaShader = OCIO_NAMESPACE::WebGPUExample::createGammaCorrectionExample();
        std::cout << gammaShader << "\n\n";
        
        // Generate 1D LUT example
        std::cout << "2. 1D LUT Lookup Shader:\n";
        std::cout << "------------------------\n";
        std::string lut1dShader = OCIO_NAMESPACE::WebGPUExample::createLUT1DExample();
        std::cout << lut1dShader << "\n\n";
        
        // Generate complete color transform pipeline
        std::cout << "3. Complete Color Transform Pipeline:\n";
        std::cout << "------------------------------------\n";
        std::string completeShader = OCIO_NAMESPACE::WebGPUExample::generateCompleteShader();
        std::cout << completeShader << "\n\n";
        
        // Generate JavaScript integration example
        std::cout << "4. JavaScript Integration Example:\n";
        std::cout << "----------------------------------\n";
        std::string jsExample = OCIO_NAMESPACE::WebGPUExample::generateJavaScriptExample();
        std::cout << jsExample << "\n\n";
        
        // Save shaders to files for testing
        std::ofstream gammaFile("gamma_correction.wgsl");
        gammaFile << gammaShader;
        gammaFile.close();
        
        std::ofstream lut1dFile("lut1d_lookup.wgsl");
        lut1dFile << lut1dShader;
        lut1dFile.close();
        
        std::ofstream completeFile("complete_transform.wgsl");
        completeFile << completeShader;
        completeFile.close();
        
        std::ofstream jsFile("webgpu_integration.js");
        jsFile << jsExample;
        jsFile.close();
        
        std::cout << "Demo completed successfully!\n";
        std::cout << "Generated files:\n";
        std::cout << "  - gamma_correction.wgsl\n";
        std::cout << "  - lut1d_lookup.wgsl\n";
        std::cout << "  - complete_transform.wgsl\n";
        std::cout << "  - webgpu_integration.js\n";
        
        return 0;
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}