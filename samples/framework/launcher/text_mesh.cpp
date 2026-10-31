/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "launcher/text_mesh.h"

#include "launcher/bitmap_font.h"

#include <algorithm>

namespace vne::xr::samples {

float measureText(std::string_view text, float pixel_size) {
    if (text.empty()) {
        return 0.f;
    }
    return static_cast<float>(static_cast<int>(text.size()) * kGlyphAdvance - 1) * pixel_size;
}

TextGeometry buildTextGeometry(std::string_view text, float pixel_size) {
    TextGeometry geometry;
    geometry.width = measureText(text, pixel_size);
    geometry.height = static_cast<float>(kGlyphHeight) * pixel_size;

    constexpr std::size_t kMaxVertices = 65532;
    int column = 0;
    for (const char character : text) {
        const GlyphRows rows = glyphFor(character);
        for (int row = 0; row < kGlyphHeight; ++row) {
            for (int bit = 0; bit < kGlyphWidth; ++bit) {
                if ((rows[static_cast<std::size_t>(row)] & (1u << (kGlyphWidth - 1 - bit))) == 0) {
                    continue;
                }
                if (geometry.vertices.size() + 4 > kMaxVertices) {
                    return geometry;
                }
                const float x0 = static_cast<float>(column * kGlyphAdvance + bit) * pixel_size;
                const float y0 = -static_cast<float>(row) * pixel_size;
                const float x1 = x0 + pixel_size;
                const float y1 = y0 - pixel_size;
                const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
                const float corners[4][2] = {{x0, y1}, {x1, y1}, {x1, y0}, {x0, y0}};
                for (const auto& corner : corners) {
                    geometry.vertices.push_back(
                        {{corner[0], corner[1], 0.f}, {kFullyLitNormal[0], kFullyLitNormal[1], kFullyLitNormal[2]}});
                }
                for (const int offset : {0, 1, 2, 0, 2, 3}) {
                    geometry.indices.push_back(static_cast<std::uint16_t>(base + offset));
                }
            }
        }
        ++column;
    }
    return geometry;
}

std::vector<std::string> wrapText(std::string_view text, std::size_t max_chars, std::size_t max_lines) {
    std::vector<std::string> lines;
    if (max_chars == 0 || max_lines == 0) {
        return lines;
    }
    std::string current;
    std::size_t position = 0;
    bool is_truncated = false;
    while (position < text.size()) {
        const std::size_t end = text.find(' ', position);
        const std::string word(text.substr(position, end == std::string_view::npos ? end : end - position));
        position = end == std::string_view::npos ? text.size() : end + 1;
        if (word.empty()) {
            continue;
        }
        const std::string candidate = current.empty() ? word : current + " " + word;
        if (candidate.size() <= max_chars) {
            current = candidate;
            continue;
        }
        if (!current.empty()) {
            lines.push_back(current);
            current.clear();
            if (lines.size() == max_lines) {
                is_truncated = true;
                break;
            }
        }
        current = word.substr(0, max_chars);
    }
    if (!is_truncated && !current.empty()) {
        lines.push_back(current);
    }
    if (lines.size() > max_lines) {
        lines.resize(max_lines);
        is_truncated = true;
    }
    if (is_truncated && !lines.empty()) {
        std::string& last = lines.back();
        if (last.size() + 3 > max_chars) {
            last.resize(max_chars > 3 ? max_chars - 3 : 0);
        }
        last += "...";
    }
    return lines;
}

}  // namespace vne::xr::samples
