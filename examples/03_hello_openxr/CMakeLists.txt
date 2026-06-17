#==============================================================================
# 03_hello_openxr: stereo render loop (Android Quest / Samsung Galaxy XR path)
#==============================================================================

if(NOT VNE_XR_WITH_OPENXR)
    return()
endif()

add_executable(example_03_hello_openxr
    main.cpp
    stereo_cube_renderer.cpp
    stereo_cube_scene.cpp
)

target_link_libraries(example_03_hello_openxr PRIVATE vne::xr vne::logging)

if(VNE_XR_WITH_RHI)
    target_link_libraries(example_03_hello_openxr PRIVATE vne::rhi vne::math)
endif()

target_include_directories(example_03_hello_openxr PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_03_hello_openxr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)

if(ANDROID)
    target_link_libraries(example_03_hello_openxr PRIVATE android log native_app_glue)
endif()
