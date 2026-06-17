#==============================================================================
# 04_openxr_windows: Win32 entry for PC VR / OpenXR simulators (Monado, MetaXR)
#==============================================================================

if(NOT WIN32 OR NOT VNE_XR_WITH_OPENXR)
    return()
endif()

add_executable(example_04_openxr_windows WIN32 main.cpp win32_loop.cpp)

target_link_libraries(example_04_openxr_windows PRIVATE vne::xr vne::logging)

if(VNE_XR_WITH_RHI)
    target_link_libraries(example_04_openxr_windows PRIVATE vne::rhi vne::math)
endif()

target_include_directories(example_04_openxr_windows PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_04_openxr_windows PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
