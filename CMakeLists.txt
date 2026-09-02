#==============================================================================
# Meadows Compiler - CMake Build Configuration
#==============================================================================

cmake_minimum_required(VERSION 3.16)
include(FetchContent)

#==============================================================================
# CMake Options
#==============================================================================
# BUILD_WASM builds only the browser-playground bridge (lexer, parser,
# semantic analysis) via Emscripten embind — no LLVM, no CLI11, no native
# executable. Configure with `emcmake cmake -DBUILD_WASM=ON`.
option(BUILD_WASM "Build the browser playground WASM module (requires emcmake)" OFF)
option(BUILD_TESTS "Build tests" ON)
option(BUILD_EXAMPLES "Build examples" ON)
option(ENABLE_COVERAGE "Enable code coverage" OFF)
option(ENABLE_WERROR "Treat warnings as errors" OFF)

if(NOT BUILD_WASM)
  FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG        v2.4.2
    GIT_SHALLOW    TRUE
  )
  set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
  set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(CLI11)
endif()

#==============================================================================
# Project Declaration
#==============================================================================
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" PROJECT_VERSION)

project(Meadows
  VERSION ${PROJECT_VERSION}
  DESCRIPTION "A simple compiled programming language"
  HOMEPAGE_URL "https://github.com/nandan-varma/meadows"
  LANGUAGES C CXX
)

#==============================================================================
# C++ Standard
#==============================================================================
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#==============================================================================
# Compiler Warnings
#==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(
    -Wall
    -Wextra
    -Wpedantic
    -Wshadow
    -Wnon-virtual-dtor
    -Wcast-align
    -Wunused
    -Woverloaded-virtual
    -Wconversion
    -Wsign-conversion
    -Wdouble-promotion
    -Wformat=2
    -Wno-system-headers
  )
  
  if(ENABLE_WERROR)
    add_compile_options(-Werror)
  endif()
endif()

#==============================================================================
# Security Hardening
#==============================================================================
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  add_compile_options(
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
  )

  if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_link_options(
      -Wl,-z,relro
      -Wl,-z,now
    )
  endif()

  if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    add_compile_options(-fPIE)
    add_link_options(-pie)
  endif()
endif()

#==============================================================================
# WebAssembly Playground Build
#==============================================================================
if(BUILD_WASM)
  if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "BUILD_WASM requires the Emscripten toolchain: configure with 'emcmake cmake -DBUILD_WASM=ON ...'")
  endif()

  add_executable(meadows_wasm
    src/wasm/WasmBridge.cpp
    src/lexer/Lexer.cpp
    src/parser/Parser.cpp
    src/parser/ParserExpressions.cpp
    src/ast/AST.cpp
    src/sema/SemanticAnalyzer.cpp
    src/interpreter/Value.cpp
    src/interpreter/Interpreter.cpp
    src/utils/ErrorCodes.cpp
    src/utils/Exceptions.cpp
    src/utils/DiagnosticsCollector.cpp
    src/utils/ErrorFormatter.cpp
    src/utils/WarningManager.cpp
  )

  target_include_directories(meadows_wasm PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

  # The Interpreter uses C++ exceptions for control flow (return/break/continue
  # signals and runtime errors) — Emscripten disables exception unwinding by
  # default, so it must be turned on explicitly, consistently at compile and
  # link time. -fwasm-exceptions uses the native Wasm EH proposal rather than
  # the older, slower JS-based unwinding emulation.
  target_compile_options(meadows_wasm PRIVATE "-fwasm-exceptions")

  target_link_options(meadows_wasm PRIVATE
    "-fwasm-exceptions"
    "--bind"
    "-sMODULARIZE=1"
    "-sEXPORT_ES6=1"
    "-sEXPORT_NAME=createMeadowsModule"
    "-sENVIRONMENT=web"
    "-sALLOW_MEMORY_GROWTH=1"
    "-sFILESYSTEM=1"
    # Generous headroom for the Interpreter's C++-recursion-based call stack
    # (several frames per Meadows function call) — paired with
    # Interpreter::kMaxCallDepth, which must trip well before this fills.
    "-sSTACK_SIZE=16777216"
  )

  set_target_properties(meadows_wasm PROPERTIES
    OUTPUT_NAME "meadows"
    SUFFIX ".js"
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
  )

else()

#==============================================================================
# LLVM Configuration
#==============================================================================

if(NOT LLVM_DIR)
  if(APPLE)
    if(EXISTS "/opt/homebrew/opt/llvm@17/lib/cmake/llvm")
      set(LLVM_DIR "/opt/homebrew/opt/llvm@17/lib/cmake/llvm")
    elseif(EXISTS "/usr/local/opt/llvm/lib/cmake/llvm")
      set(LLVM_DIR "/usr/local/opt/llvm/lib/cmake/llvm")
    endif()
  elseif(UNIX)
    if(EXISTS "/usr/lib/llvm-17/lib/cmake/llvm")
      set(LLVM_DIR "/usr/lib/llvm-17/lib/cmake/llvm")
    elseif(EXISTS "/usr/lib/llvm-16/lib/cmake/llvm")
      set(LLVM_DIR "/usr/lib/llvm-16/lib/cmake/llvm")
    endif()
  endif()
endif()

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Map LLVM components to libraries
llvm_map_components_to_libnames(LLVM_LIBS
  support
  core
  irreader
  native
  nativecodegen
  passes
)

#==============================================================================
# Source Files
#==============================================================================
set(MEADOWS_LIB_SOURCES
  src/lexer/Lexer.cpp
  src/lexer/ModuleResolver.cpp
  src/parser/Parser.cpp
  src/parser/ParserExpressions.cpp
  src/ast/AST.cpp
  src/codegen/CodeGen.cpp
  src/codegen/StringUtils.cpp
  src/codegen/SymbolTable.cpp
  src/sema/SemanticAnalyzer.cpp
  src/interpreter/Value.cpp
  src/interpreter/Interpreter.cpp
  src/lsp/LSPInterface.cpp
  src/utils/ErrorCodes.cpp
  src/utils/Exceptions.cpp
  src/utils/DiagnosticsCollector.cpp
  src/utils/ErrorFormatter.cpp
  src/utils/PathValidation.cpp
  src/utils/WarningManager.cpp
)

set(MEADOWS_HEADERS
  src/lexer/Lexer.h
  src/lexer/ModuleResolver.h
  src/lexer/Token.h
  src/parser/Parser.h
  src/ast/AST.h
  src/codegen/CodeGen.h
  src/codegen/SymbolTable.h
  src/codegen/StringUtils.h
  src/codegen/TypeUtils.h
  src/interpreter/Value.h
  src/interpreter/Interpreter.h
  src/lsp/LSPInterface.h
  src/utils/PathValidation.h
  src/utils/ErrorCodes.h
  src/utils/Exceptions.h
  src/utils/DiagnosticsCollector.h
  src/utils/ErrorFormatter.h
  src/utils/Timer.h
  src/utils/ASTPrinter.h
  src/utils/WarningManager.h
  src/utils/MemoryUtils.h
)

#==============================================================================
# Object Library (shared between main executable and tests)
#==============================================================================
add_library(meadows_lib OBJECT ${MEADOWS_LIB_SOURCES})

target_link_libraries(meadows_lib
  PRIVATE
    ${LLVM_LIBS}
)

target_include_directories(meadows_lib
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${LLVM_INCLUDE_DIRS}
)

#==============================================================================
# Main Executable
#==============================================================================
add_executable(${PROJECT_NAME} src/main/meadows.cpp)

target_link_libraries(${PROJECT_NAME}
  PRIVATE
    meadows_lib
    CLI11::CLI11
    ${LLVM_LIBS}
)

target_compile_definitions(${PROJECT_NAME}
  PRIVATE MEADOWS_VERSION="${PROJECT_VERSION}"
)

# Set output directory.
# On Linux, embed RPATH so the binary finds LLVM shared libs without LD_LIBRARY_PATH.
# Skip macOS: embedding LLVM's lib dir as RPATH causes dyld to load LLVM's
# libc++.dylib before the system one, producing symbol conflicts.
set_target_properties(${PROJECT_NAME} PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set_target_properties(${PROJECT_NAME} PROPERTIES
    BUILD_RPATH  "${LLVM_LIBRARY_DIRS}"
    INSTALL_RPATH "${LLVM_LIBRARY_DIRS}"
    INSTALL_RPATH_USE_LINK_PATH TRUE
  )
endif()

#==============================================================================
# Installation
#==============================================================================
include(GNUInstallDirs)

install(TARGETS ${PROJECT_NAME}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

install(FILES ${MEADOWS_HEADERS}
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/meadows
)

#==============================================================================
# Tests
#==============================================================================
if(BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
endif()

#==============================================================================
# Examples
#==============================================================================
if(BUILD_EXAMPLES)
  # Copy example files to build directory
  file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/examples
    DESTINATION ${CMAKE_BINARY_DIR}
  )
endif()

#==============================================================================
# Coverage
#==============================================================================
if(ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

#==============================================================================
# CPack Configuration
#==============================================================================
set(CPACK_PACKAGE_NAME "meadows")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Meadows Compiler")
set(CPACK_PACKAGE_VENDOR "Meadows Contributors")
set(CPACK_PACKAGE_CONTACT "https://github.com/nandan-varma/meadows/issues")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

# Platform-specific package generators
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  set(CPACK_GENERATOR "TGZ;DEB;RPM")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  set(CPACK_GENERATOR "TGZ;DragNDrop")
elseif(CMAKE_SYSTEM_NAME STREQUAL "Windows")
  set(CPACK_GENERATOR "ZIP;NSIS")
else()
  set(CPACK_GENERATOR "TGZ")
endif()

include(CPack)

#==============================================================================
# Print Configuration Summary
#==============================================================================
message("")
message("=== Meadows Compiler Configuration ===")
message("Version: ${PROJECT_VERSION}")
message("Build Type: ${CMAKE_BUILD_TYPE}")
message("C++ Standard: ${CMAKE_CXX_STANDARD}")
message("Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message("")
message("Options:")
message("  Build Tests: ${BUILD_TESTS}")
message("  Build Examples: ${BUILD_EXAMPLES}")
message("  Enable Coverage: ${ENABLE_COVERAGE}")
message("  Treat Warnings as Errors: ${ENABLE_WERROR}")
message("")
message("LLVM:")
message("  Version: ${LLVM_PACKAGE_VERSION}")
message("  Directory: ${LLVM_DIR}")
message("=====================================")
message("")

endif() # BUILD_WASM
