#==============================================================================
# vnexr samples framework (layers + XrSampleApp)
#==============================================================================

if(NOT TARGET vnexr)
    return()
endif()

add_library(vne_xr_samples_framework STATIC
    layers/xr_layer.cpp
    layers/xr_layer_stack.cpp
    app/xr_sample_app.cpp
    app/platform/desktop_loop.cpp
)

add_library(vne::xr::samples::framework ALIAS vne_xr_samples_framework)

target_include_directories(vne_xr_samples_framework
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(vne_xr_samples_framework
    PUBLIC
        vne::xr
        vne::logging
)

target_compile_features(vne_xr_samples_framework PUBLIC cxx_std_20)
