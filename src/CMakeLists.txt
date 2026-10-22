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

target_link_libraries(vnexr PUBLIC vne::rhi vne::math)

if(VNE_XR_WITH_OPENXR)
    target_compile_definitions(vnexr PUBLIC VNE_XR_WITH_OPENXR=1)
    if(VNE_XR_BACKEND_OPENXR_GRAPHICS STREQUAL "vulkan")
        # OpenXR platform headers gate Vulkan structs behind this define.
        # Do not set XR_USE_PLATFORM_WIN32 unless Win32/D3D OpenXR types are needed —
        # that path requires windows.h + COM headers and is unused for Vulkan-only.
        target_compile_definitions(vnexr PUBLIC
            VNE_XR_OPENXR_GRAPHICS_VULKAN=1
            XR_USE_GRAPHICS_API_VULKAN=1
        )
        # vnerhi links Vulkan headers PRIVATE; vnexr still needs vulkan.h / vulkan.hpp
        # for openxr_platform_prelude.h and vulkan_device.h. Always attach nested
        # Vulkan-Headers when present (reliable on Android NDK + Windows SDK CI).
        set(_vne_xr_vk_nested
            "${CMAKE_SOURCE_DIR}/deps/internal/vnerhi/deps/external/vulkan-headers/include")
        set(_vne_xr_vk_includes_ok FALSE)
        if(EXISTS "${_vne_xr_vk_nested}/vulkan/vulkan.h")
            target_include_directories(vnexr SYSTEM PRIVATE "${_vne_xr_vk_nested}")
            set(_vne_xr_vk_includes_ok TRUE)
        endif()
        if(TARGET vne_vulkan_headers)
            target_link_libraries(vnexr PRIVATE vne_vulkan_headers)
            set(_vne_xr_vk_includes_ok TRUE)
        elseif(TARGET vne::vulkan_headers)
            target_link_libraries(vnexr PRIVATE vne::vulkan_headers)
            set(_vne_xr_vk_includes_ok TRUE)
        endif()
        find_package(Vulkan QUIET)
        if(TARGET Vulkan::Headers)
            target_link_libraries(vnexr PRIVATE Vulkan::Headers)
            set(_vne_xr_vk_includes_ok TRUE)
        endif()
        if(Vulkan_INCLUDE_DIRS)
            target_include_directories(vnexr SYSTEM PRIVATE ${Vulkan_INCLUDE_DIRS})
            set(_vne_xr_vk_includes_ok TRUE)
        endif()
        if(DEFINED ENV{VULKAN_SDK})
            foreach(_vne_xr_vk_cand
                    "$ENV{VULKAN_SDK}/Include"
                    "$ENV{VULKAN_SDK}/include")
                if(EXISTS "${_vne_xr_vk_cand}/vulkan/vulkan.h")
                    target_include_directories(vnexr SYSTEM PRIVATE "${_vne_xr_vk_cand}")
                    set(_vne_xr_vk_includes_ok TRUE)
                    break()
                endif()
            endforeach()
        endif()
        if(NOT _vne_xr_vk_includes_ok)
            message(FATAL_ERROR
                "vulkan/vulkan.h not found for vnexr OpenXR/Vulkan.\n"
                "  Install a Vulkan SDK, or run: ./scripts/init_submodules.sh full")
        endif()
        # openxr_vulkan_binding calls vkGetInstanceProcAddr, vkCreateInstance, and
        # vkCreateDevice. Headers alone do not provide those symbols.
        set(_vne_xr_vk_loader_ok FALSE)
        if(TARGET Vulkan::Vulkan)
            target_link_libraries(vnexr PRIVATE Vulkan::Vulkan)
            set(_vne_xr_vk_loader_ok TRUE)
        endif()
        if(NOT _vne_xr_vk_loader_ok)
            set(_vne_xr_vk_loader_hints)
            if(DEFINED ENV{VULKAN_SDK})
                list(APPEND _vne_xr_vk_loader_hints
                    "$ENV{VULKAN_SDK}/Lib"
                    "$ENV{VULKAN_SDK}/lib")
            endif()
            find_library(_vne_xr_vk_loader
                NAMES vulkan-1 vulkan
                HINTS ${_vne_xr_vk_loader_hints})
            if(_vne_xr_vk_loader)
                target_link_libraries(vnexr PRIVATE "${_vne_xr_vk_loader}")
                set(_vne_xr_vk_loader_ok TRUE)
            elseif(ANDROID)
                target_link_libraries(vnexr PRIVATE vulkan)
                set(_vne_xr_vk_loader_ok TRUE)
            endif()
        endif()
        if(NOT _vne_xr_vk_loader_ok)
            message(FATAL_ERROR
                "Vulkan loader not found for vnexr OpenXR/Vulkan.\n"
                "  Install the Vulkan SDK loader (vulkan-1) or libvulkan-dev.")
        endif()
    endif()
    if(ANDROID)
        target_compile_definitions(vnexr PUBLIC XR_USE_PLATFORM_ANDROID=1)
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
    # VNE_XR_DLL must be set while *building* the DLL too — export.h only
    # enables __declspec(dllexport) when both VNE_XR_DLL and VNE_XR_BUILDING_DLL
    # are defined. INTERFACE-only left symbols unexported and MSVC omitted vnexr.lib.
    target_compile_definitions(vnexr
        PUBLIC VNE_XR_DLL
        PRIVATE VNE_XR_BUILDING_DLL
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
