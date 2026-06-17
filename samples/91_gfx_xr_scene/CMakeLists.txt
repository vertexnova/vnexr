#==============================================================================
# 06_gfx_xr_scene: optional vnegfx ECS content rendered into XR views
#==============================================================================

if(NOT VNE_XR_WITH_GFX_SAMPLE OR NOT TARGET vne::gfx)
    return()
endif()

add_executable(sample_91_gfx_xr_scene main.cpp)

target_link_libraries(sample_91_gfx_xr_scene PRIVATE vne::xr vne::gfx vne::logging vne::rhi vne::math)

target_include_directories(sample_91_gfx_xr_scene PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../framework
)

set_target_properties(sample_91_gfx_xr_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)
