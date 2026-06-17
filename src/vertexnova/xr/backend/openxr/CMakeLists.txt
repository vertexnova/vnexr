#==============================================================================
# OpenXR backend (gated on VNE_XR_WITH_OPENXR)
#==============================================================================

if(NOT VNE_XR_WITH_OPENXR)
    return()
endif()

set(_openxr_sources
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_instance_lifetime.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_platform_handle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_session.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_graphics.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_binding.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_texture.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/openxr_swapchain_bridge.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/openxr_composition_layer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/input/openxr_input.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/math/openxr_math.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/openxr_android.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/openxr_debug.cpp
)

set(_openxr_headers
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_instance_lifetime.h
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_platform_handle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_session.h
    ${CMAKE_CURRENT_SOURCE_DIR}/openxr_util.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_graphics.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_binding.h
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics/openxr_vulkan_texture.h
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/openxr_swapchain_bridge.h
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation/openxr_composition_layer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/input/openxr_input.h
    ${CMAKE_CURRENT_SOURCE_DIR}/math/openxr_math.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/openxr_android.h
    ${CMAKE_CURRENT_SOURCE_DIR}/platform/openxr_debug.h
)

target_sources(vnexr PRIVATE ${_openxr_sources} ${_openxr_headers})

target_include_directories(vnexr PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/graphics
    ${CMAKE_CURRENT_SOURCE_DIR}/presentation
    ${CMAKE_CURRENT_SOURCE_DIR}/input
    ${CMAKE_CURRENT_SOURCE_DIR}/math
    ${CMAKE_CURRENT_SOURCE_DIR}/platform
)
