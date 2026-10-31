/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 *
 * @file framework_text_test.cpp
 * @brief Font atlas baking, glyph layout and wrapping for in-headset labels.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "render/font_atlas.h"
#include "render/font_layout.h"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <string>
#include <vector>

namespace vne::xr::samples {

namespace {

// A hand-made atlas: no font file needed. 100x100 pixels, cap height 0.7 em.
FontAtlas syntheticAtlas() {
    FontAtlas atlas;
    atlas.width = 100;
    atlas.height = 100;
    atlas.em_pixels = 40.f;
    atlas.distance_range = 12.f;
    atlas.cap_height = 0.7f;
    atlas.rgba.assign(100 * 100 * 4, 0);

    FontGlyph space;
    space.codepoint = ' ';
    space.advance = 0.25f;

    FontGlyph question;
    question.codepoint = '?';
    question.advance = 0.5f;
    question.plane_left = 0.05f;
    question.plane_right = 0.45f;
    question.plane_bottom = -0.1f;
    question.plane_top = 0.8f;
    question.atlas_left = 50.f;
    question.atlas_top = 0.f;
    question.atlas_right = 70.f;
    question.atlas_bottom = 40.f;

    FontGlyph a;
    a.codepoint = 'A';
    a.advance = 0.6f;
    a.plane_left = 0.1f;
    a.plane_right = 0.5f;
    a.plane_bottom = -0.1f;
    a.plane_top = 0.8f;
    a.atlas_left = 0.f;
    a.atlas_top = 0.f;
    a.atlas_right = 25.f;
    a.atlas_bottom = 50.f;

    atlas.glyphs = {space, question, a};  // sorted by codepoint: ' ' (32), '?' (63), 'A' (65)
    return atlas;
}

std::vector<std::uint8_t> readSystemFont() {
    for (const char* path : {"/System/Library/Fonts/Supplemental/Arial.ttf",
                             "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
                             "C:\\Windows\\Fonts\\arial.ttf"}) {
        std::ifstream file(path, std::ios::binary);
        if (file) {
            return {std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
        }
    }
    return {};
}

}  // namespace

TEST(FontLayoutTest, WidthIsTheSumOfAdvancesTimesTheEmSize) {
    const FontAtlas atlas = syntheticAtlas();

    EXPECT_NEAR(measureFontText(atlas, "AA", 0.1f), 0.12f, 1e-6f);
    EXPECT_NEAR(measureFontText(atlas, "A A", 0.2f), (0.6f + 0.25f + 0.6f) * 0.2f, 1e-6f);
    EXPECT_FLOAT_EQ(measureFontText(atlas, "", 0.1f), 0.f);
}

TEST(FontLayoutTest, EveryGlyphWithABitmapMakesOneQuadAndSpacesOnlyAdvance) {
    const FontAtlas atlas = syntheticAtlas();

    const FontGeometry geometry = layoutFontText(atlas, "A A", 0.1f);

    EXPECT_EQ(geometry.vertices.size(), 8u);
    EXPECT_EQ(geometry.indices.size(), 12u);
    for (const auto index : geometry.indices) {
        EXPECT_LT(index, geometry.vertices.size());
    }
    // The second 'A' starts after 'A' and the space.
    const float second_left = geometry.vertices[4].position[0];
    EXPECT_NEAR(second_left, (0.6f + 0.25f + 0.1f) * 0.1f, 1e-6f);
    EXPECT_NEAR(geometry.width, (0.6f + 0.25f + 0.6f) * 0.1f, 1e-6f);
}

TEST(FontLayoutTest, CapitalsHangFromTheCapLineAtYZero) {
    const FontAtlas atlas = syntheticAtlas();

    const FontGeometry geometry = layoutFontText(atlas, "A", 1.f);

    // Glyph top is 0.8 em above the baseline and the cap line is 0.7 em: the quad pokes 0.1 above y = 0 (the bitmap
    // has padding); its bottom is 0.1 em below the baseline, which sits 0.7 below the cap line.
    float top = -1e9f;
    float bottom = 1e9f;
    for (const FontVertex& v : geometry.vertices) {
        top = std::max(top, v.position[1]);
        bottom = std::min(bottom, v.position[1]);
    }
    EXPECT_NEAR(top, 0.1f, 1e-6f);
    EXPECT_NEAR(bottom, -0.8f, 1e-6f);
}

TEST(FontLayoutTest, TextureCoordinatesStayInsideTheAtlasAndTopIsUp) {
    const FontAtlas atlas = syntheticAtlas();

    const FontGeometry geometry = layoutFontText(atlas, "A", 1.f);

    ASSERT_EQ(geometry.vertices.size(), 4u);
    for (const FontVertex& v : geometry.vertices) {
        EXPECT_GE(v.uv[0], 0.f);
        EXPECT_LE(v.uv[0], 1.f);
        EXPECT_GE(v.uv[1], 0.f);
        EXPECT_LE(v.uv[1], 1.f);
    }
    // Vertices 2 and 3 are the top of the quad: atlas row 0 (v = 0); 0 and 1 are the bottom (50 / 100).
    EXPECT_FLOAT_EQ(geometry.vertices[3].uv[1], 0.f);
    EXPECT_FLOAT_EQ(geometry.vertices[0].uv[1], 0.5f);
    EXPECT_FLOAT_EQ(geometry.vertices[0].uv[0], 0.f);
    EXPECT_FLOAT_EQ(geometry.vertices[1].uv[0], 0.25f);
}

TEST(FontLayoutTest, UnknownAndNonAsciiCharactersShowAsAQuestionMark) {
    const FontAtlas atlas = syntheticAtlas();

    EXPECT_NEAR(measureFontText(atlas, "~", 1.f), 0.5f, 1e-6f);
    EXPECT_NEAR(measureFontText(atlas, "\xC3\xA9", 1.f), 1.0f, 1e-6f) << "Two bytes, two question marks";
    EXPECT_EQ(layoutFontText(atlas, "~", 1.f).vertices.size(), 4u);
}

TEST(WrapTextToWidthTest, BreaksOnWordsAndKeepsLinesWithinTheWidth) {
    const auto measure = [](std::string_view s) { return static_cast<float>(s.size()); };

    const auto lines = wrapTextToWidth("one two three", 7.f, 3, measure);

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "one two");
    EXPECT_EQ(lines[1], "three");
}

TEST(WrapTextToWidthTest, TruncatesWithAnEllipsisThatStillFits) {
    const auto measure = [](std::string_view s) { return static_cast<float>(s.size()); };

    const auto lines = wrapTextToWidth("alpha beta gamma delta", 10.f, 2, measure);

    ASSERT_EQ(lines.size(), 2u);
    EXPECT_EQ(lines[0], "alpha beta");
    EXPECT_EQ(lines[1], "gamma...");
    for (const auto& line : lines) {
        EXPECT_LE(measure(line), 10.f);
    }
}

TEST(WrapTextToWidthTest, AWordWiderThanTheLimitGetsItsOwnLine) {
    const auto measure = [](std::string_view s) { return static_cast<float>(s.size()); };

    const auto lines = wrapTextToWidth("a supercalifragilistic b", 6.f, 4, measure);

    ASSERT_EQ(lines.size(), 3u);
    EXPECT_EQ(lines[1], "supercalifragilistic");
}

TEST(WrapTextToWidthTest, EmptyTextAndZeroLinesGiveNothing) {
    const auto measure = [](std::string_view s) { return static_cast<float>(s.size()); };

    EXPECT_TRUE(wrapTextToWidth("", 10.f, 3, measure).empty());
    EXPECT_TRUE(wrapTextToWidth("text", 10.f, 0, measure).empty());
}

TEST(FontAtlasTest, FindFallsBackToTheQuestionMark) {
    const FontAtlas atlas = syntheticAtlas();

    ASSERT_NE(atlas.find('A'), nullptr);
    EXPECT_EQ(atlas.find('A')->codepoint, static_cast<std::uint32_t>('A'));
    EXPECT_EQ(atlas.find('Z')->codepoint, static_cast<std::uint32_t>('?'));
}

TEST(FontAtlasTest, RejectsDataThatIsNotAFont) {
    FontAtlas atlas;
    const std::vector<std::uint8_t> garbage(256, 0x5A);

    EXPECT_FALSE(bakeFontAtlas(garbage.data(), garbage.size(), atlas));
    EXPECT_FALSE(bakeFontAtlas(nullptr, 0, atlas));
}

TEST(FontAtlasTest, BakesPrintableAsciiFromARealFont) {
    const std::vector<std::uint8_t> font = readSystemFont();
    if (font.empty()) {
        GTEST_SKIP() << "No system TrueType font found on this machine";
    }
    FontAtlas atlas;

    ASSERT_TRUE(bakeFontAtlas(font.data(), font.size(), atlas));

    EXPECT_EQ(atlas.glyphs.size(), 95u);
    EXPECT_TRUE(atlas.isValid());
    EXPECT_GT(atlas.cap_height, 0.55f);
    EXPECT_LT(atlas.cap_height, 0.85f);
    EXPECT_GT(atlas.line_height, 1.0f);
    const FontGlyph* a = atlas.find('A');
    ASSERT_NE(a, nullptr);
    EXPECT_TRUE(a->hasBitmap());
    EXPECT_GT(a->advance, 0.4f);
    EXPECT_LT(a->plane_left, a->plane_right);
    EXPECT_LT(a->plane_bottom, a->plane_top);
    EXPECT_FALSE(atlas.find(' ')->hasBitmap()) << "The space has an advance and no bitmap";
    EXPECT_GT(atlas.find(' ')->advance, 0.f);

    // Inside the stem of an 'I' the field is well above the outline value (128); outside the glyph it is below.
    const FontGlyph* eye = atlas.find('I');
    ASSERT_NE(eye, nullptr);
    const auto at = [&atlas](float x, float y) {
        return atlas.rgba[(static_cast<std::size_t>(y) * atlas.width + static_cast<std::size_t>(x)) * 4];
    };
    const float center_x = (eye->atlas_left + eye->atlas_right) * 0.5f;
    const float center_y = (eye->atlas_top + eye->atlas_bottom) * 0.5f;
    EXPECT_GT(at(center_x, center_y), 160);
    EXPECT_LT(at(eye->atlas_left + 1.f, eye->atlas_top + 1.f), 100) << "The padding corner is far from the glyph";
    // Every channel carries the field, so a median-of-RGB shader reads it back unchanged.
    const std::size_t pixel =
        (static_cast<std::size_t>(center_y) * atlas.width + static_cast<std::size_t>(center_x)) * 4;
    EXPECT_EQ(atlas.rgba[pixel], atlas.rgba[pixel + 1]);
    EXPECT_EQ(atlas.rgba[pixel], atlas.rgba[pixel + 3]);
}

TEST(FontAtlasTest, GlyphsDoNotOverlapInTheAtlas) {
    const std::vector<std::uint8_t> font = readSystemFont();
    if (font.empty()) {
        GTEST_SKIP() << "No system TrueType font found on this machine";
    }
    FontAtlas atlas;
    ASSERT_TRUE(bakeFontAtlas(font.data(), font.size(), atlas));

    for (std::size_t i = 0; i < atlas.glyphs.size(); ++i) {
        const FontGlyph& first = atlas.glyphs[i];
        if (!first.hasBitmap()) {
            continue;
        }
        EXPECT_LE(first.atlas_right, static_cast<float>(atlas.width));
        EXPECT_LE(first.atlas_bottom, static_cast<float>(atlas.height));
        for (std::size_t j = i + 1; j < atlas.glyphs.size(); ++j) {
            const FontGlyph& second = atlas.glyphs[j];
            if (!second.hasBitmap()) {
                continue;
            }
            const bool is_separate = first.atlas_right <= second.atlas_left || second.atlas_right <= first.atlas_left
                                     || first.atlas_bottom <= second.atlas_top
                                     || second.atlas_bottom <= first.atlas_top;
            EXPECT_TRUE(is_separate) << "glyphs " << first.codepoint << " and " << second.codepoint;
        }
    }
}

}  // namespace vne::xr::samples
