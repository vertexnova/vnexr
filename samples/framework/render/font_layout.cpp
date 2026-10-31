/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "render/font_layout.h"

namespace vne::xr::samples {

namespace {
std::uint32_t asciiOrQuestionMark(char c) {
    const auto byte = static_cast<unsigned char>(c);
    return byte < 0x80 ? byte : static_cast<std::uint32_t>('?');
}
}  // namespace

float measureFontText(const FontAtlas& atlas, std::string_view text, float em_meters) {
    float width = 0.f;
    for (const char c : text) {
        if (const FontGlyph* glyph = atlas.find(asciiOrQuestionMark(c))) {
            width += glyph->advance;
        }
    }
    return width * em_meters;
}

FontGeometry layoutFontText(const FontAtlas& atlas, std::string_view text, float em_meters) {
    FontGeometry geometry;
    constexpr std::size_t kMaxVertices = 65532;
    const float atlas_w = static_cast<float>(atlas.width);
    const float atlas_h = static_cast<float>(atlas.height);
    float pen = 0.f;  // em
    for (const char c : text) {
        const FontGlyph* glyph = atlas.find(asciiOrQuestionMark(c));
        if (glyph == nullptr) {
            continue;
        }
        if (glyph->hasBitmap()) {
            if (geometry.vertices.size() + 4 > kMaxVertices) {
                break;
            }
            // Cap line at y = 0, baseline at y = -cap_height.
            const float x0 = (pen + glyph->plane_left) * em_meters;
            const float x1 = (pen + glyph->plane_right) * em_meters;
            const float y_top = (glyph->plane_top - atlas.cap_height) * em_meters;
            const float y_bottom = (glyph->plane_bottom - atlas.cap_height) * em_meters;
            const float u0 = glyph->atlas_left / atlas_w;
            const float u1 = glyph->atlas_right / atlas_w;
            const float v_top = glyph->atlas_top / atlas_h;
            const float v_bottom = glyph->atlas_bottom / atlas_h;
            const auto base = static_cast<std::uint16_t>(geometry.vertices.size());
            geometry.vertices.push_back({{x0, y_bottom, 0.f}, {u0, v_bottom}});
            geometry.vertices.push_back({{x1, y_bottom, 0.f}, {u1, v_bottom}});
            geometry.vertices.push_back({{x1, y_top, 0.f}, {u1, v_top}});
            geometry.vertices.push_back({{x0, y_top, 0.f}, {u0, v_top}});
            for (const int offset : {0, 1, 2, 0, 2, 3}) {
                geometry.indices.push_back(static_cast<std::uint16_t>(base + offset));
            }
        }
        pen += glyph->advance;
    }
    geometry.width = pen * em_meters;
    return geometry;
}

std::vector<std::string> wrapTextToWidth(std::string_view text,
                                         float max_width,
                                         std::size_t max_lines,
                                         const std::function<float(std::string_view)>& measure) {
    std::vector<std::string> lines;
    if (max_lines == 0) {
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
        if (current.empty() || measure(candidate) <= max_width) {
            current = candidate;
            continue;
        }
        lines.push_back(current);
        current = word;
        if (lines.size() == max_lines) {
            is_truncated = true;
            break;
        }
    }
    if (!is_truncated && !current.empty()) {
        lines.push_back(current);
    }
    if (is_truncated && !lines.empty()) {
        std::string& last = lines.back();
        while (!last.empty() && measure(last + "...") > max_width) {
            last.pop_back();
        }
        last += "...";
    }
    return lines;
}

}  // namespace vne::xr::samples
