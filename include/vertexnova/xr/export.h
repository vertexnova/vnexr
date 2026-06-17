#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ---------------------------------------------------------------------- */

#if defined(_WIN32) && defined(VNE_XR_DLL)
#if defined(VNE_XR_BUILDING_DLL)
#define VNE_XR_API __declspec(dllexport)
#else
#define VNE_XR_API __declspec(dllimport)
#endif
#else
#define VNE_XR_API
#endif
