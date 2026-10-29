#==============================================================================
# vnexr_samples -- one binary that runs any registered sample by name
#   desktop: vnexr_samples --list | --sample <name>
#   Android: NativeActivity library; the sample comes from the launch intent extra "sample"
#==============================================================================

get_property(_register_functions GLOBAL PROPERTY VNE_XR_SAMPLE_REGISTER_FUNCTIONS)
get_property(_sample_libraries GLOBAL PROPERTY VNE_XR_SAMPLE_LIBRARIES)

set(SAMPLE_REGISTER_DECLARATIONS "")
set(SAMPLE_REGISTER_CALLS "")
foreach(_function IN LISTS _register_functions)
    string(APPEND SAMPLE_REGISTER_DECLARATIONS "void ${_function}(SampleRegistry& registry);\n")
    string(APPEND SAMPLE_REGISTER_CALLS "    ${_function}(registry);\n")
endforeach()
configure_file(registered_samples.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/registered_samples.cpp" @ONLY)

if(ANDROID)
    # NativeActivity loads the shared library named by android.app.lib_name.
    add_library(vnexr_samples SHARED android_main.cpp "${CMAKE_CURRENT_BINARY_DIR}/registered_samples.cpp")
    target_link_libraries(vnexr_samples PRIVATE android log native_app_glue)
    target_link_options(vnexr_samples PRIVATE "-u" "ANativeActivity_onCreate")
else()
    add_executable(vnexr_samples main.cpp "${CMAKE_CURRENT_BINARY_DIR}/registered_samples.cpp")
endif()

target_link_libraries(vnexr_samples PRIVATE vne::xr::samples::framework ${_sample_libraries})
set_target_properties(vnexr_samples PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin/samples"
)

if(ANDROID)
    include(VneXrAndroidGradle)
    vne_xr_register_android_sample(vnexr_samples FLAVOR samples LABEL "VNE XR Samples")
endif()
