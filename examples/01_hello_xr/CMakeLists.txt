#==============================================================================
# 01_hello_xr Example: minimal vnexr usage (hello, get_version)
#==============================================================================

add_executable(example_01_hello_xr main.cpp)

target_link_libraries(example_01_hello_xr PRIVATE vne::xr vne::logging)

target_include_directories(example_01_hello_xr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_01_hello_xr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
