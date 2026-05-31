#include "WeaponViewModel.h"

#include "raymath.h"

#include <array>

namespace {
constexpr int kWeaponModelCount = 8;
constexpr std::array<const char*, kWeaponModelCount> kWeaponModelPaths = {
    "assets/models/weapons/laser_rifle.obj",
    "assets/models/weapons/flamethrower.obj",
    "assets/models/weapons/rocket_launcher.obj",
    "assets/models/weapons/shotgun.obj",
    "assets/models/weapons/gravity_nailer.obj",
    "assets/models/weapons/infinity_gauntlet.obj",
    "assets/models/weapons/recoil_lance.obj",
    "assets/models/weapons/rift_cutter.obj",
};
constexpr float kWeaponScale = 0.42f;
constexpr float kLanceScale = 0.58f;
constexpr int kMaterialMapCount = MATERIAL_MAP_BRDF + 1;
constexpr Vector3 kMuzzleLocal = {0.0f, 0.0f, 0.70f};

int ModelIndex(WeaponVisualMode mode) {
    if (mode == WeaponVisualMode::Flamethrower) {
        return 1;
    }
    if (mode == WeaponVisualMode::RocketLauncher) {
        return 2;
    }
    if (mode == WeaponVisualMode::Shotgun) {
        return 3;
    }
    if (mode == WeaponVisualMode::GravityNailer) {
        return 4;
    }
    if (mode == WeaponVisualMode::InfinityGauntlet) {
        return 5;
    }
    if (mode == WeaponVisualMode::RecoilLance) {
        return 6;
    }
    if (mode == WeaponVisualMode::NanoConstructor) {
        return 7;
    }
    return 0;
}

Color ModeTint(WeaponVisualMode mode, float charge) {
    if (mode == WeaponVisualMode::LaserCharge) {
        unsigned char blue = static_cast<unsigned char>(210 + charge * 45.0f);
        return Color{120, 220, blue, 255};
    }
    if (mode == WeaponVisualMode::Flamethrower) {
        return Color{255, 205, 135, 255};
    }
    if (mode == WeaponVisualMode::RocketLauncher) {
        return Color{230, 245, 215, 255};
    }
    if (mode == WeaponVisualMode::Shotgun) {
        return Color{255, 230, 190, 255};
    }
    if (mode == WeaponVisualMode::GravityNailer) {
        return Color{190, 215, 255, 255};
    }
    if (mode == WeaponVisualMode::InfinityGauntlet) {
        return Color{255, 230, 170, 255};
    }
    if (mode == WeaponVisualMode::RecoilLance) {
        return Color{210, 245, 255, 255};
    }
    if (mode == WeaponVisualMode::NanoConstructor) {
        return Color{255, 225, 120, 255};
    }
    return Color{230, 230, 220, 255};
}
}

WeaponViewModel::WeaponViewModel() {
    for (int i = 0; i < kModelCount; ++i) {
        models_[i] = LoadModel(kWeaponModelPaths[i]);
        modelLoaded_[i] = IsModelValid(models_[i]);
    }
}

WeaponViewModel::~WeaponViewModel() {
    for (int i = 0; i < kModelCount; ++i) {
        if (modelLoaded_[i]) {
            UnloadModel(models_[i]);
        }
    }
}

Vector3 WeaponViewModel::Forward(const Camera3D& camera) const {
    return Vector3Normalize(Vector3Subtract(camera.target, camera.position));
}

Vector3 WeaponViewModel::Right(const Camera3D& camera) const {
    return Vector3Normalize(Vector3CrossProduct(Forward(camera), camera.up));
}

Vector3 WeaponViewModel::Up(const Camera3D& camera) const {
    return Vector3Normalize(Vector3CrossProduct(Right(camera), Forward(camera)));
}

Vector3 WeaponViewModel::AnchorPosition(const Camera3D& camera) const {
    Vector3 position = camera.position;
    position = Vector3Add(position, Vector3Scale(Forward(camera), 0.82f));
    position = Vector3Add(position, Vector3Scale(Right(camera), 0.48f));
    position = Vector3Add(position, Vector3Scale(Up(camera), -0.42f));
    return position;
}

Matrix WeaponViewModel::ModelTransform(const Camera3D& camera, Vector3 position, float scale) const {
    Vector3 forward = Forward(camera);
    Vector3 right = Right(camera);
    Vector3 up = Up(camera);

    Matrix transform = MatrixIdentity();
    transform.m0 = right.x * scale;
    transform.m1 = right.y * scale;
    transform.m2 = right.z * scale;

    transform.m4 = up.x * scale;
    transform.m5 = up.y * scale;
    transform.m6 = up.z * scale;

    transform.m8 = forward.x * scale;
    transform.m9 = forward.y * scale;
    transform.m10 = forward.z * scale;

    transform.m12 = position.x;
    transform.m13 = position.y;
    transform.m14 = position.z;
    return transform;
}

Vector3 WeaponViewModel::TransformLocalPoint(const Camera3D& camera, Vector3 position, Vector3 localPoint) const {
    Vector3 world = position;
    world = Vector3Add(world, Vector3Scale(Right(camera), localPoint.x * kWeaponScale));
    world = Vector3Add(world, Vector3Scale(Up(camera), localPoint.y * kWeaponScale));
    world = Vector3Add(world, Vector3Scale(Forward(camera), localPoint.z * kWeaponScale));
    return world;
}

Vector3 WeaponViewModel::MuzzlePosition(const Camera3D& camera) const {
    return TransformLocalPoint(camera, AnchorPosition(camera), kMuzzleLocal);
}

void WeaponViewModel::Draw(const Camera3D& camera, WeaponVisualMode mode, float charge) const {
    int modelIndex = ModelIndex(mode);
    if (!modelLoaded_[modelIndex]) {
        return;
    }
    const Model& model = models_[modelIndex];

    Vector3 position = AnchorPosition(camera);
    float sway = static_cast<float>(GetTime()) * 2.4f;
    position = Vector3Add(position, Vector3Scale(Right(camera), std::sin(sway) * 0.01f));
    position = Vector3Add(position, Vector3Scale(Up(camera), std::cos(sway * 0.8f) * 0.012f));

    Color tint = ModeTint(mode, charge);
    Matrix transform = ModelTransform(camera, position, mode == WeaponVisualMode::RecoilLance ? kLanceScale : kWeaponScale);

    for (int i = 0; i < model.meshCount; ++i) {
        Material material = model.materials[model.meshMaterial[i]];
        std::array<MaterialMap, kMaterialMapCount> maps;
        for (int map = 0; map < kMaterialMapCount; ++map) {
            maps[map] = material.maps[map];
        }
        material.maps = maps.data();

        Color base = material.maps[MATERIAL_MAP_DIFFUSE].color;
        material.maps[MATERIAL_MAP_DIFFUSE].color = Color{
            static_cast<unsigned char>((static_cast<int>(base.r) * tint.r) / 255),
            static_cast<unsigned char>((static_cast<int>(base.g) * tint.g) / 255),
            static_cast<unsigned char>((static_cast<int>(base.b) * tint.b) / 255),
            static_cast<unsigned char>((static_cast<int>(base.a) * tint.a) / 255),
        };
        DrawMesh(model.meshes[i], material, transform);
    }

    if (mode == WeaponVisualMode::LaserCharge) {
        Vector3 muzzle = TransformLocalPoint(camera, position, kMuzzleLocal);
        DrawSphereEx(muzzle, 0.035f + charge * 0.055f, 6, 4, Color{120, 220, 255, 220});
    }
}
