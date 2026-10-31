/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "launcher/launcher_layer.h"

#include "vertexnova/logging/logging.h"

#include <algorithm>
#include <cmath>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.launcher");

using xrmath::Vec3;

constexpr float kPanelDistance = 1.3f;
// Text sizes are the height of one em, in meters. At the panel's 1.3 m distance an em of 0.026 m is about 1.1 degrees.
constexpr float kHeadingEm = 0.070f;
constexpr float kTitleEm = 0.042f;
constexpr float kDescriptionEm = 0.027f;
constexpr float kFooterEm = 0.025f;
constexpr float kTextMargin = 0.035f;
constexpr float kZPanel = 0.f;
constexpr float kZCard = 0.004f;
constexpr float kZText = 0.008f;
constexpr float kZPointer = 0.012f;

constexpr std::size_t kDescriptionLines = 3;
constexpr float kLinePitch = 1.3f;  // line spacing as a multiple of the em

constexpr float kPanelColor[4] = {0.05f, 0.06f, 0.09f, 1.f};
constexpr float kCardColor[4] = {0.13f, 0.15f, 0.21f, 1.f};
constexpr float kCardHoverColor[4] = {0.22f, 0.30f, 0.45f, 1.f};
constexpr float kExitColor[4] = {0.22f, 0.10f, 0.10f, 1.f};
constexpr float kExitHoverColor[4] = {0.45f, 0.18f, 0.18f, 1.f};
constexpr float kWhite[4] = {1.f, 1.f, 1.f, 1.f};
constexpr float kGray[4] = {0.62f, 0.66f, 0.74f, 1.f};
constexpr float kStatusColor[4] = {1.f, 0.7f, 0.3f, 1.f};
constexpr float kPointerColor[4] = {1.f, 1.f, 1.f, 1.f};

xrmath::Mat4 placed(
    const xrmath::Similarity& panel, float x, float y, float z, float half_w = 1.f, float half_h = 1.f) {
    const xrmath::Mat4 local = xrmath::fromTrs({x, y, z}, {}, {half_w, half_h, 1.f});
    return xrmath::multiply(xrmath::toMat4(panel), local);
}

}  // namespace

LauncherLayer::LauncherLayer(std::vector<LauncherEntry> entries, SelectCallback on_select, TextRenderer* text)
    : IXrLayer("Launcher")
    , entries_(std::move(entries))
    , on_select_(std::move(on_select))
    , layout_(buildLauncherLayout(entries_.size()))
    , text_(text) {}

LauncherLayer::~LauncherLayer() {
    onDetach();
}

void LauncherLayer::show(const Pose& head) {
    const Vec3 head_position = xrmath::positionOf(head);
    Vec3 forward = xrmath::rotate(xrmath::normalized(xrmath::orientationOf(head)), Vec3{0.f, 0.f, -1.f});
    forward = xrmath::normalized(Vec3{forward.x, 0.f, forward.z});
    if (xrmath::length(forward) < 0.5f) {
        forward = Vec3{0.f, 0.f, -1.f};
    }
    // Turn the panel so its +Z (the face) points back at the user.
    const float yaw = std::atan2(-forward.x, -forward.z);
    panel_pose_.q = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, yaw);
    panel_pose_.t = head_position + forward * kPanelDistance;
    panel_pose_.s = 1.f;
    // Slightly below eye level reads more comfortably than straight ahead.
    panel_pose_.t.y = head_position.y - 0.08f;
    is_visible_ = true;
    frames_visible_ = 0;
    hover_card_.reset();
    hover_hit_.reset();
    VNE_LOG_INFO << "Launcher panel placed " << kPanelDistance << " m ahead of the head";
}

void LauncherLayer::hide() {
    is_visible_ = false;
    hover_card_.reset();
    hover_hit_.reset();
}

void LauncherLayer::setStatus(const std::string& status) {
    if (status != status_) {
        status_ = status;
        is_status_dirty_ = true;
    }
}

void LauncherLayer::buildMeshes() {
    destroyMeshes();
    if (text_ == nullptr) {
        return;
    }
    card_meshes_.resize(entries_.size());
    const PanelRect* first_card = layout_.cards.empty() ? nullptr : &layout_.cards.front();
    const float text_width = first_card != nullptr ? 2.f * first_card->half_width - kTextMargin - 0.03f : 0.5f;
    for (std::size_t i = 0; i < entries_.size(); ++i) {
        card_meshes_[i].title = text_->createMesh(entries_[i].title, kTitleEm);
        for (const std::string& line :
             text_->wrap(entries_[i].description, kDescriptionEm, text_width, kDescriptionLines)) {
            card_meshes_[i].description_lines.push_back(text_->createMesh(line, kDescriptionEm));
        }
    }
    heading_ = text_->createMesh("VertexNova XR", kHeadingEm);
    footer_ = text_->createMesh("Menu: launcher    Grip: move    Two grips: zoom    Stick: turn    B/Y: reset view",
                                kFooterEm);
    is_status_dirty_ = true;
}

void LauncherLayer::destroyMeshes() {
    if (text_ != nullptr) {
        for (CardMeshes& card : card_meshes_) {
            text_->destroyMesh(card.title);
            for (TextMesh& line : card.description_lines) {
                text_->destroyMesh(line);
            }
        }
        text_->destroyMesh(heading_);
        text_->destroyMesh(footer_);
        text_->destroyMesh(status_mesh_);
    }
    card_meshes_.clear();
}

void LauncherLayer::onAttach(ISession* session) {
    onDetach();
    if (session == nullptr || text_ == nullptr || !renderer_.initialize(session->graphicsDevice())) {
        VNE_LOG_ERROR << "Launcher: no graphics device or text renderer; the panel will not be drawn";
        return;
    }
    buildMeshes();
    is_ready_ = true;
}

void LauncherLayer::onDetach() {
    destroyMeshes();
    renderer_.shutdown();
    is_ready_ = false;
}

void LauncherLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    (void)out_layers;
    const InputState& input = params.input;
    edges_.update(input);

    if (is_ready_ && is_status_dirty_) {
        text_->destroyMesh(status_mesh_);
        status_mesh_ = status_.empty() ? TextMesh{} : text_->createMesh(status_, kFooterEm);
        is_status_dirty_ = false;
    }
    if (!is_visible_) {
        return;
    }

    hover_card_.reset();
    hover_hit_.reset();
    ++frames_visible_;
    if (!input.is_focused) {
        return;
    }
    // Prefer the right hand, which most people point with.
    for (const std::uint32_t hand : {1u, 0u}) {
        if (!input.aim_valid[hand]) {
            continue;
        }
        const Vec3 origin = xrmath::positionOf(input.aim_poses[hand]);
        const Vec3 direction =
            xrmath::rotate(xrmath::normalized(xrmath::orientationOf(input.aim_poses[hand])), Vec3{0.f, 0.f, -1.f});
        const auto hit = intersectPanel(panel_pose_, origin, direction);
        if (!hit) {
            continue;
        }
        const auto card = hitTestCards(layout_, hit->x, hit->y);
        if (!hover_hit_ || card) {
            hover_hit_ = hit;
            hover_card_ = card;
        }
        constexpr std::uint32_t kSettleFrames = 36;  // Half a second at 72 Hz.
        if (card && edges_.triggerPressed(hand) && frames_visible_ >= kSettleFrames) {
            if (on_select_) {
                on_select_(entries_[*card]);
            }
            return;
        }
    }
}

void LauncherLayer::onRenderEye(const EyeRenderContext& context) {
    if (!is_visible_ || !is_ready_ || !renderer_.prepare(context)) {
        return;
    }
    const bool is_text_ready = text_->prepare(context);

    const PanelRect& panel = layout_.panel;
    renderer_.draw(context,
                   renderer_.quad(),
                   placed(panel_pose_, 0.f, 0.f, kZPanel, panel.half_width, panel.half_height),
                   kPanelColor);

    // Text hangs from its cap line: `top` is the y of the capitals' upper edge, `left` the start of the text.
    const auto draw_text = [&](const TextMesh& mesh, float left, float top, const float* color) {
        if (is_text_ready && mesh.isValid()) {
            text_->draw(context, mesh, placed(panel_pose_, left, top, kZText), color);
        }
    };

    // Heading, centered in the title area.
    draw_text(heading_,
              layout_.title_area.center_x - heading_.width * 0.5f,
              layout_.title_area.center_y + text_->capHeight(kHeadingEm) * 0.5f,
              kWhite);

    for (std::size_t i = 0; i < entries_.size() && i < layout_.cards.size() && i < card_meshes_.size(); ++i) {
        const PanelRect& card = layout_.cards[i];
        const LauncherEntry& entry = entries_[i];
        const bool is_hover = hover_card_ && *hover_card_ == i;
        const float* background =
            entry.is_exit ? (is_hover ? kExitHoverColor : kExitColor) : (is_hover ? kCardHoverColor : kCardColor);
        renderer_.draw(context,
                       renderer_.quad(),
                       placed(panel_pose_, card.center_x, card.center_y, kZCard, card.half_width, card.half_height),
                       background);

        // Accent stripe on the card's left edge.
        const float accent[4] = {entry.accent[0], entry.accent[1], entry.accent[2], 1.f};
        constexpr float kStripeHalfWidth = 0.008f;
        renderer_.draw(context,
                       renderer_.quad(),
                       placed(panel_pose_,
                              card.center_x - card.half_width + kStripeHalfWidth,
                              card.center_y,
                              kZText,
                              kStripeHalfWidth,
                              card.half_height),
                       accent);

        const float left = card.center_x - card.half_width + kTextMargin;
        float top = card.center_y + card.half_height - 0.032f;
        draw_text(card_meshes_[i].title, left, top, kWhite);
        top -= text_->capHeight(kTitleEm) + 0.028f;
        for (const TextMesh& line : card_meshes_[i].description_lines) {
            draw_text(line, left, top, kGray);
            top -= kDescriptionEm * kLinePitch;
        }
    }

    draw_text(footer_,
              layout_.footer_area.center_x - footer_.width * 0.5f,
              layout_.footer_area.center_y + text_->capHeight(kFooterEm) * 0.5f - 0.012f,
              kGray);
    draw_text(status_mesh_,
              layout_.footer_area.center_x - status_mesh_.width * 0.5f,
              layout_.footer_area.center_y + text_->capHeight(kFooterEm) * 0.5f + 0.034f,
              kStatusColor);

    if (hover_hit_) {
        constexpr float kDotHalf = 0.008f;
        renderer_.draw(context,
                       renderer_.quad(),
                       placed(panel_pose_, hover_hit_->x, hover_hit_->y, kZPointer, kDotHalf, kDotHalf),
                       kPointerColor);
    }
}

}  // namespace vne::xr::samples
