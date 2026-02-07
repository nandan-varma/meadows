# Tests CMakeLists.txt

# Download Catch2 if not present
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.hpp")
    message(STATUS "Downloading Catch2...")
    file(DOWNLOAD 
        https://github.com/catchorg/Catch2/releases/download/v3.5.2/catch_amalgamated.hpp
        ${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.hpp
    )
    file(DOWNLOAD 
        https://github.com/catchorg/Catch2/releases/download/v3.5.2/catch_amalgamated.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/catch_amalgamated.cpp
    )
endif()

# Create Catch2 library
add_library(Catch2 STATIC catch_amalgamated.cpp)
target_include_directories(Catch2 PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Collect test source files
set(TEST_SOURCES
    unit/lexer/Lexer.test.cpp
    unit/parser/Parser.test.cpp
)

# Create test executable
add_executable(meadows_tests ${TEST_SOURCES})

# Include directories
target_include_directories(meadows_tests PRIVATE 
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/tests
)

# Link with source files and Catch2
target_sources(meadows_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src/lexer/Lexer.cpp
    ${CMAKE_SOURCE_DIR}/src/lexer/Token.h
    ${CMAKE_SOURCE_DIR}/src/parser/Parser.cpp
    ${CMAKE_SOURCE_DIR}/src/ast/AST.cpp
)

target_link_libraries(meadows_tests PRIVATE Catch2)

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
