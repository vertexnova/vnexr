/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "app/sample_registry.h"

#include "framework/common/logging_guard.h"

#include <algorithm>
#include <utility>

namespace vne::xr::samples {

void SampleRegistry::add(SampleInfo info) {
    if (find(info.name) != nullptr) {
        VNE_LOG_WARN << "Sample '" << info.name << "' is registered twice; ignoring the second registration";
        return;
    }
    samples_.push_back(std::move(info));
}

const SampleInfo* SampleRegistry::find(std::string_view name) const noexcept {
    const auto it =
        std::find_if(samples_.begin(), samples_.end(), [name](const SampleInfo& info) { return info.name == name; });
    return it == samples_.end() ? nullptr : &*it;
}

const SampleInfo* SampleRegistry::defaultSample() const noexcept {
    return samples_.empty() ? nullptr : &samples_.front();
}

}  // namespace vne::xr::samples
