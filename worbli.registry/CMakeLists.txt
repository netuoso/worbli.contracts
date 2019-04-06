add_contract(worbli.registry worbli.registry ${CMAKE_CURRENT_SOURCE_DIR}/src/worbli.registry.cpp)
#add_executable(worbli.registry.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/worbli.registry.cpp)
target_include_directories(worbli.registry.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)

set_target_properties(worbli.registry.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

