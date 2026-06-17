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

#include "vertexnova/xr/backend/openxr/math/openxr_math.h"

#include <cmath>
#include <cstring>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace vne::xr::OpenXrMath {

namespace {

void storeMat4(const glm::mat4& m, float out[16]) {
    std::memcpy(out, glm::value_ptr(m), sizeof(float) * 16);
}

}  // namespace

void poseToMatrix(const Pose& pose, float out_matrix[16]) {
    const glm::quat rotation(pose.orientation[3], pose.orientation[0], pose.orientation[1], pose.orientation[2]);
    glm::mat4 m = glm::translate(glm::mat4(1.f), glm::vec3(pose.position[0], pose.position[1], pose.position[2]));
    m = m * glm::mat4_cast(rotation);
    storeMat4(m, out_matrix);
}

void createProjectionFov(const FieldOfView& fov, float near_z, float far_z, float out_matrix[16]) {
    const float tan_left = std::tan(fov.angle_left);
    const float tan_right = std::tan(fov.angle_right);
    const float tan_down = std::tan(fov.angle_down);
    const float tan_up = std::tan(fov.angle_up);

    const float sum_rl = tan_right + tan_left;
    const float sum_tb = tan_up + tan_down;
    const float inv_rl = 1.f / (tan_right - tan_left);
    const float inv_tb = 1.f / (tan_up - tan_down);

    glm::mat4 proj(0.f);
    proj[0][0] = 2.f * inv_rl;
    proj[1][1] = 2.f * inv_tb;
    proj[2][0] = sum_rl * inv_rl;
    proj[2][1] = sum_tb * inv_tb;
    proj[2][2] = far_z / (near_z - far_z);
    proj[2][3] = -1.f;
    proj[3][2] = (far_z * near_z) / (near_z - far_z);

    storeMat4(proj, out_matrix);
}

void populateViewMatrices(const XrView& xr_view, float near_z, float far_z, View& out_view) {
    Pose pose{};
    pose.position[0] = xr_view.pose.position.x;
    pose.position[1] = xr_view.pose.position.y;
    pose.position[2] = xr_view.pose.position.z;
    pose.orientation[0] = xr_view.pose.orientation.x;
    pose.orientation[1] = xr_view.pose.orientation.y;
    pose.orientation[2] = xr_view.pose.orientation.z;
    pose.orientation[3] = xr_view.pose.orientation.w;
    out_view.pose = pose;

    FieldOfView fov{};
    fov.angle_left = xr_view.fov.angleLeft;
    fov.angle_right = xr_view.fov.angleRight;
    fov.angle_up = xr_view.fov.angleUp;
    fov.angle_down = xr_view.fov.angleDown;
    out_view.fov = fov;

    float world_matrix[16]{};
    poseToMatrix(pose, world_matrix);
    glm::mat4 world = glm::make_mat4(world_matrix);
    glm::mat4 view = glm::inverse(world);
    storeMat4(view, out_view.view_matrix);
    createProjectionFov(fov, near_z, far_z, out_view.projection_matrix);
}

}  // namespace vne::xr::OpenXrMath
