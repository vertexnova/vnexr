#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "common/xr_math.h"

#include <cstddef>
#include <optional>
#include <vector>

namespace vne::xr::samples {

/** @brief Axis-aligned rectangle in panel coordinates (meters, origin at the panel center, +Y up). */
struct PanelRect {
    float center_x = 0.f;
    float center_y = 0.f;
    float half_width = 0.f;
    float half_height = 0.f;

    [[nodiscard]] bool contains(float x, float y) const {
        return x >= center_x - half_width && x <= center_x + half_width && y >= center_y - half_height
               && y <= center_y + half_height;
    }
};

/** @brief Where every part of the launcher panel sits. Pure geometry, so it can be tested without a headset. */
struct LauncherLayout {
    PanelRect panel;
    PanelRect title_area;
    PanelRect footer_area;
    std::vector<PanelRect> cards;
};

/** @brief Two-column grid of @p card_count cards under a title, above a footer. */
[[nodiscard]] LauncherLayout buildLauncherLayout(std::size_t card_count);

/** @brief Index of the card under (@p x, @p y), if any. */
[[nodiscard]] std::optional<std::size_t> hitTestCards(const LauncherLayout& layout, float x, float y);

struct PanelHit {
    float x = 0.f;  //!< Panel coordinates of the hit.
    float y = 0.f;
    float distance = 0.f;  //!< Along the ray.
};

/**
 * @brief Where the ray from @p origin along @p direction meets the panel plane.
 * @param panel Panel placement: position is its center, its +Z is the face the user sees.
 * @return Nothing when the ray is parallel to, points away from, or starts behind the panel.
 */
[[nodiscard]] std::optional<PanelHit> intersectPanel(const xrmath::Similarity& panel,
                                                     xrmath::Vec3 origin,
                                                     xrmath::Vec3 direction);

}  // namespace vne::xr::samples
