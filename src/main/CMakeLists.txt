include(FetchContent)
FetchContent_Declare(
  CLI11
  GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
  GIT_TAG v2.4.2
  GIT_SHALLOW TRUE
)
set(CLI11_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(CLI11_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(CLI11)

add_executable(Meadows main.cpp)
target_link_libraries(Meadows PRIVATE meadows_core CLI11::CLI11)
target_compile_definitions(Meadows PRIVATE MEADOWS_VERSION="${MEADOWS_VERSION}")
set_target_properties(Meadows PROPERTIES RUNTIME_OUTPUT_DIRECTORY
                      ${CMAKE_BINARY_DIR}/bin)
install(TARGETS Meadows RUNTIME DESTINATION bin)
