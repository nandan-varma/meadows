# ==============================================================================
# Meadows Compiler - CMake Configuration
# ==============================================================================

cmake_minimum_required(VERSION 3.15)
project(MeadowsCompiler LANGUAGES C CXX)

# ==============================================================================
# Project Settings
# ==============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ==============================================================================
# LLVM Configuration
# ==============================================================================

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# LLVM definitions and includes
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

# ==============================================================================
# Source Files
# ==============================================================================

include_directories(src)

# Library sources (exclude main.cpp and test_runner.cpp)
file(GLOB_RECURSE LIB_SOURCES 
    "src/lexer/*.cpp"
    "src/parser/*.cpp" 
    "src/ast/*.cpp"
    "src/visitor/*.cpp"
    "src/common/*.cpp"
    "src/testing/*.cpp"
    "src/Compiler.cpp"
)

file(GLOB_RECURSE HEADERS "src/*.h")

# ==============================================================================
# Library Target
# ==============================================================================

add_library(meadows_lib ${LIB_SOURCES} ${HEADERS})
target_include_directories(meadows_lib PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(meadows_lib PRIVATE ${LLVM_DEFINITIONS})

# ==============================================================================
# Executable Targets
# ==============================================================================

# Main compiler executable
add_executable(meadows src/main.cpp)
target_link_libraries(meadows meadows_lib ${LLVM_LIBS})

# Test runner executable
add_executable(meadows_test src/test_runner.cpp)
target_link_libraries(meadows_test meadows_lib ${LLVM_LIBS})

# Target-specific LLVM configuration
target_include_directories(meadows PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(meadows PRIVATE ${LLVM_DEFINITIONS})

target_include_directories(meadows_test PRIVATE ${LLVM_INCLUDE_DIRS})
target_compile_definitions(meadows_test PRIVATE ${LLVM_DEFINITIONS})

# ==============================================================================
# LLVM Libraries
# ==============================================================================

llvm_map_components_to_libnames(LLVM_LIBS 
    Core
    ExecutionEngine
    MC
    Support
    Target
    Analysis
    Object
    native
)

target_link_libraries(meadows_lib ${LLVM_LIBS})

# ==============================================================================
# Platform-Specific Configuration
# ==============================================================================

if(APPLE)
    target_compile_options(meadows PRIVATE -stdlib=libc++)
    target_link_options(meadows PRIVATE -stdlib=libc++)
    target_compile_options(meadows_test PRIVATE -stdlib=libc++)
    target_link_options(meadows_test PRIVATE -stdlib=libc++)
    target_compile_options(meadows_lib PRIVATE -stdlib=libc++)
endif()

# ==============================================================================
# Compiler-Specific Options
# ==============================================================================

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(meadows PRIVATE 
        -Wall 
        -Wextra 
        -Wno-unused-parameter
        -Wno-unused-variable
    )
    target_compile_options(meadows_test PRIVATE 
        -Wall 
        -Wextra 
        -Wno-unused-parameter
        -Wno-unused-variable
    )
    target_compile_options(meadows_lib PRIVATE 
        -Wall 
        -Wextra 
        -Wno-unused-parameter
        -Wno-unused-variable
    )
endif()

# ==============================================================================
# WebAssembly Build Support
# ==============================================================================

option(BUILD_WASM "Build WebAssembly version" OFF)

if(BUILD_WASM)
    message(STATUS "Building WebAssembly version")
    
    # Check for Emscripten
    if(NOT DEFINED ENV{EMSCRIPTEN})
        message(FATAL_ERROR "EMSCRIPTEN environment variable not set. Please install and activate Emscripten SDK.")
    endif()
    
    # WebAssembly-specific sources
    file(GLOB_RECURSE WASM_SOURCES 
        "src/lexer/*.cpp"
        "src/parser/*.cpp" 
        "src/ast/*.cpp"
        "src/visitor/*.cpp"
        "src/common/*.cpp"
        "src/web/*.cpp"
        "src/Compiler.cpp"
    )
    
    # Create WebAssembly library
    add_executable(meadows_wasm ${WASM_SOURCES})
    
    # WebAssembly-specific settings
    target_include_directories(meadows_wasm PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(meadows_wasm PRIVATE ${LLVM_DEFINITIONS})
    target_link_libraries(meadows_wasm ${LLVM_LIBS})
    
    # Emscripten-specific flags
    set_target_properties(meadows_wasm PROPERTIES
        COMPILE_FLAGS "-s USE_PTHREADS=0 -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1"
        LINK_FLAGS "-s USE_PTHREADS=0 -s DISABLE_EXCEPTION_CATCHING=0 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=['ccall','cwrap'] -s MODULARIZE=1 -s EXPORT_NAME='MeadowsModule' --bind --pre-js ${CMAKE_SOURCE_DIR}/web/pre.js"
    )
    
    # Set output directory for web files
    set_target_properties(meadows_wasm PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/web"
        OUTPUT_NAME "meadows"
    )
    
    # Custom command to copy additional web files
    add_custom_command(TARGET meadows_wasm POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/web/index.html"
        "${CMAKE_SOURCE_DIR}/web/styles.css"
        "${CMAKE_SOURCE_DIR}/web/app.js"
        "${CMAKE_BINARY_DIR}/"
        COMMENT "Copying web files to build directory"
    )
endif()

# ==============================================================================
# Build Information
# ==============================================================================

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files found: ${SOURCES}")
if(BUILD_WASM)
    message(STATUS "WebAssembly build enabled")
endif()
