#==============================================================================
# vnexr samples
#==============================================================================

if(NOT TARGET vnexr)
    return()
endif()

add_subdirectory(framework)

# A sample is a static library plus one register function (void register<Name>Sample(SampleRegistry&)). The
# launcher calls every registered function from a generated file, so samples never depend on static initializers.
#   vne_xr_add_sample_library(<name> REGISTER <function> SOURCES <files...> [LIBRARIES <targets...>])
define_property(GLOBAL PROPERTY VNE_XR_SAMPLE_REGISTER_FUNCTIONS BRIEF_DOCS "Sample register functions")
define_property(GLOBAL PROPERTY VNE_XR_SAMPLE_LIBRARIES BRIEF_DOCS "Sample library targets")
function(vne_xr_add_sample_library NAME)
    cmake_parse_arguments(ARG "" "REGISTER" "SOURCES;LIBRARIES" ${ARGN})
    if(NOT ARG_REGISTER OR NOT ARG_SOURCES)
        message(FATAL_ERROR "vne_xr_add_sample_library(${NAME}): REGISTER and SOURCES are required")
    endif()
    set(_target vne_xr_sample_${NAME})
    add_library(${_target} STATIC ${ARG_SOURCES})
    target_link_libraries(${_target} PUBLIC vne::xr::samples::framework ${ARG_LIBRARIES})
    # Android links the samples into a shared library.
    set_target_properties(${_target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
    set_property(GLOBAL APPEND PROPERTY VNE_XR_SAMPLE_REGISTER_FUNCTIONS ${ARG_REGISTER})
    set_property(GLOBAL APPEND PROPERTY VNE_XR_SAMPLE_LIBRARIES ${_target})
endfunction()

# Getting started (no headset / CI)
add_subdirectory(00_hello_xr)
add_subdirectory(01_null_simulator)

# OpenXR (Linux Monado, Windows, Android)
if(VNE_XR_WITH_OPENXR)
    add_subdirectory(02_openxr_stereo)
    add_subdirectory(04_xr_diagnostics)
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

# One launcher binary that runs any registered sample by name (desktop executable / Android NativeActivity library).
add_subdirectory(launcher)

# Android Studio Gradle project (a single APK: the launcher).
if(ANDROID AND VNE_XR_WITH_OPENXR AND VNE_XR_GENERATE_ANDROID_GRADLE)
    include(VneXrAndroidGradle)
    vne_xr_generate_android_gradle_project(
        OUTPUT_DIR "${CMAKE_BINARY_DIR}/android-studio"
        VNEXR_ROOT "${CMAKE_SOURCE_DIR}"
        GRADLE_TEMPLATE_DIR "${CMAKE_SOURCE_DIR}/samples/framework/app/platform/android/gradle"
    )
endif()
