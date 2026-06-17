#==============================================================================
# VneXR — forward vnerhi backend options (mirrors vnegfx VneGfxRhi.cmake)
#==============================================================================

include_guard(GLOBAL)

option(VNE_XR_BACKEND_NULL "Nested vnerhi: Null backend" ON)
option(VNE_XR_BACKEND_VULKAN "Nested vnerhi: Vulkan backend" OFF)
option(VNE_XR_BACKEND_METAL "Nested vnerhi: Metal backend" OFF)
option(VNE_XR_BACKEND_WEBGPU "Nested vnerhi: WebGPU backend" OFF)
option(VNE_XR_RHI_SPIRV_REFLECTION
    "Nested vnerhi: SPIRV-Cross runtime reflection" ON)

function(_xr_set_vnerhi_backend_defaults)
    if(VNE_XR_CI AND NOT VNE_XR_WITH_OPENXR)
        return()
    endif()

    if(VNE_TARGET_PLATFORM STREQUAL "macOS"
       OR VNE_TARGET_PLATFORM STREQUAL "iOS"
       OR VNE_TARGET_PLATFORM STREQUAL "visionOS")
        set(VNE_XR_BACKEND_METAL ON CACHE BOOL "" FORCE)
    elseif(VNE_TARGET_PLATFORM STREQUAL "Android")
        set(VNE_XR_BACKEND_VULKAN ON CACHE BOOL "" FORCE)
    elseif(VNE_TARGET_PLATFORM STREQUAL "Web")
        set(VNE_XR_BACKEND_WEBGPU ON CACHE BOOL "" FORCE)
    endif()

    if(VNE_XR_WITH_OPENXR AND VNE_TARGET_PLATFORM STREQUAL "Android")
        set(VNE_XR_BACKEND_VULKAN ON CACHE BOOL "" FORCE)
    endif()
endfunction()

function(_xr_apply_vnerhi_ci_preset)
    if(NOT VNE_XR_CI)
        return()
    endif()
    if(VNE_XR_WITH_OPENXR OR VNE_XR_WITH_VISIONOS)
        return()
    endif()
    set(VNE_XR_BACKEND_NULL ON CACHE BOOL "" FORCE)
    set(VNE_XR_BACKEND_VULKAN OFF CACHE BOOL "" FORCE)
    set(VNE_XR_BACKEND_METAL OFF CACHE BOOL "" FORCE)
    set(VNE_XR_BACKEND_WEBGPU OFF CACHE BOOL "" FORCE)
    set(VNE_XR_RHI_SPIRV_REFLECTION OFF CACHE BOOL "" FORCE)
    message(STATUS "VneXR CI preset: nested vnerhi Null-only")
endfunction()

function(xr_configure_vnerhi_options)
    _xr_apply_vnerhi_ci_preset()
    _xr_set_vnerhi_backend_defaults()
    message(STATUS "VneXR nested vnerhi: NULL=${VNE_XR_BACKEND_NULL} VULKAN=${VNE_XR_BACKEND_VULKAN} "
                   "METAL=${VNE_XR_BACKEND_METAL} WEBGPU=${VNE_XR_BACKEND_WEBGPU}")
endfunction()

function(xr_get_vnerhi_cache_vars _out_var)
    set(_cache_vars
        VNE_RHI_TESTS OFF
        VNE_RHI_SAMPLES OFF
        VNE_RHI_DEV OFF
        VNE_RHI_CI OFF
        VNE_RHI_LIB_TYPE "${VNE_XR_LIB_TYPE}"
        BUILD_TESTS OFF
        BUILD_EXAMPLES OFF
        VNE_RHI_BACKEND_NULL "${VNE_XR_BACKEND_NULL}"
        VNE_RHI_BACKEND_VULKAN "${VNE_XR_BACKEND_VULKAN}"
        VNE_RHI_BACKEND_METAL "${VNE_XR_BACKEND_METAL}"
        VNE_RHI_BACKEND_WEBGPU "${VNE_XR_BACKEND_WEBGPU}"
        VNE_RHI_SPIRV_REFLECTION_ENABLED "${VNE_XR_RHI_SPIRV_REFLECTION}"
    )
    set(${_out_var} ${_cache_vars} PARENT_SCOPE)
endfunction()
