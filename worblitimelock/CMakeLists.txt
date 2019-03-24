add_contract(worblitimelock worblitimelock ${CMAKE_CURRENT_SOURCE_DIR}/src/worblitimelock.cpp)
#add_executable(worbli.timelock.wasm ${CMAKE_CURRENT_SOURCE_DIR}/src/worblitimelock.cpp)
target_include_directories(worblitimelock.wasm
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(worblitimelock.wasm
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

