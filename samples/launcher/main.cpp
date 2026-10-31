/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file main.cpp
 * @brief Desktop launcher: vnexr_samples [--list] [--launcher] [--sample <name>].
 */

#include "app/sample_launcher.h"
#include "app/sample_registry.h"
#include "app/xr_sample_host.h"
#include "framework/common/logging_guard.h"

#include <cstdio>
#include <cstring>
#include <string_view>

namespace {

void printUsage(const vne::xr::samples::SampleRegistry& registry) {
    std::printf("Usage: vnexr_samples [--list] [--launcher] [--sample <name>]\n\nSamples:\n");
    for (const auto& sample : registry.samples()) {
        std::printf("  %-18s %s\n", sample.name.c_str(), sample.description.c_str());
    }
    if (const auto* fallback = registry.defaultSample()) {
        std::printf("\nWith no --sample, '%s' runs.\n", fallback->name.c_str());
    }
    std::printf("--launcher opens the in-headset launcher (OpenXR runtime, for example Monado).\n");
}

}  // namespace

int main(int argc, char** argv) {
    vne::xr::samples::LoggingGuard guard;
    vne::xr::samples::SampleRegistry registry;
    vne::xr::samples::registerAllSamples(registry);

    std::string_view requested;
    bool is_launcher = false;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--list") == 0 || std::strcmp(argv[i], "--help") == 0) {
            printUsage(registry);
            return 0;
        }
        if (std::strcmp(argv[i], "--launcher") == 0) {
            is_launcher = true;
        } else if (std::strcmp(argv[i], "--sample") == 0 && i + 1 < argc) {
            requested = argv[++i];
        } else {
            std::fprintf(stderr, "Unknown argument: %s\n\n", argv[i]);
            printUsage(registry);
            return 2;
        }
    }

    if (is_launcher) {
        vne::xr::SessionConfig config;
        config.backend = vne::xr::BackendType::eOpenXr;
        config.application_name = "vnexr";
        vne::xr::samples::XrSampleHost host(registry, config);
        host.setInitialSample(std::string(requested));
        return host.run();
    }

    const vne::xr::samples::SampleInfo* sample =
        requested.empty() ? registry.defaultSample() : registry.find(requested);
    if (sample == nullptr) {
        std::fprintf(stderr, "No sample named '%.*s'.\n\n", static_cast<int>(requested.size()), requested.data());
        printUsage(registry);
        return 2;
    }
    return vne::xr::samples::launchSample(*sample);
}
