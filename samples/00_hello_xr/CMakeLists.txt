#==============================================================================
# 00_hello_xr -- library smoke test (null session, no headset)
#==============================================================================

vne_xr_add_sample_library(hello_xr
    REGISTER registerHelloXrSample
    SOURCES hello_xr_sample.cpp
    LIBRARIES vne::xr vne::logging
)

add_executable(sample_00_hello_xr main.cpp)
target_link_libraries(sample_00_hello_xr PRIVATE vne_xr_sample_hello_xr)
set_target_properties(sample_00_hello_xr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
