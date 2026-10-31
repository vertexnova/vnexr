/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

#include "launcher/controller_visuals_layer.h"

#include "launcher/launcher_layout.h"

#include "vertexnova/logging/logging.h"

#include <cmath>

namespace vne::xr::samples {

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.samples.controller_visuals");

constexpr float kRayLength = 1.5f;
// Thinner than a few pixels and the ray breaks up into crawling dots.
constexpr float kRayHalfThickness = 0.003f;
constexpr float kMenuTextEm = 0.020f;
constexpr float kMenuTagHalfWidth = 0.04f;
constexpr float kMenuTagHalfHeight = 0.016f;
constexpr float kMenuTagColor[4] = {0.10f, 0.12f, 0.18f, 1.f};
constexpr float kMenuTagHoverColor[4] = {0.25f, 0.40f, 0.65f, 1.f};
constexpr float kMenuTextColor[4] = {1.f, 1.f, 1.f, 1.f};
constexpr float kHandColors[2][4] = {{0.30f, 0.60f, 1.00f, 1.f}, {1.00f, 0.60f, 0.30f, 1.f}};
constexpr float kRayColors[2][4] = {{0.45f, 0.70f, 1.00f, 1.f}, {1.00f, 0.75f, 0.45f, 1.f}};
}  // namespace

ControllerVisualsLayer::ControllerVisualsLayer(TextRenderer* text)
    : IXrLayer("ControllerVisuals")
    , text_(text) {}

ControllerVisualsLayer::~ControllerVisualsLayer() {
    onDetach();
}

void ControllerVisualsLayer::onAttach(ISession* session) {
    onDetach();
    is_ready_ = session != nullptr && renderer_.initialize(session->graphicsDevice());
    if (!is_ready_) {
        VNE_LOG_ERROR << "Controller visuals: no graphics device";
        return;
    }
    if (text_ != nullptr) {
        menu_text_ = text_->createMesh("Menu", kMenuTextEm);
    }
}

void ControllerVisualsLayer::onDetach() {
    if (text_ != nullptr) {
        text_->destroyMesh(menu_text_);
    }
    renderer_.shutdown();
    is_ready_ = false;
}

bool ControllerVisualsLayer::menuTagPose(xrmath::Similarity& out_pose) const {
    if (!is_menu_tag_visible_ || !input_.is_focused || !input_.pose_valid[0]) {
        return false;
    }
    // Just above the left controller, toward the wrist.
    const xrmath::Similarity grip = xrmath::fromPoseSimilarity(input_.controller_poses[0]);
    out_pose = xrmath::compose(grip, xrmath::Similarity{{0.f, 0.07f, 0.03f}, {}, 1.f});
    if (is_head_pose_valid_) {
        // Turn the tag to face the head so its label can be read from wherever the controller is held.
        const xrmath::Vec3 to_head = xrmath::positionOf(head_pose_) - out_pose.t;
        const float horizontal = std::sqrt(to_head.x * to_head.x + to_head.z * to_head.z);
        const xrmath::Quat yaw = xrmath::quatFromAxisAngle({0.f, 1.f, 0.f}, std::atan2(to_head.x, to_head.z));
        // A positive turn about +X tips +Z downward, so look up with a negative angle.
        const xrmath::Quat pitch = xrmath::quatFromAxisAngle({1.f, 0.f, 0.f}, -std::atan2(to_head.y, horizontal));
        out_pose.q = xrmath::normalized(yaw * pitch);
    }
    return true;
}

void ControllerVisualsLayer::onUpdate(const FrameParams& params, LayerParams& out_layers) {
    (void)out_layers;
    input_ = params.input;
    head_pose_ = params.frame.head_pose;
    is_head_pose_valid_ = params.frame.is_head_pose_valid;
    edges_.update(input_);

    is_menu_hovered_ = false;
    right_ray_length_ = kRayLength;
    xrmath::Similarity tag;
    if (!menuTagPose(tag) || !input_.aim_valid[1]) {
        return;
    }
    const xrmath::Vec3 origin = xrmath::positionOf(input_.aim_poses[1]);
    const xrmath::Vec3 direction =
        xrmath::rotate(xrmath::normalized(xrmath::orientationOf(input_.aim_poses[1])), xrmath::Vec3{0.f, 0.f, -1.f});
    const auto hit = intersectPanel(tag, origin, direction);
    if (!hit || std::abs(hit->x) > kMenuTagHalfWidth || std::abs(hit->y) > kMenuTagHalfHeight) {
        return;
    }
    is_menu_hovered_ = true;
    right_ray_length_ = hit->distance;
    if (edges_.triggerPressed(1) && on_menu_) {
        VNE_LOG_INFO << "Wrist MENU tag selected";
        on_menu_();
    }
}

void ControllerVisualsLayer::onRenderEye(const EyeRenderContext& context) {
    if (!is_ready_ || !input_.is_focused || !renderer_.prepare(context)) {
        return;
    }
    for (std::size_t hand = 0; hand < InputState::kMaxControllers; ++hand) {
        if (input_.pose_valid[hand]) {
            // The grip pose is the palm: the body of the controller extends forward from it.
            const xrmath::Mat4 body = xrmath::multiply(xrmath::fromPose(input_.controller_poses[hand]),
                                                       xrmath::fromTrs({0.f, 0.f, -0.03f}, {}, {1.f, 1.f, 1.f}));
            renderer_.drawBox(context, body, {0.02f, 0.015f, 0.06f}, kHandColors[hand]);
        }
        if (is_ray_visible_ && input_.aim_valid[hand]) {
            const float length = hand == 1 ? right_ray_length_ : kRayLength;
            const xrmath::Mat4 ray = xrmath::multiply(xrmath::fromPose(input_.aim_poses[hand]),
                                                      xrmath::fromTrs({0.f, 0.f, -length * 0.5f}, {}, {1.f, 1.f, 1.f}));
            renderer_.drawBox(context, ray, {kRayHalfThickness, kRayHalfThickness, length * 0.5f}, kRayColors[hand]);
        }
    }

    xrmath::Similarity tag;
    if (menuTagPose(tag)) {
        const xrmath::Mat4 tag_matrix = xrmath::toMat4(tag);
        renderer_.draw(
            context,
            renderer_.quad(),
            xrmath::multiply(tag_matrix, xrmath::fromTrs({}, {}, {kMenuTagHalfWidth, kMenuTagHalfHeight, 1.f})),
            is_menu_hovered_ ? kMenuTagHoverColor : kMenuTagColor);
        if (text_ != nullptr && menu_text_.isValid() && text_->prepare(context)) {
            const float top = text_->capHeight(kMenuTextEm) * 0.5f;
            text_->draw(context,
                        menu_text_,
                        xrmath::multiply(tag_matrix,
                                         xrmath::fromTrs({-menu_text_.width * 0.5f, top, 0.002f}, {}, {1.f, 1.f, 1.f})),
                        kMenuTextColor);
        }
    }
}

}  // namespace vne::xr::samples
