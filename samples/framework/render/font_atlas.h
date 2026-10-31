#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file font_atlas.h
 * @brief CPU-side font atlas: glyph metrics plus an RGBA distance-field image, in the conventions of vnegfx's
 *        MSDF atlases so a vnegfx atlas can be loaded into the same structure later.
 */

#include <cstddef>
#include <cstdint>
#include <vector>

namespace vne::xr::samples {

/**
 * @brief One glyph. Plane bounds are in em units, y up from the baseline; atlas bounds are in atlas pixels, y down
 * from the top of the image. A glyph without a bitmap (the space) has only an advance.
 */
struct FontGlyph {
    std::uint32_t codepoint = 0;
    float advance = 0.f;  //!< Pen advance in em.
    float plane_left = 0.f;
    float plane_bottom = 0.f;
    float plane_right = 0.f;
    float plane_top = 0.f;
    float atlas_left = 0.f;
    float atlas_top = 0.f;
    float atlas_right = 0.f;
    float atlas_bottom = 0.f;

    [[nodiscard]] bool hasBitmap() const { return atlas_right > atlas_left && atlas_bottom > atlas_top; }
};

/**
 * @brief Glyph table and distance-field image for one font.
 *
 * @details The image holds the distance field in all four channels, so a shader that takes the median of the RGB
 * channels (an MSDF shader) works unchanged for this single-channel field and for a real MSDF atlas.
 */
struct FontAtlas {
    std::vector<FontGlyph> glyphs;  //!< Sorted by codepoint.
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    float em_pixels = 0.f;           //!< Atlas pixels per em.
    float distance_range = 0.f;      //!< Atlas pixels the field spans edge to edge (the field is 0.5 at the outline).
    float line_height = 1.f;         //!< em
    float ascender = 0.f;            //!< em above the baseline
    float descender = 0.f;           //!< em, negative
    float cap_height = 0.7f;         //!< em
    std::vector<std::uint8_t> rgba;  //!< width * height * 4

    [[nodiscard]] bool isValid() const {
        return width > 0 && height > 0 && rgba.size() == std::size_t(width) * height * 4 && !glyphs.empty();
    }
    /** @brief The glyph for @p codepoint, or for '?' when the font has none, or null when neither exists. */
    [[nodiscard]] const FontGlyph* find(std::uint32_t codepoint) const;
};

/**
 * @brief Bake printable ASCII (U+0020 to U+007E) of the TrueType/OpenType font in @p ttf into @p out.
 * @return False when the data is not a usable font or the glyphs do not fit a 2048-pixel-tall atlas.
 */
[[nodiscard]] bool bakeFontAtlas(const std::uint8_t* ttf, std::size_t size, FontAtlas& out);

}  // namespace vne::xr::samples
