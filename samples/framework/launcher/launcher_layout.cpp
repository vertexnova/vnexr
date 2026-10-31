/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "launcher/launcher_layout.h"

#include <cmath>

namespace vne::xr::samples {

namespace {
constexpr std::size_t kColumns = 2;
constexpr float kCardHalfWidth = 0.31f;
constexpr float kCardHalfHeight = 0.12f;
constexpr float kGap = 0.04f;
constexpr float kTitleHeight = 0.20f;
constexpr float kFooterHeight = 0.14f;
}  // namespace

LauncherLayout buildLauncherLayout(std::size_t card_count) {
    LauncherLayout layout;
    const std::size_t rows = (card_count + kColumns - 1) / kColumns;
    const float grid_width =
        static_cast<float>(kColumns) * (2.f * kCardHalfWidth) + static_cast<float>(kColumns - 1) * kGap;
    const float grid_height =
        static_cast<float>(rows) * (2.f * kCardHalfHeight) + static_cast<float>(rows > 0 ? rows - 1 : 0) * kGap;

    const float panel_width = grid_width + 2.f * kGap;
    const float panel_height = kTitleHeight + grid_height + kFooterHeight + 2.f * kGap;
    layout.panel = {0.f, 0.f, panel_width * 0.5f, panel_height * 0.5f};

    const float top = panel_height * 0.5f;
    layout.title_area = {0.f, top - kTitleHeight * 0.5f, grid_width * 0.5f, kTitleHeight * 0.5f};
    layout.footer_area = {0.f, -top + kFooterHeight * 0.5f, grid_width * 0.5f, kFooterHeight * 0.5f};

    const float grid_top = top - kTitleHeight - kGap * 0.5f;
    for (std::size_t i = 0; i < card_count; ++i) {
        const std::size_t row = i / kColumns;
        const std::size_t column = i % kColumns;
        const float x =
            -grid_width * 0.5f + kCardHalfWidth + static_cast<float>(column) * (2.f * kCardHalfWidth + kGap);
        const float y = grid_top - kCardHalfHeight - static_cast<float>(row) * (2.f * kCardHalfHeight + kGap);
        layout.cards.push_back({x, y, kCardHalfWidth, kCardHalfHeight});
    }
    return layout;
}

std::optional<std::size_t> hitTestCards(const LauncherLayout& layout, float x, float y) {
    for (std::size_t i = 0; i < layout.cards.size(); ++i) {
        if (layout.cards[i].contains(x, y)) {
            return i;
        }
    }
    return std::nullopt;
}

std::optional<PanelHit> intersectPanel(const xrmath::Similarity& panel, xrmath::Vec3 origin, xrmath::Vec3 direction) {
    const xrmath::Vec3 normal = xrmath::rotate(panel.q, {0.f, 0.f, 1.f});
    const float denominator = xrmath::dot(direction, normal);
    // Facing the user means the ray runs against the normal.
    if (denominator > -1e-4f) {
        return std::nullopt;
    }
    const float distance = xrmath::dot(panel.t - origin, normal) / denominator;
    if (distance <= 0.f) {
        return std::nullopt;
    }
    const xrmath::Vec3 hit = origin + direction * distance;
    const xrmath::Vec3 offset = hit - panel.t;
    return PanelHit{xrmath::dot(offset, xrmath::rotate(panel.q, {1.f, 0.f, 0.f})),
                    xrmath::dot(offset, xrmath::rotate(panel.q, {0.f, 1.f, 0.f})),
                    distance};
}

}  // namespace vne::xr::samples
