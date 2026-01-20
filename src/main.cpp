#include <iostream>
#include <vector>
#include <cstdarg>
#include <thread>

// Raylib
#include "raylib.h"
#include "raymath.h"

// Jolt Physics Headers
#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyActivationListener.h>
#include <Jolt/Physics/Body/MotionType.h>   // <--- ！！！关键修复：必须有这行！！！
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

// Jolt 样板代码 (Layers 定义)
namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
};

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
};

class MyObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override {
        switch (inObject1) {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING;
            case Layers::MOVING:     return true;
            default:                 return false;
        }
    }
};

class MyBPLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
public:
    MyBPLayerInterfaceImpl() {
        mObjectToBroadPhase[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        mObjectToBroadPhase[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }
    virtual JPH::uint GetNumBroadPhaseLayers() const override { return BroadPhaseLayers::NUM_LAYERS; }
    virtual JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override { return mObjectToBroadPhase[inLayer]; }
#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    virtual const char *GetBroadPhaseLayerName(JPH::BroadPhaseLayer inLayer) const override { return "Layer"; }
#endif
private:
    JPH::BroadPhaseLayer mObjectToBroadPhase[Layers::NUM_LAYERS];
};

class MyObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    virtual bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override {
        switch (inLayer1) {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:     return true;
            default:                 return false;
        }
    }
};

// 主程序
int main() {
    // 1. 初始化 Jolt
    JPH::RegisterDefaultAllocator();
    JPH::Factory::sInstance = new JPH::Factory();
    JPH::RegisterTypes();

    JPH::TempAllocatorImpl temp_allocator(10 * 1024 * 1024);
    JPH::JobSystemThreadPool job_system(JPH::cMaxPhysicsJobs, JPH::cMaxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    MyBPLayerInterfaceImpl broad_phase_layer_interface;
    MyObjectVsBroadPhaseLayerFilter object_vs_broadphase_layer_filter;
    MyObjectLayerPairFilter object_layer_pair_filter;

    JPH::PhysicsSystem physics_system;
    physics_system.Init(1024, 0, 1024, 1024, broad_phase_layer_interface, object_vs_broadphase_layer_filter, object_layer_pair_filter);
    
    JPH::BodyInterface &body_interface = physics_system.GetBodyInterface();

    // 2. 创建物理场景 (地面)
    JPH::BoxShapeSettings floor_shape_settings(JPH::Vec3(50.0f, 1.0f, 50.0f));
    
    // 这里的 JPH::MotionType::Static 应该能被识别了
    JPH::BodyCreationSettings floor_settings(&floor_shape_settings, JPH::RVec3(0.0f, -1.0f, 0.0f), JPH::Quat::sIdentity(), JPH::EMotionType::Static, Layers::NON_MOVING);
    
    JPH::Body* floor_body = body_interface.CreateBody(floor_settings);
    body_interface.AddBody(floor_body->GetID(), JPH::EActivation::DontActivate);

    // 3. 初始化 Raylib
    const int screenWidth = 1280;
    const int screenHeight = 720;
    InitWindow(screenWidth, screenHeight, "Violence Nature (Dev Build)");
    SetTargetFPS(60);

    Camera3D camera = { 0 };
    camera.position = (Vector3){ 0.0f, 4.0f, 10.0f };
    camera.target = (Vector3){ 0.0f, 2.0f, 0.0f };
    camera.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    camera.fovy = 60.0f;
    camera.projection = CAMERA_PERSPECTIVE;

    DisableCursor(); 

    std::vector<JPH::BodyID> bullets;

    // 4. 游戏循环
    while (!WindowShouldClose()) {
        
        UpdateCamera(&camera, CAMERA_FIRST_PERSON);

        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            JPH::BoxShapeSettings bullet_shape(JPH::Vec3(0.25f, 0.25f, 0.25f));
            
            Vector3 camPos = camera.position;
            Vector3 camFwd = Vector3Subtract(camera.target, camera.position);
            camFwd = Vector3Normalize(camFwd);
            
            JPH::RVec3 spawnPos(camPos.x + camFwd.x * 2.0f, camPos.y + camFwd.y * 2.0f, camPos.z + camFwd.z * 2.0f);

            // JPH::MotionType::Dynamic
            JPH::BodyCreationSettings bullet_settings(&bullet_shape, spawnPos, JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Layers::MOVING);
            JPH::BodyID bullet_id = body_interface.CreateAndAddBody(bullet_settings, JPH::EActivation::Activate);
            
            JPH::Vec3 velocity(camFwd.x * 20.0f, camFwd.y * 20.0f, camFwd.z * 20.0f);
            body_interface.SetLinearVelocity(bullet_id, velocity);

            bullets.push_back(bullet_id);
        }

        // 更新物理
        physics_system.Update(1.0f / 60.0f, 1, &temp_allocator, &job_system);

        // 渲染
        BeginDrawing();
        ClearBackground(BLACK);

        BeginMode3D(camera);
            
            DrawGrid(100, 1.0f); 
            // 简单的地面
            DrawCube((Vector3){0, -1, 0}, 100.0f, 2.0f, 100.0f, DARKGRAY);
            DrawCubeWires((Vector3){0, -1, 0}, 100.0f, 2.0f, 100.0f, GRAY);

            // 绘制子弹
            for (const auto& id : bullets) {
                JPH::RVec3 pos = body_interface.GetCenterOfMassPosition(id);
                Vector3 rPos = { (float)pos.GetX(), (float)pos.GetY(), (float)pos.GetZ() };
                
                DrawCube(rPos, 0.5f, 0.5f, 0.5f, RED);
                DrawCubeWires(rPos, 0.5f, 0.5f, 0.5f, MAROON);
            }

        EndMode3D();
        
        DrawText("FPS Demo - Jolt Local Source", 10, 10, 20, WHITE);
        DrawFPS(10, 40);

        EndDrawing();
    }

    // 清理
    body_interface.RemoveBody(floor_body->GetID());
    body_interface.DestroyBody(floor_body->GetID());

    JPH::UnregisterTypes();
    delete JPH::Factory::sInstance;
    CloseWindow();

    return 0;
}