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
)

add_library(vne::xr::samples::framework ALIAS vne_xr_samples_framework)

target_include_directories(vne_xr_samples_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(vne_xr_samples_framework
    PUBLIC
        vne::xr
        vne::logging
)

target_compile_features(vne_xr_samples_framework PUBLIC cxx_std_20)

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
