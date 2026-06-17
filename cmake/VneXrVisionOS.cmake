#==============================================================================
# visionOS CompositorServices options
#==============================================================================

include_guard(GLOBAL)

function(xr_configure_visionos)
    if(NOT VNE_XR_WITH_VISIONOS)
        return()
    endif()

    if(NOT VNE_TARGET_PLATFORM STREQUAL "visionOS")
        message(WARNING "VNE_XR_WITH_VISIONOS is intended for visionOS builds (platform=${VNE_TARGET_PLATFORM})")
    endif()

    find_library(COMPOSITOR_SERVICES_FRAMEWORK CompositorServices)
    if(NOT COMPOSITOR_SERVICES_FRAMEWORK)
        message(WARNING "CompositorServices framework not found; visionOS backend may not link")
    else()
        set(VNE_XR_COMPOSITOR_SERVICES_FRAMEWORK "${COMPOSITOR_SERVICES_FRAMEWORK}" PARENT_SCOPE)
    endif()
endfunction()
