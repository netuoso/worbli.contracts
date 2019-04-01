add_contract(verified.token verified.token ${CMAKE_CURRENT_SOURCE_DIR}/src/verified.token.cpp)
#add_executable(verified.token.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/verified.token.cpp)
target_include_directories(verified.token.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include
   ${CMAKE_CURRENT_SOURCE_DIR}/../eosio.token/include)

set_target_properties(verified.token.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

