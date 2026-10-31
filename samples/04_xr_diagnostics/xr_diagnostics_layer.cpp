/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "xr_diagnostics_layer.h"

#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <cmath>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.diagnostics");

using xrmath::Vec3;

constexpr float kGridExtent = 4.f;
constexpr float kGridStep = 0.5f;
// Lines narrower than a few pixels at a couple of meters break up into crawling dots.
constexpr float kLineHalfWidth = 0.0075f;
constexpr double kNanosecondsPerMillisecond = 1.0e6;
/** A frame this much longer than the typical one counts as late. */
constexpr float kLateFactor = 1.4f;

constexpr float kGridColor[4] = {0.25f, 0.28f, 0.36f, 1.f};
constexpr float kAxisRed[4] = {0.95f, 0.2f, 0.2f, 1.f};
constexpr float kAxisGreen[4] = {0.2f, 0.9f, 0.3f, 1.f};
constexpr float kAxisBlue[4] = {0.25f, 0.45f, 1.f, 1.f};
constexpr float kCubeColor[4] = {0.85f, 0.85f, 0.9f, 1.f};
constexpr float kOnTime[4] = {0.2f, 0.85f, 0.35f, 1.f};
constexpr float kLate[4] = {0.95f, 0.2f, 0.2f, 1.f};
constexpr float kGray[4] = {0.62f, 0.64f, 0.68f, 1.f};
constexpr float kOrange[4] = {1.f, 0.55f, 0.10f, 1.f};

/** Greeting: one em is 14 cm, so the capitals are about 10 cm tall; it floats between the cube and the frame strip. */
constexpr float kGreetingEm = 0.14f;
constexpr float kGreetingTop = 1.45f;
constexpr float kGreetingGapEm = 0.3f;

xrmath::Mat4 at(Vec3 position) {
    return xrmath::fromTrs(position, {}, {1.f, 1.f, 1.f});
}

}  // namespace

XrDiagnosticsLayer::XrDiagnosticsLayer(TextRenderer* text)
    : IXrLayer("XrDiagnostics")
    , text_(text) {}

XrDiagnosticsLayer::~XrDiagnosticsLayer() {
    onDetach();
}

void XrDiagnosticsLayer::onAttach(ISession* session) {
    onDetach();
    is_ready_ = session != nullptr && renderer_.initialize(session->graphicsDevice());
    if (!is_ready_) {
        VNE_LOG_ERROR << "Diagnostics: no graphics device; nothing will be drawn";
    }
    if (is_ready_ && text_ != nullptr) {
        hello_ = text_->createMesh("Hello", kGreetingEm);
        brand_ = text_->createMesh("VertexNova!", kGreetingEm);
    }
    last_display_time_ = 0.0;
    history_count_ = 0;
    history_next_ = 0;
}

void XrDiagnosticsLayer::onDetach() {
    if (text_ != nullptr) {
        text_->destroyMesh(hello_);
        text_->destroyMesh(brand_);
    }
    renderer_.shutdown();
    is_ready_ = false;
}

void XrDiagnosticsLayer::recordFrameTime(double predicted_display_time) {
    if (last_display_time_ > 0.0) {
        const auto ms = static_cast<float>((predicted_display_time - last_display_time_) / kNanosecondsPerMillisecond);
        frame_ms_[history_next_] = ms;
        history_next_ = (history_next_ + 1) % kHistory;
        history_count_ = std::min(history_count_ + 1, kHistory);

        std::array<float, kHistory> sorted = frame_ms_;
        std::sort(sorted.begin(), sorted.begin() + static_cast<std::ptrdiff_t>(history_count_));
        median_ms_ = sorted[history_count_ / 2];
        for (std::size_t i = 0; i < history_count_; ++i) {
            is_late_[i] = frame_ms_[i] > median_ms_ * kLateFactor;
        }
    }
    last_display_time_ = predicted_display_time;
}

void XrDiagnosticsLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    if (!params.frame.should_render) {
        return;
    }
    input_ = params.input;
    recordFrameTime(params.frame.predicted_display_time);
    out_layers.clear_color[0] = 0.02f;
    out_layers.clear_color[1] = 0.02f;
    out_layers.clear_color[2] = 0.03f;
}

void XrDiagnosticsLayer::drawIndicators(const EyeRenderContext& context, std::size_t hand) const {
    if (!input_.pose_valid[hand]) {
        return;
    }
    // Trigger, grip, primary (X/A), secondary (Y/B), thumbstick x and y: brightness follows the value.
    const float values[kIndicators] = {input_.select_value[hand],
                                       input_.squeeze_value[hand],
                                       input_.change_color_pressed[hand] ? 1.f : 0.f,
                                       input_.secondary_pressed[hand] ? 1.f : 0.f,
                                       std::abs(input_.thumbstick[hand][0]),
                                       std::abs(input_.thumbstick[hand][1])};
    const float hues[kIndicators][3] = {{1.f, 0.4f, 0.2f},
                                        {0.3f, 0.7f, 1.f},
                                        {0.4f, 1.f, 0.4f},
                                        {1.f, 0.9f, 0.3f},
                                        {0.9f, 0.4f, 1.f},
                                        {0.9f, 0.4f, 1.f}};
    const xrmath::Mat4 hand_frame = xrmath::fromPose(input_.controller_poses[hand]);
    for (std::size_t i = 0; i < kIndicators; ++i) {
        const float level = std::clamp(values[i], 0.f, 1.f);
        const float brightness = 0.15f + 0.85f * level;
        const float color[4] = {hues[i][0] * brightness, hues[i][1] * brightness, hues[i][2] * brightness, 1.f};
        const float x = (static_cast<float>(i) - 2.5f) * 0.025f;
        const xrmath::Mat4 model = xrmath::multiply(hand_frame, at({x, 0.07f, -0.03f}));
        renderer_.drawBox(context, model, {0.01f, 0.01f + 0.01f * level, 0.01f}, color);
    }
}

void XrDiagnosticsLayer::onRenderEye(const EyeRenderContext& context) {
    if (!is_ready_ || !renderer_.prepare(context)) {
        return;
    }

    // Floor grid.
    for (float offset = -kGridExtent; offset <= kGridExtent + 1e-3f; offset += kGridStep) {
        renderer_.drawBox(context, at({offset, 0.f, 0.f}), {kLineHalfWidth, kLineHalfWidth, kGridExtent}, kGridColor);
        renderer_.drawBox(context, at({0.f, 0.f, offset}), {kGridExtent, kLineHalfWidth, kLineHalfWidth}, kGridColor);
    }

    // World axes, one meter long, thicker than the grid.
    renderer_.drawBox(context, at({0.5f, 0.01f, 0.f}), {0.5f, 0.01f, 0.01f}, kAxisRed);
    renderer_.drawBox(context, at({0.f, 0.5f, 0.f}), {0.01f, 0.5f, 0.01f}, kAxisGreen);
    renderer_.drawBox(context, at({0.f, 0.01f, 0.5f}), {0.01f, 0.01f, 0.5f}, kAxisBlue);

    // The reference cube: exactly one meter wide, two meters ahead, standing on the floor.
    renderer_.drawBox(context, at({0.f, 0.5f, -2.f}), {0.5f, 0.5f, 0.5f}, kCubeColor);

    for (std::size_t hand = 0; hand < InputState::kMaxControllers; ++hand) {
        drawIndicators(context, hand);
    }

    // "Hello VertexNova!": gray "Hello", orange "VertexNova!", centered two meters ahead.
    if (text_ != nullptr && hello_.isValid() && brand_.isValid() && text_->prepare(context)) {
        const float gap = kGreetingEm * kGreetingGapEm;
        const float left = -(hello_.width + gap + brand_.width) * 0.5f;
        text_->draw(context, hello_, at({left, kGreetingTop, -2.f}), kGray);
        text_->draw(context, brand_, at({left + hello_.width + gap, kGreetingTop, -2.f}), kOrange);
    }

    // Frame-time strip, oldest on the left; late frames are red. Floats above the reference cube.
    for (std::size_t i = 0; i < history_count_; ++i) {
        const std::size_t slot = (history_next_ + kHistory - history_count_ + i) % kHistory;
        const float x = (static_cast<float>(i) - static_cast<float>(kHistory) * 0.5f) * 0.012f;
        renderer_.drawBox(context, at({x, 1.7f, -2.f}), {0.005f, 0.02f, 0.005f}, is_late_[slot] ? kLate : kOnTime);
    }
}

}  // namespace vne::xr::samples
