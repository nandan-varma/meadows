add_executable(meadows_wasm WasmBridge.cpp)
target_link_libraries(meadows_wasm PRIVATE meadows_core)
target_compile_definitions(meadows_wasm PRIVATE
  MEADOWS_VERSION="${MEADOWS_VERSION}")
target_link_options(meadows_wasm PRIVATE
  -fwasm-exceptions
  --bind
  -sMODULARIZE=1
  -sEXPORT_ES6=1
  -sEXPORT_NAME=createMeadows
  -sENVIRONMENT=web,worker,node
  -sALLOW_MEMORY_GROWTH=1
  -sSTACK_SIZE=1048576
  -sFORCE_FILESYSTEM=1
  -sEXPORTED_RUNTIME_METHODS=FS
)
set_target_properties(meadows_wasm PROPERTIES
  OUTPUT_NAME meadows
  SUFFIX .js
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

find_program(NODE node)
if(NODE)
  add_test(NAME conformance-wasm
           COMMAND ${NODE} ${CMAKE_SOURCE_DIR}/tests/wasm/conformance.mjs
                   $<TARGET_FILE:meadows_wasm>)
endif()
