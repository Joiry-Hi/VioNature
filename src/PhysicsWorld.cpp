#include "PhysicsWorld.h"

namespace BroadPhaseLayers {
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr JPH::uint NUM_LAYERS(2);
}

namespace {
int DetectWorkerThreadCount() {
    unsigned int hardwareThreads = std::thread::hardware_concurrency();
    if (hardwareThreads <= 1) {
        return 1;
    }
    return static_cast<int>(hardwareThreads - 1);
}
}

class PhysicsWorld::Runtime {
public:
    Runtime() {
        JPH::RegisterDefaultAllocator();
        JPH::Factory::sInstance = new JPH::Factory();
        JPH::RegisterTypes();
    }

    ~Runtime() {
        JPH::UnregisterTypes();
        delete JPH::Factory::sInstance;
        JPH::Factory::sInstance = nullptr;
    }
};

class PhysicsWorld::ObjectLayerPairFilter : public JPH::ObjectLayerPairFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer object1, JPH::ObjectLayer object2) const override {
        switch (object1) {
            case Layers::NON_MOVING:
                return object2 == Layers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
        }
    }
};

class PhysicsWorld::BroadPhaseLayerInterfaceImpl : public JPH::BroadPhaseLayerInterface {
public:
    BroadPhaseLayerInterfaceImpl() {
        objectToBroadPhase_[Layers::NON_MOVING] = BroadPhaseLayers::NON_MOVING;
        objectToBroadPhase_[Layers::MOVING] = BroadPhaseLayers::MOVING;
    }

    JPH::uint GetNumBroadPhaseLayers() const override {
        return BroadPhaseLayers::NUM_LAYERS;
    }

    JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer layer) const override {
        return objectToBroadPhase_[layer];
    }

#if defined(JPH_EXTERNAL_PROFILE) || defined(JPH_PROFILE_ENABLED)
    const char* GetBroadPhaseLayerName(JPH::BroadPhaseLayer layer) const override {
        if (layer == BroadPhaseLayers::NON_MOVING) {
            return "NON_MOVING";
        }
        return "MOVING";
    }
#endif

private:
    JPH::BroadPhaseLayer objectToBroadPhase_[Layers::NUM_LAYERS];
};

class PhysicsWorld::ObjectVsBroadPhaseLayerFilter : public JPH::ObjectVsBroadPhaseLayerFilter {
public:
    bool ShouldCollide(JPH::ObjectLayer layer1, JPH::BroadPhaseLayer layer2) const override {
        switch (layer1) {
            case Layers::NON_MOVING:
                return layer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING:
                return true;
            default:
                return false;
        }
    }
};

PhysicsWorld::PhysicsWorld()
    : runtime_(new Runtime()),
      tempAllocator_(10 * 1024 * 1024),
      jobSystem_(
          JPH::cMaxPhysicsJobs,
          JPH::cMaxPhysicsBarriers,
          DetectWorkerThreadCount()),
      broadPhaseLayerInterface_(new BroadPhaseLayerInterfaceImpl()),
      objectVsBroadPhaseLayerFilter_(new ObjectVsBroadPhaseLayerFilter()),
      objectLayerPairFilter_(new ObjectLayerPairFilter()) {
    physicsSystem_.Init(
        4096,
        0,
        4096,
        4096,
        *broadPhaseLayerInterface_,
        *objectVsBroadPhaseLayerFilter_,
        *objectLayerPairFilter_);
}

PhysicsWorld::~PhysicsWorld() {
    delete objectLayerPairFilter_;
    delete objectVsBroadPhaseLayerFilter_;
    delete broadPhaseLayerInterface_;
}

JPH::BodyInterface& PhysicsWorld::Bodies() {
    return physicsSystem_.GetBodyInterface();
}

const JPH::BodyInterface& PhysicsWorld::Bodies() const {
    return physicsSystem_.GetBodyInterface();
}

JPH::BodyID PhysicsWorld::CreateBody(
    const JPH::Shape* shape,
    JPH::RVec3Arg position,
    JPH::QuatArg rotation,
    const BodyConfig& config,
    JPH::EActivation activation) {
    JPH::BodyCreationSettings settings(shape, position, rotation, config.motionType, config.layer);
    settings.mLinearVelocity = config.linearVelocity;
    settings.mMotionQuality = config.motionQuality;
    settings.mGravityFactor = config.gravityFactor;
    settings.mLinearDamping = config.linearDamping;
    settings.mAngularDamping = config.angularDamping;
    settings.mFriction = config.friction;
    settings.mRestitution = config.restitution;
    settings.mAllowSleeping = config.allowSleeping;

    return Bodies().CreateAndAddBody(settings, activation);
}

void PhysicsWorld::DestroyBody(JPH::BodyID id) {
    if (id.IsInvalid()) {
        return;
    }

    Bodies().RemoveBody(id);
    Bodies().DestroyBody(id);
}

void PhysicsWorld::Step(float dt) {
    constexpr int collisionSteps = 1;
    physicsSystem_.Update(dt, collisionSteps, &tempAllocator_, &jobSystem_);
}
