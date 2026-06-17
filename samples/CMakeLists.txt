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

# Platform-specific (optional)
if(VNE_XR_WITH_VISIONOS)
    add_subdirectory(90_visionos_immersive)
endif()

if(VNE_XR_WITH_GFX_SAMPLE)
    add_subdirectory(91_gfx_xr_scene)
endif()
