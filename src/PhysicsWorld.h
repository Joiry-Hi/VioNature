#pragma once

#include <memory>
#include <thread>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Core/TempAllocator.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Body/MotionQuality.h>
#include <Jolt/Physics/Body/MotionType.h>
#include <Jolt/Physics/Collision/ObjectLayer.h>
#include <Jolt/Physics/Collision/Shape/Shape.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/RegisterTypes.h>

namespace Layers {
    static constexpr JPH::ObjectLayer NON_MOVING = 0;
    static constexpr JPH::ObjectLayer MOVING = 1;
    static constexpr JPH::ObjectLayer NUM_LAYERS = 2;
}

class PhysicsWorld {
public:
    struct BodyConfig {
        JPH::EMotionType motionType = JPH::EMotionType::Dynamic;
        JPH::ObjectLayer layer = Layers::MOVING;
        JPH::Vec3 linearVelocity = JPH::Vec3::sZero();
        JPH::EMotionQuality motionQuality = JPH::EMotionQuality::Discrete;
        float gravityFactor = 1.0f;
        float linearDamping = 0.05f;
        float angularDamping = 0.05f;
        float friction = 0.2f;
        float restitution = 0.0f;
        bool allowSleeping = true;
    };

    PhysicsWorld();
    ~PhysicsWorld();

    PhysicsWorld(const PhysicsWorld&) = delete;
    PhysicsWorld& operator=(const PhysicsWorld&) = delete;

    JPH::BodyInterface& Bodies();
    const JPH::BodyInterface& Bodies() const;

    JPH::BodyID CreateBody(
        const JPH::Shape* shape,
        JPH::RVec3Arg position,
        JPH::QuatArg rotation,
        const BodyConfig& config,
        JPH::EActivation activation = JPH::EActivation::Activate);

    void DestroyBody(JPH::BodyID id);
    void Step(float dt);

private:
    class Runtime;
    class ObjectLayerPairFilter;
    class BroadPhaseLayerInterfaceImpl;
    class ObjectVsBroadPhaseLayerFilter;

    std::unique_ptr<Runtime> runtime_;
    JPH::TempAllocatorImpl tempAllocator_;
    JPH::JobSystemThreadPool jobSystem_;
    BroadPhaseLayerInterfaceImpl* broadPhaseLayerInterface_;
    ObjectVsBroadPhaseLayerFilter* objectVsBroadPhaseLayerFilter_;
    ObjectLayerPairFilter* objectLayerPairFilter_;
    JPH::PhysicsSystem physicsSystem_;
};
