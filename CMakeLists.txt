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
# Build Information
# ==============================================================================

message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "Source files found: ${SOURCES}")
