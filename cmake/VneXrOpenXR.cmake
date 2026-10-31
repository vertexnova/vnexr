#==============================================================================
# OpenXR loader (FetchContent) when VNE_XR_WITH_OPENXR=ON
#==============================================================================

include_guard(GLOBAL)

set(_VNE_XR_OPENXR_CMAKE_DIR "${CMAKE_CURRENT_LIST_DIR}")

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
    # The pinned OpenXR SDK still calls find_package(PythonInterp), a module CMake removed (CMP0148). Give it a small
    # replacement for that subproject only.
    set(_vne_xr_saved_module_path "${CMAKE_MODULE_PATH}")
    list(PREPEND CMAKE_MODULE_PATH "${_VNE_XR_OPENXR_CMAKE_DIR}/openxr_shims")
    set(CMAKE_POLICY_DEFAULT_CMP0148 NEW)
    FetchContent_MakeAvailable(openxr-sdk)
    unset(CMAKE_POLICY_DEFAULT_CMP0148)
    set(CMAKE_MODULE_PATH "${_vne_xr_saved_module_path}")

    if(NOT TARGET openxr_loader)
        message(FATAL_ERROR "OpenXR FetchContent did not provide openxr_loader")
    endif()

    if(NOT TARGET OpenXR::openxr_loader)
        add_library(OpenXR::openxr_loader ALIAS openxr_loader)
    endif()

    message(STATUS "VneXR OpenXR: Khronos loader ${_openxr_tag}")
endfunction()
