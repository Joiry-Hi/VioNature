#pragma once

#include <algorithm>
#include <cmath>

#include "PhysicsWorld.h"

#include "raylib.h"
#include "raymath.h"

inline constexpr float kFixedFrame = 1.0f / 60.0f;
inline constexpr float kMouseSensitivity = 0.085f;
inline constexpr float kDegToRad = 0.017453292519943295f;
inline constexpr float kFlatBackWorldDepth = 1.0f;

inline Vector3 ToRayVector(JPH::RVec3Arg value) {
    return Vector3{static_cast<float>(value.GetX()), static_cast<float>(value.GetY()), static_cast<float>(value.GetZ())};
}

inline JPH::RVec3 ToJoltVector(Vector3 value) {
    return JPH::RVec3(value.x, value.y, value.z);
}

inline JPH::Vec3 ToJoltVelocity(Vector3 value) {
    return JPH::Vec3(value.x, value.y, value.z);
}

inline Color FadeColor(Color color, float alpha) {
    color.a = static_cast<unsigned char>(std::clamp(alpha, 0.0f, 1.0f) * 255.0f);
    return color;
}

inline float RandomFloat(float minValue, float maxValue) {
    return minValue + (maxValue - minValue) * (static_cast<float>(GetRandomValue(0, 10000)) / 10000.0f);
}

inline Vector3 RotateY(Vector3 value, float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Vector3{value.x * c - value.z * s, value.y, value.x * s + value.z * c};
}

inline Vector3 RotateAroundAxis(Vector3 value, Vector3 axis, float radians) {
    if (Vector3Length(axis) <= 0.001f) {
        return value;
    }
    axis = Vector3Normalize(axis);
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Vector3Add(
        Vector3Add(Vector3Scale(value, c), Vector3Scale(Vector3CrossProduct(axis, value), s)),
        Vector3Scale(axis, Vector3DotProduct(axis, value) * (1.0f - c)));
}

inline Vector3 SafeNormalize(Vector3 value, Vector3 fallback) {
    if (Vector3Length(value) <= 0.001f) {
        return fallback;
    }
    return Vector3Normalize(value);
}
