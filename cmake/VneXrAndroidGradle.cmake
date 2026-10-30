#==============================================================================
# Generates an Android Studio Gradle project for registered vnexr samples
# (one product flavor per registered NativeActivity library; vnexr registers just the vnexr_samples launcher).
# Modeled on vnerhi VneRhiAndroidGradle.
#==============================================================================

include_guard(GLOBAL)

define_property(GLOBAL PROPERTY VNE_XR_ANDROID_SAMPLE_NAMES
    BRIEF_DOCS "Ordered list of Android sample target names registered by vne_xr_register_android_sample"
)

function(vne_xr_android_sample_flavor_from_name SAMPLE_NAME OUT_VAR)
    # sample_02_openxr_stereo -> openxr_stereo (strip sample_ before digits); vne_xr_register_android_sample can override.
    string(REGEX REPLACE "^sample_" "" _suffix "${SAMPLE_NAME}")
    string(REGEX REPLACE "^[0-9]+" "" _suffix "${_suffix}")
    string(REGEX REPLACE "^_+" "" _suffix "${_suffix}")
    if(_suffix STREQUAL "")
        set(_suffix "sample")
    endif()
    set(${OUT_VAR} "${_suffix}" PARENT_SCOPE)
endfunction()

# XML-escape a display label, then escape that text for a Groovy single-quoted string.
# OUT_XML is for strings.xml and XML attributes. OUT_GRADLE is for build.gradle. The raw label is left unchanged.
function(vne_xr_escape_app_label RAW OUT_XML OUT_GRADLE)
    string(REPLACE "\r" " " _oneline "${RAW}")
    string(REPLACE "\n" " " _oneline "${_oneline}")
    string(REPLACE "&" "&amp;" _xml "${_oneline}")
    string(REPLACE "<" "&lt;" _xml "${_xml}")
    string(REPLACE ">" "&gt;" _xml "${_xml}")
    string(REPLACE "\"" "&quot;" _xml "${_xml}")
    string(REPLACE "'" "&apos;" _xml "${_xml}")
    string(REPLACE "\\" "\\\\" _gradle "${_xml}")
    string(REPLACE "'" "\\'" _gradle "${_gradle}")
    set(${OUT_XML} "${_xml}" PARENT_SCOPE)
    set(${OUT_GRADLE} "${_gradle}" PARENT_SCOPE)
endfunction()

# vne_xr_register_android_sample(<target> [FLAVOR <name>] [LABEL <app label>])
# Registers the NativeActivity library <target> as an APK flavor. FLAVOR defaults to a name derived from the target.
function(vne_xr_register_android_sample TARGET_NAME)
    cmake_parse_arguments(ARG "" "FLAVOR;LABEL" "" ${ARGN})
    if(NOT TARGET "${TARGET_NAME}")
        message(FATAL_ERROR "vne_xr_register_android_sample: target '${TARGET_NAME}' does not exist")
    endif()

    if(ARG_FLAVOR)
        set(_flavor "${ARG_FLAVOR}")
    else()
        vne_xr_android_sample_flavor_from_name("${TARGET_NAME}" _flavor)
    endif()
    set(_package_id "com.vertexnova.vnexr.${_flavor}")
    if(ARG_LABEL)
        set(_app_label "${ARG_LABEL}")
    else()
        set(_app_label "VNE XR ${_flavor}")
    endif()
    # VNE_ANDROID_APP_LABEL stays the raw label (vne_samples.txt). XML and Gradle copies are for generated files.
    vne_xr_escape_app_label("${_app_label}" _app_label_xml _app_label_gradle)

    set_target_properties("${TARGET_NAME}" PROPERTIES
        VNE_ANDROID_FLAVOR "${_flavor}"
        VNE_ANDROID_PACKAGE "${_package_id}"
        VNE_ANDROID_APP_LABEL "${_app_label}"
        VNE_ANDROID_APP_LABEL_XML "${_app_label_xml}"
        VNE_ANDROID_APP_LABEL_GRADLE "${_app_label_gradle}"
    )
    set_property(GLOBAL APPEND PROPERTY VNE_XR_ANDROID_SAMPLE_NAMES "${TARGET_NAME}")
endfunction()

function(vne_xr_generate_android_gradle_project)
    set(_one_value OUTPUT_DIR VNEXR_ROOT GRADLE_TEMPLATE_DIR)
    cmake_parse_arguments(_GEN "" "${_one_value}" "" ${ARGN})

    if(NOT _GEN_OUTPUT_DIR OR NOT _GEN_VNEXR_ROOT OR NOT _GEN_GRADLE_TEMPLATE_DIR)
        message(FATAL_ERROR
            "vne_xr_generate_android_gradle_project: OUTPUT_DIR, VNEXR_ROOT, GRADLE_TEMPLATE_DIR required")
    endif()

    get_property(_sample_names GLOBAL PROPERTY VNE_XR_ANDROID_SAMPLE_NAMES)
    if(NOT _sample_names)
        message(WARNING "vne_xr_generate_android_gradle_project: no Android samples registered")
        return()
    endif()

    set(_gradle_root "${_GEN_OUTPUT_DIR}")
    set(_app_gradle "${_gradle_root}/app/build.gradle")
    set(_native_cmake "${_gradle_root}/native/CMakeLists.txt")
    set(_samples_manifest "${_gradle_root}/vne_samples.txt")

    file(MAKE_DIRECTORY "${_gradle_root}/app/src/main")
    file(MAKE_DIRECTORY "${_gradle_root}/native")

    file(COPY "${_GEN_GRADLE_TEMPLATE_DIR}/settings.gradle"
         "${_GEN_GRADLE_TEMPLATE_DIR}/build.gradle"
         "${_GEN_GRADLE_TEMPLATE_DIR}/gradle.properties"
         DESTINATION "${_gradle_root}")

    file(COPY "${_GEN_GRADLE_TEMPLATE_DIR}/app/src/main/AndroidManifest.xml"
         DESTINATION "${_gradle_root}/app/src/main")
    if(EXISTS "${_GEN_GRADLE_TEMPLATE_DIR}/app/src/main/java")
        file(COPY "${_GEN_GRADLE_TEMPLATE_DIR}/app/src/main/java"
             DESTINATION "${_gradle_root}/app/src/main")
    endif()

    set(VNEXR_ROOT "${_GEN_VNEXR_ROOT}")
    configure_file(
        "${_GEN_GRADLE_TEMPLATE_DIR}/native/CMakeLists.txt.in"
        "${_native_cmake}"
        @ONLY
    )

    set(_flavor_blocks "")
    file(WRITE "${_samples_manifest}"
"# vnexr Android samples (NativeActivity + SHARED lib per flavor)
# name,flavor,package,app_label
")

    file(MAKE_DIRECTORY "${_gradle_root}/.idea/runConfigurations")

    foreach(_name IN LISTS _sample_names)
        get_target_property(_flavor "${_name}" VNE_ANDROID_FLAVOR)
        get_target_property(_package "${_name}" VNE_ANDROID_PACKAGE)
        get_target_property(_app_label "${_name}" VNE_ANDROID_APP_LABEL)
        get_target_property(_app_label_xml "${_name}" VNE_ANDROID_APP_LABEL_XML)
        get_target_property(_app_label_gradle "${_name}" VNE_ANDROID_APP_LABEL_GRADLE)

        string(APPEND _flavor_blocks "        ${_flavor} {\n")
        string(APPEND _flavor_blocks "            dimension 'sample'\n")
        string(APPEND _flavor_blocks "            applicationId '${_package}'\n")
        string(APPEND _flavor_blocks "            manifestPlaceholders = [\n")
        string(APPEND _flavor_blocks "                vneNativeLib: '${_name}',\n")
        string(APPEND _flavor_blocks "                vneAppLabel : '${_app_label_gradle}'\n")
        string(APPEND _flavor_blocks "            ]\n")
        string(APPEND _flavor_blocks "            externalNativeBuild {\n")
        string(APPEND _flavor_blocks "                cmake {\n")
        string(APPEND _flavor_blocks "                    arguments '-DVNE_XR_ANDROID_SAMPLE=${_name}',\n")
        string(APPEND _flavor_blocks "                              '-DVNE_XR_ANDROID_PACKAGE=${_package}'\n")
        string(APPEND _flavor_blocks "                    targets '${_name}'\n")
        string(APPEND _flavor_blocks "                }\n")
        string(APPEND _flavor_blocks "            }\n")
        string(APPEND _flavor_blocks "        }\n")

        file(APPEND "${_samples_manifest}" "${_name},${_flavor},${_package},${_app_label}\n")

        set(_flavor_src "${_gradle_root}/app/src/${_flavor}")
        file(MAKE_DIRECTORY "${_flavor_src}/res/values")
        file(WRITE "${_flavor_src}/res/values/strings.xml"
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<resources>\n    <string name=\"vne_app_label\">${_app_label_xml}</string>\n</resources>\n")
        file(WRITE "${_flavor_src}/AndroidManifest.xml"
"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n<manifest xmlns:android=\"http://schemas.android.com/apk/res/android\"\n    xmlns:tools=\"http://schemas.android.com/tools\">\n    <application\n        android:label=\"@string/vne_app_label\"\n        tools:replace=\"android:label\">\n        <activity\n            android:name=\"com.vertexnova.vnexr.samples.VneXrNativeActivity\"\n            android:taskAffinity=\"${_package}\"\n            tools:node=\"merge\">\n            <meta-data\n                android:name=\"android.app.lib_name\"\n                android:value=\"${_name}\"\n                tools:replace=\"android:value\" />\n        </activity>\n    </application>\n</manifest>\n")

        set(_run_config_variant "${_flavor}Debug")
        set(_run_config_name "${_app_label_xml} (${_run_config_variant})")
        set(VNE_RUN_CONFIG_NAME "${_run_config_name}")
        set(VNE_RUN_CONFIG_VARIANT "${_run_config_variant}")
        set(VNE_RUN_CONFIG_PACKAGE "${_package}")
        configure_file(
            "${_GEN_GRADLE_TEMPLATE_DIR}/idea/runConfiguration.xml.in"
            "${_gradle_root}/.idea/runConfigurations/${_flavor}Debug.xml"
            @ONLY
        )
    endforeach()

    set(VNE_ANDROID_PRODUCT_FLAVORS "${_flavor_blocks}")
    if(ANDROID_ABI)
        set(VNE_ANDROID_ABI_FILTERS "'${ANDROID_ABI}'")
    else()
        set(VNE_ANDROID_ABI_FILTERS "'arm64-v8a', 'x86_64'")
    endif()

    set(_vne_android_ndk_root "${ANDROID_NDK}")
    if((NOT _vne_android_ndk_root) AND DEFINED CMAKE_ANDROID_NDK)
        set(_vne_android_ndk_root "${CMAKE_ANDROID_NDK}")
    endif()

    if(_vne_android_ndk_root)
        get_filename_component(_vne_android_ndk_version "${_vne_android_ndk_root}" NAME)
        if(NOT _vne_android_ndk_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
            set(_vne_ndk_props "${_vne_android_ndk_root}/source.properties")
            if(EXISTS "${_vne_ndk_props}")
                file(STRINGS "${_vne_ndk_props}" _vne_ndk_props_lines)
                foreach(_line IN LISTS _vne_ndk_props_lines)
                    if(_line MATCHES "^Pkg\\.Revision[ \t]*=[ \t]*(.+)$")
                        set(_vne_android_ndk_version "${CMAKE_MATCH_1}")
                        string(STRIP "${_vne_android_ndk_version}" _vne_android_ndk_version)
                        break()
                    endif()
                endforeach()
            endif()
        endif()
        if(_vne_android_ndk_version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+$")
            set(VNE_ANDROID_NDK_VERSION_LINE "    ndkVersion \"${_vne_android_ndk_version}\"")
        else()
            set(VNE_ANDROID_NDK_VERSION_LINE "")
        endif()
    else()
        set(VNE_ANDROID_NDK_VERSION_LINE "")
    endif()

    set(VNE_ANDROID_CMAKE_VERSION_LINE "")
    # vnexr needs CMake 3.26 or newer (native/CMakeLists.txt); pick the newest SDK CMake that satisfies it, since
    # Android Studio's default 3.22.1 would fail the configure. Only the one SDK Gradle will use is searched
    # (ANDROID_HOME, then ANDROID_SDK_ROOT, then the default install folders), so the pinned version always exists
    # there. A cmake.dir entry in local.properties already selects an installation, so no version is pinned then.
    set(_vne_sdk_root "")
    foreach(_sdk_root IN ITEMS "$ENV{ANDROID_HOME}" "$ENV{ANDROID_SDK_ROOT}" "$ENV{HOME}/Android/Sdk"
                               "$ENV{HOME}/Library/Android/sdk")
        if(_sdk_root AND IS_DIRECTORY "${_sdk_root}")
            set(_vne_sdk_root "${_sdk_root}")
            break()
        endif()
    endforeach()

    set(_vne_cmake_dir_set FALSE)
    if(EXISTS "${_gradle_root}/local.properties")
        file(STRINGS "${_gradle_root}/local.properties" _vne_local_props REGEX "^[ \t]*cmake\\.dir[ \t]*=")
        if(_vne_local_props)
            set(_vne_cmake_dir_set TRUE)
        endif()
    endif()

    if(_vne_cmake_dir_set)
        message(STATUS "vne_xr_generate_android_gradle_project: cmake.dir set in local.properties; "
                       "not pinning a CMake version")
    else()
        if(_vne_sdk_root AND EXISTS "${_vne_sdk_root}/cmake")
            file(GLOB _cmake_dirs "${_vne_sdk_root}/cmake/*")
            list(SORT _cmake_dirs COMPARE NATURAL)
            list(REVERSE _cmake_dirs)
            foreach(_cmake_dir IN LISTS _cmake_dirs)
                get_filename_component(_cmake_ver "${_cmake_dir}" NAME)
                if(_cmake_ver VERSION_GREATER_EQUAL "3.26")
                    set(VNE_ANDROID_CMAKE_VERSION_LINE "            version '${_cmake_ver}'")
                    break()
                endif()
            endforeach()
        endif()
        if(VNE_ANDROID_CMAKE_VERSION_LINE STREQUAL "")
            message(WARNING "No CMake 3.26 or newer found in the Android SDK (${_vne_sdk_root}); Gradle would use "
                            "its default (3.22.1). Install one: sdkmanager \"cmake;3.31.6\"")
        endif()
    endif()

    configure_file(
        "${_GEN_GRADLE_TEMPLATE_DIR}/app/build.gradle.in"
        "${_app_gradle}"
        @ONLY
    )

    file(WRITE "${_gradle_root}/.gitignore" "local.properties\n/.gradle/\n/app/build/\n/app/.cxx/\n")

    list(JOIN _sample_names ", " _sample_names_joined)
    file(WRITE "${_gradle_root}/ANDROID_STUDIO.md"
"# vnexr Android Studio — OpenXR samples APK

One immersive OpenXR app (NativeActivity) that runs any registered sample, chosen at launch.

1. Open Build Variants and pick samplesDebug.
2. Deploy to Galaxy XR (arm64-v8a) or a Vulkan-capable device.
3. Mirror with scrcpy for demos: scrcpy --video-codec=h264

## CLI

```
./scripts/build_android.sh -a studio --dev -abi arm64-v8a
./scripts/build_android.sh -t Debug -a run -sample stereo_cube -abi arm64-v8a
```

## Registered flavors

${_sample_names_joined}
")

    message(STATUS "vne_xr_generate_android_gradle_project: generated ${_gradle_root}")
    message(STATUS "vne_xr_generate_android_gradle_project: Android flavors -> ${_sample_names_joined}")
endfunction()
