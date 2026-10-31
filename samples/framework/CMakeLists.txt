#==============================================================================
# vnexr samples framework (layers, XrSampleApp, sample registry and launcher)
#==============================================================================

if(NOT TARGET vnexr)
    return()
endif()

add_library(vne_xr_samples_framework STATIC
    layers/xr_layer.cpp
    layers/xr_layer_stack.cpp
    layers/exit_after_frames_layer.cpp
    app/xr_sample_app.cpp
    app/sample_registry.cpp
    app/sample_launcher.cpp
    app/xr_sample_host.cpp
    interaction/placement.cpp
    interaction/scene_navigator.cpp
    launcher/controller_visuals_layer.cpp
    launcher/launcher_layer.cpp
    launcher/launcher_layout.cpp
    launcher/text_mesh.cpp
    render/flat_color_renderer.cpp
    render/font_atlas.cpp
    render/font_layout.cpp
    render/text_renderer.cpp
    render/xr_frame_renderer.cpp
)

add_library(vne::xr::samples::framework ALIAS vne_xr_samples_framework)

# stb_truetype (deps/external/stb_truetype) is third-party code: compile its translation unit without our warning set.
# TODO: take it from vneio's stb_image module once vneio ships stb_truetype (see deps/external/stb_truetype/README.md).
set_source_files_properties(render/font_atlas.cpp PROPERTIES COMPILE_FLAGS "-w")

target_include_directories(vne_xr_samples_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
    PRIVATE
        ${PROJECT_SOURCE_DIR}/deps/external/stb_truetype
)

target_link_libraries(vne_xr_samples_framework
    PUBLIC
        vne::xr
        vne::rhi
        vne::logging
)

target_compile_features(vne_xr_samples_framework PUBLIC cxx_std_20)

# Android: console logging is invisible to `adb logcat`, so forward stdout/stderr to it.
if(ANDROID)
    target_sources(vne_xr_samples_framework PRIVATE app/platform/android/logcat_bridge.cpp)
    target_link_libraries(vne_xr_samples_framework PUBLIC log)
endif()

# NDK native_app_glue for Android NativeActivity samples.
if(ANDROID AND NOT TARGET native_app_glue)
    set(_vne_android_ndk_root "${ANDROID_NDK}")
    if(NOT _vne_android_ndk_root AND DEFINED CMAKE_ANDROID_NDK)
        set(_vne_android_ndk_root "${CMAKE_ANDROID_NDK}")
    endif()
    if(_vne_android_ndk_root AND EXISTS
            "${_vne_android_ndk_root}/sources/android/native_app_glue/android_native_app_glue.c")
        add_library(native_app_glue STATIC
            "${_vne_android_ndk_root}/sources/android/native_app_glue/android_native_app_glue.c"
        )
        target_include_directories(native_app_glue PUBLIC
            "${_vne_android_ndk_root}/sources/android/native_app_glue"
        )
    else()
        message(WARNING "native_app_glue.c not found under NDK; Android samples may fail to link")
    endif()
endif()
