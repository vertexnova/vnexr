#==============================================================================
# 05_visionos_immersive: SwiftUI shell + CompositorServices bridge (visionOS only)
#==============================================================================

if(NOT VNE_XR_WITH_VISIONOS OR NOT VNE_TARGET_PLATFORM STREQUAL "visionOS")
    return()
endif()

enable_language(OBJCXX)

add_library(vnexr_visionos_bridge STATIC
    bridge/VisionOsXrBridge.mm
)

target_link_libraries(vnexr_visionos_bridge PUBLIC vne::xr)
target_include_directories(vnexr_visionos_bridge PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/bridge)

if(VNE_XR_COMPOSITOR_SERVICES_FRAMEWORK)
    target_link_libraries(vnexr_visionos_bridge PRIVATE "${VNE_XR_COMPOSITOR_SERVICES_FRAMEWORK}")
endif()

add_executable(sample_90_visionos_immersive
    cpp/main.cpp
)

target_link_libraries(sample_90_visionos_immersive PRIVATE vnexr_visionos_bridge vne::logging)

set_target_properties(sample_90_visionos_immersive PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

# SwiftUI shell is built via Xcode project overlay; C++ core runs the portable session loop.
message(STATUS "visionOS: build Swift shell from samples/90_visionos_immersive/swift/ in Xcode")
