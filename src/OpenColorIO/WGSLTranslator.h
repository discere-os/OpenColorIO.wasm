// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#ifndef INCLUDED_OCIO_WGSLTRANSLATOR_H
#define INCLUDED_OCIO_WGSLTRANSLATOR_H

#include <string>
#include <map>

namespace OCIO_NAMESPACE
{

/// Utility class to translate GLSL shader code to WGSL for WebGPU compute shaders
/// Handles the conversion of OCIO's existing GLSL generation to WebGPU-compatible WGSL
class WGSLTranslator
{
public:
    /// Convert GLSL function body to WGSL compute shader compatible code
    static std::string translateGLSLToWGSL(const std::string& glslCode);
    
    /// Translate GLSL data types to WGSL equivalents
    static std::string translateDataType(const std::string& glslType);
    
    /// Translate GLSL built-in functions to WGSL equivalents
    static std::string translateBuiltinFunction(const std::string& functionCall);
    
    /// Translate GLSL texture operations to WGSL buffer operations
    static std::string translateTextureLookup(const std::string& textureLookup);
    
    /// Generate WGSL helper functions for common OCIO operations
    static std::string generateWGSLHelperFunctions();
    
private:
    static std::map<std::string, std::string> getGLSLToWGSLTypeMap();
    static std::map<std::string, std::string> getGLSLToWGSLFunctionMap();
    
    static std::string replaceFunctionCalls(const std::string& code);
    static std::string replaceDataTypes(const std::string& code);
    static std::string replaceTextureLookups(const std::string& code);
    static std::string replaceVariableDeclarations(const std::string& code);
};

} // namespace OCIO_NAMESPACE

#endif