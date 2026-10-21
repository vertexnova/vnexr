#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

set(SOURCE_FILES
    vertexnova/xr/xr.cpp
    vertexnova/xr/xr_factory.cpp
    vertexnova/xr/core/session.cpp
    vertexnova/xr/core/null_session.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/xr/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr_error.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr_factory.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/types.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/view.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/frame.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/swapchain_image.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/input.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/render_session.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/session.h
    ${VNE_SRC_DIR}/vertexnova/xr/core/null_session.h
)

if(VNE_XR_WITH_VISIONOS)
    list(APPEND SOURCE_FILES vertexnova/xr/backend/visionos/visionos_session.cpp)
    list(APPEND HEADER_FILES ${VNE_SRC_DIR}/vertexnova/xr/backend/visionos/visionos_session.h)
endif()

if(VNE_XR_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vnexr ${_lib_type} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(vnexr
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${VNE_SRC_DIR}
)

target_link_libraries(vnexr
    PUBLIC
        vne::xr::Warnings
        vne::xr::BuildSettings
)

if(TARGET vne::common)
    target_link_libraries(vnexr PUBLIC vne::common)
endif()
if(TARGET vne::logging)
    target_link_libraries(vnexr PUBLIC vne::logging)
endif()

if(VNE_XR_WITH_RHI)
    target_compile_definitions(vnexr PUBLIC VNE_XR_WITH_RHI=1)
    target_link_libraries(vnexr PUBLIC vne::rhi vne::math)
endif()

if(VNE_XR_WITH_OPENXR)
    target_compile_definitions(vnexr PUBLIC VNE_XR_WITH_OPENXR=1)
    if(VNE_XR_BACKEND_OPENXR_GRAPHICS STREQUAL "vulkan")
        # OpenXR platform headers gate Vulkan structs behind this define.
        target_compile_definitions(vnexr PUBLIC
            VNE_XR_OPENXR_GRAPHICS_VULKAN=1
            XR_USE_GRAPHICS_API_VULKAN=1
        )
        # vnerhi links Vulkan headers PRIVATE. vnexr TUs still need vulkan.h /
        # vulkan.hpp (openxr_platform.h, vulkan_device.h). On Windows/Linux the
        # SDK path is used; on Android NDK, fall back to nested Vulkan-Headers.
        set(_vne_xr_vk_includes_ok FALSE)
        if(TARGET vne_vulkan_headers)
            target_link_libraries(vnexr PRIVATE vne_vulkan_headers)
            set(_vne_xr_vk_includes_ok TRUE)
        elseif(TARGET vne::vulkan_headers)
            target_link_libraries(vnexr PRIVATE vne::vulkan_headers)
            set(_vne_xr_vk_includes_ok TRUE)
        endif()
        if(NOT _vne_xr_vk_includes_ok)
            find_package(Vulkan QUIET)
            if(TARGET Vulkan::Headers)
                target_link_libraries(vnexr PRIVATE Vulkan::Headers)
                set(_vne_xr_vk_includes_ok TRUE)
            endif()
            if(Vulkan_INCLUDE_DIRS)
                target_include_directories(vnexr SYSTEM PRIVATE ${Vulkan_INCLUDE_DIRS})
                set(_vne_xr_vk_includes_ok TRUE)
            endif()
        endif()
        if(NOT _vne_xr_vk_includes_ok AND DEFINED ENV{VULKAN_SDK})
            foreach(_vne_xr_vk_cand
                    "$ENV{VULKAN_SDK}/Include"
                    "$ENV{VULKAN_SDK}/include")
                if(EXISTS "${_vne_xr_vk_cand}/vulkan/vulkan.hpp")
                    target_include_directories(vnexr SYSTEM PRIVATE "${_vne_xr_vk_cand}")
                    set(_vne_xr_vk_includes_ok TRUE)
                    break()
                endif()
            endforeach()
        endif()
        if(NOT _vne_xr_vk_includes_ok)
            set(_vne_xr_vk_hpp
                "${CMAKE_SOURCE_DIR}/deps/internal/vnerhi/deps/external/vulkan-headers/include")
            if(EXISTS "${_vne_xr_vk_hpp}/vulkan/vulkan.hpp")
                target_include_directories(vnexr SYSTEM PRIVATE "${_vne_xr_vk_hpp}")
                set(_vne_xr_vk_includes_ok TRUE)
            endif()
        endif()
        if(NOT _vne_xr_vk_includes_ok)
            message(FATAL_ERROR
                "vulkan/vulkan.hpp not found for vnexr OpenXR/Vulkan.\n"
                "  Install a Vulkan SDK, or run: ./scripts/init_submodules.sh full")
        endif()
    endif()
    if(ANDROID)
        target_compile_definitions(vnexr PUBLIC XR_USE_PLATFORM_ANDROID=1)
    endif()
    if(WIN32)
        target_compile_definitions(vnexr PUBLIC XR_USE_PLATFORM_WIN32=1)
    endif()
    target_link_libraries(vnexr PRIVATE OpenXR::openxr_loader)
    add_subdirectory(vertexnova/xr/backend/openxr)
endif()

if(VNE_XR_WITH_VISIONOS)
    target_compile_definitions(vnexr PUBLIC VNE_XR_WITH_VISIONOS=1)
    if(VNE_XR_COMPOSITOR_SERVICES_FRAMEWORK)
        target_link_libraries(vnexr PRIVATE "${VNE_XR_COMPOSITOR_SERVICES_FRAMEWORK}")
    endif()
endif()

if(VNE_XR_WITH_EVENTS AND TARGET vne::events)
    target_link_libraries(vnexr PUBLIC vne::events)
endif()

target_compile_features(vnexr PUBLIC cxx_std_20)

if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vnexr
        PRIVATE VNE_XR_BUILDING_DLL
        INTERFACE VNE_XR_DLL
    )
endif()

add_library(vne::xr ALIAS vnexr)

source_group(TREE ${VNE_SRC_DIR} PREFIX "Sources" FILES ${SOURCE_FILES})
set(_xr_public_headers
    ${VNE_INCLUDE_DIR}/vertexnova/xr/export.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr_error.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr_factory.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/types.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/view.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/frame.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/swapchain_image.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/input.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/render_session.h
    ${VNE_INCLUDE_DIR}/vertexnova/xr/session.h
)
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${_xr_public_headers})
