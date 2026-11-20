include(FetchContent)
FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG v3.5.2
  GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(Catch2)

add_executable(meadows_tests
  unit/CodeGen.test.cpp
  unit/Diagnostics.test.cpp
  unit/Interpreter.test.cpp
  unit/Lexer.test.cpp
  unit/Loader.test.cpp
  unit/Parser.test.cpp
  unit/Sema.test.cpp
)
target_link_libraries(meadows_tests PRIVATE meadows_codegen Catch2::Catch2WithMain)
target_compile_definitions(meadows_tests PRIVATE
  MEADOWS_CONFORMANCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/conformance")
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
add_test(NAME conformance-native
         COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/run_conformance.sh
                 $<TARGET_FILE:Meadows> native)

find_package(Python3 COMPONENTS Interpreter)
if(Python3_FOUND)
  add_test(NAME differential-fuzz
           COMMAND ${Python3_EXECUTABLE}
                   ${CMAKE_CURRENT_SOURCE_DIR}/fuzz/differential.py
                   $<TARGET_FILE:Meadows> --programs 40)
endif()
