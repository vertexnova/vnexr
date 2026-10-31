#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/font_atlas.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace vne::xr::samples {

struct FontVertex {
    float position[3];
    float uv[2];
};

/** @brief Glyph quads for one line of text, ready for a vertex and an index buffer. */
struct FontGeometry {
    std::vector<FontVertex> vertices;
    std::vector<std::uint16_t> indices;
    float width = 0.f;  //!< Meters.
};

/**
 * @brief Quads for @p text, one per glyph that has a bitmap.
 *
 * @details One em is @p em_meters. The origin is the left end of the cap line: the text grows right and its capital
 * letters hang down from y = 0 (baseline at y = -cap_height * em), matching the pixel font's top-left convention.
 * Anything past what a 16-bit index can address is dropped. Non-ASCII bytes show as '?'.
 */
[[nodiscard]] FontGeometry layoutFontText(const FontAtlas& atlas, std::string_view text, float em_meters);

/** @brief Width in meters of @p text, the sum of its glyph advances. */
[[nodiscard]] float measureFontText(const FontAtlas& atlas, std::string_view text, float em_meters);

/**
 * @brief Break @p text into at most @p max_lines lines no wider than @p max_width, on word boundaries.
 * @details A single word wider than the limit gets its own line. When the text does not fit the last line ends in
 * "..." (shortened until it fits).
 * @param measure Width of a string in the same unit as @p max_width.
 */
[[nodiscard]] std::vector<std::string> wrapTextToWidth(std::string_view text,
                                                       float max_width,
                                                       std::size_t max_lines,
                                                       const std::function<float(std::string_view)>& measure);

}  // namespace vne::xr::samples
