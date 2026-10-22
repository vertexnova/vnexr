/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   June 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include "vertexnova/xr/backend/openxr/input/openxr_input.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/xr/backend/openxr/openxr_util.h"

#include <cstddef>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vne.xr.openxr.input");
}

namespace vne::xr {

namespace {

constexpr std::array<const char*, 2> kHandSubpaths = {
    "/user/hand/left",
    "/user/hand/right",
};

}  // namespace

OpenXrInput::~OpenXrInput() {
    destroy();
}

Result OpenXrInput::createAction(XrActionSet action_set,
                                 const char* name,
                                 XrActionType type,
                                 XrAction& out_action,
                                 const XrPath* subaction_paths,
                                 std::uint32_t subaction_path_count) {
    XrActionCreateInfo info{XR_TYPE_ACTION_CREATE_INFO};
    info.actionType = type;
    OpenXrUtil::copyXrFixedString(info.actionName, XR_MAX_ACTION_NAME_SIZE, name);
    OpenXrUtil::copyXrFixedString(info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, name);
    info.countSubactionPaths = subaction_path_count;
    info.subactionPaths = subaction_paths;
    OPENXR_CHECK(xrCreateAction(action_set, &info, &out_action));
    return Result::eSuccess;
}

Result OpenXrInput::suggestBindings(XrInstance instance) {
    const XrActionSuggestedBinding khr_bindings[] = {
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/squeeze/value")},
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/squeeze/value")},
        {spawn_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/select/click")},
        {spawn_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/select/click")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/grip/pose")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/grip/pose")},
    };

    XrInteractionProfileSuggestedBinding suggested{XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING};
    suggested.interactionProfile = OpenXrUtil::stringToPath(instance, "/interaction_profiles/khr/simple_controller");
    suggested.countSuggestedBindings = static_cast<std::uint32_t>(std::size(khr_bindings));
    suggested.suggestedBindings = khr_bindings;
    const XrResult result = xrSuggestInteractionProfileBindings(instance, &suggested);
    if (XR_FAILED(result)) {
        VNE_LOG_WARN << "KHR simple controller bindings failed; continuing";
    }

    const XrActionSuggestedBinding touch_bindings[] = {
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/squeeze/value")},
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/squeeze/value")},
        {spawn_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/trigger/value")},
        {spawn_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/trigger/value")},
        {change_color_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/x/click")},
        {change_color_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/a/click")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/grip/pose")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/grip/pose")},
    };
    suggested.interactionProfile = OpenXrUtil::stringToPath(instance, "/interaction_profiles/oculus/touch_controller");
    suggested.countSuggestedBindings = static_cast<std::uint32_t>(std::size(touch_bindings));
    suggested.suggestedBindings = touch_bindings;
    if (XR_FAILED(xrSuggestInteractionProfileBindings(instance, &suggested))) {
        VNE_LOG_WARN << "Oculus Touch bindings failed; continuing";
    }

    // XR_EXT_hand_interaction: bare-hand pinch/grasp on Galaxy XR and similar HMDs.
    // pinch_ext is float (spawn); grasp_ext is float (grab); grip/pose is palm.
    const XrActionSuggestedBinding hand_bindings[] = {
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/grasp_ext/value")},
        {grab_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/grasp_ext/value")},
        {pinch_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/pinch_ext/value")},
        {pinch_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/pinch_ext/value")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/left/input/grip/pose")},
        {palm_pose_action_, OpenXrUtil::stringToPath(instance, "/user/hand/right/input/grip/pose")},
    };
    suggested.interactionProfile = OpenXrUtil::stringToPath(instance, "/interaction_profiles/ext/hand_interaction_ext");
    suggested.countSuggestedBindings = static_cast<std::uint32_t>(std::size(hand_bindings));
    suggested.suggestedBindings = hand_bindings;
    if (XR_FAILED(xrSuggestInteractionProfileBindings(instance, &suggested))) {
        VNE_LOG_WARN << "EXT hand_interaction bindings failed; continuing (controllers still work)";
    }
    return Result::eSuccess;
}

Result OpenXrInput::create(XrInstance instance, XrSession session, XrSpace reference_space) {
    destroy();
    instance_ = instance;
    session_ = session;
    reference_space_ = reference_space;

    XrActionSetCreateInfo set_info{XR_TYPE_ACTION_SET_CREATE_INFO};
    OpenXrUtil::copyXrFixedString(set_info.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "gameplay");
    OpenXrUtil::copyXrFixedString(set_info.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "Gameplay");
    set_info.priority = 0;
    OPENXR_CHECK(xrCreateActionSet(instance, &set_info, &action_set_));

    const std::array<XrPath, 2> hand_paths = {
        OpenXrUtil::stringToPath(instance, kHandSubpaths[0]),
        OpenXrUtil::stringToPath(instance, kHandSubpaths[1]),
    };
    constexpr std::uint32_t kHandPathCount = static_cast<std::uint32_t>(hand_paths.size());

    VNE_XR_CHECK(
        createAction(action_set_, "grab", XR_ACTION_TYPE_FLOAT_INPUT, grab_action_, hand_paths.data(), kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "spawn",
                              XR_ACTION_TYPE_BOOLEAN_INPUT,
                              spawn_action_,
                              hand_paths.data(),
                              kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "pinch",
                              XR_ACTION_TYPE_FLOAT_INPUT,
                              pinch_action_,
                              hand_paths.data(),
                              kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "change_color",
                              XR_ACTION_TYPE_BOOLEAN_INPUT,
                              change_color_action_,
                              hand_paths.data(),
                              kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "palm_pose",
                              XR_ACTION_TYPE_POSE_INPUT,
                              palm_pose_action_,
                              hand_paths.data(),
                              kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "buzz",
                              XR_ACTION_TYPE_VIBRATION_OUTPUT,
                              buzz_action_,
                              hand_paths.data(),
                              kHandPathCount));

    VNE_XR_CHECK(suggestBindings(instance));

    XrSessionActionSetsAttachInfo attach{XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO};
    attach.countActionSets = 1;
    attach.actionSets = &action_set_;
    OPENXR_CHECK(xrAttachSessionActionSets(session, &attach));

    for (std::uint32_t i = 0; i < palm_spaces_.size(); ++i) {
        XrActionSpaceCreateInfo space_info{XR_TYPE_ACTION_SPACE_CREATE_INFO};
        space_info.action = palm_pose_action_;
        space_info.subactionPath = hand_paths[i];
        space_info.poseInActionSpace.orientation.w = 1.f;
        OPENXR_CHECK(xrCreateActionSpace(session, &space_info, &palm_spaces_[i]));
    }

    return Result::eSuccess;
}

void OpenXrInput::destroy() {
    for (auto& space : palm_spaces_) {
        if (space != XR_NULL_HANDLE && session_ != XR_NULL_HANDLE) {
            xrDestroySpace(space);
            space = XR_NULL_HANDLE;
        }
    }
    if (action_set_ != XR_NULL_HANDLE && instance_ != XR_NULL_HANDLE) {
        xrDestroyActionSet(action_set_);
        action_set_ = XR_NULL_HANDLE;
    }
    session_ = XR_NULL_HANDLE;
    instance_ = XR_NULL_HANDLE;
    reference_space_ = XR_NULL_HANDLE;
}

Result OpenXrInput::poll(XrTime predicted_time, InputState& out_input) {
    out_input = {};
    if (session_ == XR_NULL_HANDLE) {
        return Result::eErrorBadState;
    }

    // XrActiveActionSet has no type field in the OpenXR 1.0 core headers.
    XrActiveActionSet active{};
    active.actionSet = action_set_;
    active.subactionPath = XR_NULL_PATH;
    XrActionsSyncInfo sync_info{XR_TYPE_ACTIONS_SYNC_INFO};
    sync_info.countActiveActionSets = 1;
    sync_info.activeActionSets = &active;
    OPENXR_CHECK(xrSyncActions(session_, &sync_info));

    out_input.controller_count = InputState::kMaxControllers;
    for (std::uint32_t i = 0; i < InputState::kMaxControllers; ++i) {
        const XrPath subpath = OpenXrUtil::stringToPath(instance_, kHandSubpaths[i]);

        XrActionStateGetInfo get_info{XR_TYPE_ACTION_STATE_GET_INFO};
        get_info.action = grab_action_;
        get_info.subactionPath = subpath;
        XrActionStateFloat grab_state{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(session_, &get_info, &grab_state))) {
            out_input.select_value[i] = grab_state.currentState;
            out_input.squeeze_pressed[i] = grab_state.currentState > 0.5f;
        }

        get_info.action = spawn_action_;
        XrActionStateBoolean spawn_state{XR_TYPE_ACTION_STATE_BOOLEAN};
        bool spawn_pressed = false;
        if (XR_SUCCEEDED(xrGetActionStateBoolean(session_, &get_info, &spawn_state))) {
            spawn_pressed = spawn_state.currentState == XR_TRUE;
        }

        get_info.action = pinch_action_;
        XrActionStateFloat pinch_state{XR_TYPE_ACTION_STATE_FLOAT};
        if (XR_SUCCEEDED(xrGetActionStateFloat(session_, &get_info, &pinch_state))) {
            constexpr float kPinchSpawnThreshold = 0.7f;
            if (pinch_state.isActive == XR_TRUE && pinch_state.currentState > kPinchSpawnThreshold) {
                spawn_pressed = true;
            }
        }
        out_input.select_pressed[i] = spawn_pressed;

        get_info.action = change_color_action_;
        XrActionStateBoolean color_state{XR_TYPE_ACTION_STATE_BOOLEAN};
        if (XR_SUCCEEDED(xrGetActionStateBoolean(session_, &get_info, &color_state))) {
            out_input.change_color_pressed[i] = color_state.currentState == XR_TRUE;
        }

        get_info.action = palm_pose_action_;
        XrActionStatePose pose_state{XR_TYPE_ACTION_STATE_POSE};
        out_input.pose_valid[i] = false;
        if (XR_SUCCEEDED(xrGetActionStatePose(session_, &get_info, &pose_state)) && pose_state.isActive == XR_TRUE
            && palm_spaces_[i] != XR_NULL_HANDLE && reference_space_ != XR_NULL_HANDLE) {
            XrSpaceLocation location{XR_TYPE_SPACE_LOCATION};
            if (XR_SUCCEEDED(xrLocateSpace(palm_spaces_[i], reference_space_, predicted_time, &location))) {
                constexpr XrSpaceLocationFlags kPoseValidBits =
                    XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
                if ((location.locationFlags & kPoseValidBits) == kPoseValidBits) {
                    out_input.controller_poses[i].position[0] = location.pose.position.x;
                    out_input.controller_poses[i].position[1] = location.pose.position.y;
                    out_input.controller_poses[i].position[2] = location.pose.position.z;
                    out_input.controller_poses[i].orientation[0] = location.pose.orientation.x;
                    out_input.controller_poses[i].orientation[1] = location.pose.orientation.y;
                    out_input.controller_poses[i].orientation[2] = location.pose.orientation.z;
                    out_input.controller_poses[i].orientation[3] = location.pose.orientation.w;
                    out_input.pose_valid[i] = true;
                }
            }
        }
    }
    return Result::eSuccess;
}

Result OpenXrInput::applyHaptic(std::uint32_t hand_index, float strength, XrDuration duration) const {
    if (session_ == XR_NULL_HANDLE || hand_index >= InputState::kMaxControllers) {
        return Result::eErrorInvalidArgument;
    }
    XrHapticActionInfo haptic_info{XR_TYPE_HAPTIC_ACTION_INFO};
    haptic_info.action = buzz_action_;
    haptic_info.subactionPath = OpenXrUtil::stringToPath(instance_, kHandSubpaths[hand_index]);
    XrHapticVibration vibration{XR_TYPE_HAPTIC_VIBRATION};
    vibration.amplitude = strength;
    vibration.duration = duration;
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
    OPENXR_CHECK(xrApplyHapticFeedback(session_, &haptic_info, reinterpret_cast<XrHapticBaseHeader*>(&vibration)));
    return Result::eSuccess;
}

}  // namespace vne::xr
