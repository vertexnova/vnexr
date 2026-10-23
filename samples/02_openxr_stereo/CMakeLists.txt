#==============================================================================
# 02_openxr_stereo — OpenXR layer-based stereo cube (Linux / Windows / Android)
#==============================================================================

set(_sample_sources
    main.cpp
    stereo_cube_scene_layer.cpp
)

if(ANDROID)
    # NativeActivity loads a shared library named by android.app.lib_name.
    add_library(sample_02_openxr_stereo SHARED ${_sample_sources})
else()
    add_executable(sample_02_openxr_stereo ${_sample_sources})
endif()

target_link_libraries(sample_02_openxr_stereo PRIVATE
    vne::xr::samples::framework
    vne::rhi
    vne::math
)

set_target_properties(sample_02_openxr_stereo PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(ANDROID)
    target_link_libraries(sample_02_openxr_stereo PRIVATE android log native_app_glue)
    target_link_options(sample_02_openxr_stereo PRIVATE "-u" "ANativeActivity_onCreate")
    include(VneXrAndroidGradle)
    vne_xr_register_android_sample(sample_02_openxr_stereo)
endif()
