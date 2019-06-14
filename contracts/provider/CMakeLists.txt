add_contract(provider provider ${CMAKE_CURRENT_SOURCE_DIR}/src/provider.cpp)

target_include_directories(provider
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(provider
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

