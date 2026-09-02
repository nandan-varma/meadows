include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.5.2
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(Catch2)

set(TEST_SOURCES
    unit/lexer/Lexer.test.cpp
    unit/lexer/ModuleResolver.test.cpp
    unit/parser/Parser.test.cpp
    unit/codegen/CodeGen.test.cpp
    unit/sema/SemanticAnalyzer.test.cpp
    unit/interpreter/Interpreter.test.cpp
    unit/utils/Exceptions.test.cpp
    unit/utils/DiagnosticsCollector.test.cpp
    unit/utils/ErrorCodes.test.cpp
    unit/utils/Timer.test.cpp
    unit/utils/WarningManager.test.cpp
)

add_executable(meadows_tests ${TEST_SOURCES})

target_include_directories(meadows_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(meadows_tests PRIVATE
    meadows_lib
    Catch2::Catch2WithMain
    ${LLVM_LIBS}
)

set_target_properties(meadows_tests PROPERTIES
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
)

include(CTest)
include(Catch)
catch_discover_tests(meadows_tests)
