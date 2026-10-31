#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/flat_color_renderer.h"

#include <string>
#include <string_view>
#include <vector>

namespace vne::xr::samples {

/** @brief Geometry of a line of text, before it is uploaded. Text lies in the XY plane, growing right and down. */
struct TextGeometry {
    std::vector<FlatVertex> vertices;
    std::vector<std::uint16_t> indices;
    float width = 0.f;
    float height = 0.f;
};

/** @brief Width in meters of @p text drawn with square font pixels of @p pixel_size meters. */
[[nodiscard]] float measureText(std::string_view text, float pixel_size);

/**
 * @brief Quads for @p text with its top-left corner at the origin.
 * @details Uses `kFullyLitNormal`, so the text shows its exact color. Output is capped to what a 16-bit index can
 * address; the rest of an over-long string is dropped.
 */
[[nodiscard]] TextGeometry buildTextGeometry(std::string_view text, float pixel_size);

/**
 * @brief Break @p text into at most @p max_lines lines of at most @p max_chars characters, on word boundaries.
 * @details The last line ends in "..." when the text did not fit.
 */
[[nodiscard]] std::vector<std::string> wrapText(std::string_view text, std::size_t max_chars, std::size_t max_lines);

}  // namespace vne::xr::samples
