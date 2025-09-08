# SPDX-License-Identifier: BSD-3-Clause
# Copyright Contributors to the OpenColorIO Project.

# CMake toolchain file for Emscripten WebAssembly builds

set(CMAKE_SYSTEM_NAME Emscripten)
set(CMAKE_SYSTEM_VERSION 1)

# Find Emscripten compiler
find_program(CMAKE_C_COMPILER emcc)
find_program(CMAKE_CXX_COMPILER em++)
find_program(CMAKE_AR emar)
find_program(CMAKE_RANLIB emranlib)

if(NOT CMAKE_C_COMPILER)
    message(FATAL_ERROR "emcc not found. Please install Emscripten SDK.")
endif()

if(NOT CMAKE_CXX_COMPILER)
    message(FATAL_ERROR "em++ not found. Please install Emscripten SDK.")
endif()

# Set the CMAKE_FIND_ROOT_PATH_MODE variables to ensure that CMake
# looks for libraries and includes in the Emscripten system directories
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# WASM-specific compiler flags
set(EMSCRIPTEN_COMMON_FLAGS 
    "-sWASM=1"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sNO_EXIT_RUNTIME=1"
    "-sMODULARIZE=1"
    "-sEXPORT_NAME='OpenColorIO'"
)

# WebAssembly SIMD flags (optional, can be disabled via OCIO_WASM_SIMD=OFF)
option(OCIO_WASM_SIMD "Enable WebAssembly SIMD optimizations" ON)
if(OCIO_WASM_SIMD)
    list(APPEND EMSCRIPTEN_COMMON_FLAGS "-msimd128")
    message(STATUS "WebAssembly SIMD enabled (-msimd128)")
else()
    message(STATUS "WebAssembly SIMD disabled")
endif()

# Optimization flags
set(EMSCRIPTEN_RELEASE_FLAGS
    "-O3"
    "-flto"
    "-sASSERTIONS=0"
)

set(EMSCRIPTEN_DEBUG_FLAGS
    "-O0"
    "-g"
    "-sASSERTIONS=1"
    "-sSAFE_HEAP=1"
)

# Join flags into strings
string(REPLACE ";" " " EMSCRIPTEN_COMMON_FLAGS_STR "${EMSCRIPTEN_COMMON_FLAGS}")
string(REPLACE ";" " " EMSCRIPTEN_RELEASE_FLAGS_STR "${EMSCRIPTEN_RELEASE_FLAGS}")
string(REPLACE ";" " " EMSCRIPTEN_DEBUG_FLAGS_STR "${EMSCRIPTEN_DEBUG_FLAGS}")

# Set compiler flags
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EMSCRIPTEN_COMMON_FLAGS_STR}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${EMSCRIPTEN_COMMON_FLAGS_STR}")

set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} ${EMSCRIPTEN_RELEASE_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${EMSCRIPTEN_RELEASE_FLAGS_STR}")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} ${EMSCRIPTEN_DEBUG_FLAGS_STR}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${EMSCRIPTEN_DEBUG_FLAGS_STR}")

# Set linker flags
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${EMSCRIPTEN_COMMON_FLAGS_STR}")
set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} ${EMSCRIPTEN_RELEASE_FLAGS_STR}")
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${EMSCRIPTEN_DEBUG_FLAGS_STR}")

# Disable unsupported features
set(CMAKE_EXECUTABLE_SUFFIX ".js")