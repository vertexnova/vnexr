/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ----------------------------------------------------------------------
 *
 * @file framework_launcher_test.cpp
 * @brief Launcher panel geometry, pointing, and text layout.
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#include "launcher/bitmap_font.h"
#include "launcher/launcher_layout.h"
#include "launcher/text_mesh.h"

namespace vne::xr::samples {

TEST(LauncherLayoutTest, CardsFormATwoColumnGridInsideThePanel) {
    const LauncherLayout layout = buildLauncherLayout(5);

    ASSERT_EQ(layout.cards.size(), 5u);
    for (const PanelRect& card : layout.cards) {
        EXPECT_GE(card.center_x - card.half_width, layout.panel.center_x - layout.panel.half_width);
        EXPECT_LE(card.center_x + card.half_width, layout.panel.center_x + layout.panel.half_width);
        EXPECT_GE(card.center_y - card.half_height,
                  layout.footer_area.center_y + layout.footer_area.half_height - 1e-4f);
        EXPECT_LE(card.center_y + card.half_height, layout.title_area.center_y - layout.title_area.half_height + 1e-4f);
    }
    EXPECT_LT(layout.cards[0].center_x, layout.cards[1].center_x) << "Left then right";
    EXPECT_FLOAT_EQ(layout.cards[0].center_x, layout.cards[2].center_x) << "Same column";
    EXPECT_GT(layout.cards[0].center_y, layout.cards[2].center_y) << "Rows go down";
}

TEST(LauncherLayoutTest, CardsDoNotOverlap) {
    const LauncherLayout layout = buildLauncherLayout(6);

    for (std::size_t a = 0; a < layout.cards.size(); ++a) {
        for (std::size_t b = a + 1; b < layout.cards.size(); ++b) {
            const PanelRect& first = layout.cards[a];
            const PanelRect& second = layout.cards[b];
            const bool is_separate_x =
                std::abs(first.center_x - second.center_x) >= first.half_width + second.half_width;
            const bool is_separate_y =
                std::abs(first.center_y - second.center_y) >= first.half_height + second.half_height;
            EXPECT_TRUE(is_separate_x || is_separate_y) << "cards " << a << " and " << b;
        }
    }
}

TEST(LauncherLayoutTest, HitTestFindsTheCardUnderThePoint) {
    const LauncherLayout layout = buildLauncherLayout(3);

    for (std::size_t i = 0; i < layout.cards.size(); ++i) {
        const auto hit = hitTestCards(layout, layout.cards[i].center_x, layout.cards[i].center_y);
        ASSERT_TRUE(hit.has_value());
        EXPECT_EQ(*hit, i);
    }
    EXPECT_FALSE(hitTestCards(layout, 10.f, 10.f).has_value());
    EXPECT_FALSE(hitTestCards(layout, layout.title_area.center_x, layout.title_area.center_y).has_value());
}

TEST(LauncherLayoutTest, AnEmptyLauncherStillHasAPanel) {
    const LauncherLayout layout = buildLauncherLayout(0);

    EXPECT_TRUE(layout.cards.empty());
    EXPECT_GT(layout.panel.half_width, 0.f);
    EXPECT_GT(layout.panel.half_height, 0.f);
}

TEST(IntersectPanelTest, RayStraightAtThePanelHitsItsCenter) {
    xrmath::Similarity panel;
    panel.t = {0.f, 1.f, -1.3f};

    const auto hit = intersectPanel(panel, {0.f, 1.f, 0.f}, {0.f, 0.f, -1.f});

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->x, 0.f, 1e-5f);
    EXPECT_NEAR(hit->y, 0.f, 1e-5f);
    EXPECT_NEAR(hit->distance, 1.3f, 1e-5f);
}

TEST(IntersectPanelTest, HitCoordinatesFollowThePanelAxes) {
    xrmath::Similarity panel;
    panel.t = {0.f, 1.f, -1.f};

    const auto hit = intersectPanel(panel, {0.25f, 1.1f, 0.f}, {0.f, 0.f, -1.f});

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->x, 0.25f, 1e-5f) << "Right of center is +x";
    EXPECT_NEAR(hit->y, 0.1f, 1e-5f) << "Above center is +y";
}

TEST(IntersectPanelTest, ATurnedPanelUsesItsOwnAxes) {
    xrmath::Similarity panel;
    panel.t = {-1.f, 1.f, 0.f};
    // Facing +X, toward a user standing at the origin.
    panel.q = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, 3.14159265f * 0.5f);

    const auto hit = intersectPanel(panel, {0.f, 1.f, 0.f}, {-1.f, 0.f, 0.f});

    ASSERT_TRUE(hit.has_value());
    EXPECT_NEAR(hit->distance, 1.f, 1e-4f);
    EXPECT_NEAR(hit->x, 0.f, 1e-4f);
}

TEST(IntersectPanelTest, MissesRaysThatPointAwayRunParallelOrStartBehind) {
    xrmath::Similarity panel;
    panel.t = {0.f, 1.f, -1.f};

    EXPECT_FALSE(intersectPanel(panel, {0.f, 1.f, 0.f}, {0.f, 0.f, 1.f})) << "Pointing away";
    EXPECT_FALSE(intersectPanel(panel, {0.f, 1.f, 0.f}, {1.f, 0.f, 0.f})) << "Parallel";
    EXPECT_FALSE(intersectPanel(panel, {0.f, 1.f, -2.f}, {0.f, 0.f, 1.f})) << "From behind the panel";
}

TEST(BitmapFontTest, EveryPrintableAsciiCharacterHasAGlyph) {
    for (char c = 'A'; c <= 'Z'; ++c) {
        const GlyphRows rows = glyphFor(c);
        int lit = 0;
        for (const auto row : rows) {
            lit += (row & 1) + ((row >> 1) & 1) + ((row >> 2) & 1);
        }
        EXPECT_GT(lit, 0) << c;
    }
    EXPECT_EQ(glyphFor('a'), glyphFor('A')) << "Lower case shows as upper case";
    EXPECT_EQ(glyphFor('\x01'), glyphFor('?')) << "Unknown characters show '?'";
}

TEST(TextMeshTest, MeasureCountsGlyphAdvancesWithoutTheTrailingGap) {
    EXPECT_FLOAT_EQ(measureText("", 0.01f), 0.f);
    EXPECT_NEAR(measureText("AB", 0.01f), (2 * kGlyphAdvance - 1) * 0.01f, 1e-6f);
}

TEST(TextMeshTest, GeometryHasFourVerticesAndSixIndicesPerLitPixel) {
    const TextGeometry geometry = buildTextGeometry("A", 0.01f);  // 'A' lights 10 pixels.

    EXPECT_EQ(geometry.vertices.size(), 40u);
    EXPECT_EQ(geometry.indices.size(), 60u);
    for (const auto index : geometry.indices) {
        EXPECT_LT(index, geometry.vertices.size());
    }
    for (const FlatVertex& vertex : geometry.vertices) {
        EXPECT_LE(vertex.position[1], 1e-6f) << "Text grows downward from the top-left origin";
        EXPECT_GE(vertex.position[0], -1e-6f);
    }
    EXPECT_TRUE(buildTextGeometry("   ", 0.01f).vertices.empty());
}

TEST(TextMeshTest, WrapBreaksOnWordsAndTruncatesWithAnEllipsis) {
    const auto fits = wrapText("one two", 20, 2);
    ASSERT_EQ(fits.size(), 1u);
    EXPECT_EQ(fits[0], "one two");

    const auto wrapped = wrapText("alpha beta gamma", 10, 3);
    ASSERT_EQ(wrapped.size(), 2u);
    EXPECT_EQ(wrapped[0], "alpha beta");
    EXPECT_EQ(wrapped[1], "gamma");

    const auto truncated = wrapText("alpha beta gamma delta epsilon", 10, 2);
    ASSERT_EQ(truncated.size(), 2u);
    for (const auto& line : truncated) {
        EXPECT_LE(line.size(), 10u);
    }
    EXPECT_GE(truncated.back().size(), 3u);
    EXPECT_EQ(truncated.back().substr(truncated.back().size() - 3), "...");
}

}  // namespace vne::xr::samples
