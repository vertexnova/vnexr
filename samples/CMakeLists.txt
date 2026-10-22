#==============================================================================
# vnexr samples
#==============================================================================

if(NOT TARGET vnexr)
    return()
endif()

add_subdirectory(framework)

# Getting started (no headset / CI)
add_subdirectory(00_hello_xr)
add_subdirectory(01_null_simulator)

# OpenXR (Linux Monado, Windows, Android)
if(VNE_XR_WITH_OPENXR)
    add_subdirectory(02_openxr_stereo)
endif()

# OpenXR + vnegfx CSG (stretch; requires VNE_XR_WITH_GFX_SAMPLE)
if(VNE_XR_WITH_OPENXR AND VNE_XR_WITH_GFX_SAMPLE)
    add_subdirectory(03_openxr_csg)
endif()

# Platform-specific (optional)
if(VNE_XR_WITH_VISIONOS)
    add_subdirectory(90_visionos_immersive)
endif()

if(VNE_XR_WITH_GFX_SAMPLE)
    add_subdirectory(91_gfx_xr_scene)
endif()

# Android Studio Gradle project (one APK flavor per registered sample).
if(ANDROID AND VNE_XR_WITH_OPENXR AND VNE_XR_GENERATE_ANDROID_GRADLE)
    include(VneXrAndroidGradle)
    vne_xr_generate_android_gradle_project(
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/android-studio"
        VNEXR_ROOT "${CMAKE_SOURCE_DIR}"
        GRADLE_TEMPLATE_DIR "${CMAKE_SOURCE_DIR}/samples/framework/app/platform/android/gradle"
    )
endif()
