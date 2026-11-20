if(NOT LLVM_DIR)
  foreach(candidate /opt/homebrew/opt/llvm@17 /usr/local/opt/llvm@17
                    /usr/lib/llvm-17)
    if(EXISTS ${candidate}/lib/cmake/llvm)
      set(LLVM_DIR ${candidate}/lib/cmake/llvm)
      break()
    endif()
  endforeach()
endif()

find_package(LLVM 17 REQUIRED CONFIG)
message(STATUS "Using LLVM ${LLVM_PACKAGE_VERSION} from ${LLVM_DIR}")
llvm_map_components_to_libnames(MEADOWS_LLVM_LIBS core support)

set(RUNTIME_DIR ${CMAKE_SOURCE_DIR}/src/runtime)
foreach(name Limits.h meadows_rt.h meadows_rt.c)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS
               ${RUNTIME_DIR}/${name})
endforeach()
file(READ ${RUNTIME_DIR}/Limits.h LIMITS_H)
file(READ ${RUNTIME_DIR}/meadows_rt.h RUNTIME_H)
file(READ ${RUNTIME_DIR}/meadows_rt.c RUNTIME_C)
configure_file(RuntimeSources.cpp.in
               ${CMAKE_CURRENT_BINARY_DIR}/RuntimeSources.cpp @ONLY)

add_library(meadows_codegen STATIC
  CodeGen.cpp
  NativeCompiler.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/RuntimeSources.cpp
)
target_include_directories(meadows_codegen PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(meadows_codegen SYSTEM PUBLIC ${LLVM_INCLUDE_DIRS})
separate_arguments(MEADOWS_LLVM_DEFINITIONS UNIX_COMMAND "${LLVM_DEFINITIONS}")
target_compile_definitions(meadows_codegen PUBLIC ${MEADOWS_LLVM_DEFINITIONS})
target_link_libraries(meadows_codegen PUBLIC meadows_core ${MEADOWS_LLVM_LIBS})

