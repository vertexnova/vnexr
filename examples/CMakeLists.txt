#==============================================================================
# Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
# Licensed under the Apache License, Version 2.0 (the "License")
#==============================================================================

if(NOT TARGET vnexr)
    return()
endif()

add_subdirectory(01_hello_xr)
add_subdirectory(02_null_session)
add_subdirectory(03_hello_openxr)
add_subdirectory(04_openxr_windows)
add_subdirectory(05_visionos_immersive)
add_subdirectory(06_gfx_xr_scene)
