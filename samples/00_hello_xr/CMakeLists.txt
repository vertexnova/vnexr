#==============================================================================
# 00_hello_xr — minimal library smoke test
#==============================================================================

add_executable(sample_00_hello_xr main.cpp)

target_link_libraries(sample_00_hello_xr PRIVATE vne::xr vne::logging)

set_target_properties(sample_00_hello_xr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
