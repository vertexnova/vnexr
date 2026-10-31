#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "vertexnova/xr/input.h"

namespace vne::xr::samples {

/**
 * @brief Pressed / released transitions of the buttons the framework reacts to.
 *
 * @details `InputState` holds button levels. Call `update()` once per frame with that frame's input; the
 * `*_pressed` and `*_released` flags are true for exactly the frame in which the level changed. While the app has
 * no input focus every button reads as released, so closing the system menu never fires a stale press.
 */
class InputEdges {
   public:
    static constexpr std::uint32_t kHands = InputState::kMaxControllers;
    static constexpr float kPullThreshold = 0.5f;
    /** A thumbstick counts as flicked past this, and must come back inside kStickRearm before it can flick again. */
    static constexpr float kStickFlick = 0.7f;
    static constexpr float kStickRearm = 0.3f;

    void update(const InputState& input) {
        for (std::uint32_t hand = 0; hand < kHands; ++hand) {
            const bool is_focused = input.is_focused;
            step(trigger_[hand],
                 is_focused && (input.select_pressed[hand] || input.select_value[hand] > kPullThreshold));
            step(squeeze_[hand], is_focused && input.squeeze_pressed[hand]);
            step(primary_[hand], is_focused && input.change_color_pressed[hand]);
            step(secondary_[hand], is_focused && input.secondary_pressed[hand]);
        }
        step(menu_, input.is_focused && input.menu_pressed);
        for (std::uint32_t hand = 0; hand < kHands; ++hand) {
            const float x = input.is_focused ? input.thumbstick[hand][0] : 0.f;
            flick_left_[hand] = false;
            flick_right_[hand] = false;
            if (is_stick_armed_[hand] && x <= -kStickFlick) {
                flick_left_[hand] = true;
                is_stick_armed_[hand] = false;
            } else if (is_stick_armed_[hand] && x >= kStickFlick) {
                flick_right_[hand] = true;
                is_stick_armed_[hand] = false;
            } else if (x > -kStickRearm && x < kStickRearm) {
                is_stick_armed_[hand] = true;
            }
        }
    }

    [[nodiscard]] bool triggerPressed(std::uint32_t hand) const { return trigger_[hand].pressed; }
    [[nodiscard]] bool triggerReleased(std::uint32_t hand) const { return trigger_[hand].released; }
    [[nodiscard]] bool triggerDown(std::uint32_t hand) const { return trigger_[hand].down; }
    [[nodiscard]] bool squeezePressed(std::uint32_t hand) const { return squeeze_[hand].pressed; }
    [[nodiscard]] bool primaryPressed(std::uint32_t hand) const { return primary_[hand].pressed; }
    [[nodiscard]] bool secondaryPressed(std::uint32_t hand) const { return secondary_[hand].pressed; }
    [[nodiscard]] bool menuPressed() const { return menu_.pressed; }
    /** @brief The thumbstick was pushed hard left (or right) this frame. */
    [[nodiscard]] bool stickFlickedLeft(std::uint32_t hand) const { return flick_left_[hand]; }
    [[nodiscard]] bool stickFlickedRight(std::uint32_t hand) const { return flick_right_[hand]; }

   private:
    struct Button {
        bool down = false;
        bool pressed = false;
        bool released = false;
    };

    static void step(Button& button, bool is_down) {
        button.pressed = is_down && !button.down;
        button.released = !is_down && button.down;
        button.down = is_down;
    }

    Button trigger_[kHands]{};
    Button squeeze_[kHands]{};
    Button primary_[kHands]{};
    Button secondary_[kHands]{};
    Button menu_{};
    bool is_stick_armed_[kHands] = {true, true};
    bool flick_left_[kHands]{};
    bool flick_right_[kHands]{};
};

}  // namespace vne::xr::samples
