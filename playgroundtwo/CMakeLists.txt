add_contract(playgroundtwo playgroundtwo ${CMAKE_CURRENT_SOURCE_DIR}/src/playgroundtwo.cpp)
target_include_directories(playgroundtwo.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(playgroundtwo.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

