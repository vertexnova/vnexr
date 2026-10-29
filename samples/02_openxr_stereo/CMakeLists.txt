#==============================================================================
# 02_openxr_stereo -- OpenXR layer-based stereo cube (Linux / Windows / Android)
#==============================================================================

vne_xr_add_sample_library(stereo_cube
    REGISTER registerStereoCubeSample
    SOURCES
        stereo_cube_sample.cpp
        stereo_cube_scene_layer.cpp
    LIBRARIES vne::rhi vne::math
)

# Android runs this sample through the vnexr_samples launcher; desktop also gets a standalone executable.
if(NOT ANDROID)
    add_executable(sample_02_openxr_stereo main.cpp)
    target_link_libraries(sample_02_openxr_stereo PRIVATE vne_xr_sample_stereo_cube)
    set_target_properties(sample_02_openxr_stereo PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
    )
endif()
