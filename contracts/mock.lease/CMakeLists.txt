add_contract(mock.lease mock.lease ${CMAKE_CURRENT_SOURCE_DIR}/src/mock.lease.cpp)

target_include_directories(mock.lease
   PUBLIC
   ${CMAKE_CURRENT_SOURCE_DIR}/include)

set_target_properties(mock.lease
   PROPERTIES
   RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}")

configure_file( ${CMAKE_CURRENT_SOURCE_DIR}/ricardian/mock.lease.contracts.md.in ${CMAKE_CURRENT_BINARY_DIR}/ricardian/mock.lease.contracts.md @ONLY )

target_compile_options( mock.lease PUBLIC -R${CMAKE_CURRENT_SOURCE_DIR}/ricardian -R${CMAKE_CURRENT_BINARY_DIR}/ricardian )
