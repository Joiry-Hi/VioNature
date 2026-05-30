#pragma once

#include <vector>

#include "GameConfig.h"
#include "PhysicsWorld.h"
#include "WeaponViewModel.h"

#include "raylib.h"

#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>

class Game {
public:
    Game();
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    void Update(float dt);
    void Draw() const;

private:
    enum class WeaponType {
        Laser,
        Flamethrower,
        RocketLauncher,
        Shotgun,
        GravityNailer,
        InfinityGauntlet,
        RecoilLance,
        RiftCutter
    };

    enum class FlamethrowerMode {
        FlameBall,
        Heatwave
    };

    enum class RocketLauncherMode {
        Rocket,
        Drone
    };

    enum class ShotgunMode {
        Pellet,
        GlassShard
    };

    enum class GauntletMode {
        TimeStop,
        Blink
    };

    enum class GravityNailerMode {
        Nail,
        BlackHole
    };

    enum class RiftCutterMode {
        BladeWave,
        Platform
    };

    enum class RecoilLanceMode {
        Throw,
        Thrust
    };

    enum class DroneState {
        Deploying,
        Active
    };

    enum class RallyPhase {
        Inactive,
        Assembling,
        Holding,
        Complete
    };

    struct Drone {
        Vector3 position = {};
        Vector3 velocity = {};
        float deployTimer = 1.0f;
        float shootTimer = 0.0f;
        float rocketTimer = 2.4f;
        float bobTimer = 0.0f;
        float life = 30.0f;
        DroneState state = DroneState::Deploying;
    };

    enum class ProjectileKind {
        LaserShot,
        Flame,
        Rocket,
        Pellet,
        EnemyShot,
        GravityNail,
        GlassShard,
        BlackHoleGrenade,
        Lance,
        DroneCanister,
        DroneBullet
    };

    enum class ProjectileOwner {
        Player,
        Enemy
    };

    enum class EnemyType {
        Skitter,
        Brute,
        Wisp,
        Spitter,
        Pouncer,
        Harrier,
        Blinker,
        Boss,
        Duelist
    };

    enum class State {
        Playing,
        Dead
    };

    enum class PickupType {
        SpaceSuit,
        FlightRig,
        Skates
    };

    struct Projectile {
        JPH::BodyID body;
        ProjectileKind kind = ProjectileKind::LaserShot;
        float life = 0.0f;
        float maxLife = 0.0f;
        float damage = 1.0f;
        float radius = 0.18f;
        float maxRadius = 0.18f;
        Color color = WHITE;
        int bouncesLeft = 0;
        Vector3 storedVelocity = {};
        ProjectileOwner owner = ProjectileOwner::Player;
        bool frozen = false;
    };

    struct Beam {
        Vector3 start = {};
        Vector3 end = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float width = 0.08f;
        float charge = 0.0f;
        Color color = WHITE;
    };

    struct Enemy {
        JPH::BodyID body;
        EnemyType type = EnemyType::Skitter;
        float radius = 0.65f;
        float speed = 3.0f;
        float health = 1.0f;
        float maxHealth = 1.0f;
        float bobTimer = 0.0f;
        float actionTimer = 0.0f;
        float cooldownTimer = 0.0f;
        float burstTimer = 0.0f;
        int weaponSlot = 0;
        float weaponSwitchTimer = 0.0f;
        float telegraphTimer = 0.0f;
        int scoreValue = 10;
        Color color = WHITE;
        Vector3 externalVelocity = {};
        Vector3 storedVelocity = {};
        bool frozen = false;
    };

    struct Particle {
        Vector3 position = {};
        Vector3 velocity = {};
        Color color = WHITE;
        float life = 0.0f;
        float maxLife = 0.0f;
        float size = 0.08f;
    };

    struct Prop {
        Vector3 position = {};
        Vector3 scale = {};
        float rotationY = 0.0f;
        Color color = WHITE;
        int shape = 0;
        bool collidable = false;
    };

    struct Pickup {
        PickupType type = PickupType::SpaceSuit;
        Vector3 position = {};
        float radius = 0.75f;
        float bobTimer = 0.0f;
    };

    struct GravityWell {
        Vector3 position = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        float force = 1.0f;
        float damagePerSecond = 0.0f;
        bool blackHole = false;
    };

    struct Shockwave {
        Vector3 position = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        Color color = WHITE;
    };

    struct HeatwavePulse {
        Vector3 origin = {};
        Vector3 forward = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float range = 1.0f;
        float halfAngle = 0.7f;
        Color color = WHITE;
    };

    struct RiftSlash {
        Vector3 center = {};
        Vector3 normal = {};
        Vector3 right = {};
        Vector3 up = {};
        Vector3 velocity = {};
        float delay = 0.0f;
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        float thickness = 0.5f;
        float planeThickness = 0.5f;
        float damagePerSecond = 1.0f;
        ProjectileOwner owner = ProjectileOwner::Player;
    };

    struct NanoPlatform {
        Vector3 position = {};
        Vector3 scale = {};
        Vector3 normal = {0.0f, 1.0f, 0.0f};
        Vector3 right = {1.0f, 0.0f, 0.0f};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float delay = 0.0f;
        float life = 0.0f;
        float maxLife = 0.0f;
    };

    enum class BethlehemLaserPhase { Inactive, Warning, Damaging };

    struct BethlehemBoss {
        Vector3 position = {};
        Vector3 laserDirection = {};
        float orbitAngle = 0.0f;
        float health = 0.0f;
        float maxHealth = 0.0f;
        float attackTimer = 0.0f;
        float phaseTimer = 0.0f;
        BethlehemLaserPhase laserPhase = BethlehemLaserPhase::Inactive;
        bool active = false;
    };

    void Reset();
    void ClearWorld();
    void UpdatePlayer(float dt);
    void UpdateLook(float dt);
    void UpdateMovement(float dt);
    void UpdateFreeCamera(float dt);
    void UpdateWeaponSwitching();
    void UpdateShooting(float dt);
    void UpdateBeam(float dt);
    void UpdateShockwaves(float dt);
    void UpdateHeatwaves(float dt);
    void UpdateGravityWells(float dt);
    void UpdateRifts(float dt);
    void UpdateNanoPlatforms(float dt);
    void UpdateEnemies(float dt);
    void UpdateWaveDirector(float dt);
    void UpdateProjectiles(float dt);
    void UpdateParticles(float dt);
    void UpdatePickups(float dt);
    void UpdateCollisions();
    void UpdateArenaBounds();
    void BuildMap();
    void ResolveMapCollision(Vector3 previousPosition);
    void SpawnStartingPickups();
    void SpawnPickup(PickupType type, int slot);
    void SpawnEnemy();
    void SpawnEnemyOfType(EnemyType type);
    bool BossAlive() const;
    bool DuelMode() const;
    bool DuelWon() const;
    void FireBossRing(Vector3 position, int count, float speedScale);
    void UpdateDuelist(Enemy& enemy, Vector3 position, Vector3 direction, float dt, float& speed, bool& skipVelocity);
    void SwitchDuelistWeapon(Enemy& enemy, float distance);
    void FireDuelistWeapon(Enemy& enemy, Vector3 position, Vector3 toPlayer);
    void FireEnemyBeam(Vector3 origin, Vector3 direction, float charge);
    void SpawnEnemyNanoPlatform(Vector3 origin, Vector3 direction);
    void FireProjectile(ProjectileKind kind, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color);
    void FireEnemyProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color);
    void FireLaser(float charge);
    void FireEnemyShot(Vector3 position, Vector3 direction);
    void FireHeatwave(Vector3 direction);
    void FireDuelistHeatwave(Vector3 origin, Vector3 direction);
    void FireRiftCutter(Vector3 direction);
    void FireNanoPlatform(Vector3 direction);
    void FireLanceThrust(Vector3 direction);
    void DetonateLance(Vector3 position, ProjectileOwner owner);
    void SpawnEnemyRift(Vector3 origin, Vector3 direction);
    void SpawnGravityWell(Vector3 position, bool blackHole = false);
    void BlinkDuelist(Enemy& enemy, Vector3 awayFrom);
    void ToggleTimeStop();
    void FreezeDynamicObjects();
    void RestoreDynamicObjects();
    void Blink();
    void SpawnShockwave(Vector3 position, float radius, Color color);
    void ExplodeRocket(Vector3 position, ProjectileOwner owner = ProjectileOwner::Player);
    void FireDroneCanister();
    void UpdateDrones(float dt);
    void ApplyExplosionImpulse(Vector3 position, float radius, float impulse);
    void ApplyShotgunRecoil(Vector3 direction);
    void ApplyLanceRecoil(Vector3 direction);
    void ApplyPlayerHit(Vector3 position, Color color, const char* eventText = nullptr);
    void AddEnemyImpulse(Enemy& enemy, Vector3 impulse);
    void AddProjectileImpulse(Projectile& projectile, Vector3 impulse);
    void SpawnHitBurst(Vector3 position, Color color, int count);
    void DestroyProjectile(size_t index);
    void DestroyEnemy(size_t index);

    Vector3 PlayerForward() const;
    Vector3 PlayerRight() const;
    Vector3 PlayerUp() const;
    Vector3 WeaponMuzzlePosition() const;
    NanoPlatform MakeNanoPlatformTarget(Vector3 direction) const;
    Vector3 GetFireControlAimPoint() const;
    void UpdateBethlehem(float dt);
    void DrawBethlehem() const;
    void SpawnBethlehem();
    void DestroyBethlehem();
    bool BethlehemAlive() const { return bethlehem_.active; }
    bool IsSphericalMap() const;
    bool IsHollowWorldMap() const;
    float SphericalRadius() const;
    float SphericalPlayerAltitude() const;
    float SphericalCleanupDistance() const;
    float SphericalAltitudeAt(Vector3 position) const;
    float SphericalSignedRadius(float altitude) const;
    Vector3 SphericalUpAt(Vector3 position) const;
    Vector3 SphericalSurfacePoint(Vector3 position, float altitude) const;
    Vector3 ProjectOnSphericalTangent(Vector3 vector, Vector3 up) const;
    float SphericalEnemyAltitude(EnemyType type) const;
    Vector3 BodyPosition(JPH::BodyID id) const;
    const char* WeaponName() const;
    const char* WeaponModeName() const;
    const char* WaveLabel() const;
    float CurrentGravity() const;
    bool IsSquareMap() const;
    bool EnemyTouchesPlayer(Vector3 enemyPosition, float enemyRadius) const;
    float DistancePointToSegment(Vector3 point, Vector3 start, Vector3 end) const;
    float DistanceXZ(Vector3 a, Vector3 b) const;

    void DrawArena() const;
    void DrawProps() const;
    void DrawEnemies() const;
    void DrawPickups() const;
    void DrawProjectiles() const;
    void DrawBeams() const;
    void DrawShockwaves() const;
    void DrawHeatwaves() const;
    void DrawGravityWells() const;
    void DrawRifts() const;
    void DrawNanoPlatforms() const;
    void DrawNanoPlatformFrame(const NanoPlatform& platform, Color color, bool dashed) const;
    void DrawDrones() const;
    void DrawRallyMarker() const;
    void DrawDashedCircle3D(Vector3 center, float radius, Vector3 normal, Color color) const;
    void DrawBlinkIndicator() const;
    void DrawFireControlOverlay() const;
    void DrawParticles() const;
    void DrawWeapon() const;
    void DrawCrosshair() const;
    void DrawHud() const;

    PhysicsWorld physics_;
    GameplayConfig config_;
    WeaponViewModel weaponViewModel_;
    Camera3D camera_ = {};
    State state_ = State::Playing;

    JPH::RefConst<JPH::Shape> floorShape_;
    JPH::RefConst<JPH::Shape> projectileShape_;
    JPH::RefConst<JPH::Shape> enemyShape_;
    JPH::BodyID floorBody_;

    std::vector<Projectile> projectiles_;
    std::vector<Enemy> enemies_;
    std::vector<Particle> particles_;
    std::vector<Beam> beams_;
    std::vector<Shockwave> shockwaves_;
    std::vector<HeatwavePulse> heatwaves_;
    std::vector<GravityWell> gravityWells_;
    std::vector<RiftSlash> rifts_;
    std::vector<NanoPlatform> nanoPlatforms_;
    std::vector<Drone> drones_;
    std::vector<Prop> props_;
    std::vector<Pickup> pickups_;

    RenderTexture2D pixelTarget_ = {};
    int pixelWidth_ = 426;
    int pixelHeight_ = 240;

    float arenaRadius_ = 28.0f;
    float squareHalfExtent_ = 31.0f;
    Vector3 asteroidReferenceForward_ = {0.0f, 0.0f, -1.0f};
    float playerRadius_ = 0.65f;
    float playerHeight_ = 2.0f;
    Vector3 playerVelocity_ = {};
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    bool grounded_ = true;
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    bool hasSpaceSuit_ = false;
    bool hasFlightRig_ = false;
    bool hasSkates_ = false;
    bool spaceSuitEnabled_ = false;
    bool flightRigEnabled_ = false;
    bool skatesEnabled_ = false;
    bool hideUI_ = false;
    float gravityScale_ = 1.0f;
    float flightTargetAltitude_ = 2.0f;
    float footstepBob_ = 0.0f;
    float thrustControlLockTimer_ = 0.0f;
    WeaponType activeWeapon_ = WeaponType::Laser;
    FlamethrowerMode flamethrowerMode_ = FlamethrowerMode::FlameBall;
    RocketLauncherMode rocketLauncherMode_ = RocketLauncherMode::Rocket;
    ShotgunMode shotgunMode_ = ShotgunMode::Pellet;
    GravityNailerMode gravityNailerMode_ = GravityNailerMode::Nail;
    RiftCutterMode riftCutterMode_ = RiftCutterMode::BladeWave;
    RecoilLanceMode recoilLanceMode_ = RecoilLanceMode::Throw;
    float riftPlatformRangeScale_ = 1.0f;
    GauntletMode gauntletMode_ = GauntletMode::TimeStop;
    float blinkDistanceScale_ = 1.0f;
    bool timeStopped_ = false;
    float timeStopTintTimer_ = 0.0f;
    float fireCooldown_ = 0.0f;
    bool chargingLaser_ = false;
    float laserCharge_ = 0.0f;
    float rightMouseHeld_ = 0.0f;
    bool fireControlActive_ = false;
    RallyPhase rallyPhase_ = RallyPhase::Inactive;
    Vector3 rallyPoint_ = {};
    float rallyHoldTimer_ = 0.0f;
    float spawnTimer_ = 0.0f;
    float spawnInterval_ = 2.0f;
    int waveIndex_ = 1;
    float eventTextTimer_ = 0.0f;
    const char* eventText_ = "";
    bool wispSurgeDone_ = false;
    bool spitterAmbushDone_ = false;
    bool pouncerRushDone_ = false;
    bool bossSpawned_ = false;
    bool bethlehemSpawned_ = false;
    BethlehemBoss bethlehem_;
    Model bethlehemModel_;
    bool bethlehemModelLoaded_ = false;
    bool duelWon_ = false;
    float nextMixedEventTime_ = 104.0f;
    int duelArmor_ = 0;
    float duelArmorInvulnTimer_ = 0.0f;
    float survivalTime_ = 0.0f;
    float cameraShake_ = 0.0f;
    int score_ = 0;
};
