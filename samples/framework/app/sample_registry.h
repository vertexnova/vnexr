#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/types.h"

#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace vne::xr::samples {

class XrSampleApp;

/** @brief One runnable sample: how to open its session and which layers to push. */
struct SampleInfo {
    std::string name;         //!< Selection key, e.g. "stereo_cube" (command line `--sample`, Android intent extra).
    std::string description;  //!< One line shown by `--list`.
    BackendType backend = BackendType::eNull;
    std::function<void(XrSampleApp&)> setup;  //!< Pushes the sample's layers onto the app.
};

/**
 * @brief The samples a launcher can run.
 *
 * Samples are added by explicit `register<Name>Sample()` calls, listed in a CMake-generated
 * `registerAllSamples()`, so a static library can never drop one at link time.
 */
class SampleRegistry {
   public:
    void add(SampleInfo info);

    [[nodiscard]] const std::vector<SampleInfo>& samples() const noexcept { return samples_; }
    /** @brief The sample called @p name, or null. */
    [[nodiscard]] const SampleInfo* find(std::string_view name) const noexcept;
    /** @brief The sample to run when none is requested: the first one added. Null when the registry is empty. */
    [[nodiscard]] const SampleInfo* defaultSample() const noexcept;

   private:
    std::vector<SampleInfo> samples_;
};

/** @brief Defined in the CMake-generated registered_samples.cpp: registers every sample built into this binary. */
void registerAllSamples(SampleRegistry& registry);

}  // namespace vne::xr::samples
