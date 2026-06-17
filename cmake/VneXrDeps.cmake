#==============================================================================
# VneXR internal dependencies (vnerhi, vnemath, optional vneevents)
#==============================================================================

include(VneUseDep)

function(_xr_save_global_build_flags _tests_out _examples_out)
    set(${_tests_out} "")
    set(${_examples_out} "")
    get_property(_ty CACHE BUILD_TESTS PROPERTY TYPE)
    if(NOT _ty STREQUAL "NOTFOUND" AND _ty)
        set(${_tests_out} "${BUILD_TESTS}")
    endif()
    get_property(_ty CACHE BUILD_EXAMPLES PROPERTY TYPE)
    if(NOT _ty STREQUAL "NOTFOUND" AND _ty)
        set(${_examples_out} "${BUILD_EXAMPLES}")
    endif()
    set(${_tests_out} "${${_tests_out}}" PARENT_SCOPE)
    set(${_examples_out} "${${_examples_out}}" PARENT_SCOPE)
endfunction()

function(_xr_restore_global_build_flags _tests_saved _examples_saved)
    if(DEFINED ${_tests_saved} AND NOT "${${_tests_saved}}" STREQUAL "")
        set(BUILD_TESTS "${${_tests_saved}}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_TESTS CACHE)
    endif()
    if(DEFINED ${_examples_saved} AND NOT "${${_examples_saved}}" STREQUAL "")
        set(BUILD_EXAMPLES "${${_examples_saved}}" CACHE BOOL "" FORCE)
    else()
        unset(BUILD_EXAMPLES CACHE)
    endif()
endfunction()

function(_xr_force_global_build_flags_off)
    set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
endfunction()

function(xr_link_local_vnerhi)
    if(NOT DEFINED ENV{VNE_VNERHI_PATH} OR "$ENV{VNE_VNERHI_PATH}" STREQUAL "")
        return()
    endif()
    set(_dest "${VNE_DEPS_INTERNAL_DIR}/vnerhi")
    if(NOT EXISTS "$ENV{VNE_VNERHI_PATH}/CMakeLists.txt")
        message(FATAL_ERROR "VNE_VNERHI_PATH does not look like vnerhi: $ENV{VNE_VNERHI_PATH}")
    endif()
    file(MAKE_DIRECTORY "${VNE_DEPS_INTERNAL_DIR}")
    if(EXISTS "${_dest}" AND NOT IS_SYMLINK "${_dest}")
        message(FATAL_ERROR "${_dest} exists and is not a symlink")
    endif()
    file(CREATE_LINK "$ENV{VNE_VNERHI_PATH}" "${_dest}" SYMBOLIC)
    message(STATUS "Linked vnerhi: ${_dest} -> $ENV{VNE_VNERHI_PATH}")
endfunction()

function(xr_configure_vnerhi_dep)
    xr_link_local_vnerhi()
    set(_vnerhi_root "${VNE_DEPS_INTERNAL_DIR}/vnerhi")
    if(NOT EXISTS "${_vnerhi_root}/CMakeLists.txt")
        message(FATAL_ERROR
            "deps/internal/vnerhi is required when VNE_XR_WITH_RHI=ON.\n"
            "  export VNE_VNERHI_PATH=/path/to/vnerhi\n"
            "  ./scripts/init_submodules.sh full")
    endif()
    _xr_save_global_build_flags(_saved_tests _saved_examples)
    _xr_force_global_build_flags_off()
    xr_get_vnerhi_cache_vars(_vnerhi_cache_vars)
    vne_use_dep(TARGET vne::rhi SUBDIR "${_vnerhi_root}"
        BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnerhi"
        CACHE_VARS ${_vnerhi_cache_vars})
    _xr_restore_global_build_flags(_saved_tests _saved_examples)
    if(NOT TARGET vne::rhi)
        message(FATAL_ERROR "vnerhi did not provide target vne::rhi")
    endif()
endfunction()

function(xr_configure_vnemath_dep)
    if(TARGET vne::math)
        return()
    endif()
    if(NOT EXISTS "${VNE_DEPS_INTERNAL_DIR}/vnemath/CMakeLists.txt")
        message(FATAL_ERROR "deps/internal/vnemath required. Init vnerhi submodules or add vnemath.")
    endif()
    _xr_save_global_build_flags(_saved_tests _saved_examples)
    _xr_force_global_build_flags_off()
    vne_use_dep(TARGET vne::math SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnemath"
        BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnemath"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF)
    _xr_restore_global_build_flags(_saved_tests _saved_examples)
endfunction()

function(xr_configure_vneevents_dep)
    if(NOT VNE_XR_WITH_EVENTS)
        return()
    endif()
    if(NOT EXISTS "${VNE_DEPS_INTERNAL_DIR}/vneevents/CMakeLists.txt")
        message(WARNING "vneevents not found; skipping VNE_XR_WITH_EVENTS")
        return()
    endif()
    _xr_save_global_build_flags(_saved_tests _saved_examples)
    _xr_force_global_build_flags_off()
    vne_use_dep(TARGET vne::events SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vneevents"
        BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vneevents"
        CACHE_VARS BUILD_TESTS OFF BUILD_EXAMPLES OFF VNEEVENTS_LIB_TYPE "${VNE_XR_LIB_TYPE}")
    _xr_restore_global_build_flags(_saved_tests _saved_examples)
endfunction()

function(xr_configure_vnegfx_dep)
    if(NOT VNE_XR_WITH_GFX_SAMPLE)
        return()
    endif()
    if(NOT EXISTS "${VNE_DEPS_INTERNAL_DIR}/vnegfx/CMakeLists.txt")
        message(FATAL_ERROR "vnegfx required for VNE_XR_WITH_GFX_SAMPLE")
    endif()
    _xr_save_global_build_flags(_saved_tests _saved_examples)
    _xr_force_global_build_flags_off()
    vne_use_dep(TARGET vne::gfx SUBDIR "${VNE_DEPS_INTERNAL_DIR}/vnegfx"
        BINARY_DIR "${CMAKE_BINARY_DIR}/deps/internal/vnegfx"
        CACHE_VARS
            VNE_GFX_TESTS OFF
            VNE_GFX_SAMPLES OFF
            VNE_GFX_DEV OFF
            VNE_GFX_LIB_TYPE "${VNE_XR_LIB_TYPE}")
    _xr_restore_global_build_flags(_saved_tests _saved_examples)
endfunction()

function(xr_configure_rhi_deps)
    include("${CMAKE_CURRENT_FUNCTION_LIST_DIR}/VneXrRhi.cmake")
    xr_configure_vnerhi_options()
    xr_configure_vnerhi_dep()
    xr_configure_vnemath_dep()
    xr_configure_vneevents_dep()
endfunction()
