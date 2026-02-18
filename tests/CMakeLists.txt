# Tests CMakeLists.txt

# Download Catch2 if not present
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.hpp")
    message(STATUS "Downloading Catch2...")
    file(DOWNLOAD
        https://github.com/catchorg/Catch2/releases/download/v3.5.2/catch_amalgamated.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.hpp
        EXPECTED_HASH SHA256=aae5b5a8b83e7d3f33d9bf3c902a5a396b4195c390881ea5557e935cdac49d38
    )
    file(DOWNLOAD
        https://github.com/catchorg/Catch2/releases/download/v3.5.2/catch_amalgamated.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.cpp
        EXPECTED_HASH SHA256=6e0d09fec95cb32a9b75574b2a8f45ea1f430bb5fbbd4df1c91d2d2f3f2e5b7c
    )
endif()

# Create Catch2 library
add_library(Catch2 STATIC catch_amalgamated.cpp)
target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(Catch2 PRIVATE -Wno-sign-conversion)

# Collect test source files
set(TEST_SOURCES
    unit/lexer/Lexer.test.cpp
    unit/parser/Parser.test.cpp
    unit/codegen/CodeGen.test.cpp
    unit/codegen/SymbolTable.test.cpp
    unit/codegen/StringUtils.test.cpp
    unit/utils/ErrorCodes.test.cpp
    unit/utils/Timer.test.cpp
    unit/utils/WarningManager.test.cpp
    unit/utils/MemoryUtils.test.cpp
    unit/config/Config.test.cpp
    unit/config/TOMLParser.test.cpp
    unit/modules/ModuleCompiler.test.cpp
    unit/modules/ModuleResolver.test.cpp
    unit/stdlib/Stdlib.test.cpp
    unit/ast/AST.test.cpp
    unit/types/TypeChecker.test.cpp
    unit/types/Types.test.cpp
)

# Create test executable
add_executable(meadows_tests ${TEST_SOURCES})

# Exclude Catch2 from -Wdouble-promotion (third-party code)
set_source_files_properties(catch_amalgamated.cpp
                          PROPERTIES COMPILE_FLAGS "-Wno-double-promotion")

# Include directories
target_include_directories(meadows_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Link with object library and Catch2
target_link_libraries(meadows_tests PRIVATE meadows_lib Catch2 ${LLVM_LIBS})

# Set C++ standard
set_target_properties(meadows_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

# Register tests with CTest
enable_testing()
add_test(NAME meadows_unit_tests COMMAND meadows_tests)

# Add verbose output option
set_property(TEST meadows_unit_tests PROPERTY ENVIRONMENT "CATCH_CONFIG_COLOR=yes")
