#==============================================================================
# 01_null_simulator -- null backend + layer stack (XR simulator without runtime)
#==============================================================================

vne_xr_add_sample_library(null_simulator
    REGISTER registerNullSimulatorSample
    SOURCES null_simulator_sample.cpp
)

add_executable(sample_01_null_simulator main.cpp)
target_link_libraries(sample_01_null_simulator PRIVATE vne_xr_sample_null_simulator)
set_target_properties(sample_01_null_simulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
