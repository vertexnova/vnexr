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

#include <algorithm>
#include <cstddef>
#include <string>

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
    XrActionCreateInfo info = OpenXrUtil::makeXrStruct<XrActionCreateInfo>(XR_TYPE_ACTION_CREATE_INFO);
    info.actionType = type;
    OpenXrUtil::copyXrFixedString(info.actionName, XR_MAX_ACTION_NAME_SIZE, name);
    OpenXrUtil::copyXrFixedString(info.localizedActionName, XR_MAX_LOCALIZED_ACTION_NAME_SIZE, name);
    info.countSubactionPaths = subaction_path_count;
    info.subactionPaths = subaction_paths;
    OPENXR_CHECK(xrCreateAction(action_set, &info, &out_action));
    return Result::eSuccess;
}

Result OpenXrInput::suggestBindings(XrInstance instance) {
    const auto path = [instance](const char* text) { return OpenXrUtil::stringToPath(instance, text); };
    XrInteractionProfileSuggestedBinding suggested =
        OpenXrUtil::makeXrStruct<XrInteractionProfileSuggestedBinding>(XR_TYPE_INTERACTION_PROFILE_SUGGESTED_BINDING);

    // khr/simple_controller has select, menu, grip/aim pose and haptics only: there is no squeeze, so listing one
    // would make the runtime reject the whole profile.
    const XrActionSuggestedBinding khr_bindings[] = {
        {spawn_action_, path("/user/hand/left/input/select/click")},
        {spawn_action_, path("/user/hand/right/input/select/click")},
        {trigger_action_, path("/user/hand/left/input/select/click")},
        {trigger_action_, path("/user/hand/right/input/select/click")},
        {menu_action_, path("/user/hand/left/input/menu/click")},
        {menu_action_, path("/user/hand/right/input/menu/click")},
        {palm_pose_action_, path("/user/hand/left/input/grip/pose")},
        {palm_pose_action_, path("/user/hand/right/input/grip/pose")},
        {aim_pose_action_, path("/user/hand/left/input/aim/pose")},
        {aim_pose_action_, path("/user/hand/right/input/aim/pose")},
        {buzz_action_, path("/user/hand/left/output/haptic")},
        {buzz_action_, path("/user/hand/right/output/haptic")},
    };
    suggested.interactionProfile = path("/interaction_profiles/khr/simple_controller");
    suggested.countSuggestedBindings = static_cast<std::uint32_t>(std::size(khr_bindings));
    suggested.suggestedBindings = khr_bindings;
    if (XR_FAILED(xrSuggestInteractionProfileBindings(instance, &suggested))) {
        VNE_LOG_WARN << "KHR simple controller bindings failed; continuing";
    }

    // The right menu button belongs to the system, so only the left one is bound on Touch controllers.
    const XrActionSuggestedBinding touch_bindings[] = {
        {grab_action_, path("/user/hand/left/input/squeeze/value")},
        {grab_action_, path("/user/hand/right/input/squeeze/value")},
        {spawn_action_, path("/user/hand/left/input/trigger/value")},
        {spawn_action_, path("/user/hand/right/input/trigger/value")},
        {trigger_action_, path("/user/hand/left/input/trigger/value")},
        {trigger_action_, path("/user/hand/right/input/trigger/value")},
        {change_color_action_, path("/user/hand/left/input/x/click")},
        {change_color_action_, path("/user/hand/right/input/a/click")},
        {secondary_action_, path("/user/hand/left/input/y/click")},
        {secondary_action_, path("/user/hand/right/input/b/click")},
        {thumbstick_action_, path("/user/hand/left/input/thumbstick")},
        {thumbstick_action_, path("/user/hand/right/input/thumbstick")},
        {menu_action_, path("/user/hand/left/input/menu/click")},
        {palm_pose_action_, path("/user/hand/left/input/grip/pose")},
        {palm_pose_action_, path("/user/hand/right/input/grip/pose")},
        {aim_pose_action_, path("/user/hand/left/input/aim/pose")},
        {aim_pose_action_, path("/user/hand/right/input/aim/pose")},
        {buzz_action_, path("/user/hand/left/output/haptic")},
        {buzz_action_, path("/user/hand/right/output/haptic")},
    };
    suggested.interactionProfile = path("/interaction_profiles/oculus/touch_controller");
    suggested.countSuggestedBindings = static_cast<std::uint32_t>(std::size(touch_bindings));
    suggested.suggestedBindings = touch_bindings;
    if (XR_FAILED(xrSuggestInteractionProfileBindings(instance, &suggested))) {
        VNE_LOG_WARN << "Oculus Touch bindings failed; continuing";
    }

    // XR_EXT_hand_interaction: bare-hand pinch/grasp on Galaxy XR and similar HMDs.
    // pinch_ext is float (spawn); grasp_ext is float (grab); grip/pose is palm, aim/pose the pointing ray.
    const XrActionSuggestedBinding hand_bindings[] = {
        {grab_action_, path("/user/hand/left/input/grasp_ext/value")},
        {grab_action_, path("/user/hand/right/input/grasp_ext/value")},
        {pinch_action_, path("/user/hand/left/input/pinch_ext/value")},
        {pinch_action_, path("/user/hand/right/input/pinch_ext/value")},
        {palm_pose_action_, path("/user/hand/left/input/grip/pose")},
        {palm_pose_action_, path("/user/hand/right/input/grip/pose")},
        {aim_pose_action_, path("/user/hand/left/input/aim/pose")},
        {aim_pose_action_, path("/user/hand/right/input/aim/pose")},
    };
    suggested.interactionProfile = path("/interaction_profiles/ext/hand_interaction_ext");
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

    XrActionSetCreateInfo set_info = OpenXrUtil::makeXrStruct<XrActionSetCreateInfo>(XR_TYPE_ACTION_SET_CREATE_INFO);
    OpenXrUtil::copyXrFixedString(set_info.actionSetName, XR_MAX_ACTION_SET_NAME_SIZE, "gameplay");
    OpenXrUtil::copyXrFixedString(set_info.localizedActionSetName, XR_MAX_LOCALIZED_ACTION_SET_NAME_SIZE, "Gameplay");
    set_info.priority = 0;
    OPENXR_CHECK(xrCreateActionSet(instance, &set_info, &action_set_));

    for (std::uint32_t i = 0; i < hand_paths_.size(); ++i) {
        hand_paths_[i] = OpenXrUtil::stringToPath(instance, kHandSubpaths[i]);
    }
    constexpr std::uint32_t kHandPathCount = InputState::kMaxControllers;
    const XrPath* hands = hand_paths_.data();

    VNE_XR_CHECK(createAction(action_set_, "grab", XR_ACTION_TYPE_FLOAT_INPUT, grab_action_, hands, kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "spawn", XR_ACTION_TYPE_BOOLEAN_INPUT, spawn_action_, hands, kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_, "pinch", XR_ACTION_TYPE_FLOAT_INPUT, pinch_action_, hands, kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "change_color",
                              XR_ACTION_TYPE_BOOLEAN_INPUT,
                              change_color_action_,
                              hands,
                              kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "secondary", XR_ACTION_TYPE_BOOLEAN_INPUT, secondary_action_, hands, kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "trigger", XR_ACTION_TYPE_FLOAT_INPUT, trigger_action_, hands, kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_,
                              "thumbstick",
                              XR_ACTION_TYPE_VECTOR2F_INPUT,
                              thumbstick_action_,
                              hands,
                              kHandPathCount));
    VNE_XR_CHECK(createAction(action_set_, "menu", XR_ACTION_TYPE_BOOLEAN_INPUT, menu_action_, hands, kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "palm_pose", XR_ACTION_TYPE_POSE_INPUT, palm_pose_action_, hands, kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "aim_pose", XR_ACTION_TYPE_POSE_INPUT, aim_pose_action_, hands, kHandPathCount));
    VNE_XR_CHECK(
        createAction(action_set_, "buzz", XR_ACTION_TYPE_VIBRATION_OUTPUT, buzz_action_, hands, kHandPathCount));

    VNE_XR_CHECK(suggestBindings(instance));

    XrSessionActionSetsAttachInfo attach =
        OpenXrUtil::makeXrStruct<XrSessionActionSetsAttachInfo>(XR_TYPE_SESSION_ACTION_SETS_ATTACH_INFO);
    attach.countActionSets = 1;
    attach.actionSets = &action_set_;
    OPENXR_CHECK(xrAttachSessionActionSets(session, &attach));

    for (std::uint32_t i = 0; i < palm_spaces_.size(); ++i) {
        XrActionSpaceCreateInfo space_info =
            OpenXrUtil::makeXrStruct<XrActionSpaceCreateInfo>(XR_TYPE_ACTION_SPACE_CREATE_INFO);
        space_info.subactionPath = hand_paths_[i];
        space_info.poseInActionSpace.orientation.w = 1.f;
        space_info.action = palm_pose_action_;
        OPENXR_CHECK(xrCreateActionSpace(session, &space_info, &palm_spaces_[i]));
        space_info.action = aim_pose_action_;
        OPENXR_CHECK(xrCreateActionSpace(session, &space_info, &aim_spaces_[i]));
    }

    return Result::eSuccess;
}

void OpenXrInput::destroy() {
    for (auto* spaces : {&palm_spaces_, &aim_spaces_}) {
        for (auto& space : *spaces) {
            if (space != XR_NULL_HANDLE && session_ != XR_NULL_HANDLE) {
                xrDestroySpace(space);
            }
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

namespace {

constexpr float kButtonThreshold = 0.5f;
constexpr float kPinchThreshold = 0.7f;

bool readBool(XrSession session, XrAction action, XrPath hand) {
    XrActionStateGetInfo info = OpenXrUtil::makeXrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = action;
    info.subactionPath = hand;
    XrActionStateBoolean state = OpenXrUtil::makeXrStruct<XrActionStateBoolean>(XR_TYPE_ACTION_STATE_BOOLEAN);
    return XR_SUCCEEDED(xrGetActionStateBoolean(session, &info, &state)) && state.isActive == XR_TRUE
           && state.currentState == XR_TRUE;
}

float readFloat(XrSession session, XrAction action, XrPath hand) {
    XrActionStateGetInfo info = OpenXrUtil::makeXrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = action;
    info.subactionPath = hand;
    XrActionStateFloat state = OpenXrUtil::makeXrStruct<XrActionStateFloat>(XR_TYPE_ACTION_STATE_FLOAT);
    if (XR_SUCCEEDED(xrGetActionStateFloat(session, &info, &state)) && state.isActive == XR_TRUE) {
        return state.currentState;
    }
    return 0.f;
}

bool readPose(XrSession session, XrAction action, XrPath hand, XrSpace space, XrSpace base, XrTime time, Pose& out) {
    if (space == XR_NULL_HANDLE || base == XR_NULL_HANDLE) {
        return false;
    }
    XrActionStateGetInfo info = OpenXrUtil::makeXrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
    info.action = action;
    info.subactionPath = hand;
    XrActionStatePose state = OpenXrUtil::makeXrStruct<XrActionStatePose>(XR_TYPE_ACTION_STATE_POSE);
    if (XR_FAILED(xrGetActionStatePose(session, &info, &state)) || state.isActive != XR_TRUE) {
        return false;
    }
    XrSpaceLocation location = OpenXrUtil::makeXrStruct<XrSpaceLocation>(XR_TYPE_SPACE_LOCATION);
    if (XR_FAILED(xrLocateSpace(space, base, time, &location))) {
        return false;
    }
    constexpr XrSpaceLocationFlags kPoseValidBits =
        XR_SPACE_LOCATION_POSITION_VALID_BIT | XR_SPACE_LOCATION_ORIENTATION_VALID_BIT;
    if ((location.locationFlags & kPoseValidBits) != kPoseValidBits) {
        return false;
    }
    out.position[0] = location.pose.position.x;
    out.position[1] = location.pose.position.y;
    out.position[2] = location.pose.position.z;
    out.orientation[0] = location.pose.orientation.x;
    out.orientation[1] = location.pose.orientation.y;
    out.orientation[2] = location.pose.orientation.z;
    out.orientation[3] = location.pose.orientation.w;
    return true;
}

}  // namespace

Result OpenXrInput::poll(XrTime predicted_time, InputState& out_input) {
    out_input = {};
    if (session_ == XR_NULL_HANDLE) {
        return Result::eErrorBadState;
    }

    // XrActiveActionSet has no type field in the OpenXR 1.0 core headers.
    XrActiveActionSet active{};
    active.actionSet = action_set_;
    active.subactionPath = XR_NULL_PATH;
    XrActionsSyncInfo sync_info = OpenXrUtil::makeXrStruct<XrActionsSyncInfo>(XR_TYPE_ACTIONS_SYNC_INFO);
    sync_info.countActiveActionSets = 1;
    sync_info.activeActionSets = &active;
    const XrResult sync_result = xrSyncActions(session_, &sync_info);
    if (XR_FAILED(sync_result)) {
        OPENXR_CHECK(sync_result);
    }
    if (sync_result == XR_SESSION_NOT_FOCUSED) {
        // The system UI (or another app) has input focus: report an idle, unfocused snapshot.
        out_input.is_focused = false;
        return Result::eSuccess;
    }

    out_input.controller_count = InputState::kMaxControllers;
    for (std::uint32_t i = 0; i < InputState::kMaxControllers; ++i) {
        const XrPath hand = hand_paths_[i];

        out_input.squeeze_value[i] = readFloat(session_, grab_action_, hand);
        out_input.squeeze_pressed[i] = out_input.squeeze_value[i] > kButtonThreshold;

        float trigger = readFloat(session_, trigger_action_, hand);
        bool select = readBool(session_, spawn_action_, hand);
        const float pinch = readFloat(session_, pinch_action_, hand);
        if (pinch > kPinchThreshold) {
            select = true;
            trigger = std::max(trigger, pinch);
        }
        out_input.select_pressed[i] = select;
        out_input.select_value[i] = trigger;

        out_input.change_color_pressed[i] = readBool(session_, change_color_action_, hand);
        out_input.secondary_pressed[i] = readBool(session_, secondary_action_, hand);

        XrActionStateGetInfo stick_info = OpenXrUtil::makeXrStruct<XrActionStateGetInfo>(XR_TYPE_ACTION_STATE_GET_INFO);
        stick_info.action = thumbstick_action_;
        stick_info.subactionPath = hand;
        XrActionStateVector2f stick = OpenXrUtil::makeXrStruct<XrActionStateVector2f>(XR_TYPE_ACTION_STATE_VECTOR2F);
        if (XR_SUCCEEDED(xrGetActionStateVector2f(session_, &stick_info, &stick)) && stick.isActive == XR_TRUE) {
            out_input.thumbstick[i][0] = stick.currentState.x;
            out_input.thumbstick[i][1] = stick.currentState.y;
        }

        if (readBool(session_, menu_action_, hand)) {
            out_input.menu_pressed = true;
        }

        out_input.pose_valid[i] = readPose(session_,
                                           palm_pose_action_,
                                           hand,
                                           palm_spaces_[i],
                                           reference_space_,
                                           predicted_time,
                                           out_input.controller_poses[i]);
        out_input.aim_valid[i] = readPose(session_,
                                          aim_pose_action_,
                                          hand,
                                          aim_spaces_[i],
                                          reference_space_,
                                          predicted_time,
                                          out_input.aim_poses[i]);
    }
    return Result::eSuccess;
}

void OpenXrInput::logCurrentProfiles() const {
    if (session_ == XR_NULL_HANDLE || instance_ == XR_NULL_HANDLE) {
        return;
    }
    for (std::uint32_t i = 0; i < InputState::kMaxControllers; ++i) {
        XrInteractionProfileState state =
            OpenXrUtil::makeXrStruct<XrInteractionProfileState>(XR_TYPE_INTERACTION_PROFILE_STATE);
        std::string profile = "none";
        if (XR_SUCCEEDED(xrGetCurrentInteractionProfile(session_, hand_paths_[i], &state))
            && state.interactionProfile != XR_NULL_PATH) {
            char buffer[XR_MAX_PATH_LENGTH] = {};
            std::uint32_t length = 0;
            if (XR_SUCCEEDED(xrPathToString(instance_, state.interactionProfile, sizeof(buffer), &length, buffer))) {
                profile = buffer;
            }
        }
        VNE_LOG_INFO << "OpenXR interaction profile " << kHandSubpaths[i] << ": " << profile;
    }
}

Result OpenXrInput::applyHaptic(std::uint32_t hand_index, float strength, XrDuration duration) const {
    if (session_ == XR_NULL_HANDLE || hand_index >= InputState::kMaxControllers) {
        return Result::eErrorInvalidArgument;
    }
    XrHapticActionInfo haptic_info = OpenXrUtil::makeXrStruct<XrHapticActionInfo>(XR_TYPE_HAPTIC_ACTION_INFO);
    haptic_info.action = buzz_action_;
    haptic_info.subactionPath = hand_paths_[hand_index];
    XrHapticVibration vibration = OpenXrUtil::makeXrStruct<XrHapticVibration>(XR_TYPE_HAPTIC_VIBRATION);
    vibration.amplitude = strength;
    vibration.duration = duration;
    vibration.frequency = XR_FREQUENCY_UNSPECIFIED;
    OPENXR_CHECK(xrApplyHapticFeedback(session_, &haptic_info, reinterpret_cast<XrHapticBaseHeader*>(&vibration)));
    return Result::eSuccess;
}

}  // namespace vne::xr
