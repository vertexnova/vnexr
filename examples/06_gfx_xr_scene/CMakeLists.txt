#==============================================================================
# 06_gfx_xr_scene: optional vnegfx ECS content rendered into XR views
#==============================================================================

if(NOT VNE_XR_WITH_GFX_SAMPLE OR NOT TARGET vne::gfx)
    return()
endif()

add_executable(example_06_gfx_xr_scene main.cpp)

target_link_libraries(example_06_gfx_xr_scene PRIVATE vne::xr vne::gfx vne::logging vne::rhi vne::math)

target_include_directories(example_06_gfx_xr_scene PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_06_gfx_xr_scene PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
