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
# Compiler Configuration
# ==============================================================================

# Force system clang++ on macOS to avoid header conflicts with Homebrew LLVM
if(APPLE AND NOT DEFINED CMAKE_CXX_COMPILER)
    set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
    set(CMAKE_C_COMPILER "/usr/bin/clang")
    message(STATUS "Forcing system compiler on macOS: ${CMAKE_CXX_COMPILER}")
endif()

# ==============================================================================
# LLVM Configuration
# ==============================================================================

if(NOT SKIP_LLVM_FIND)
    find_package(LLVM REQUIRED CONFIG)
    message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
    message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

    # LLVM definitions and includes
    add_definitions(${LLVM_DEFINITIONS})
    include_directories(${LLVM_INCLUDE_DIRS})
else()
    message(STATUS "Skipping LLVM find_package as requested")
    set(LLVM_DEFINITIONS "")
    set(LLVM_INCLUDE_DIRS "")
    set(LLVM_LIBS "")
endif()

# ==============================================================================
# Build Type Configuration
# ==============================================================================

# Set default build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug" CACHE STRING 
        "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel" FORCE)
endif()

# Release build optimizations
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Configuring Release build with optimizations")
    
    # Optimization flags for all targets
    set(RELEASE_COMPILE_OPTIONS
        -O3                    # Maximum optimization
        -DNDEBUG              # Disable debug assertions
        -march=native         # Optimize for current CPU architecture
        -flto                 # Link-time optimization
        -ffunction-sections   # Separate functions for better optimization
        -fdata-sections       # Separate data sections
    )
    
    set(RELEASE_LINK_OPTIONS
        -O3                   # Optimization at link time
        -flto                 # Link-time optimization
    )
    
    # Platform-specific link options
    if(APPLE)
        list(APPEND RELEASE_LINK_OPTIONS
            -Wl,-dead_strip   # Remove unused code (macOS equivalent of --gc-sections)
        )
    else()
        list(APPEND RELEASE_LINK_OPTIONS
            -Wl,--gc-sections # Remove unused sections (Linux/Unix)
            -s                # Strip symbols for smaller binary
        )
    endif()
    
    # Apply optimization flags to all targets
    add_compile_options(${RELEASE_COMPILE_OPTIONS})
    add_link_options(${RELEASE_LINK_OPTIONS})
endif()

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
if(LLVM_INCLUDE_DIRS)
    target_include_directories(meadows_lib PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(meadows_lib PRIVATE ${LLVM_DEFINITIONS})
endif()

# ==============================================================================
# Executable Targets
# ==============================================================================

# Main compiler executable
add_executable(meadows src/main.cpp)
target_link_libraries(meadows meadows_lib)
if(LLVM_LIBS)
    target_link_libraries(meadows ${LLVM_LIBS})
endif()

# Test runner executable
add_executable(meadows_test src/test_runner.cpp)
target_link_libraries(meadows_test meadows_lib)
if(LLVM_LIBS)
    target_link_libraries(meadows_test ${LLVM_LIBS})
endif()

# Target-specific LLVM configuration
if(LLVM_INCLUDE_DIRS)
    target_include_directories(meadows PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(meadows PRIVATE ${LLVM_DEFINITIONS})

    target_include_directories(meadows_test PRIVATE ${LLVM_INCLUDE_DIRS})
    target_compile_definitions(meadows_test PRIVATE ${LLVM_DEFINITIONS})
endif()

# ==============================================================================
# LLVM Libraries
# ==============================================================================

if(NOT SKIP_LLVM_FIND)
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
endif()

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

message(STATUS "==========================================")
message(STATUS "Meadows Compiler Build Configuration")
message(STATUS "==========================================")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Optimizations: Enabled (O3, LTO, native arch)")
endif()
if(LLVM_PACKAGE_VERSION)
    message(STATUS "LLVM Version: ${LLVM_PACKAGE_VERSION}")
endif()
message(STATUS "==========================================")

# ==============================================================================
# Custom Targets
# ==============================================================================

# Release build target
add_custom_target(release
    COMMAND ${CMAKE_COMMAND} -DCMAKE_BUILD_TYPE=Release -B build-release -S .
    COMMAND ${CMAKE_COMMAND} --build build-release --parallel
    COMMENT "Building optimized release version"
    VERBATIM
)

# Clean release build
add_custom_target(clean-release
    COMMAND ${CMAKE_COMMAND} -E remove_directory build-release
    COMMENT "Cleaning release build directory"
)

# Install target (optional)
install(TARGETS meadows 
    RUNTIME DESTINATION bin
    COMPONENT runtime
)
install(TARGETS meadows_test
    RUNTIME DESTINATION bin
    COMPONENT testing
)
