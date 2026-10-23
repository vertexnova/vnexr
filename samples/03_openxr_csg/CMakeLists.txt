#==============================================================================
# 03_openxr_csg — OpenXR stereo + vnegfx CSG on the session device (stretch)
#==============================================================================

if(NOT VNE_XR_WITH_OPENXR OR NOT VNE_XR_WITH_GFX_SAMPLE OR NOT TARGET vne::gfx)
    return()
endif()

add_executable(sample_03_openxr_csg
    main.cpp
    csg_xr_scene_layer.cpp
)

target_link_libraries(sample_03_openxr_csg PRIVATE
    vne::xr::samples::framework
    vne::gfx
    vne::rhi
    vne::math
    vne::logging
)

set_target_properties(sample_03_openxr_csg PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
