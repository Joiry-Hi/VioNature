#pragma once

#include "raylib.h"

#include <array>

enum class WeaponVisualMode {
    Laser,
    LaserCharge,
    Flamethrower,
    RocketLauncher,
    Shotgun,
    GravityNailer,
    InfinityGauntlet,
    RecoilLance,
    RiftCutter
};

class WeaponViewModel {
public:
    WeaponViewModel();
    ~WeaponViewModel();

    WeaponViewModel(const WeaponViewModel&) = delete;
    WeaponViewModel& operator=(const WeaponViewModel&) = delete;

    Vector3 MuzzlePosition(const Camera3D& camera) const;
    void Draw(const Camera3D& camera, WeaponVisualMode mode, float charge) const;

private:
    Vector3 Forward(const Camera3D& camera) const;
    Vector3 Right(const Camera3D& camera) const;
    Vector3 Up(const Camera3D& camera) const;
    Vector3 AnchorPosition(const Camera3D& camera) const;
    Matrix ModelTransform(const Camera3D& camera, Vector3 position) const;
    Vector3 TransformLocalPoint(const Camera3D& camera, Vector3 position, Vector3 localPoint) const;

    static constexpr int kModelCount = 8;
    std::array<Model, kModelCount> models_ = {};
    std::array<bool, kModelCount> modelLoaded_ = {};
};
