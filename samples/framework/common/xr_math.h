#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 * ---------------------------------------------------------------------- */

/**
 * @file xr_math.h
 * @brief Small vector / quaternion / 4x4 helpers for the sample framework.
 *
 * Matrices are column-major (`m[column * 4 + row]`), like the view and projection matrices in `vne::xr::View`.
 * Quaternions are (x, y, z, w) with w the scalar part, like `vne::xr::Pose`.
 */

#include "vertexnova/xr/types.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <cstring>

namespace vne::xr::samples::xrmath {

struct Vec3 {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
};

struct Quat {
    float x = 0.f;
    float y = 0.f;
    float z = 0.f;
    float w = 1.f;
};

using Mat4 = std::array<float, 16>;

[[nodiscard]] inline Vec3 operator+(Vec3 a, Vec3 b) {
    return {a.x + b.x, a.y + b.y, a.z + b.z};
}
[[nodiscard]] inline Vec3 operator-(Vec3 a, Vec3 b) {
    return {a.x - b.x, a.y - b.y, a.z - b.z};
}
[[nodiscard]] inline Vec3 operator*(Vec3 a, float s) {
    return {a.x * s, a.y * s, a.z * s};
}
[[nodiscard]] inline float dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}
[[nodiscard]] inline Vec3 cross(Vec3 a, Vec3 b) {
    return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}
[[nodiscard]] inline float length(Vec3 a) {
    return std::sqrt(dot(a, a));
}
[[nodiscard]] inline Vec3 normalized(Vec3 a) {
    const float len = length(a);
    return len > 1e-6f ? a * (1.f / len) : Vec3{};
}

[[nodiscard]] inline Quat conjugate(Quat q) {
    return {-q.x, -q.y, -q.z, q.w};
}
[[nodiscard]] inline Quat operator*(Quat a, Quat b) {
    return {a.w * b.x + a.x * b.w + a.y * b.z - a.z * b.y,
            a.w * b.y - a.x * b.z + a.y * b.w + a.z * b.x,
            a.w * b.z + a.x * b.y - a.y * b.x + a.z * b.w,
            a.w * b.w - a.x * b.x - a.y * b.y - a.z * b.z};
}
[[nodiscard]] inline Quat normalized(Quat q) {
    const float len = std::sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
    return len > 1e-6f ? Quat{q.x / len, q.y / len, q.z / len, q.w / len} : Quat{};
}
[[nodiscard]] inline Vec3 rotate(Quat q, Vec3 v) {
    const Vec3 u{q.x, q.y, q.z};
    const Vec3 t = cross(u, v) * 2.f;
    return v + t * q.w + cross(u, t);
}
[[nodiscard]] inline Quat quatFromAxisAngle(Vec3 axis, float radians) {
    const Vec3 n = normalized(axis);
    const float s = std::sin(radians * 0.5f);
    return {n.x * s, n.y * s, n.z * s, std::cos(radians * 0.5f)};
}
/** @brief Rotation that turns unit vector @p from onto unit vector @p to. */
[[nodiscard]] inline Quat quatFromTo(Vec3 from, Vec3 to) {
    const float d = dot(from, to);
    if (d > 0.99999f) {
        return {};
    }
    if (d < -0.99999f) {
        Vec3 axis = cross(Vec3{1.f, 0.f, 0.f}, from);
        if (length(axis) < 1e-3f) {
            axis = cross(Vec3{0.f, 1.f, 0.f}, from);
        }
        return quatFromAxisAngle(axis, 3.14159265f);
    }
    const Vec3 c = cross(from, to);
    return normalized(Quat{c.x, c.y, c.z, 1.f + d});
}

[[nodiscard]] inline Vec3 positionOf(const Pose& pose) {
    return {pose.position[0], pose.position[1], pose.position[2]};
}
[[nodiscard]] inline Quat orientationOf(const Pose& pose) {
    return {pose.orientation[0], pose.orientation[1], pose.orientation[2], pose.orientation[3]};
}
[[nodiscard]] inline Pose makePose(Vec3 position, Quat orientation) {
    Pose pose;
    pose.position[0] = position.x;
    pose.position[1] = position.y;
    pose.position[2] = position.z;
    pose.orientation[0] = orientation.x;
    pose.orientation[1] = orientation.y;
    pose.orientation[2] = orientation.z;
    pose.orientation[3] = orientation.w;
    return pose;
}

[[nodiscard]] inline Mat4 identity() {
    Mat4 m{};
    m[0] = m[5] = m[10] = m[15] = 1.f;
    return m;
}

/** @brief Copy a column-major float[16] (such as `View::view_matrix`). */
[[nodiscard]] inline Mat4 fromColumnMajor(const float* values) {
    Mat4 m{};
    std::memcpy(m.data(), values, sizeof(float) * 16);
    return m;
}

/** @brief a * b. */
[[nodiscard]] inline Mat4 multiply(const Mat4& a, const Mat4& b) {
    Mat4 out{};
    for (std::size_t col = 0; col < 4; ++col) {
        for (std::size_t row = 0; row < 4; ++row) {
            out[col * 4 + row] = a[0 * 4 + row] * b[col * 4 + 0] + a[1 * 4 + row] * b[col * 4 + 1]
                                 + a[2 * 4 + row] * b[col * 4 + 2] + a[3 * 4 + row] * b[col * 4 + 3];
        }
    }
    return out;
}

/** @brief Translate * rotate * scale. */
[[nodiscard]] inline Mat4 fromTrs(Vec3 t, Quat q, Vec3 s) {
    const float xx = q.x * q.x, yy = q.y * q.y, zz = q.z * q.z;
    const float xy = q.x * q.y, xz = q.x * q.z, yz = q.y * q.z;
    const float wx = q.w * q.x, wy = q.w * q.y, wz = q.w * q.z;
    Mat4 m{};
    m[0] = (1.f - 2.f * (yy + zz)) * s.x;
    m[1] = (2.f * (xy + wz)) * s.x;
    m[2] = (2.f * (xz - wy)) * s.x;
    m[4] = (2.f * (xy - wz)) * s.y;
    m[5] = (1.f - 2.f * (xx + zz)) * s.y;
    m[6] = (2.f * (yz + wx)) * s.y;
    m[8] = (2.f * (xz + wy)) * s.z;
    m[9] = (2.f * (yz - wx)) * s.z;
    m[10] = (1.f - 2.f * (xx + yy)) * s.z;
    m[12] = t.x;
    m[13] = t.y;
    m[14] = t.z;
    m[15] = 1.f;
    return m;
}

[[nodiscard]] inline Mat4 fromPose(const Pose& pose, Vec3 scale = {1.f, 1.f, 1.f}) {
    return fromTrs(positionOf(pose), normalized(orientationOf(pose)), scale);
}

/**
 * @brief Rotation, uniform scale and translation: `p -> t + q * (s * p)`.
 *
 * @details The scene navigator moves and zooms a whole scene with one of these.
 */
struct Similarity {
    Vec3 t{};
    Quat q{};
    float s = 1.f;
};

[[nodiscard]] inline Vec3 apply(const Similarity& x, Vec3 p) {
    return x.t + rotate(x.q, p * x.s);
}
/** @brief a after b: `apply(compose(a, b), p) == apply(a, apply(b, p))`. */
[[nodiscard]] inline Similarity compose(const Similarity& a, const Similarity& b) {
    return {apply(a, b.t), normalized(a.q * b.q), a.s * b.s};
}
[[nodiscard]] inline Similarity inverse(const Similarity& x) {
    const Quat inv_q = conjugate(x.q);
    const float inv_s = 1.f / x.s;
    return {rotate(inv_q, x.t) * (-inv_s), inv_q, inv_s};
}
[[nodiscard]] inline Similarity fromPoseSimilarity(const Pose& pose) {
    return {positionOf(pose), normalized(orientationOf(pose)), 1.f};
}
[[nodiscard]] inline Mat4 toMat4(const Similarity& x) {
    return fromTrs(x.t, x.q, {x.s, x.s, x.s});
}

}  // namespace vne::xr::samples::xrmath
