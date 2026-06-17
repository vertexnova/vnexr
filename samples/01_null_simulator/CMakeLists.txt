#==============================================================================
# 01_null_simulator — null backend + layer stack (XR simulator without runtime)
#==============================================================================

add_executable(sample_01_null_simulator
    main.cpp
    exit_after_frames_layer.cpp
)

target_link_libraries(sample_01_null_simulator PRIVATE vne::xr::samples::framework)

set_target_properties(sample_01_null_simulator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
