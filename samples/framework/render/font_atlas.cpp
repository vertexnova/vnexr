/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/font_atlas.h"

#include <algorithm>
#include <cstring>

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "third_party/stb/stb_truetype.h"

namespace vne::xr::samples {

namespace {

constexpr float kEmPixels = 44.f;
// Pixels of field kept around each glyph. The field spans +-kPadding pixels around the outline.
constexpr int kPadding = 6;
constexpr std::uint32_t kAtlasWidth = 1024;
constexpr std::uint32_t kMaxAtlasHeight = 2048;
constexpr std::uint32_t kFirstCodepoint = 32;
constexpr std::uint32_t kLastCodepoint = 126;

struct BakedGlyph {
    FontGlyph glyph;
    std::vector<std::uint8_t> field;  //!< width * height, empty for a glyph without a bitmap.
    int width = 0;
    int height = 0;
    int x = 0;
    int y = 0;
};

std::uint32_t nextPowerOfTwo(std::uint32_t value) {
    std::uint32_t result = 1;
    while (result < value) {
        result <<= 1;
    }
    return result;
}

}  // namespace

const FontGlyph* FontAtlas::find(std::uint32_t codepoint) const {
    const auto lookup = [this](std::uint32_t cp) -> const FontGlyph* {
        const auto it = std::lower_bound(glyphs.begin(), glyphs.end(), cp, [](const FontGlyph& g, std::uint32_t value) {
            return g.codepoint < value;
        });
        return it != glyphs.end() && it->codepoint == cp ? &*it : nullptr;
    };
    if (const FontGlyph* glyph = lookup(codepoint)) {
        return glyph;
    }
    return lookup('?');
}

bool bakeFontAtlas(const std::uint8_t* ttf, std::size_t size, FontAtlas& out) {
    out = {};
    if (ttf == nullptr || size < 12) {
        return false;
    }
    stbtt_fontinfo info{};
    const int offset = stbtt_GetFontOffsetForIndex(ttf, 0);
    if (offset < 0 || stbtt_InitFont(&info, ttf, offset) == 0) {
        return false;
    }

    const float scale = stbtt_ScaleForMappingEmToPixels(&info, kEmPixels);
    // The field goes from 0 to 255 across 2 * kPadding pixels, with 128 on the outline.
    const float field_per_pixel = 128.f / static_cast<float>(kPadding);

    std::vector<BakedGlyph> baked;
    baked.reserve(kLastCodepoint - kFirstCodepoint + 1);
    for (std::uint32_t cp = kFirstCodepoint; cp <= kLastCodepoint; ++cp) {
        BakedGlyph entry;
        entry.glyph.codepoint = cp;
        int advance = 0;
        int left_bearing = 0;
        stbtt_GetCodepointHMetrics(&info, static_cast<int>(cp), &advance, &left_bearing);
        entry.glyph.advance = static_cast<float>(advance) * scale / kEmPixels;

        int w = 0;
        int h = 0;
        int x_offset = 0;
        int y_offset = 0;
        unsigned char* field = stbtt_GetCodepointSDF(&info,
                                                     scale,
                                                     static_cast<int>(cp),
                                                     kPadding,
                                                     128,
                                                     field_per_pixel,
                                                     &w,
                                                     &h,
                                                     &x_offset,
                                                     &y_offset);
        if (field != nullptr && w > 0 && h > 0) {
            entry.width = w;
            entry.height = h;
            entry.field.assign(field, field + static_cast<std::size_t>(w) * static_cast<std::size_t>(h));
            // stb offsets are y down from the baseline; the plane is y up.
            entry.glyph.plane_left = static_cast<float>(x_offset) / kEmPixels;
            entry.glyph.plane_right = static_cast<float>(x_offset + w) / kEmPixels;
            entry.glyph.plane_top = -static_cast<float>(y_offset) / kEmPixels;
            entry.glyph.plane_bottom = -static_cast<float>(y_offset + h) / kEmPixels;
        }
        if (field != nullptr) {
            stbtt_FreeSDF(field, nullptr);
        }
        baked.push_back(std::move(entry));
    }

    // Shelf packing with a 1 pixel gap, left to right, top to bottom.
    int pen_x = 0;
    int pen_y = 0;
    int row_height = 0;
    for (BakedGlyph& entry : baked) {
        if (entry.width == 0) {
            continue;
        }
        if (pen_x + entry.width + 1 > static_cast<int>(kAtlasWidth)) {
            pen_x = 0;
            pen_y += row_height + 1;
            row_height = 0;
        }
        entry.x = pen_x;
        entry.y = pen_y;
        pen_x += entry.width + 1;
        row_height = std::max(row_height, entry.height);
    }
    const std::uint32_t height = nextPowerOfTwo(static_cast<std::uint32_t>(pen_y + row_height + 1));
    if (height > kMaxAtlasHeight) {
        return false;
    }

    out.width = kAtlasWidth;
    out.height = height;
    out.em_pixels = kEmPixels;
    out.distance_range = static_cast<float>(2 * kPadding);
    out.rgba.assign(static_cast<std::size_t>(out.width) * out.height * 4, 0);
    for (BakedGlyph& entry : baked) {
        if (entry.width > 0) {
            for (int row = 0; row < entry.height; ++row) {
                for (int column = 0; column < entry.width; ++column) {
                    const std::uint8_t value =
                        entry.field[static_cast<std::size_t>(row) * static_cast<std::size_t>(entry.width)
                                    + static_cast<std::size_t>(column)];
                    const std::size_t pixel = (static_cast<std::size_t>(entry.y + row) * out.width
                                               + static_cast<std::size_t>(entry.x + column))
                                              * 4;
                    out.rgba[pixel + 0] = value;
                    out.rgba[pixel + 1] = value;
                    out.rgba[pixel + 2] = value;
                    out.rgba[pixel + 3] = value;
                }
            }
            entry.glyph.atlas_left = static_cast<float>(entry.x);
            entry.glyph.atlas_top = static_cast<float>(entry.y);
            entry.glyph.atlas_right = static_cast<float>(entry.x + entry.width);
            entry.glyph.atlas_bottom = static_cast<float>(entry.y + entry.height);
        }
        out.glyphs.push_back(entry.glyph);
    }

    int ascent = 0;
    int descent = 0;
    int line_gap = 0;
    stbtt_GetFontVMetrics(&info, &ascent, &descent, &line_gap);
    out.ascender = static_cast<float>(ascent) * scale / kEmPixels;
    out.descender = static_cast<float>(descent) * scale / kEmPixels;
    out.line_height = static_cast<float>(ascent - descent + line_gap) * scale / kEmPixels;
    int x0 = 0;
    int y0 = 0;
    int x1 = 0;
    int y1 = 0;
    if (stbtt_GetCodepointBox(&info, 'H', &x0, &y0, &x1, &y1) != 0 && y1 > 0) {
        out.cap_height = static_cast<float>(y1) * scale / kEmPixels;
    }
    return out.isValid();
}

}  // namespace vne::xr::samples
