#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#
# Author:    Ajeet Singh Yadav
# Created:   February 2026
#
# Autodoc:   yes
#==============================================================================

set(SOURCE_FILES
    vertexnova/xr/xr.cpp
)

set(HEADER_FILES
    ${VNE_INCLUDE_DIR}/vertexnova/xr/xr.h
)

# One library per build: static or shared, always named vnexr (alias vne::xr).
if(VNE_XR_LIB_TYPE STREQUAL "shared")
    set(_lib_type SHARED)
else()
    set(_lib_type STATIC)
endif()

add_library(vnexr ${_lib_type} ${SOURCE_FILES} ${HEADER_FILES})

target_include_directories(vnexr
    PUBLIC
        $<BUILD_INTERFACE:${VNE_INCLUDE_DIR}>
        $<BUILD_INTERFACE:${VNE_SRC_DIR}>
        $<INSTALL_INTERFACE:include>
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

target_compile_features(vnexr PUBLIC cxx_std_20)

# Set export/import defines for shared builds. The header uses them only on Windows
# (VNE_XR_API becomes __declspec(dllexport)/dllimport); on other platforms they are no-ops.
if(_lib_type STREQUAL "SHARED")
    target_compile_definitions(vnexr
        PRIVATE VNE_XR_BUILDING_DLL
        INTERFACE VNE_XR_DLL
    )
endif()

add_library(vne::xr ALIAS vnexr)

source_group("Sources" FILES ${SOURCE_FILES})
source_group(TREE ${VNE_INCLUDE_DIR} PREFIX "Headers" FILES ${HEADER_FILES})
