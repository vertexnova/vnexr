#==============================================================================
# 02_openxr_stereo — OpenXR layer-based stereo cube (Linux / Windows / Android)
#==============================================================================

add_executable(sample_02_openxr_stereo
    main.cpp
    stereo_cube_scene_layer.cpp
)

target_link_libraries(sample_02_openxr_stereo PRIVATE vne::xr::samples::framework)

if(VNE_XR_WITH_RHI)
    target_link_libraries(sample_02_openxr_stereo PRIVATE vne::rhi vne::math)
endif()

set_target_properties(sample_02_openxr_stereo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(ANDROID)
    target_link_libraries(sample_02_openxr_stereo PRIVATE android log native_app_glue)
endif()
