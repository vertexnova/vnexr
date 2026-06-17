#==============================================================================
# OpenXR loader (FetchContent) when VNE_XR_WITH_OPENXR=ON
#==============================================================================

include_guard(GLOBAL)

function(xr_configure_openxr)
    if(NOT VNE_XR_WITH_OPENXR)
        return()
    endif()

    if(APPLE)
        message(FATAL_ERROR "VNE_XR_WITH_OPENXR is not supported on Apple platforms. Use VNE_XR_WITH_VISIONOS.")
    endif()

    include(FetchContent)
    set(_openxr_tag "release-1.0.34")
    FetchContent_Declare(
        openxr-sdk
        GIT_REPOSITORY https://github.com/KhronosGroup/OpenXR-SDK.git
        GIT_TAG ${_openxr_tag}
        GIT_SHALLOW TRUE
    )
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_CONFORMANCE_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_API_LAYERS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(openxr-sdk)

    if(NOT TARGET openxr_loader)
        message(FATAL_ERROR "OpenXR FetchContent did not provide openxr_loader")
    endif()

    if(NOT TARGET OpenXR::openxr_loader)
        add_library(OpenXR::openxr_loader ALIAS openxr_loader)
    endif()

    message(STATUS "VneXR OpenXR: Khronos loader ${_openxr_tag}")
endfunction()
