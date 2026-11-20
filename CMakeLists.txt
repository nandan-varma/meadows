cmake_minimum_required(VERSION 3.16)

file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" MEADOWS_VERSION)

project(Meadows
  VERSION ${MEADOWS_VERSION}
  DESCRIPTION "The Meadows programming language"
  HOMEPAGE_URL "https://github.com/nandan-varma/meadows"
  LANGUAGES C CXX
)

option(BUILD_WASM "Build the browser playground module (requires emcmake)" OFF)
option(BUILD_TESTS "Build the test suites" ON)
option(ENABLE_WERROR "Treat compiler warnings as errors" OFF)
option(ENABLE_COVERAGE "Instrument for code coverage" OFF)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

add_library(meadows_warnings INTERFACE)
target_compile_options(meadows_warnings INTERFACE
  -Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wnon-virtual-dtor
  -Woverloaded-virtual)
if(ENABLE_WERROR)
  target_compile_options(meadows_warnings INTERFACE -Werror)
endif()
if(ENABLE_COVERAGE)
  add_compile_options(--coverage -O0 -g)
  add_link_options(--coverage)
endif()

add_library(meadows_core STATIC
  src/ast/AST.cpp
  src/ast/AstPrinter.cpp
  src/driver/Frontend.cpp
  src/interpreter/BytecodeCompiler.cpp
  src/interpreter/Interpreter.cpp
  src/interpreter/Value.cpp
  src/lexer/Lexer.cpp
  src/loader/ModuleLoader.cpp
  src/parser/Parser.cpp
  src/sema/Builtins.cpp
  src/sema/SemanticAnalyzer.cpp
  src/utils/Diagnostics.cpp
  src/utils/ErrorCodes.cpp
  src/utils/PathValidation.cpp
  src/utils/SourceMap.cpp
)
target_include_directories(meadows_core PUBLIC src)
target_link_libraries(meadows_core PUBLIC meadows_warnings)
if(EMSCRIPTEN)
  target_compile_options(meadows_core PUBLIC -fwasm-exceptions)
else()
  find_package(Threads REQUIRED)
  target_link_libraries(meadows_core PUBLIC Threads::Threads)
endif()


if(BUILD_WASM)
  enable_testing()
  add_subdirectory(src/wasm)
else()
  add_subdirectory(src/codegen)
  add_subdirectory(src/main)
  if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
  endif()
endif()

set(CPACK_PACKAGE_NAME meadows)
set(CPACK_PACKAGE_VERSION ${MEADOWS_VERSION})
set(CPACK_PACKAGE_VENDOR "Nandan Varma")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")
set(CPACK_GENERATOR TGZ)
include(CPack)
