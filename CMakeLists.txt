cmake_minimum_required(VERSION 3.10)
project(meadows)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Add source files
file(GLOB LEXER_SOURCES "src/lexer/*.cpp")
file(GLOB PARSER_SOURCES "src/parser/*.cpp")
file(GLOB AST_SOURCES "src/ast/*.cpp")
file(GLOB CODEGEN_SOURCES "src/codegen/*.cpp")
file(GLOB MAIN_SOURCES "src/main/*.cpp")

add_executable(meadows ${LEXER_SOURCES} ${PARSER_SOURCES} ${AST_SOURCES} ${CODEGEN_SOURCES} ${MAIN_SOURCES})

llvm_map_components_to_libnames(llvm_libs support core irreader)
target_link_libraries(meadows ${llvm_libs})