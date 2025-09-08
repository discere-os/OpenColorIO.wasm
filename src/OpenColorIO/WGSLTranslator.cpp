// SPDX-License-Identifier: BSD-3-Clause
// Copyright Contributors to the OpenColorIO Project.

#include "WGSLTranslator.h"
#include <regex>
#include <sstream>

namespace OCIO_NAMESPACE
{

std::string WGSLTranslator::translateGLSLToWGSL(const std::string& glslCode)
{
    std::string wgslCode = glslCode;
    
    // Apply transformations in order
    wgslCode = replaceDataTypes(wgslCode);
    wgslCode = replaceFunctionCalls(wgslCode);
    wgslCode = replaceTextureLookups(wgslCode);
    wgslCode = replaceVariableDeclarations(wgslCode);
    
    return wgslCode;
}

std::string WGSLTranslator::translateDataType(const std::string& glslType)
{
    auto typeMap = getGLSLToWGSLTypeMap();
    auto it = typeMap.find(glslType);
    return (it != typeMap.end()) ? it->second : glslType;
}

std::string WGSLTranslator::translateBuiltinFunction(const std::string& functionCall)
{
    auto funcMap = getGLSLToWGSLFunctionMap();
    for (const auto& pair : funcMap)
    {
        std::string pattern = pair.first + "\\(";
        if (functionCall.find(pair.first + "(") != std::string::npos)
        {
            return std::regex_replace(functionCall, std::regex(pair.first), pair.second);
        }
    }
    return functionCall;
}

std::string WGSLTranslator::translateTextureLookup(const std::string& textureLookup)
{
    // Convert GLSL texture2D() calls to WGSL buffer lookups
    // This is a simplified conversion - real implementation would need more sophistication
    
    std::string result = textureLookup;
    
    // Replace texture2D with buffer array access
    result = std::regex_replace(result, 
                               std::regex(R"(texture2D\s*\(\s*(\w+)\s*,\s*([^)]+)\))"),
                               "getLUTValue($1, $2)");
    
    return result;
}

std::string WGSLTranslator::generateWGSLHelperFunctions()
{
    std::ostringstream helpers;
    
    helpers << "// WGSL Helper Functions for OpenColorIO\n\n";
    
    // LUT lookup helper for 1D LUTs
    helpers << "fn getLUT1DValue(lut: array<vec3<f32>>, coord: f32, size: u32) -> vec3<f32> {\n";
    helpers << "    let scaledCoord = clamp(coord * f32(size - 1u), 0.0, f32(size - 1u));\n";
    helpers << "    let index = u32(floor(scaledCoord));\n";
    helpers << "    let fract = scaledCoord - f32(index);\n";
    helpers << "    \n";
    helpers << "    if (index >= size - 1u) {\n";
    helpers << "        return lut[size - 1u];\n";
    helpers << "    }\n";
    helpers << "    \n";
    helpers << "    return mix(lut[index], lut[index + 1u], fract);\n";
    helpers << "}\n\n";
    
    // LUT lookup helper for 3D LUTs
    helpers << "fn getLUT3DValue(lut: array<vec3<f32>>, coord: vec3<f32>, size: u32) -> vec3<f32> {\n";
    helpers << "    let scaledCoord = clamp(coord * f32(size - 1u), vec3<f32>(0.0), vec3<f32>(f32(size - 1u)));\n";
    helpers << "    let baseIndex = vec3<u32>(floor(scaledCoord));\n";
    helpers << "    let fract = scaledCoord - vec3<f32>(baseIndex);\n";
    helpers << "    \n";
    helpers << "    // Clamp to valid range\n";
    helpers << "    let clampedIndex = min(baseIndex, vec3<u32>(size - 1u));\n";
    helpers << "    \n";
    helpers << "    // Trilinear interpolation\n";
    helpers << "    let i000 = clampedIndex.x + clampedIndex.y * size + clampedIndex.z * size * size;\n";
    helpers << "    let i001 = i000 + size * size;\n";
    helpers << "    let i010 = i000 + size;\n";
    helpers << "    let i011 = i010 + size * size;\n";
    helpers << "    let i100 = i000 + 1u;\n";
    helpers << "    let i101 = i100 + size * size;\n";
    helpers << "    let i110 = i100 + size;\n";
    helpers << "    let i111 = i110 + size * size;\n";
    helpers << "    \n";
    helpers << "    let c000 = lut[i000];\n";
    helpers << "    let c001 = lut[i001];\n";
    helpers << "    let c010 = lut[i010];\n";
    helpers << "    let c011 = lut[i011];\n";
    helpers << "    let c100 = lut[i100];\n";
    helpers << "    let c101 = lut[i101];\n";
    helpers << "    let c110 = lut[i110];\n";
    helpers << "    let c111 = lut[i111];\n";
    helpers << "    \n";
    helpers << "    // Interpolate\n";
    helpers << "    let c00 = mix(c000, c100, fract.x);\n";
    helpers << "    let c01 = mix(c001, c101, fract.x);\n";
    helpers << "    let c10 = mix(c010, c110, fract.x);\n";
    helpers << "    let c11 = mix(c011, c111, fract.x);\n";
    helpers << "    \n";
    helpers << "    let c0 = mix(c00, c10, fract.y);\n";
    helpers << "    let c1 = mix(c01, c11, fract.y);\n";
    helpers << "    \n";
    helpers << "    return mix(c0, c1, fract.z);\n";
    helpers << "}\n\n";
    
    // Matrix transformation helper
    helpers << "fn matrixTransform(color: vec3<f32>, matrix: array<f32, 16>) -> vec3<f32> {\n";
    helpers << "    let m = mat4x4<f32>(\n";
    helpers << "        vec4<f32>(matrix[0], matrix[1], matrix[2], matrix[3]),\n";
    helpers << "        vec4<f32>(matrix[4], matrix[5], matrix[6], matrix[7]),\n";
    helpers << "        vec4<f32>(matrix[8], matrix[9], matrix[10], matrix[11]),\n";
    helpers << "        vec4<f32>(matrix[12], matrix[13], matrix[14], matrix[15])\n";
    helpers << "    );\n";
    helpers << "    \n";
    helpers << "    let result = m * vec4<f32>(color, 1.0);\n";
    helpers << "    return result.xyz;\n";
    helpers << "}\n\n";
    
    // Gamma correction helper
    helpers << "fn gammaCorrect(color: vec3<f32>, gamma: f32) -> vec3<f32> {\n";
    helpers << "    return pow(max(color, vec3<f32>(0.0)), vec3<f32>(gamma));\n";
    helpers << "}\n\n";
    
    // Log/Linear conversion helpers
    helpers << "fn linToLog(color: vec3<f32>, base: f32, logSlope: f32, logOffset: f32) -> vec3<f32> {\n";
    helpers << "    return log(max(color, vec3<f32>(1e-10))) / log(base) * logSlope + logOffset;\n";
    helpers << "}\n\n";
    
    helpers << "fn logToLin(color: vec3<f32>, base: f32, logSlope: f32, logOffset: f32) -> vec3<f32> {\n";
    helpers << "    return pow(vec3<f32>(base), (color - logOffset) / logSlope);\n";
    helpers << "}\n\n";
    
    return helpers.str();
}

std::map<std::string, std::string> WGSLTranslator::getGLSLToWGSLTypeMap()
{
    return {
        {"float", "f32"},
        {"vec2", "vec2<f32>"},
        {"vec3", "vec3<f32>"},
        {"vec4", "vec4<f32>"},
        {"int", "i32"},
        {"ivec2", "vec2<i32>"},
        {"ivec3", "vec3<i32>"},
        {"ivec4", "vec4<i32>"},
        {"uint", "u32"},
        {"uvec2", "vec2<u32>"},
        {"uvec3", "vec3<u32>"},
        {"uvec4", "vec4<u32>"},
        {"bool", "bool"},
        {"bvec2", "vec2<bool>"},
        {"bvec3", "vec3<bool>"},
        {"bvec4", "vec4<bool>"},
        {"mat3", "mat3x3<f32>"},
        {"mat4", "mat4x4<f32>"},
        {"sampler2D", "array<vec3<f32>>"}  // Texture -> Buffer
    };
}

std::map<std::string, std::string> WGSLTranslator::getGLSLToWGSLFunctionMap()
{
    return {
        {"pow", "pow"},
        {"sqrt", "sqrt"},
        {"log", "log"},
        {"log2", "log2"},
        {"exp", "exp"},
        {"exp2", "exp2"},
        {"sin", "sin"},
        {"cos", "cos"},
        {"tan", "tan"},
        {"asin", "asin"},
        {"acos", "acos"},
        {"atan", "atan"},
        {"floor", "floor"},
        {"ceil", "ceil"},
        {"round", "round"},
        {"min", "min"},
        {"max", "max"},
        {"clamp", "clamp"},
        {"mix", "mix"},
        {"step", "step"},
        {"smoothstep", "smoothstep"},
        {"abs", "abs"},
        {"sign", "sign"},
        {"fract", "fract"},
        {"mod", "fmod"},  // Note: WGSL uses fmod instead of mod
        {"dot", "dot"},
        {"cross", "cross"},
        {"normalize", "normalize"},
        {"length", "length"},
        {"distance", "distance"},
        {"reflect", "reflect"},
        {"texture2D", "getLUTValue"}  // Custom function
    };
}

std::string WGSLTranslator::replaceFunctionCalls(const std::string& code)
{
    std::string result = code;
    
    auto funcMap = getGLSLToWGSLFunctionMap();
    for (const auto& pair : funcMap)
    {
        if (pair.first == pair.second) continue;  // No change needed
        
        std::string pattern = "\\b" + pair.first + "\\b";
        result = std::regex_replace(result, std::regex(pattern), pair.second);
    }
    
    return result;
}

std::string WGSLTranslator::replaceDataTypes(const std::string& code)
{
    std::string result = code;
    
    auto typeMap = getGLSLToWGSLTypeMap();
    for (const auto& pair : typeMap)
    {
        if (pair.first == pair.second) continue;  // No change needed
        
        std::string pattern = "\\b" + pair.first + "\\b";
        result = std::regex_replace(result, std::regex(pattern), pair.second);
    }
    
    return result;
}

std::string WGSLTranslator::replaceTextureLookups(const std::string& code)
{
    std::string result = code;
    
    // Replace texture2D() calls with buffer array access patterns
    result = std::regex_replace(result, 
                               std::regex(R"(texture2D\s*\(\s*(\w+)\s*,\s*([^)]+)\))"),
                               "getLUT1DValue($1, $2.x, arrayLength(&$1))");
    
    return result;
}

std::string WGSLTranslator::replaceVariableDeclarations(const std::string& code)
{
    std::string result = code;
    
    // Replace GLSL variable declarations with WGSL syntax
    // GLSL: "float var = value;" -> WGSL: "let var = value;" or "var var: f32 = value;"
    result = std::regex_replace(result, 
                               std::regex(R"(\b(float|vec[234]|int|uint|bool)\s+(\w+)\s*=\s*([^;]+);)"),
                               "let $2 = $3;");
    
    return result;
}

} // namespace OCIO_NAMESPACE