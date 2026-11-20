include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(meadows_tests
  unit/Diagnostics.test.cpp
  unit/Interpreter.test.cpp
  unit/Lexer.test.cpp
  unit/Loader.test.cpp
  unit/Parser.test.cpp
  unit/Sema.test.cpp
)
target_link_libraries(meadows_tests PRIVATE meadows_core Catch2::Catch2WithMain)
target_include_directories(meadows_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
set_target_properties(meadows_tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                      ${CMAKE_BINARY_DIR}/tests)

include(CTest)
include(Catch)
catch_discover_tests(meadows_tests)

add_test(NAME conformance-interpreter
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_conformance.sh
                 $<TARGET_FILE:Meadows> interpret)
add_test(NAME conformance-diagnostics
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_conformance.sh
                 $<TARGET_FILE:Meadows> diagnostics)
