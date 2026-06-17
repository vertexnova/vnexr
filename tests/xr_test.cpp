/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>
#include "vertexnova/xr/xr.h"

TEST(VneXR, GetVersion) {
    const char* ver = vne::xr::getVersion();
    ASSERT_NE(ver, nullptr);
    EXPECT_STRNE(ver, "");
}

TEST(VneXR, Hello) {
    const char* msg = vne::xr::hello();
    ASSERT_NE(msg, nullptr);
    EXPECT_STREQ(msg, "Hello from VneXR");
}
