#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <fstream>

#include <rlgl.h>

namespace {
// Codepoints needed for Chinese tutorial text + ASCII printable
static const int cjkCodepoints[] = {
    32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
    48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63,
    64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79,
    80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 94, 95,
    96, 97, 98, 99, 100, 101, 102, 103, 104, 105, 106, 107, 108, 109, 110, 111,
    112, 113, 114, 115, 116, 117, 118, 119, 120, 121, 122, 123, 124, 125, 126, 8212,
    19977, 19979, 19981, 20013, 20024, 20041, 20063, 20107, 20113, 20132, 20154, 20165, 20174, 20179, 20202, 20214,
    20260, 20301, 20302, 20379, 20391, 20445, 20572, 20809, 20837, 20840, 20851, 20914, 20915, 20923, 20934, 20987,
    20992, 20999, 21033, 21046, 21106, 21147, 21152, 21160, 21319, 21322, 21442, 21449, 21453, 21457, 21462, 21463,
    21487, 21488, 21491, 21512, 21516, 21518, 21943, 22120, 22270, 22320, 22330, 22352, 22806, 22826, 22871, 23384,
    23398, 23450, 23475, 23478, 23545, 23556, 23576, 24038, 24039, 24050, 24149, 24179, 24213, 24230, 24320, 24335,
    24341, 24377, 24418, 24452, 24471, 24615, 24748, 25112, 25163, 25216, 25226, 25237, 25307, 25321, 25342, 25345,
    25351, 25353, 25377, 25381, 25442, 25506, 25511, 25512, 25527, 25552, 25705, 25745, 25758, 25830, 25903, 25932,
    25945, 25955, 25968, 25972, 26007, 26080, 26102, 26143, 26381, 26426, 26432, 26463, 26495, 26497, 26500, 26538,
    26684, 27060, 27169, 27493, 27494, 27573, 27874, 27934, 27979, 28010, 28369, 28378, 28608, 28779, 28856, 28857,
    28909, 28976, 29190, 29255, 29301, 29609, 29615, 29616, 29627, 29699, 29827, 29983, 29992, 30001, 30028, 30340,
    30596, 30636, 30683, 30862, 31034, 31163, 31186, 31227, 31354, 31359, 31435, 31449, 31561, 31570, 31661, 31859,
    32034, 32435, 32447, 32469, 32493, 32511, 32534, 32622, 32626, 32773, 32960, 33050, 33080, 33192, 33258, 33267,
    33719, 33988, 34013, 34255, 34892, 35010, 35013, 35270, 35282, 35299, 35774, 35797, 35825, 35843, 36148, 36229,
    36291, 36305, 36317, 36339, 36718, 36753, 36807, 36827, 36828, 36830, 36861, 36873, 36879, 36880, 36895, 36896,
    37096, 37325, 37327, 38025, 38035, 38142, 38181, 38190, 38271, 38378, 38388, 38459, 38477, 38480, 38544, 38553,
    38598, 38647, 38704, 38738, 38754, 38899, 39030, 39069, 39118, 39134, 39640, 40060, 40657,
	    // 朗基努斯之枪 + 神秘法杖
	    20043, 21162, 22522, 26031, 26454, 26391, 27861, 31070, 31192,
	    24103,
	    27974, 30005,
};
static constexpr int cjkCodepointCount = sizeof(cjkCodepoints) / sizeof(cjkCodepoints[0]);

}


Game::Game() {
    config_ = LoadGameplayConfig();
    arenaRadius_ = config_.circleRadius;

    camera_.position = Vector3{0.0f, playerHeight_, 9.0f};
    camera_.target = Vector3{0.0f, playerHeight_, 0.0f};
    camera_.up = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy = 72.0f;
    camera_.projection = CAMERA_PERSPECTIVE;

    float floorHalfExtent = std::max(arenaRadius_, squareHalfExtent_);
    floorShape_ = new JPH::BoxShape(JPH::Vec3(floorHalfExtent, 0.5f, floorHalfExtent));
    projectileShape_ = new JPH::SphereShape(0.18f);
    enemyShape_ = new JPH::SphereShape(0.65f);
    pixelTarget_ = LoadRenderTexture(pixelWidth_, pixelHeight_);
    SetTextureFilter(pixelTarget_.texture, TEXTURE_FILTER_POINT);

    bethlehemModel_ = LoadModel("assets/models/bosses/star_of_bethlehem.obj");
    bethlehemModelLoaded_ = IsModelValid(bethlehemModel_);

    essenceModel_ = LoadModel("assets/models/pickups/stella_octangula_upright.obj");
    essenceModelLoaded_ = IsModelValid(essenceModel_);

    // Load CJK font for Chinese tutorial text
    const char* cjkFontPaths[] = {
        "assets/fonts/simhei.ttf",
        "C:/Windows/Fonts/simhei.ttf",
        "/usr/share/fonts/windows-fonts/simhei.ttf",
        "/usr/share/fonts/truetype/noto/NotoSansSC-VF.ttf",
    };
    for (const char* path : cjkFontPaths) {
        cjkFont_ = LoadFontEx(path, 48, const_cast<int*>(cjkCodepoints), cjkCodepointCount);
        if (cjkFont_.glyphCount > 0) {
            cjkFontLoaded_ = true;
            SetTextureFilter(cjkFont_.texture, TEXTURE_FILTER_POINT);
            break;
        }
    }

    Reset();
}
Game::~Game() {
    ClearWorld();
    if (pixelTarget_.id != 0) {
        UnloadRenderTexture(pixelTarget_);
    }
    if (bethlehemModelLoaded_) {
        UnloadModel(bethlehemModel_);
        bethlehemModelLoaded_ = false;
    }
    if (essenceModelLoaded_) {
        UnloadModel(essenceModel_);
        essenceModelLoaded_ = false;
    }
    if (cjkFontLoaded_) {
        UnloadFont(cjkFont_);
        cjkFontLoaded_ = false;
    }
}
void Game::Reset() {
    ClearWorld();

    camera_.position = IsSphericalMap()
        ? SphericalSurfacePoint(Vector3{0.0f, IsHollowWorldMap() ? -1.0f : 1.0f, 0.0f}, SphericalPlayerAltitude())
        : Vector3{0.0f, playerHeight_, 9.0f};
    yaw_ = -90.0f;
    pitch_ = 0.0f;
    playerVelocity_ = Vector3Zero();
    grounded_ = true;
    playerWorld_ = 0;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    hasSpaceSuit_ = false;
    hasFlightRig_ = false;
    hasSkates_ = false;
    spaceSuitEnabled_ = false;
    flightRigEnabled_ = false;
    skatesEnabled_ = false;
    hideUI_ = false;
    gravityScale_ = 1.0f;
    flightTargetAltitude_ = std::clamp(
        IsSphericalMap() ? SphericalAltitudeAt(camera_.position) : camera_.position.y,
        config_.flightMinAltitude,
        config_.flightMaxAltitude);
    footstepBob_ = 0.0f;
    thrustControlLockTimer_ = 0.0f;
    asteroidReferenceForward_ = Vector3{0.0f, 0.0f, -1.0f};
    camera_.up = UpForWorldAt(camera_.position, playerWorld_);
    camera_.target = Vector3Add(camera_.position, PlayerForward());

    PhysicsWorld::BodyConfig floorConfig;
    floorConfig.motionType = JPH::EMotionType::Static;
    floorConfig.layer = Layers::NON_MOVING;
    if (!IsSphericalMap()) {
        floorBody_ = physics_.CreateBody(
            floorShape_,
            JPH::RVec3(0.0f, -0.55f, 0.0f),
            JPH::Quat::sIdentity(),
            floorConfig,
            JPH::EActivation::DontActivate);
    } else {
        floorBody_ = JPH::BodyID();
    }

    state_ = State::Playing;
    activeWeapon_ = WeaponType::Laser;
    flamethrowerMode_ = FlamethrowerMode::FlameBall;
    rocketLauncherMode_ = RocketLauncherMode::Rocket;
    fireControlActive_ = false;
    rallyPhase_ = RallyPhase::Inactive;
    rallyPoint_ = {};
    rallyHoldTimer_ = 0.0f;
    drones_.clear();
    shotgunMode_ = ShotgunMode::Pellet;
    gravityNailerMode_ = GravityNailerMode::Nail;
    nanoConstructorMode_ = NanoConstructorMode::NanoBlade;
    longinusSpearMode_ = LonginusSpearMode::Throw;
    nanoPlatformRangeScale_ = 1.0f;
    gauntletMode_ = GauntletMode::TimeStop;
    blinkDistanceScale_ = 1.0f;
    mysticStaffMode_ = MysticStaffMode::CurseOrb;
    mysticStaffShieldActive_ = false;
    mysticStaffShieldCooldown_ = 0.0f;
    mysticStaffChanneling_ = false;
    mysticStaffChannelProgress_ = 0.0f;
    timeStopped_ = false;
    timeStopTintTimer_ = 0.0f;
    fireCooldown_ = 0.0f;
    chargingLaser_ = false;
    laserCharge_ = 0.0f;
    rightMouseHeld_ = 0.0f;
    spawnTimer_ = 0.6f;
    spawnInterval_ = 2.0f;
    waveIndex_ = 1;
    eventTextTimer_ = 2.0f;
    eventText_ = "WAVE 1";
    wispSurgeDone_ = false;
    spitterAmbushDone_ = false;
    pouncerRushDone_ = false;
    bossSpawned_ = false;
    slimeKingSpawned_ = false;
    bethlehemSpawned_ = false;
    for (int i = 0; i < 9; ++i) weaponTipShown_[i] = false;
    tutorialTip_[0] = '\0';
    tutorialTipTimer_ = 0.0f;
    tutorialTipDuration_ = 0.0f;
    tutorialHintTimer_ = 8.0f;
    tutorialHintIndex_ = 0;
    configReminderTimer_ = 90.0f;
    configReminderIndex_ = 0;
    for (int i = 0; i < 4; ++i) pickupTipShown_[i] = false;
    bethlehem_ = {};
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    duelWon_ = false;
    nextMixedEventTime_ = 104.0f;
    duelArmor_ = DuelMode() ? config_.duelPlayerArmor : 0;
    duelArmorInvulnTimer_ = 0.0f;
    essence_ = config_.startingEssence;
    essenceInvulnTimer_ = 0.0f;
    essenceSpawnTimer_ = 8.0f;  // first essence spawns quickly
    survivalTime_ = 0.0f;
    cameraShake_ = 0.0f;
    damageFlash_ = 0.0f;
    score_ = 0;
    totalDamageDealt_ = 0.0f;

    BuildMap();

    SpawnStartingPickups();
    if (TutorialMode()) {
        ShowTutorialTip("Welcome to tutorial!\nYou can change the game mode in gameplay.cfg.");
        tutorialTipTimer_ = 8.0f;
        tutorialTipDuration_ = 8.0f;
        tutorialHintTimer_ = 0.3f;
        eventText_ = "TUTORIAL | No enemies | Test all weapons";
        eventTextTimer_ = 5.0f;
    }
    if (DuelMode()) {
        int count = config_.duelistCount;
        for (int d = 0; d < count; ++d) {
            SpawnEnemyOfType(EnemyType::Duelist);
            // Assign AI tiers: first duelist uses smart AI if enabled, rest random
            if (!enemies_.empty()) {
                Enemy& duelist = enemies_.back();
                duelist.aiTier = (d == 0 && config_.duelistSmartAi) ? 1 : 0;
                duelist.equipmentTimer = RandomFloat(8.0f, 15.0f);
            }
        }
        eventText_ = count > 1 ? TextFormat("DUEL x%d", count) : "DUEL";
        eventTextTimer_ = 2.0f;
    }
}
void Game::ClearWorld() {
    for (const Projectile& projectile : projectiles_) {
        physics_.DestroyBody(projectile.body);
    }
    projectiles_.clear();

    beams_.clear();
    shockwaves_.clear();
    heatwaves_.clear();
    gravityWells_.clear();
    nanoBlades_.clear();
    for (NanoPlatform& platform : nanoPlatforms_) {
        physics_.DestroyBody(platform.platformBody);
    }
    nanoPlatforms_.clear();
    slimeSpawnPods_.clear();
    magicCircles_.clear();
    wormholes_.clear();

    for (const Enemy& enemy : enemies_) {
        physics_.DestroyBody(enemy.body);
    }
    enemies_.clear();

    physics_.DestroyBody(floorBody_);
    floorBody_ = JPH::BodyID();
    particles_.clear();
    damageNumbers_.clear();
    totalDamageDealt_ = 0.0f;
    props_.clear();
    pickups_.clear();
}
void Game::Update(float dt) {
    if (IsKeyPressed(KEY_R) && !consoleOpen_) {
        Reset();
    }

    if (IsKeyPressed(KEY_Z) && hasSpaceSuit_ && !consoleOpen_) {
        spaceSuitEnabled_ = !spaceSuitEnabled_;
        gravityScale_ = spaceSuitEnabled_ ? config_.spaceSuitGravityScale : 1.0f;
        eventText_ = spaceSuitEnabled_ ? "SUIT ON" : "SUIT OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_X) && hasFlightRig_ && !consoleOpen_) {
            flightRigEnabled_ = !flightRigEnabled_;
            if (flightRigEnabled_) {
                flightTargetAltitude_ = std::clamp(
                IsSphericalMap() ? SphericalAltitudeAt(camera_.position) : camera_.position.y,
                config_.flightMinAltitude,
                config_.flightMaxAltitude);
            }
        eventText_ = flightRigEnabled_ ? "FLIGHT ON" : "FLIGHT OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_C) && hasSkates_ && !consoleOpen_) {
        skatesEnabled_ = !skatesEnabled_;
        eventText_ = skatesEnabled_ ? "SKATES ON" : "SKATES OFF";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_P) && !consoleOpen_) {
        hideUI_ = !hideUI_;
        eventText_ = hideUI_ ? "HUD OFF" : "HUD ON";
        eventTextTimer_ = 1.0f;
    }
    if (IsKeyPressed(KEY_K) && !consoleOpen_) {
        showKeybindOverlay_ = !showKeybindOverlay_;
    }
    if (IsKeyPressed(KEY_GRAVE)) {
        if (!consoleOpen_) {
            consoleOpen_ = true;
            consoleInput_[0] = '\0';
            consoleCursor_ = 0;
            consoleCompletions_.clear();
            consoleFeedback_.clear();
        } else {
            consoleOpen_ = false;
        }
    }
    if (consoleOpen_) {
        UpdateConsole();
    }
    if (IsKeyPressed(KEY_F11)) {
        ToggleFullscreen();
    }

    thrustControlLockTimer_ = std::max(0.0f, thrustControlLockTimer_ - dt);
    UpdateLook(dt);

    if (state_ == State::Playing) {
        if (!timeStopped_) {
            survivalTime_ += dt;
        }
        spawnInterval_ = std::max(0.45f, 1.9f - survivalTime_ * 0.025f);

        // Snapshot boss health before damage processing
        float bossHealthBefore[16];
        JPH::BodyID bossBodies[16];
        int bossSnapCount = 0;
        for (const Enemy& enemy : enemies_) {
            if ((enemy.type == EnemyType::Boss || enemy.type == EnemyType::Duelist
                 || enemy.type == EnemyType::DummyBoss || enemy.type == EnemyType::SlimeKing)
                && bossSnapCount < 16) {
                bossBodies[bossSnapCount] = enemy.body;
                bossHealthBefore[bossSnapCount] = enemy.health;
                ++bossSnapCount;
            }
        }

        if (!consoleOpen_) {
            UpdatePlayer(dt);
            UpdateWeaponSwitching();
            UpdateShooting(dt);
        }
        UpdateBeam(dt);
        UpdateWormholes(dt);
        UpdateShockwaves(dt);
        UpdateHeatwaves(dt);
        if (!timeStopped_) {
            UpdateGravityWells(dt);
            UpdateNanoBlades(dt);
            UpdateNanoPlatforms(dt);
            UpdateMagicCircles(dt);
        }
        UpdateSlimeSpawnPods(dt);
        if (!timeStopped_ && !DuelMode() && !TutorialMode()) {
            UpdateWaveDirector(dt);
        }
        if (!timeStopped_) {
            UpdateEnemies(dt);
            UpdateDrones(dt);
            UpdateBethlehem(dt);
            UpdateProjectiles(dt);
            UpdateCollisions();

            // Update boss health bar priority after all damage is applied
            for (Enemy& enemy : enemies_) {
                if (enemy.type != EnemyType::Boss && enemy.type != EnemyType::Duelist
                    && enemy.type != EnemyType::DummyBoss && enemy.type != EnemyType::SlimeKing) continue;
                for (int si = 0; si < bossSnapCount; ++si) {
                    if (bossBodies[si] == enemy.body && enemy.health < bossHealthBefore[si]) {
                        enemy.lastDamageTime = survivalTime_;
                        break;
                    }
                }
            }
        }
        UpdatePickups(dt);
        UpdateEssenceSpawn(dt);
        UpdateArenaBounds();

        if (!timeStopped_) {
            physics_.Step(kFixedFrame);
        }

        // Mystic Staff shield update
        if (mysticStaffShieldActive_) {
            mysticStaffShieldPosition_ = camera_.position;
            if (!timeStopped_) {
                // Check enemy projectile hits on shield
                for (size_t pi = 0; pi < projectiles_.size();) {
                    Projectile& proj = projectiles_[pi];
                    if (proj.owner != ProjectileOwner::Enemy && proj.kind != ProjectileKind::EnemyShot) { ++pi; continue; }
                    Vector3 pp = BodyPosition(proj.body);
                    float dist = Vector3Distance(mysticStaffShieldPosition_, pp);
                    if (dist <= mysticStaffShieldRadius_ + proj.radius) {
                        BreakMysticStaffShield();
                        DestroyProjectile(pi);
                        break;
                    }
                    ++pi;
                }
                // Push enemies away from shield
                for (Enemy& enemy : enemies_) {
                    Vector3 ep = BodyPosition(enemy.body);
                    Vector3 toEnemy = Vector3Subtract(ep, mysticStaffShieldPosition_);
                    float dist = Vector3Length(toEnemy);
                    float pushDist = mysticStaffShieldRadius_ + enemy.radius;
                    if (dist < pushDist && dist > 0.01f) {
                        Vector3 pushDir = Vector3Scale(toEnemy, 1.0f / dist);
                        float penetration = pushDist - dist;
                        AddEnemyImpulse(enemy, Vector3Scale(pushDir, penetration * 18.0f));
                    }
                }
            }
        }
        mysticStaffShieldCooldown_ = std::max(0.0f, mysticStaffShieldCooldown_ - dt);
    } else {
        UpdateFreeCamera(dt);
        camera_.target = Vector3Add(camera_.position, PlayerForward());
    }

    UpdateParticles(dt);
    for (size_t i = 0; i < damageNumbers_.size();) {
        DamageNumber& dn = damageNumbers_[i];
        dn.life -= dt;
        dn.screenYOffset += dt * 35.0f;
        if (physics_.Bodies().IsAdded(dn.enemyBody)) {
            Vector3 pos = BodyPosition(dn.enemyBody);
            Vector3 up = IsSphericalMap() ? SphericalUpAt(pos) : Vector3{0.0f, 1.0f, 0.0f};
            dn.worldPosition = Vector3Add(pos, Vector3Scale(up, dn.heightOffset));
        }
        if (dn.life <= 0.0f) {
            damageNumbers_[i] = damageNumbers_.back();
            damageNumbers_.pop_back();
            continue;
        }
        ++i;
    }
    eventTextTimer_ = std::max(0.0f, eventTextTimer_ - dt);
    tutorialTipTimer_ = std::max(0.0f, tutorialTipTimer_ - dt);

    if (TutorialMode() && state_ == State::Playing) {
        // Periodic combat & movement tips (bottom center, cycles every 18s)
        if (tutorialTipTimer_ <= 0.0f) {
            tutorialHintTimer_ = std::max(0.0f, tutorialHintTimer_ - dt);
            if (tutorialHintTimer_ <= 0.0f) {
                const char* combatTips[] = {
                    "战斗技巧: 火箭跳\n对脚下发射火箭,利用爆炸冲量获得额外高度",
                    "战斗技巧: 空中位移链\n火箭跳->霰弹反冲->长枪反冲,三段超远位移",
                    "战斗技巧: 刀波钓鱼\n发射刀波后闪现换位,引诱追逐的敌人撞入刀波",
                    "战斗技巧: 时停连招\n时停->贴脸全部火箭->黑洞手雷->解冻瞬间爆发",
                    "战斗技巧: 无人机交叉火力\n不同位置部署无人机,长按右键设集合点",
                    "战斗技巧: 角动量保持\n球形地图切线速度不受重力,环绕加速",
                };
                const char* movementTips[] = {
                    "移动提示: Shift加速跑动\n空中也保持加速度(Quake风格空中控制)",
                    "移动提示: 太空服(蓝)拾取后按Z开关\n低重力0.24x,可高跳远跃",
                    "移动提示: 飞行装置(青)拾取后按X悬停\n空格升高 / Ctrl降低,悬停瞄准",
                    "移动提示: 滑板(绿)拾取后按C切换滑行\n极低摩擦,保持高动量滑动",
                };
                const char* welcomeTip = "教学模式 | P键隐藏HUD | 自由探索9把武器\n编辑 gameplay.cfg 自定义200+可调参数";
                const char* allTips[] = {
                    welcomeTip,
                    combatTips[0], movementTips[0], combatTips[1], movementTips[1],
                    combatTips[2], movementTips[2], combatTips[3], movementTips[3],
                    combatTips[4], combatTips[5],
                };
                constexpr int totalTips = sizeof(allTips) / sizeof(allTips[0]);
                ShowTutorialTip(allTips[tutorialHintIndex_ % totalTips]);
                tutorialHintIndex_ = (tutorialHintIndex_ + 1) % totalTips;
                tutorialHintTimer_ = 1.0f;
            }
        }

        // Config file editing reminders (top-left gold eventText, every 90s)
        configReminderTimer_ = std::max(0.0f, configReminderTimer_ - dt);
        if (configReminderTimer_ <= 0.0f && eventTextTimer_ <= 0.0f) {
            const char* configReminders[] = {
                "TIP: Edit gameplay.cfg to tune 200+ params",
                "TIP: game_mode = survival | duel | tutorial",
                "TIP: map_type = circle | square | asteroid | hollow_world",
            };
            eventText_ = configReminders[configReminderIndex_ % 3];
            eventTextTimer_ = 5.0f;
            configReminderIndex_ = (configReminderIndex_ + 1) % 3;
            configReminderTimer_ = 90.0f;
        }

        // Spawn training dummies
        spawnTimer_ -= dt;
        if (spawnTimer_ <= 0.0f) {
            int dummyCount = 0, dummyBossCount = 0;
            for (const Enemy& e : enemies_) {
                if (e.type == EnemyType::Dummy) ++dummyCount;
                if (e.type == EnemyType::DummyBoss) ++dummyBossCount;
            }
            if (dummyCount < config_.dummyMaxCount) {
                SpawnEnemyOfType(EnemyType::Dummy);
            }
            if (dummyBossCount < 1 && survivalTime_ >= config_.dummyBossSpawnTime) {
                SpawnEnemyOfType(EnemyType::DummyBoss);
            }
            spawnTimer_ = config_.dummySpawnInterval;
        }
        if (!bethlehemSpawned_ && survivalTime_ >= config_.dummyBethlehemSpawnTime) {
            SpawnBethlehem();
            bethlehemSpawned_ = true;
        }
    }
    timeStopTintTimer_ = std::max(0.0f, timeStopTintTimer_ - dt);
    duelArmorInvulnTimer_ = std::max(0.0f, duelArmorInvulnTimer_ - dt);
    longinusSpearThrustInvulnTimer_ = std::max(0.0f, longinusSpearThrustInvulnTimer_ - dt);
    essenceInvulnTimer_ = std::max(0.0f, essenceInvulnTimer_ - dt);
    damageFlash_ = std::max(0.0f, damageFlash_ - dt * 4.0f);
    cameraShake_ = std::max(0.0f, cameraShake_ - dt * 5.0f);
}
void Game::Draw() {
    BeginTextureMode(pixelTarget_);
    ClearBackground(Color{8, 8, 10, 255});

    BeginMode3D(camera_);
    DrawArena();
    DrawProps();
    DrawNanoPlatforms();
    DrawSlimeSpawnPods();
    DrawDrones();
    DrawEnemies();
    DrawBethlehem();
    DrawPickups();
    DrawProjectiles();
    DrawBeams();
    DrawShockwaves();
    DrawHeatwaves();
    DrawGravityWells();
    DrawNanoBlades();
    DrawMagicCircles();
    DrawMysticStaffShield();
    DrawParticles();
    DrawRallyMarker();
    DrawBlinkIndicator();
    if (!fireControlActive_ && !hideUI_) DrawWeapon();
    EndMode3D();

    // Second 3D pass: X-ray octahedron markers visible through obstacles
    if (fireControlActive_) {
        BeginMode3D(camera_);
        rlDisableDepthTest();
        Color markerColor = Color{80, 235, 150, 190};
        for (const Enemy& enemy : enemies_) {
            if (enemy.world != playerWorld_) continue;
            Vector3 pos = BodyPosition(enemy.body);
            float s = enemy.radius * 1.6f;
            Vector3 vx = {s, 0, 0}, nvx = {-s, 0, 0};
            Vector3 vy = {0, s, 0}, nvy = {0, -s, 0};
            Vector3 vz = {0, 0, s}, nvz = {0, 0, -s};
            // Octahedron: 12 edges, each vertex connects to all except its opposite
            auto p = [&](Vector3 v) { return Vector3Add(pos, v); };
            DrawLine3D(p(vx), p(vy), markerColor);
            DrawLine3D(p(vx), p(nvy), markerColor);
            DrawLine3D(p(vx), p(vz), markerColor);
            DrawLine3D(p(vx), p(nvz), markerColor);
            DrawLine3D(p(nvx), p(vy), markerColor);
            DrawLine3D(p(nvx), p(nvy), markerColor);
            DrawLine3D(p(nvx), p(vz), markerColor);
            DrawLine3D(p(nvx), p(nvz), markerColor);
            DrawLine3D(p(vy), p(vz), markerColor);
            DrawLine3D(p(vy), p(nvz), markerColor);
            DrawLine3D(p(nvy), p(vz), markerColor);
            DrawLine3D(p(nvy), p(nvz), markerColor);
        }
        rlEnableDepthTest();
        EndMode3D();

        DrawFireControlOverlay();
    } else {
        if (!hideUI_) DrawCrosshair();
        if (!hideUI_) DrawHud();
    }

    EndTextureMode();

    float screenWidth = static_cast<float>(GetScreenWidth());
    float screenHeight = static_cast<float>(GetScreenHeight());
    float scale = std::min(screenWidth / static_cast<float>(pixelWidth_), screenHeight / static_cast<float>(pixelHeight_));
    float targetWidth = static_cast<float>(pixelWidth_) * scale;
    float targetHeight = static_cast<float>(pixelHeight_) * scale;
    Rectangle source = Rectangle{0.0f, 0.0f, static_cast<float>(pixelWidth_), -static_cast<float>(pixelHeight_)};
    Rectangle dest = Rectangle{(screenWidth - targetWidth) * 0.5f, (screenHeight - targetHeight) * 0.5f, targetWidth, targetHeight};
    DrawTexturePro(pixelTarget_.texture, source, dest, Vector2Zero(), 0.0f, WHITE);
    if (timeStopped_ || timeStopTintTimer_ > 0.0f) {
        float alpha = timeStopped_ ? 0.16f : timeStopTintTimer_ * 0.28f;
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(Color{80, 70, 145, 255}, alpha));
    }
    DrawConsole();
}

// --- Console ---

std::vector<std::string> Game::GetConfigKeys() const {
    std::vector<std::string> keys;
    std::ifstream file("config/gameplay.cfg");
    if (!file.is_open()) return keys;
    std::string line;
    while (std::getline(file, line)) {
        auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        std::string key = line.substr(0, eq);
        // Trim trailing spaces
        while (!key.empty() && key.back() == ' ') key.pop_back();
        // Skip comments and section headers
        if (key.empty() || key[0] == '#') continue;
        keys.push_back(key);
    }
    return keys;
}

bool Game::SetConfigValue(const std::string& key, const std::string& value) {
    auto& c = config_;
    // Float map lookup (reuse the pattern from GameConfig.cpp)
    if (key == "game_mode") { c.gameMode = value; return true; }
    if (key == "map_type") { c.mapType = value; return true; }
    if (key == "invincible") { bool b; if (value == "true" || value == "1") { c.invincible = true; return true; } else if (value == "false" || value == "0") { c.invincible = false; return true; } return false; }
    if (key == "boss_rush_mode") { bool b; if (value == "true" || value == "1") { c.bossRushMode = true; return true; } else if (value == "false" || value == "0") { c.bossRushMode = false; return true; } return false; }
    if (key == "time_stop_enabled") { bool b; if (value == "true" || value == "1") { c.timeStopEnabled = true; return true; } else if (value == "false" || value == "0") { c.timeStopEnabled = false; return true; } return false; }
    if (key == "blink_enabled") { bool b; if (value == "true" || value == "1") { c.blinkEnabled = true; return true; } else if (value == "false" || value == "0") { c.blinkEnabled = false; return true; } return false; }
    if (key == "duelist_smart_ai") { bool b; if (value == "true" || value == "1") { c.duelistSmartAi = true; return true; } else if (value == "false" || value == "0") { c.duelistSmartAi = false; return true; } return false; }
    // Int fields
    if (key == "shotgun_pellet_count") { c.shotgunPelletCount = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "shotgun_shard_count") { c.shotgunShardCount = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "slime_king_shoot_count") { c.slimeKingShootCount = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "slime_king_split_count") { c.slimeKingSplitCount = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "slime_king_max_generations") { c.slimeKingMaxGenerations = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "drone_max_count") { c.droneMaxCount = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "duelist_count") { c.duelistCount = std::clamp(std::atoi(value.c_str()), 1, 10); return true; }
    if (key == "duel_player_armor") { c.duelPlayerArmor = std::max(0, std::atoi(value.c_str())); return true; }
    if (key == "starting_essence") { c.startingEssence = std::max(0, std::atoi(value.c_str())); return true; }
    if (key == "essence_max_on_map") { c.essenceMaxOnMap = std::clamp(std::atoi(value.c_str()), 1, 10); return true; }
    if (key == "dummy_max_count") { c.dummyMaxCount = std::max(0, std::atoi(value.c_str())); return true; }
    if (key == "boss_homing_burst_count") { c.bossHomingBurstCount = std::clamp(std::atoi(value.c_str()), 1, 6); return true; }

    // Generic float lookup (covers 200+ params)
    auto floatMap = c.FloatMap();
    auto it = floatMap.find(key);
    if (it != floatMap.end()) {
        float f = std::strtof(value.c_str(), nullptr);
        *it->second = std::max(0.0f, f);  // simple clamp, per-key clamps in LoadGameplayConfig
        return true;
    }

    return false;  // Unknown key
}

std::string Game::GetConfigValue(const std::string& key) const {
    auto& c = config_;
    // Bool
    if (key == "invincible") return c.invincible ? "true" : "false";
    if (key == "boss_rush_mode") return c.bossRushMode ? "true" : "false";
    if (key == "duelist_smart_ai") return c.duelistSmartAi ? "true" : "false";
    // Int
    if (key == "shotgun_pellet_count") return std::to_string(c.shotgunPelletCount);
    if (key == "duelist_count") return std::to_string(c.duelistCount);
    if (key == "starting_essence") return std::to_string(c.startingEssence);
    if (key == "duel_player_armor") return std::to_string(c.duelPlayerArmor);
    if (key == "drone_max_count") return std::to_string(c.droneMaxCount);
    if (key == "essence_max_on_map") return std::to_string(c.essenceMaxOnMap);
    // String
    if (key == "game_mode") return c.gameMode;
    if (key == "map_type") return c.mapType;
    // Generic float lookup (covers 200+ params)
    auto floatMap = const_cast<GameplayConfig&>(c).FloatMap();
    auto it = floatMap.find(key);
    if (it != floatMap.end()) {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.4g", *it->second);
        // Trim trailing zeros for cleaner display
        std::string s(buf);
        if (s.find('.') != std::string::npos) {
            while (s.back() == '0') s.pop_back();
            if (s.back() == '.') s.pop_back();
        }
        return s;
    }
    return "?";
}

void Game::UpdateConsole() {
    // Character input
    int key = GetCharPressed();
    while (key > 0) {
        if (key >= 32 && key <= 126 && key != 96 && key != 126 && consoleCursor_ < 126) {
            // Insert character at cursor
            int len = static_cast<int>(strlen(consoleInput_));
            for (int i = len; i >= consoleCursor_; --i)
                consoleInput_[i + 1] = consoleInput_[i];
            consoleInput_[consoleCursor_] = static_cast<char>(key);
            ++consoleCursor_;
            consoleCompletions_.clear();
        }
        key = GetCharPressed();
    }

    // Backspace: instant on first frame, then repeat every 0.05s while held
    if (IsKeyDown(KEY_BACKSPACE) && consoleCursor_ > 0) {
        consoleBackspaceTimer_ += GetFrameTime();
        float repeatRate = consoleBackspaceTimer_ < 0.4f ? 0.4f : 0.04f;
        if (IsKeyPressed(KEY_BACKSPACE) || consoleBackspaceTimer_ >= repeatRate) {
            consoleBackspaceTimer_ = consoleBackspaceTimer_ >= repeatRate ? consoleBackspaceTimer_ - repeatRate : 0.0f;
            int len = static_cast<int>(strlen(consoleInput_));
            for (int i = consoleCursor_ - 1; i < len; ++i)
                consoleInput_[i] = consoleInput_[i + 1];
            --consoleCursor_;
            consoleCompletions_.clear();
        }
    } else {
        consoleBackspaceTimer_ = 0.0f;
    }

    // Delete
    if (IsKeyPressed(KEY_DELETE)) {
        int len = static_cast<int>(strlen(consoleInput_));
        if (consoleCursor_ < len) {
            for (int i = consoleCursor_; i < len; ++i)
                consoleInput_[i] = consoleInput_[i + 1];
            consoleCompletions_.clear();
        }
    }

    // Left/Right
    if (IsKeyPressed(KEY_LEFT) && consoleCursor_ > 0) --consoleCursor_;
    if (IsKeyPressed(KEY_RIGHT) && consoleCursor_ < static_cast<int>(strlen(consoleInput_))) ++consoleCursor_;

    // Home/End
    if (IsKeyPressed(KEY_HOME)) consoleCursor_ = 0;
    if (IsKeyPressed(KEY_END)) consoleCursor_ = static_cast<int>(strlen(consoleInput_));

    // Tab completion
    if (IsKeyPressed(KEY_TAB)) {
        if (consoleCompletions_.empty()) {
            auto keys = GetConfigKeys();
            std::string prefix(consoleInput_, consoleCursor_);
            for (const auto& k : keys) {
                if (k.find(prefix) == 0) consoleCompletions_.push_back(k);
            }
            consoleCompletionIdx_ = 0;
        } else {
            consoleCompletionIdx_ = (consoleCompletionIdx_ + 1) % static_cast<int>(consoleCompletions_.size());
        }
        if (!consoleCompletions_.empty()) {
            const auto& sel = consoleCompletions_[consoleCompletionIdx_];
            strncpy(consoleInput_, sel.c_str(), 126);
            consoleInput_[sel.size()] = '\0';
            consoleCursor_ = static_cast<int>(sel.size());
        }
    }

    // Arrow history
    if (IsKeyPressed(KEY_UP) && !consoleHistory_.empty()) {
        if (consoleHistoryIdx_ == -1) consoleHistoryIdx_ = static_cast<int>(consoleHistory_.size()) - 1;
        else if (consoleHistoryIdx_ > 0) --consoleHistoryIdx_;
        strncpy(consoleInput_, consoleHistory_[consoleHistoryIdx_].c_str(), 126);
        consoleCursor_ = static_cast<int>(strlen(consoleInput_));
    }
    if (IsKeyPressed(KEY_DOWN) && !consoleHistory_.empty()) {
        if (consoleHistoryIdx_ != -1) {
            if (consoleHistoryIdx_ < static_cast<int>(consoleHistory_.size()) - 1) {
                ++consoleHistoryIdx_;
                strncpy(consoleInput_, consoleHistory_[consoleHistoryIdx_].c_str(), 126);
            } else {
                consoleHistoryIdx_ = -1;
                consoleInput_[0] = '\0';
            }
            consoleCursor_ = static_cast<int>(strlen(consoleInput_));
        }
    }

    // Enter = execute
    if (IsKeyPressed(KEY_ENTER) && consoleInput_[0] != '\0') {
        std::string cmd(consoleInput_);
        consoleHistory_.push_back(cmd);
        consoleHistoryIdx_ = -1;
        auto eq = cmd.find('=');
        if (eq != std::string::npos) {
            std::string k = cmd.substr(0, eq);
            std::string v = cmd.substr(eq + 1);
            // Trim spaces
            while (!k.empty() && k.back() == ' ') k.pop_back();
            while (!v.empty() && v.front() == ' ') v.erase(0, 1);
            if (SetConfigValue(k, v)) {
                consoleFeedback_ = "OK: " + k + " = " + v;
            } else {
                consoleFeedback_ = "ERR: unknown key '" + k + "'";
            }
        } else {
            consoleFeedback_ = "Usage: key = value";
        }
        consoleFeedbackTimer_ = 3.0f;
        consoleInput_[0] = '\0';
        consoleCursor_ = 0;
        consoleCompletions_.clear();
    }

    // Esc = close
    if (IsKeyPressed(KEY_ESCAPE)) {
        consoleOpen_ = false;
    }
}

void Game::DrawConsole() {
    if (!consoleOpen_) return;
    consoleFeedbackTimer_ = std::max(0.0f, consoleFeedbackTimer_ - GetFrameTime());
    int h = 18;
    int y = pixelHeight_ - h - 4;
    int screenW = pixelWidth_;

    // Background
    DrawRectangle(0, y - 4, screenW, h + 8 + (consoleCompletions_.size() > 1 ? static_cast<int>(consoleCompletions_.size()) * 8 : 0), Color{0, 0, 0, 210});

    // Completion suggestions
    if (consoleCompletions_.size() > 1) {
        for (size_t i = 0; i < consoleCompletions_.size(); ++i) {
            Color c = (static_cast<int>(i) == consoleCompletionIdx_) ? Color{255, 200, 60, 255} : Color{160, 160, 170, 255};
            DrawText(consoleCompletions_[i].c_str(), 8, y - 12 - static_cast<int>(i) * 8 - 4, 7, c);
        }
    }

    // Input line
    DrawText(">", 4, y, 8, Color{180, 220, 255, 255});
    DrawText(consoleInput_, 14, y, 8, Color{220, 235, 255, 255});

    // Show current value hint when a valid key is typed
    std::string input(consoleInput_, consoleCursor_);
    std::string val = GetConfigValue(input);
    if (val != "?") {
        DrawText(TextFormat("= %s", val.c_str()), 14 + MeasureText(consoleInput_, 8) + 4, y, 8, Color{120, 130, 150, 255});
    }

    // Cursor blink
    if (static_cast<int>(GetTime() * 2.0f) % 2 == 0) {
        int curX = 14 + MeasureText(TextFormat("%.*s", consoleCursor_, consoleInput_), 8);
        DrawLine(curX, y + 1, curX, y + h - 2, Color{255, 255, 255, 200});
    }

    // Feedback
    if (consoleFeedbackTimer_ > 0.0f) {
        Color fbColor = consoleFeedback_.find("OK:") == 0 ? Color{120, 255, 140, 255} : Color{255, 140, 120, 255};
        DrawText(consoleFeedback_.c_str(), 14, y - 12, 7, FadeColor(fbColor, std::min(1.0f, consoleFeedbackTimer_)));
    }
}
