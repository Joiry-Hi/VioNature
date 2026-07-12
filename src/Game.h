#pragma once

#include <algorithm>
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
    void Draw();
    bool IsConsoleOpen() const { return consoleOpen_; }
    void CloseConsole() { consoleOpen_ = false; }
    const char* T(const char* cn, const char* en) const {
        return config_.tutorialLanguage == "english" ? en : cn;
    }
    bool WantsQuit() const { return wantsQuit_; }
    bool IsExiting() const { return exitHoldTimer_ > 0.0f; }
    float ExitProgress() const { return exitHoldTimer_ / kExitHoldDuration; }

private:
    enum class WeaponType {
        Laser,
        Flamethrower,
        RocketLauncher,
        Shotgun,
        GravityNailer,
        InfinityGauntlet,
        LonginusSpear,
        NanoConstructor,
        MysticStaff
    };

    enum class MysticStaffMode {
        CurseOrb,
        Shield
    };

    enum class LaserMode {
        Plasma,
        Beam
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

    enum class NanoConstructorMode {
        NanoBlade,
        NanoPlatform
    };

    enum class LonginusSpearMode {
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
        int world = 0;
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
        DroneBullet,
        HomingShot,
        UfoOrb,
        CurseOrb,
        SoulOrb
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
        SlimeKing,
        Minotaur,
        Duelist,
        Dummy,
        DummyBoss
    };

    enum class State {
        Playing,
        Dead
    };

    enum class ScavengerUfoState {
        Inactive,
        Pending,
        Active,
        Escaping,
        DefeatedFalling,
        DefeatedLanded,
        PlayerPiloted,
        ParkedHover,
        Gone
    };

    enum class UfoPilotWeapon {
        Orb,
        Tractor
    };

    enum class UfoOrbMode {
        Projectile,
        Laser
    };

    enum class UfoTravelState {
        Inactive,
        Charging,
        Hyperspace,
        Arriving
    };

    enum class PickupType {
        SpaceSuit,
        FlightRig,
        Skates,
        Essence
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
        JPH::BodyID lastHitEnemy;
        float turnRate = 0.0f;
        bool fromMagicCircle = false;
        int world = 0;
        JPH::BodyID shooterBody;  // enemy that fired this (prevents self-damage)
    };

    struct Beam {
        Vector3 start = {};
        Vector3 end = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float width = 0.08f;
        float charge = 0.0f;
        Color color = WHITE;
        float damagePerFrame = 0.0f;  // >0 = sustained damage beam
        float hue = 0.0f;             // >0 = rainbow beam
        int ownerWorld = 0;
        bool followPlayerMuzzle = false;
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
        int burstCount = 0;
        int weaponSlot = 0;
        float weaponSwitchTimer = 0.0f;
        float telegraphTimer = 0.0f;
        int scoreValue = 10;
        Color color = WHITE;
        Vector3 externalVelocity = {};
        Vector3 storedVelocity = {};
        bool frozen = false;
        int slimeGeneration = 0;
        bool cursed = false;
        float curseDps = 0.0f;
        bool killedBySoulOrb = false;
        float lastDamageTime = -999.0f;
        int aiTier = 0;  // 0=random barrage, 1=strategic mirror
        float equipmentTimer = 0.0f;  // cooldown for gear toggling
        bool usingSpaceSuit = false;
        bool usingFlightRig = false;
        bool usingSkates = false;
        bool shieldActive = false;
        float shieldCooldown = 0.0f;
        bool ignited = false;
        float igniteTimer = 0.0f;
        float igniteDps = 0.0f;
        float igniteSpreadTimer = 0.0f;
        int world = 0;
        float warCommandTimer = 0.0f;
        float warEnrageTimer = 0.0f;
        int warGrowthStacks = 0;
        float plagueTimer = 0.0f;
        bool plagueBurstOnDeath = false;
    };

    struct DamageNumber {
        JPH::BodyID enemyBody;
        Vector3 worldPosition = {};
        float value = 0.0f;
        float life = 0.0f;
        float maxLife = 1.2f;
        float screenYOffset = 0.0f;
        float heightOffset = 0.0f;
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
        Vector3 velocity = {};
        float radius = 0.75f;
        float bobTimer = 0.0f;
        float horizontalDrag = 0.0f;  // >0 for falling essence
        float gravityScale = 0.0f;    // >0 for falling essence
        float age = 0.0f;             // >0 while timed essence is fading
        float maxLife = 0.0f;         // 0 = persistent pickup
    };

    struct ScavengerUfoBoss {
        ScavengerUfoState state = ScavengerUfoState::Inactive;
        Vector3 position = {};
        Vector3 velocity = {};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float spawnTimer = 0.0f;
        float attackTimer = 0.0f;
        float altitudeTarget = 0.0f;
        float altitudeTimer = 0.0f;
        float escapeTimer = 0.0f;
        float fallSpeed = 0.0f;
        float bobTimer = 0.0f;
        float pilotOrbTimer = 0.0f;
        float pilotJumpCooldown = 0.0f;
        int targetPickupIndex = -1;
        int collected = 0;
        int pilotEssence = 0;
        int pilotTotalCollected = 0;
        bool triggeredThisRun = false;
        bool tractoring = false;
        int world = 0;
    };

    struct ThroneAngelBoss {
        bool active = false;
        bool defeated = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 wanderTarget = {};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float summonTimer = 0.0f;
        float pulseTimer = 0.0f;
        float wanderTimer = 0.0f;
        float hitFlash = 0.0f;
        float jacobLadderTimer = 0.0f;
        bool jacobLadderEntered = false;
        float ringAngles[3] = {};
        float ringSpeeds[3] = {};
        Vector3 ringAxes[3] = {};
        int world = 0;
    };

    struct CherubMinion {
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 target = {};
        float health = 0.0f;
        float shootTimer = 0.0f;
        float reacquireTimer = 0.0f;
        float antigravityTimer = 0.0f;
        float wingTimer = 0.0f;
        float flashTimer = 0.0f;
        int world = 0;
    };

    struct SeraphBoss {
        bool active = false;
        bool defeated = false;
        bool edenApocalypse = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 wanderTarget = {};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float attackTimer = 0.0f;
        float wanderTimer = 0.0f;
        float wingTimer = 0.0f;
        float attackFlash = 0.0f;
        float hitFlash = 0.0f;
        int world = 0;
    };

    struct WarRiderBoss {
        bool active = false;
        bool defeated = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float chargeTimer = 0.0f;
        float chargeTimeLeft = 0.0f;
        float chargeDistanceLeft = 0.0f;
        Vector3 chargeTarget = {};
        Vector3 chargeDirection = {0.0f, 0.0f, 1.0f};
        float chargeFireballTimer = 0.0f;
        float slashTimer = 0.0f;
        float commandTimer = 0.0f;
        float orbitAngle = 0.0f;
        float hitFlash = 0.0f;
        float gallopTimer = 0.0f;
        float contactCooldown = 0.0f;
        int world = 0;
    };

    struct ConquestRiderBoss {
        bool active = false;
        bool defeated = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float arrowTimer = 0.0f;
        float summonTimer = 0.0f;
        float orbitAngle = 0.0f;
        float hitFlash = 0.0f;
        float gallopTimer = 0.0f;
        int world = 0;
    };

    struct FamineRiderBoss {
        bool active = false;
        bool defeated = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float health = 0.0f;
        float maxHealth = 0.0f;
        float witherTimer = 0.0f;
        float orbitAngle = 0.0f;
        float hitFlash = 0.0f;
        float gallopTimer = 0.0f;
        float scaleTipTimer = 0.0f;
        float witherRadiusBonus = 0.0f;
        Vector3 wanderTarget = {};
        float wanderTimer = 0.0f;
        int world = 0;
    };

    struct DeathRiderBoss {
        bool active = false;
        bool defeated = false;
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float health = 0.0f;
        float maxHealth = 0.0f;
        int souls = 0;
        float skullTimer = 0.0f;
        float orbitAngle = 0.0f;
        float hitFlash = 0.0f;
        float gallopTimer = 0.0f;
        int world = 0;
    };

    struct DeathSoul {
        Vector3 position = {};
        Vector3 velocity = {};
        float life = 0.0f;
        float radius = 0.32f;
        int world = 0;
    };

    struct DeathSkull {
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        float health = 1.0f;
        float life = 0.0f;
        float waitTimer = 0.0f;
        float radius = 0.55f;
        int world = 0;
    };

    struct PlagueArrow {
        Vector3 position = {};
        Vector3 prevPosition = {};
        Vector3 velocity = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        Vector3 side = {1.0f, 0.0f, 0.0f};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 0.3f;
        int world = 0;
    };

    struct SeraphFireball {
        Vector3 position = {};
        Vector3 prevPosition = {};
        Vector3 velocity = {};
        Vector3 flightDirection = {};
        Vector3 tipDirection = {};
        Vector3 visualSide = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 0.42f;
        float damage = 1.0f;
        int world = 0;
        bool warFire = false;
        bool sodomFire = false;
    };

    struct UfoHyperspaceObstacle {
        float angle = 0.0f;
        float altitude = 0.0f;
        float distance = 0.0f;
        float radius = 1.0f;
        bool hit = false;
    };

    struct UfoPreservedState {
        int ufoEssence = 0;
        int playerEssence = 0;
        int totalCollected = 0;
        UfoPilotWeapon weapon = UfoPilotWeapon::Orb;
        UfoOrbMode orbMode = UfoOrbMode::Projectile;
    };

    struct GravityWell {
        Vector3 position = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        float force = 1.0f;
        float damagePerSecond = 0.0f;
        bool blackHole = false;
        bool enemyOrigin = false;  // created by duelist — pulls player
    };

    struct Shockwave {
        Vector3 position = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        Color color = WHITE;
    };

    struct BallLightning {
        Vector3 position = {};
        Vector3 velocity = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 2.5f;
        float fireTimer = 0.0f;
        float hue = 0.0f;
        int world = 0;
    };

    struct WaterDropletCraft {
        Vector3 position = {};
        float progress = 0.0f;
        int essenceSpent = 0;
        int world = 0;
    };

    struct WaterDroplet {
        Vector3 position = {};
        Vector3 velocity = {};
        Vector3 targetPos = {};
        bool hasTarget = false;
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 0.8f;
        float damage = 500.0f;
        float reacquireTimer = 0.0f;
        float trailTimer = 0.0f;
        int world = 0;
        Vector3 trailHistory[8] = {};
        int trailHead = 0;
        int trailCount = 0;
    };

    struct NapalmGrenade {
        Vector3 position = {};
        Vector3 velocity = {};
        float fuse = 2.5f;
        float life = 0.0f;
        int bounces = 0;
        int world = 0;
        JPH::BodyID shooterBody;  // duelist that fired this (prevents self-damage)
    };

    struct FirePatch {
        Vector3 position = {};
        Vector3 up = {0.0f, 1.0f, 0.0f};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 3.0f;
        float damagePerSecond = 2.0f;
        int world = 0;
        JPH::BodyID sourceBody;  // duelist that created this patch (prevents self-damage)
        bool hurtsPlayer = false;
        bool hurtsEnemies = true;
        Color outerColor = Color{255, 140, 30, 255};
        Color innerColor = Color{255, 200, 60, 255};
        Color particleColor = Color{255, 120, 20, 200};
        bool infectsEnemies = false;
        bool plague = false;
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

    struct NanoBlade {
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
        int world = 0;
    };

    struct JudgmentStigma {
        Vector3 start = {};
        Vector3 end = {};
        Vector3 forward = {};
        Vector3 right = {};
        Vector3 up = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        float damagePerSecond = 1.0f;
        int world = 0;
    };

    struct EdenGuardian {
        Vector3 position = {};
        Vector3 radial = {1.0f, 0.0f, 0.0f};
        float attackTimer = 0.0f;
        float stepPhase = 0.0f;
    };

    struct EdenFireSlash {
        Vector3 center = {};
        Vector3 normal = {};
        Vector3 right = {};
        Vector3 up = {};
        Vector3 velocity = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 1.0f;
        float thickness = 0.5f;
        float planeThickness = 0.5f;
    };

    struct EdenArk {
        bool active = false;
        bool piloted = false;
        Vector3 position = {};
        Vector3 forward = {0.0f, 0.0f, 1.0f};
        JPH::BodyID body;
        float heading = 0.0f;
        float speed = 0.0f;
        float cameraYawOffset = 3.14159265f;
        float cameraPitch = -0.68f;
        float interactCooldown = 0.0f;
        float wakeTimer = 0.0f;
    };

    struct LabyrinthCell {
        int x = 1;
        int y = 1;
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
        int world = 0;
        JPH::BodyID platformBody;
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
        float essenceTimer = 15.0f;
        BethlehemLaserPhase laserPhase = BethlehemLaserPhase::Inactive;
        bool active = false;
    };

    struct SlimeSpawnPod {
        Vector3 position = {};
        Vector3 velocity = {};
        float timer = 0.0f;
        float maxTimer = 0.0f;
        float health = 0.0f;
        float radius = 0.0f;
        int generation = 0;
    };

    struct MagicCircle {
        Vector3 position = {};
        float life = 0.0f;
        float maxLife = 0.0f;
        float radius = 3.5f;
        float fireCooldown = 0.0f;
        float fireInterval = 0.15f;
        bool isWormhole = false;
        bool activated = false;
        bool activatedByLaserBeam = false;
        bool activatedByNapalm = false;
        ProjectileKind activatedKind = ProjectileKind::LaserShot;
        float fireRateMult = 1.0f;
        float homingTurnRate = 3.5f;
        int world = 0;
    };

    struct WormholePortal {
        Vector3 frontPosition = {};
        Vector3 backPosition = {};
        Vector3 mirrorAnchor = {};
        Vector3 mirrorNormal = {0.0f, 1.0f, 0.0f};
        float playerCooldown = 0.0f;
        float enemyCooldown = 0.0f;
        int circleIndex = -1;
    };

    void Reset();
    void ClearWorld();

    // GamePlayer.cpp
    void UpdatePlayer(float dt);
    void UpdateLook(float dt);
    void UpdateMovement(float dt);
    void UpdateFreeCamera(float dt);

    // GameWeapons.cpp
    void UpdateWeaponSwitching();
    void UpdateConsole();
    bool SetConfigValue(const std::string& key, const std::string& value);
    std::string GetConfigValue(const std::string& key) const;
    std::vector<std::string> GetConfigKeys() const;
    void UpdateShooting(float dt);

    // GameProjectiles.cpp
    void UpdateBeam(float dt);
    void UpdateShockwaves(float dt);
    void UpdateHeatwaves(float dt);
    void UpdateFirePatches(float dt);
    void UpdateGravityWells(float dt);
    void UpdateNanoBlades(float dt);
    void UpdateJudgmentStigmas(float dt);
    void ResetEdenGuardians();
    void UpdateEdenGuardians(float dt);
    void FireEdenGuardianSlash(EdenGuardian& guardian);
    void UpdateEdenFireSlashes(float dt);
    void UpdateNanoPlatforms(float dt);
    void UpdateSlimeSpawnPods(float dt);
    void UpdateMagicCircles(float dt);
    void UpdateWormholes(float dt);
    void UpdateEnemies(float dt);
    void UpdateWaveDirector(float dt);
    void UpdateProjectiles(float dt);
    void UpdateParticles(float dt);
    void UpdateCollisions();
    void FireBossRing(Vector3 position, int count, float speedScale, int world = 0);
    void FireEnemyBeam(Vector3 origin, Vector3 direction, float charge);
    void SpawnEnemyNanoPlatform(Vector3 origin, Vector3 direction, int world = 0);
    void FireProjectile(ProjectileKind kind, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color);
    void FireEnemyProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color, int world = 0, JPH::BodyID shooterBody = JPH::BodyID());
    void FireLaser(float charge);
    void FireBallLightning();
    void UpdateBallLightnings(float dt);
    void ExplodeBallLightning(BallLightning& ball);
    void TriggerScavengerUfo(Vector3 origin);
    void SpawnScavengerUfo();
    void UpdateScavengerUfo(float dt);
    void UpdateUfoPilot(float dt);
    bool UfoPilotActive() const;
    bool UfoHyperspaceActive() const;
    bool UfoEnterAvailable() const;
    void EnterScavengerUfo();
    void ExitScavengerUfo();
    void TeleportPilotedUfo();
    void BeginUfoHyperspaceCharge();
    void BeginUfoHyperspace();
    void UpdateUfoHyperspace(float dt);
    void CompleteUfoHyperspaceArrival();
    void ResetWorldForUfoArrival(const std::string& mode, const std::string& map, const UfoPreservedState& preserved);
    void ApplyUfoArrivalVariant(const GameplayConfig& baseConfig);
    void DamageScavengerUfo(float damage, Vector3 hitPosition, Color color);
    void DamageScavengerUfoInRadius(Vector3 position, float radius, float damage, Color color);
    bool ScavengerUfoDamageable() const;
    bool ScavengerUfoEncounterActive() const;
    void SpawnThroneAngel();
    void UpdateThroneAngel(float dt);
    void DamageThroneAngel(float damage, Vector3 hitPosition, Color color);
    void DamageThroneAngelInRadius(Vector3 position, float radius, float damage, Color color);
    void SpawnCherubs(int count);
    void UpdateCherubs(float dt);
    void TriggerThronePulse();
    bool ThroneJacobLadderActive() const;
    bool PlayerInsideThroneLadderBeam(float* beamT = nullptr, Vector3* axisPoint = nullptr, float* radialRatio = nullptr) const;
    void UpdateThroneJacobLadder(float dt);
    void TriggerEdenGatePlaceholder();
    void ApplyAntigravity(float duration);
    void SpawnSeraph();
    void SpawnEdenApocalypseSeraphs();
    void UpdateSeraph(float dt);
    void FireSeraphBurst(SeraphBoss& seraph);
    void UpdateSeraphFireballs(float dt);
    void ExplodeSeraphFireball(Vector3 position, int world, bool hitPlayer, bool makeFireLayer);
    void DamageSeraph(float damage, Vector3 hitPosition, Color color);
    void DamageSeraphInRadius(Vector3 position, float radius, float damage, Color color);
    void SpawnWarRider();
    void UpdateWarRider(float dt);
    void FireWarRiderSlash();
    void CommandWarRiderMinions();
    void EmpowerWarMinion(Enemy& enemy, Vector3 position);
    void DamageWarRider(float damage, Vector3 hitPosition, Color color);
    void DamageWarRiderInRadius(Vector3 position, float radius, float damage, Color color);
    void SpawnConquestRider();
    void UpdateConquestRider(float dt);
    void FireConquestRiderArrow();
    void SummonConquestRiderMinions();
    void UpdatePlagueArrows(float dt);
    void ExplodePlagueArrow(Vector3 position, int world, bool makePlagueCircle);
    void SpawnPlagueCircle(Vector3 position, int world, float radius, float duration);
    void DamageConquestRider(float damage, Vector3 hitPosition, Color color);
    void DamageConquestRiderInRadius(Vector3 position, float radius, float damage, Color color);
    void SpawnFamineRider();
    void UpdateFamineRider(float dt);
    void TriggerFamineWither();
    float FamineWitherRadius() const;
    void FeedFamineWitheredEssence(int count = 1);
    void DamageFamineRider(float damage, Vector3 hitPosition, Color color);
    void DamageFamineRiderInRadius(Vector3 position, float radius, float damage, Color color);
    void SpawnDeathRider();
    void UpdateDeathRider(float dt);
    void SpawnDeathSoul(Vector3 position, int world);
    void UpdateDeathSouls(float dt);
    void FireDeathSkull(Vector3 origin, float waitTimer = 0.0f, Vector3 initialDirection = {});
    void TriggerDeathSkullSwarm();
    void UpdateDeathSkulls(float dt);
    void DamageDeathSkullsInRadius(Vector3 position, float radius, float damage, Color color);
    void DamageDeathRider(float damage, Vector3 hitPosition, Color color);
    void DamageDeathRiderInRadius(Vector3 position, float radius, float damage, Color color);
    void ApplyPlayerPlague(Vector3 position);
    void UpdatePlayerPlague(float dt);
    void FireUfoOrb(Vector3 target);
    void FirePilotedUfoOrb();
    void UpdatePilotedUfoOrbLaser(float dt);
    void UpdatePilotedUfoTractor(float dt);
    void ExplodeUfoOrb(Vector3 position, int world, bool playerOwned = false, float damage = -1.0f);
    void FireSuperRainbowBeam(float chargeRatio);
    void SpawnWaterDroplet(Vector3 position, int world);
    void UpdateWaterDroplets(float dt);
    void FireEnemyShot(Vector3 position, Vector3 direction, int world = 0);
    void FireHomingShot(Vector3 position, Vector3 direction, float speed, float turnRate, float life, float damage, Color color, ProjectileOwner owner, int world = 0);
    void FireHeatwave(Vector3 direction);
    void FireNapalmGrenade();
    void UpdateNapalmGrenades(float dt);
    void DetonateNapalm(Vector3 position, int world, JPH::BodyID shooterBody = JPH::BodyID());
    void IgniteEnemy(Enemy& enemy);
    void FireDuelistHeatwave(Vector3 origin, Vector3 direction);
    void FireNanoBlade(Vector3 direction);
    void FireLonginusJudgmentStigma(Vector3 direction);
    void FireNanoPlatform(Vector3 direction);
    void FireCurseOrb(Vector3 direction);
    void FireSoulOrb(Vector3 position, float damage, Vector3 direction);
    void FireMagicLaserBeam(Vector3 position, Vector3 direction, int world = 0);
    void FireMagicProjectile(ProjectileKind kind, Vector3 position, Vector3 direction, float speed, float damage,
        float life, float radius, float maxRadius, Color color, float turnRate, int world = 0);
    float MagicCircleBaseCooldown(const MagicCircle& circle) const;
    bool MagicCircleCanActivate(ProjectileKind kind) const;
    Color MagicCircleTint(const MagicCircle& circle) const;
    Color MagicCircleTint(ProjectileKind kind) const;
    const char* MagicCircleKindName(const MagicCircle& circle) const;
    const char* MagicCircleKindName(ProjectileKind kind) const;
    void DeployMysticStaffShield();
    void BreakMysticStaffShield();
    void SpawnMysticStaffShockwave(Vector3 position);
    void CompleteMagicCircleChannel();
    void FireSpearThrust(Vector3 direction);
    void DetonateSpear(Vector3 position, ProjectileOwner owner);
    void SpawnEnemyNanoBlade(Vector3 origin, Vector3 direction, int world = 0);
    void SpawnGravityWell(Vector3 position, bool blackHole = false, bool enemyOrigin = false);
    void SpawnShockwave(Vector3 position, float radius, Color color);
    void ExplodeRocket(Vector3 position, ProjectileOwner owner = ProjectileOwner::Player, JPH::BodyID shooterBody = JPH::BodyID());
    void FireDroneCanister();
    void UpdateDrones(float dt);
    void AddEnemyImpulse(Enemy& enemy, Vector3 impulse);
    void AddProjectileImpulse(Projectile& projectile, Vector3 impulse);
    void SpawnHitBurst(Vector3 position, Color color, int count);
    void DestroyProjectile(size_t index);
    void DestroyEnemy(size_t index);
    void PlayEnemyHitSfx(Vector3 position);

    // GameEnemies.cpp
    void SpawnEnemy();
    void SpawnEnemyOfType(EnemyType type);
    bool BossAlive() const;
    bool DuelMode() const;
    bool TutorialMode() const;
    bool DuelWon() const;
    bool FaminePressureActive() const;
    void ShowTutorialTip(const char* text);
    void RecordDummyDamage(const Enemy& enemy, float damage);
    void UpdateDuelist(Enemy& enemy, Vector3 position, Vector3 direction, float dt, float& speed, bool& skipVelocity);
    void SwitchDuelistWeapon(Enemy& enemy, float distance);
    void FireDuelistWeapon(Enemy& enemy, Vector3 position, Vector3 toPlayer);
    void UpdateBethlehem(float dt);
    void SpawnBethlehem();
    void DestroyBethlehem();
    bool BethlehemAlive() const { return bethlehem_.active; }

    // GameWorld.cpp
    void UpdatePickups(float dt);
    void UpdateEssenceSpawn(float dt);
    void UpdateEdenEssenceField(float dt);
    int EdenRiverAt(Vector3 position) const;
    void UpdateEdenRiverBlessings(float dt);
    Vector3 EdenTreePosition(int index, bool* lifeTree = nullptr) const;
    Vector3 EdenFloatingStonePosition(int index) const;
    void ResetEdenArk();
    bool EdenArkEnterAvailable() const;
    void EnterEdenArk();
    void ExitEdenArk();
    void UpdateEdenArk(float dt);
    void UpdateEdenArkBody();
    bool ResolveEdenArkCollision(Vector3 previousPosition);
    bool ResolveEdenFloatingStoneCollision(Vector3 previousPosition);
    bool IsLabyrinthMap() const;
    void GenerateLabyrinth(unsigned int seed, std::vector<unsigned char>& out) const;
    void BuildLabyrinthProps();
    void UpdateLabyrinth(float dt);
    void ApplyPendingLabyrinth();
    bool LabyrinthCellOpen(int x, int y, const std::vector<unsigned char>* grid = nullptr) const;
    LabyrinthCell LabyrinthCellForPosition(Vector3 position) const;
    Vector3 LabyrinthCellCenter(int x, int y) const;
    LabyrinthCell LabyrinthFarthestCellFrom(int x, int y) const;
    LabyrinthCell LabyrinthNextStepToward(LabyrinthCell from, LabyrinthCell to) const;
    bool LabyrinthHasLineOfSight(LabyrinthCell a, LabyrinthCell b) const;
    void ResolveLabyrinthPlayerOverlap();
    void UpdateEdenFireRain(float dt);
    void SpawnEdenFireRain();
    void ExplodeEdenFireRain(Vector3 position, bool hitPlayer);
    void UpdateArenaBounds();
    void BuildMap();
    void ResolveMapCollision(Vector3 previousPosition);
    void SpawnStartingPickups();
    void SpawnPickup(PickupType type, int slot);
    bool ActivateWormhole(MagicCircle& circle, int circleIndex, Vector3 octaCenter);
    bool CloseWormhole(Vector3 position);
    bool CloseWormholeAlongSegment(Vector3 start, Vector3 end, float extraRadius = 0.0f);
    bool HasWormhole() const;
    Vector3 WormholeCenterForWorld(const WormholePortal& portal, int world) const;
    float FlatGroundYForWorld(int world) const;
    Vector3 FlatUpForWorld(int world) const;
    Vector3 UpForWorldAt(Vector3 position, int world) const;
    Vector3 MirrorPosition(Vector3 position, const WormholePortal& portal) const;
    Vector3 TeleportThroughWormhole(Vector3 position, int targetWorld, float altitude) const;
    Vector3 ReflectVelocityThroughWormhole(Vector3 velocity, Vector3 targetPosition, int targetWorld) const;
    bool IsEdenMap() const;
    float EdenCombatBoundaryRadius() const;
    float EdenHeightAt(float x, float z) const;
    float EdenGroundYAt(Vector3 position) const;
    Vector3 RandomEdenSpawnPoint() const;
    float EdenExitFade() const;
    void EnterEdenFromGate();
    void ExitEden(bool forceSurvivalMode = false);
    Vector3 EdenForbiddenFruitPosition() const;
    void ResetEdenForbiddenFruit();
    bool EdenForbiddenFruitInteractAvailable() const;
    void ClaimEdenForbiddenFruit();

    // GamePlayer.cpp
    void BlinkDuelist(Enemy& enemy, Vector3 awayFrom);
    void ToggleTimeStop();
    void FreezeDynamicObjects();
    void RestoreDynamicObjects();
    void Blink();
    void PerformGauntletSnap();
    void ApplyExplosionImpulse(Vector3 position, float radius, float impulse);
    void ApplyShotgunRecoil(Vector3 direction);
    void ApplySpearRecoil(Vector3 direction);
    void ApplyPlayerHit(Vector3 position, Color color, const char* eventText = nullptr);
    Vector3 PlayerForward() const;
    Vector3 PlayerRight() const;
    Vector3 PlayerUp() const;
    Vector3 WeaponMuzzlePosition() const;

    // GameWorld.cpp
    NanoPlatform MakeNanoPlatformTarget(Vector3 direction) const;
    Vector3 GetFireControlAimPoint() const;
    bool IsSphericalMap() const;
    bool IsHollowWorldMap() const;
    bool IsHollowPhysicsForWorld(int world) const;
    bool SphericalTouchesSurface(Vector3 position, float radius, int world) const;
    bool SphericalOutOfBounds(Vector3 position, float padding, int world) const;
    float SphericalRadius() const;
    float SphericalPlayerAltitude() const;
    float SphericalCleanupDistance() const;
    float SphericalAltitudeAt(Vector3 position, int world = -1) const;
    float SphericalSignedRadius(float altitude, int world = -1) const;
    Vector3 SphericalUpAt(Vector3 position, int world = -1) const;
    Vector3 SphericalSurfacePoint(Vector3 position, float altitude, int world = -1) const;
    Vector3 ProjectOnSphericalTangent(Vector3 vector, Vector3 up) const;
    float SphericalEnemyAltitude(EnemyType type) const;
    Vector3 BodyPosition(JPH::BodyID id) const;
    const char* WeaponName() const;
    const char* WeaponModeName() const;
    const char* WaveLabel() const;
    float CurrentGravity() const;
    float EdenPlayerGravityScale() const;
    bool IsSquareMap() const;
    bool EnemyTouchesPlayer(Vector3 enemyPosition, float enemyRadius) const;
    float DistancePointToSegment(Vector3 point, Vector3 start, Vector3 end) const;
    float DistanceXZ(Vector3 a, Vector3 b) const;

    // GameRender.cpp
    void DrawBethlehem() const;
    void DrawScavengerUfo() const;
    void DrawThroneAngel() const;
    void DrawCherubs() const;
    void DrawSeraph() const;
    void DrawWarRider() const;
    void DrawConquestRider() const;
    void DrawFamineRider() const;
    void DrawDeathRider() const;
    void DrawDeathSouls() const;
    void DrawDeathSkulls() const;
    void DrawSeraphFireballs() const;
    void DrawPlagueArrows() const;
    void DrawUfoHyperspace();
    void DrawEdenSkySphere() const;
    void DrawEdenForbiddenFruit() const;
    void DrawEdenArk() const;
    void DrawArena() const;
    void DrawProps() const;
    void DrawEnemies() const;
    void DrawPickups();
    void DrawProjectiles() const;
    void DrawBeams() const;
    void DrawBallLightnings() const;
    void DrawWaterDropletCrafts() const;
    void DrawWaterDroplets() const;
    void DrawShockwaves() const;
    void DrawHeatwaves() const;
    void DrawFirePatches() const;
    void DrawNapalmGrenades() const;
    void DrawGravityWells() const;
    void DrawNanoBlades() const;
    void DrawJudgmentStigmas() const;
    void DrawEdenFireSlashes() const;
    void DrawNanoPlatforms() const;
    void DrawNanoPlatformFrame(const NanoPlatform& platform, Color color, bool dashed) const;
    void DrawSlimeSpawnPods() const;
    void DrawMagicCircles() const;
    void DrawMysticStaffShield() const;
    void DrawConsole();
    void DrawDrones() const;
    void DrawRallyMarker() const;
    void DrawDashedCircle3D(Vector3 center, float radius, Vector3 normal, Color color) const;
    void DrawBlinkIndicator() const;
    void DrawFireControlOverlay() const;
    void DrawUfoCockpitOverlay() const;
    void DrawUfoPilotWeapon() const;
    void DrawParticles() const;
    void DrawWeapon() const;
    void DrawCrosshair() const;
    void DrawHud() const;

    PhysicsWorld physics_;
    GameplayConfig config_;
    GameplayConfig ufoArrivalBaseConfig_;
    bool ufoArrivalBaseConfigCaptured_ = false;
    WeaponViewModel weaponViewModel_;
    Camera3D camera_ = {};
    State state_ = State::Playing;

    JPH::RefConst<JPH::Shape> floorShape_;
    JPH::RefConst<JPH::Shape> projectileShape_;
    JPH::RefConst<JPH::Shape> enemyShape_;
    JPH::RefConst<JPH::Shape> edenArkShape_;
    JPH::BodyID floorBody_;

    std::vector<Projectile> projectiles_;
    std::vector<Enemy> enemies_;
    std::vector<Particle> particles_;
    std::vector<DamageNumber> damageNumbers_;
    std::vector<Beam> beams_;
    std::vector<BallLightning> ballLightnings_;
    std::vector<SeraphFireball> seraphFireballs_;
    std::vector<FirePatch> firePatches_;
    std::vector<NapalmGrenade> napalmGrenades_;
    std::vector<WaterDropletCraft> waterDropletCrafts_;
    // Weapon SFX
    Sound sfxLaserPlasma_ = {};
    Sound sfxLaserBeam_ = {};
    Sound sfxLaserBeamAlias_[4] = {};  // aliases for stretched rainbow beam
    int sfxLaserBeamAliasIdx_ = 0;
    Sound sfxLaserSuperCharge_ = {};
    Sound sfxLaserSuperChargeAlias_[4] = {};
    int sfxLaserSuperChargeAliasIdx_ = 0;
    Sound sfxFlamethrowerFireball_ = {};
    Sound sfxFlamethrowerNapalm_ = {};
    Sound sfxRocketLauncher_ = {};
    Sound sfxRocketDrone_ = {};
    Sound sfxShotgunPellet_ = {};
    Sound sfxShotgunGlass_ = {};
    Sound sfxGravityNailer_ = {};
    Sound sfxGravityBlackHole_ = {};
    Sound sfxGauntletTimeStop_ = {};
    Sound sfxGauntletTimeStopRelease_ = {};
    Sound sfxGauntletBlink_ = {};
    Sound sfxGauntletSnap_ = {};
    Sound sfxSpearThrow_ = {};
    Sound sfxSpearThrust_ = {};
    Sound sfxSpearJudgment_ = {};
    Sound sfxNanoCommand_ = {};
    Sound sfxNanoBlade_ = {};
    Sound sfxNanoPlatform_ = {};
    Sound sfxNanoWaterDroplet_ = {};
    Sound sfxMysticCurseOrb_ = {};
    Sound sfxMysticShield_ = {};
    Sound sfxMysticCircleChannel_ = {};
    Sound sfxMysticCircle_ = {};
    Sound sfxEssence_ = {};
    Sound sfxWeaponSwitch_ = {};
    Sound sfxWeaponModeSwitch_ = {};
    Sound sfxRocketExplosion_ = {};
    Sound sfxNapalmExplosion_ = {};
    Sound sfxGravityWellOpen_ = {};
    Sound sfxBlackHoleOpen_ = {};
    Sound sfxDroneDeploy_ = {};
    Sound sfxSpearImpact_ = {};
    Sound sfxBallLightningExplosion_ = {};
    Sound sfxBallLightningHum_ = {};
    Sound sfxWaterDropletBurst_ = {};
    Sound sfxEnemyHit_ = {};
    Sound sfxEnemyKill_ = {};
    Sound sfxPlayerHit_ = {};
    Sound sfxArmorHit_ = {};
    Sound sfxEssenceConsume_ = {};
    Sound sfxMagicCircleActivate_ = {};
    Sound sfxMagicCircleClear_ = {};
    Sound sfxWormholeOpen_ = {};
    Sound sfxWormholeTravel_ = {};
    Sound sfxWormholeClose_ = {};
    Sound sfxBossSpawn_ = {};
    Sound sfxBossPhase_ = {};
    Sound sfxBossDeath_ = {};
    Sound sfxThronePulse_ = {};
    Sound sfxBethlehemLaserWarn_ = {};
    Sound sfxBethlehemLaserFire_ = {};
    Sound sfxBossBarrage_ = {};
    Sound sfxSeraphFireBurst_ = {};
    Sound sfxWarRiderSpawn_ = {};
    Sound sfxWarRiderCommand_ = {};
    Sound sfxWarRiderSlash_ = {};
    Sound sfxSlimeSlam_ = {};
    Sound sfxUfoHyperspaceCharge_ = {};
    Sound sfxUfoTractor_ = {};
    Sound sfxArkFloodCurrent_ = {};
    Sound sfxArkFloodSurge_ = {};
    float sfxEnemyHitCooldown_ = 0.0f;
#ifndef VIONATURE_NO_AUDIO
    Music bgmMusic_ = {};
    bool bgmLoaded_ = false;
    float bgmLoopDelayTimer_ = 0.0f;
    Music heavenFallsBgmMusic_ = {};
    bool heavenFallsBgmLoaded_ = false;
    Music throneBgmMusic_ = {};
    bool throneBgmLoaded_ = false;
    Music seraphBgmMusic_ = {};
    bool seraphBgmLoaded_ = false;
    Music ufoBgmMusic_ = {};
    bool ufoBgmLoaded_ = false;
    Music ufoHyperspaceBgmMusic_ = {};
    bool ufoHyperspaceBgmLoaded_ = false;
    void UpdateBgm(float dt);
#else
    void UpdateBgm(float) {}
#endif
#ifdef VIONATURE_NO_AUDIO
    void PlaySfx(Sound&) {}
    void PlaySfxAt(Sound&, Vector3, float = 64.0f, float = 1.0f) {}
    void UpdateLoopingSfxAt(Sound&, Vector3, float = 64.0f, float = 1.0f) {}
    static void StopSfx(Sound&) {}
    static constexpr int kSfxAliasCount = 1;
    Sound sfxLaserPlasmaAlias_[1] = {};
    Sound sfxFlamethrowerFireballAlias_[1] = {};
    int sfxLaserPlasmaIdx_ = 0;
    int sfxFlamethrowerFireballIdx_ = 0;
    void PlaySfxLaserPlasma() {}
    void PlaySfxFlamethrowerFireball() {}
#else
    void PlaySfx(Sound& s) {
        if (s.frameCount <= 0) return;
        PlaySfxAt(s, camera_.position, 64.0f, 1.0f);
    }
    void PlaySfxAt(Sound& s, Vector3 position, float maxDistance = 64.0f, float volume = 1.0f);
    void UpdateLoopingSfxAt(Sound& s, Vector3 position, float maxDistance = 64.0f, float volume = 1.0f);
    static void StopSfx(Sound& s) { if (s.frameCount > 0 && IsSoundPlaying(s)) StopSound(s); }
    static constexpr int kSfxAliasCount = 8;
    Sound sfxLaserPlasmaAlias_[kSfxAliasCount] = {};
    Sound sfxFlamethrowerFireballAlias_[kSfxAliasCount] = {};
    int sfxLaserPlasmaIdx_ = 0;
    int sfxFlamethrowerFireballIdx_ = 0;
    void PlaySfxLaserPlasma() {
        if (sfxLaserPlasmaAlias_[0].frameCount > 0) {
            SetSoundVolume(sfxLaserPlasmaAlias_[sfxLaserPlasmaIdx_], std::clamp(config_.sfxVolume, 0.0f, 1.0f));
            SetSoundPan(sfxLaserPlasmaAlias_[sfxLaserPlasmaIdx_], 0.5f);
            PlaySound(sfxLaserPlasmaAlias_[sfxLaserPlasmaIdx_]);
            sfxLaserPlasmaIdx_ = (sfxLaserPlasmaIdx_ + 1) % kSfxAliasCount;
        }
    }
    void PlaySfxFlamethrowerFireball() {
        if (sfxFlamethrowerFireballAlias_[0].frameCount > 0) {
            SetSoundVolume(sfxFlamethrowerFireballAlias_[sfxFlamethrowerFireballIdx_], std::clamp(config_.sfxVolume, 0.0f, 1.0f));
            SetSoundPan(sfxFlamethrowerFireballAlias_[sfxFlamethrowerFireballIdx_], 0.5f);
            PlaySound(sfxFlamethrowerFireballAlias_[sfxFlamethrowerFireballIdx_]);
            sfxFlamethrowerFireballIdx_ = (sfxFlamethrowerFireballIdx_ + 1) % kSfxAliasCount;
        }
    }
#endif
    std::vector<WaterDroplet> waterDroplets_;
    std::vector<Shockwave> shockwaves_;
    std::vector<HeatwavePulse> heatwaves_;
    std::vector<GravityWell> gravityWells_;
    std::vector<NanoBlade> nanoBlades_;
    std::vector<JudgmentStigma> judgmentStigmas_;
    std::vector<EdenGuardian> edenGuardians_;
    std::vector<EdenFireSlash> edenFireSlashes_;
    std::vector<NanoPlatform> nanoPlatforms_;
    std::vector<SlimeSpawnPod> slimeSpawnPods_;
    std::vector<MagicCircle> magicCircles_;
    std::vector<WormholePortal> wormholes_;
    std::vector<Drone> drones_;
    std::vector<CherubMinion> cherubs_;
    std::vector<Prop> props_;
    std::vector<Pickup> pickups_;

    RenderTexture2D pixelTarget_ = {};
    int pixelWidth_ = 426;
    int pixelHeight_ = 240;

    float arenaRadius_ = 28.0f;
    float squareHalfExtent_ = 31.0f;
    struct EdenReturnState {
        bool valid = false;
        std::string gameMode;
        std::string mapType;
        Vector3 position = {};
        float yaw = -90.0f;
        float pitch = 0.0f;
        int essence = 0;
        float survivalTime = 0.0f;
    };
    EdenReturnState edenReturn_;
    float edenExitFade_ = 0.0f;
    float edenFallOutTimer_ = 0.0f;
    struct EdenForbiddenFruitState {
        bool active = false;
        bool claimed = false;
        Vector3 position = {};
        float spin = 0.0f;
        float apocalypse = 0.0f;
        int absorbedEssence = 0;
    };
    EdenForbiddenFruitState edenForbiddenFruit_;
    int edenRiverBlessing_ = -1;
    float edenRiverBlessingTimer_ = 0.0f;
    float edenRiverEssenceTimer_ = 0.0f;
    EdenArk edenArk_;
    std::vector<unsigned char> labyrinthGrid_;
    std::vector<unsigned char> labyrinthPendingGrid_;
    int labyrinthWidth_ = 0;
    int labyrinthHeight_ = 0;
    unsigned int labyrinthRuntimeSeed_ = 0;
    float labyrinthShiftTimer_ = 0.0f;
    bool labyrinthShiftWarning_ = false;
    bool labyrinthMinotaurSpawned_ = false;
    Vector3 asteroidReferenceForward_ = {0.0f, 0.0f, -1.0f};
    float playerRadius_ = 0.65f;
    float playerHeight_ = 2.0f;
    Vector3 playerVelocity_ = {};
    float yaw_ = -90.0f;
    float pitch_ = 0.0f;
    bool grounded_ = true;
    int playerWorld_ = 0;
    float coyoteTimer_ = 0.0f;
    float jumpBufferTimer_ = 0.0f;
    bool hasSpaceSuit_ = false;
    bool hasFlightRig_ = false;
    bool hasSkates_ = false;
    bool spaceSuitEnabled_ = false;
    bool flightRigEnabled_ = false;
    bool skatesEnabled_ = false;
    bool hideUI_ = false;
    bool consoleOpen_ = false;
    char consoleInput_[128] = {};
    float consoleBackspaceTimer_ = 0.0f;
    float consoleArrowTimer_ = 0.0f;
    bool wantsQuit_ = false;
    float exitHoldTimer_ = 0.0f;
    static constexpr float kExitHoldDuration = 1.8f;
    int consoleCursor_ = 0;
    std::vector<std::string> consoleHistory_;
    int consoleHistoryIdx_ = -1;
    std::vector<std::string> consoleCompletions_;
    int consoleCompletionIdx_ = 0;
    std::string consoleFeedback_;
    float consoleFeedbackTimer_ = 0.0f;
    float gravityScale_ = 1.0f;
    float flightTargetAltitude_ = 2.0f;
    float footstepBob_ = 0.0f;
    float thrustControlLockTimer_ = 0.0f;
    WeaponType activeWeapon_ = WeaponType::Laser;
    LaserMode laserMode_ = LaserMode::Plasma;
    FlamethrowerMode flamethrowerMode_ = FlamethrowerMode::FlameBall;
    RocketLauncherMode rocketLauncherMode_ = RocketLauncherMode::Rocket;
    ShotgunMode shotgunMode_ = ShotgunMode::Pellet;
    GravityNailerMode gravityNailerMode_ = GravityNailerMode::Nail;
    NanoConstructorMode nanoConstructorMode_ = NanoConstructorMode::NanoBlade;
    LonginusSpearMode longinusSpearMode_ = LonginusSpearMode::Throw;
    float nanoPlatformRangeScale_ = 1.0f;
    GauntletMode gauntletMode_ = GauntletMode::TimeStop;
    float blinkDistanceScale_ = 1.0f;
    MysticStaffMode mysticStaffMode_ = MysticStaffMode::CurseOrb;
    bool mysticStaffShieldActive_ = false;
    Vector3 mysticStaffShieldPosition_ = {};
    float mysticStaffShieldRadius_ = 2.8f;
    float mysticStaffShieldCooldown_ = 0.0f;
    bool mysticStaffChanneling_ = false;
    float mysticStaffChannelProgress_ = 0.0f;
    bool timeStopped_ = false;
    bool weaponTipShown_[9] = {};
    bool showKeybindOverlay_ = false;
    char tutorialTip_[256] = {};
    float tutorialTipTimer_ = 0.0f;
    float tutorialTipDuration_ = 0.0f;
    float tutorialHintTimer_ = 8.0f;
    int tutorialHintIndex_ = 0;
    float configReminderTimer_ = 90.0f;
    int configReminderIndex_ = 0;
    bool pickupTipShown_[4] = {};
    float timeStopTintTimer_ = 0.0f;
    float fireCooldown_ = 0.0f;
    float famineFireRateDebuffTimer_ = 0.0f;
    bool chargingLaser_ = false;
    float laserCharge_ = 0.0f;
    // Laser super mode (essence-powered)
    bool superCharging_ = false;
    bool superChargePaused_ = false;
    bool superCharged_ = false;
    int superEssenceConsumed_ = 0;
    float superEssenceTimer_ = 0.0f;
    bool gauntletSnapCharging_ = false;
    float gauntletSnapCharge_ = 0.0f;
    bool longinusJudgmentCharging_ = false;
    float longinusJudgmentCharge_ = 0.0f;
    bool suppressRightClickModeToggle_ = false;
    // Water droplet crafting
    bool waterDropletCrafting_ = false;
    float waterDropletCraftTimer_ = 0.0f;
    int waterDropletCraftTarget_ = -1;
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
    float jacobLadderPullFade_ = 0.0f;
    bool wispSurgeDone_ = false;
    bool spitterAmbushDone_ = false;
    bool pouncerRushDone_ = false;
    bool bossSpawned_ = false;
    bool slimeKingSpawned_ = false;
    bool bethlehemSpawned_ = false;
    bool throneAngelSpawned_ = false;
    bool seraphSpawned_ = false;
    bool warRiderSpawned_ = false;
    bool conquestRiderSpawned_ = false;
    bool famineRiderSpawned_ = false;
    bool deathRiderSpawned_ = false;
    BethlehemBoss bethlehem_;
    ScavengerUfoBoss scavengerUfo_;
    ThroneAngelBoss throneAngel_;
    WarRiderBoss warRider_;
    ConquestRiderBoss conquestRider_;
    FamineRiderBoss famineRider_;
    DeathRiderBoss deathRider_;
    std::vector<SeraphBoss> seraphs_;
    std::vector<PlagueArrow> plagueArrows_;
    std::vector<DeathSoul> deathSouls_;
    std::vector<DeathSkull> deathSkulls_;
    std::vector<SeraphFireball> edenFireRain_;
    float edenFireRainTimer_ = 0.0f;
    UfoPilotWeapon ufoPilotWeapon_ = UfoPilotWeapon::Orb;
    UfoOrbMode ufoOrbMode_ = UfoOrbMode::Projectile;
    UfoTravelState ufoTravelState_ = UfoTravelState::Inactive;
    std::vector<UfoHyperspaceObstacle> ufoHyperspaceObstacles_;
    float ufoHyperspaceHoldTimer_ = 0.0f;
    float ufoHyperspaceTimer_ = 0.0f;
    float ufoHyperspaceObstacleTimer_ = 0.0f;
    float ufoHyperspaceAngle_ = 0.0f;
    float ufoHyperspaceAltitude_ = 1.2f;
    float ufoHyperspaceFlash_ = 0.0f;
    float ufoEssenceTransferTimer_ = 0.0f;
    Model bethlehemModel_;
    bool bethlehemModelLoaded_ = false;
    Model scavengerUfoModel_;
    bool scavengerUfoModelLoaded_ = false;
    Model essenceModel_;
    bool essenceModelLoaded_ = false;
    Font cjkFont_ = {};
    bool cjkFontLoaded_ = false;
    bool duelWon_ = false;
    float nextMixedEventTime_ = 104.0f;
    int duelArmor_ = 0;
    float duelArmorInvulnTimer_ = 0.0f;
    float longinusSpearThrustInvulnTimer_ = 0.0f;
    float playerAntigravityTimer_ = 0.0f;
    float playerPlagueTimer_ = 0.0f;
    float playerPlagueTickTimer_ = 0.0f;
    float damageFlash_ = 0.0f;
    int essence_ = 0;
    float essenceInvulnTimer_ = 0.0f;
    float essenceSpawnTimer_ = 0.0f;
    float edenEssenceRespawnTimer_ = 0.0f;
    float survivalTime_ = 0.0f;
    float cameraShake_ = 0.0f;
    int score_ = 0;
    float totalDamageDealt_ = 0.0f;
};
