add_contract(playground playground ${CMAKE_CURRENT_SOURCE_DIR}/src/playground.cpp)
#add_executable(worbli.timelock.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/playground.cpp)
target_include_directories(playground.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(playground.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

