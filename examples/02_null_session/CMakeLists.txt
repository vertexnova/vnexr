#==============================================================================
# 02_null_session: Null XR backend + vnerhi Null GPU device
#==============================================================================

add_executable(example_02_null_session main.cpp)

target_link_libraries(example_02_null_session PRIVATE vne::xr vne::logging)

if(VNE_XR_WITH_RHI)
    target_link_libraries(example_02_null_session PRIVATE vne::rhi vne::math)
endif()

target_include_directories(example_02_null_session PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)

set_target_properties(example_02_null_session PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/examples"
)
