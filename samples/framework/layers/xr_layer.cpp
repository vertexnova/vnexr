/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "layers/xr_layer.h"

namespace vne::xr::samples {

IXrLayer::IXrLayer(std::string name)
    : name_(std::move(name)) {}

}  // namespace vne::xr::samples
