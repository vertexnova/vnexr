#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file bitmap_font.h
 * @brief A 3x5 pixel font (upper case, digits, basic punctuation) for in-headset UI labels.
 */

#include <array>
#include <cstdint>

namespace vne::xr::samples {

inline constexpr int kGlyphWidth = 3;
inline constexpr int kGlyphHeight = 5;
/** Pixels from one glyph's left edge to the next: the glyph plus one blank column. */
inline constexpr int kGlyphAdvance = kGlyphWidth + 1;

/** @brief Five rows, top to bottom; in each row bit 2 is the left pixel and bit 0 the right one. */
using GlyphRows = std::array<std::uint8_t, kGlyphHeight>;

namespace detail {

struct GlyphEntry {
    char character;
    GlyphRows rows;
};

inline constexpr GlyphEntry kGlyphs[] = {
    {' ', {0b000, 0b000, 0b000, 0b000, 0b000}}, {'A', {0b010, 0b101, 0b111, 0b101, 0b101}},
    {'B', {0b110, 0b101, 0b110, 0b101, 0b110}}, {'C', {0b011, 0b100, 0b100, 0b100, 0b011}},
    {'D', {0b110, 0b101, 0b101, 0b101, 0b110}}, {'E', {0b111, 0b100, 0b110, 0b100, 0b111}},
    {'F', {0b111, 0b100, 0b110, 0b100, 0b100}}, {'G', {0b011, 0b100, 0b101, 0b101, 0b011}},
    {'H', {0b101, 0b101, 0b111, 0b101, 0b101}}, {'I', {0b111, 0b010, 0b010, 0b010, 0b111}},
    {'J', {0b001, 0b001, 0b001, 0b101, 0b010}}, {'K', {0b101, 0b101, 0b110, 0b101, 0b101}},
    {'L', {0b100, 0b100, 0b100, 0b100, 0b111}}, {'M', {0b101, 0b111, 0b111, 0b101, 0b101}},
    {'N', {0b111, 0b101, 0b101, 0b101, 0b101}}, {'O', {0b111, 0b101, 0b101, 0b101, 0b111}},
    {'P', {0b110, 0b101, 0b110, 0b100, 0b100}}, {'Q', {0b010, 0b101, 0b101, 0b110, 0b011}},
    {'R', {0b110, 0b101, 0b110, 0b101, 0b101}}, {'S', {0b011, 0b100, 0b010, 0b001, 0b110}},
    {'T', {0b111, 0b010, 0b010, 0b010, 0b010}}, {'U', {0b101, 0b101, 0b101, 0b101, 0b111}},
    {'V', {0b101, 0b101, 0b101, 0b101, 0b010}}, {'W', {0b101, 0b101, 0b111, 0b111, 0b101}},
    {'X', {0b101, 0b101, 0b010, 0b101, 0b101}}, {'Y', {0b101, 0b101, 0b010, 0b010, 0b010}},
    {'Z', {0b111, 0b001, 0b010, 0b100, 0b111}}, {'0', {0b111, 0b101, 0b101, 0b101, 0b111}},
    {'1', {0b010, 0b110, 0b010, 0b010, 0b111}}, {'2', {0b110, 0b001, 0b010, 0b100, 0b111}},
    {'3', {0b110, 0b001, 0b010, 0b001, 0b110}}, {'4', {0b101, 0b101, 0b111, 0b001, 0b001}},
    {'5', {0b111, 0b100, 0b110, 0b001, 0b110}}, {'6', {0b011, 0b100, 0b111, 0b101, 0b111}},
    {'7', {0b111, 0b001, 0b010, 0b010, 0b010}}, {'8', {0b111, 0b101, 0b111, 0b101, 0b111}},
    {'9', {0b111, 0b101, 0b111, 0b001, 0b110}}, {'.', {0b000, 0b000, 0b000, 0b000, 0b010}},
    {',', {0b000, 0b000, 0b000, 0b010, 0b100}}, {':', {0b000, 0b010, 0b000, 0b010, 0b000}},
    {'-', {0b000, 0b000, 0b111, 0b000, 0b000}}, {'_', {0b000, 0b000, 0b000, 0b000, 0b111}},
    {'/', {0b001, 0b001, 0b010, 0b100, 0b100}}, {'+', {0b000, 0b010, 0b111, 0b010, 0b000}},
    {'=', {0b000, 0b111, 0b000, 0b111, 0b000}}, {'(', {0b010, 0b100, 0b100, 0b100, 0b010}},
    {')', {0b010, 0b001, 0b001, 0b001, 0b010}}, {'!', {0b010, 0b010, 0b010, 0b000, 0b010}},
    {'?', {0b110, 0b001, 0b010, 0b000, 0b010}}, {'>', {0b100, 0b010, 0b001, 0b010, 0b100}},
    {'<', {0b001, 0b010, 0b100, 0b010, 0b001}}, {'*', {0b101, 0b010, 0b111, 0b010, 0b101}},
};

}  // namespace detail

/** @brief Glyph for @p character (lower case is shown as upper case); unknown characters show '?'. */
[[nodiscard]] constexpr GlyphRows glyphFor(char character) {
    if (character >= 'a' && character <= 'z') {
        character = static_cast<char>(character - 'a' + 'A');
    }
    for (const auto& entry : detail::kGlyphs) {
        if (entry.character == character) {
            return entry.rows;
        }
    }
    return glyphFor('?');
}

}  // namespace vne::xr::samples
