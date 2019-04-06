add_contract(worbli.controller worbli.controller ${CMAKE_CURRENT_SOURCE_DIR}/src/worbli.controller.cpp)
#add_executable(worbli.controller.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/worbli.controller.cpp)
target_include_directories(worbli.controller.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)

set_target_properties(worbli.controller.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

