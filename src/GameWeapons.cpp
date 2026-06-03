#include "Game.h"
#include "GameMath.h"

#include <algorithm>
#include <cmath>

void Game::UpdateWeaponSwitching() {
    if (IsMouseButtonPressed(MOUSE_BUTTON_RIGHT)) {
        rightMouseHeld_ = 0.0f;
    }
    if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
        rightMouseHeld_ += GetFrameTime();
        if (activeWeapon_ == WeaponType::RocketLauncher && rightMouseHeld_ > 0.22f && state_ == State::Playing) {
            fireControlActive_ = true;
        }
    }
    if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT)) {
        fireControlActive_ = false;
    }

    WeaponType previousWeapon = activeWeapon_;
    float wheel = GetMouseWheelMove();
    bool wheelAdjustedPlatformRange = false;
    if (wheel != 0.0f && activeWeapon_ == WeaponType::NanoConstructor && nanoConstructorMode_ == NanoConstructorMode::NanoPlatform) {
        nanoPlatformRangeScale_ = std::clamp(nanoPlatformRangeScale_ + (wheel > 0.0f ? 0.08f : -0.08f), 0.35f, 1.85f);
        wheelAdjustedPlatformRange = true;
    }
    if (wheel != 0.0f && activeWeapon_ == WeaponType::InfinityGauntlet && gauntletMode_ == GauntletMode::Blink) {
        constexpr float kBlinkStep = 1.2f;
        blinkDistanceScale_ = std::clamp(blinkDistanceScale_ * (wheel > 0.0f ? kBlinkStep : 1.0f / kBlinkStep), config_.blinkDistanceMin, config_.blinkDistanceMax);
        wheelAdjustedPlatformRange = true;
    }
    if (IsKeyPressed(KEY_ONE)) {
        activeWeapon_ = WeaponType::Laser;
    } else if (IsKeyPressed(KEY_TWO)) {
        activeWeapon_ = WeaponType::Flamethrower;
    } else if (IsKeyPressed(KEY_THREE)) {
        activeWeapon_ = WeaponType::RocketLauncher;
    } else if (IsKeyPressed(KEY_FOUR)) {
        activeWeapon_ = WeaponType::Shotgun;
    } else if (IsKeyPressed(KEY_FIVE)) {
        activeWeapon_ = WeaponType::GravityNailer;
    } else if (IsKeyPressed(KEY_SIX)) {
        activeWeapon_ = WeaponType::InfinityGauntlet;
    } else if (IsKeyPressed(KEY_SEVEN)) {
        activeWeapon_ = WeaponType::LonginusSpear;
    } else if (IsKeyPressed(KEY_EIGHT)) {
        activeWeapon_ = WeaponType::NanoConstructor;
    } else if (IsKeyPressed(KEY_ZERO)) {
        activeWeapon_ = WeaponType::MysticStaff;
    } else if (wheel != 0.0f && !wheelAdjustedPlatformRange) {
        if (activeWeapon_ == WeaponType::MysticStaff) {
            activeWeapon_ = wheel > 0.0f ? WeaponType::NanoConstructor : WeaponType::Laser;
        } else {
            int index = 0;
            if (activeWeapon_ == WeaponType::Flamethrower) {
                index = 1;
            } else if (activeWeapon_ == WeaponType::RocketLauncher) {
                index = 2;
            } else if (activeWeapon_ == WeaponType::Shotgun) {
                index = 3;
            } else if (activeWeapon_ == WeaponType::GravityNailer) {
                index = 4;
            } else if (activeWeapon_ == WeaponType::InfinityGauntlet) {
                index = 5;
            } else if (activeWeapon_ == WeaponType::LonginusSpear) {
                index = 6;
            } else if (activeWeapon_ == WeaponType::NanoConstructor) {
                index = 7;
            }

            index = (index + (wheel > 0.0f ? -1 : 1) + 8) % 8;
            if (index == 0) {
                activeWeapon_ = WeaponType::Laser;
            } else if (index == 1) {
                activeWeapon_ = WeaponType::Flamethrower;
            } else if (index == 2) {
                activeWeapon_ = WeaponType::RocketLauncher;
            } else if (index == 3) {
                activeWeapon_ = WeaponType::Shotgun;
            } else if (index == 4) {
                activeWeapon_ = WeaponType::GravityNailer;
            } else if (index == 5) {
                activeWeapon_ = WeaponType::InfinityGauntlet;
            } else if (index == 6) {
                activeWeapon_ = WeaponType::LonginusSpear;
            } else {
                activeWeapon_ = WeaponType::NanoConstructor;
            }
        }
    }

    if (activeWeapon_ != WeaponType::Laser && IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && rightMouseHeld_ < 0.22f) {
        if (activeWeapon_ == WeaponType::Flamethrower) {
            flamethrowerMode_ = flamethrowerMode_ == FlamethrowerMode::FlameBall ? FlamethrowerMode::Heatwave : FlamethrowerMode::FlameBall;
            eventText_ = flamethrowerMode_ == FlamethrowerMode::Heatwave ? "HEATWAVE" : "FLAME BALL";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::Shotgun) {
            shotgunMode_ = shotgunMode_ == ShotgunMode::Pellet ? ShotgunMode::GlassShard : ShotgunMode::Pellet;
            eventText_ = shotgunMode_ == ShotgunMode::GlassShard ? "GLASS SHARDS" : "PELLETS";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::GravityNailer) {
            gravityNailerMode_ = gravityNailerMode_ == GravityNailerMode::Nail ? GravityNailerMode::BlackHole : GravityNailerMode::Nail;
            eventText_ = gravityNailerMode_ == GravityNailerMode::BlackHole ? "BLACK HOLE" : "GRAV NAIL";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::LonginusSpear) {
            longinusSpearMode_ = longinusSpearMode_ == LonginusSpearMode::Throw ? LonginusSpearMode::Thrust : LonginusSpearMode::Throw;
            eventText_ = longinusSpearMode_ == LonginusSpearMode::Thrust ? "AT THRUST" : "SPEAR THROW";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::RocketLauncher) {
            rocketLauncherMode_ = rocketLauncherMode_ == RocketLauncherMode::Rocket ? RocketLauncherMode::Drone : RocketLauncherMode::Rocket;
            eventText_ = rocketLauncherMode_ == RocketLauncherMode::Drone ? "DRONE" : "ROCKET";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::NanoConstructor) {
            nanoConstructorMode_ = nanoConstructorMode_ == NanoConstructorMode::NanoBlade ? NanoConstructorMode::NanoPlatform : NanoConstructorMode::NanoBlade;
            eventText_ = nanoConstructorMode_ == NanoConstructorMode::NanoPlatform ? "NANO PLATFORM" : "NANO BLADE";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::InfinityGauntlet) {
            gauntletMode_ = gauntletMode_ == GauntletMode::TimeStop ? GauntletMode::Blink : GauntletMode::TimeStop;
            eventText_ = gauntletMode_ == GauntletMode::Blink ? "BLINK" : "TIME STOP";
            eventTextTimer_ = 1.4f;
        } else if (activeWeapon_ == WeaponType::MysticStaff) {
            mysticStaffMode_ = static_cast<MysticStaffMode>((static_cast<int>(mysticStaffMode_) + 1) % 2);
            eventText_ = mysticStaffMode_ == MysticStaffMode::CurseOrb ? "CURSE ORB" : "MYSTIC SHIELD";
            eventTextTimer_ = 1.4f;
        }
    }

    if (activeWeapon_ != previousWeapon) {
        chargingLaser_ = false;
        laserCharge_ = 0.0f;
        fireCooldown_ = std::min(fireCooldown_, 0.12f);
        if (TutorialMode()) {
            int idx = static_cast<int>(activeWeapon_);
            if (idx < 9) {
                bool eng = config_.tutorialLanguage == "english";
                const char* tips[] = {
                    eng ? "Laser Rifle\nLMB rapid-fire plasma bolts  |  RMB+LMB charged piercing beam"
                        : "外星激光枪\n左键高速连射电浆球  |  右键+左键蓄力穿透光束",
                    eng ? "Flamethrower\nLMB expanding fireball  |  RMB heatwave cone (deflects projectiles)"
                        : "火焰喷射器\n左键火球(半径膨胀)  |  右键切换热浪冲击波(锥形推飞弹幕)",
                    eng ? "Rocket Launcher\nLMB rocket (rocket-jump)  |  RMB drone canister  |  Hold RMB for command overlay"
                        : "火箭筒\n左键火箭(火箭跳)  |  右键切换无人机仓  |  长按右键指挥界面",
                    eng ? "Shotgun\nLMB pellet spread (recoil dash)  |  RMB glass shard cloud (persistent damage)"
                        : "霰弹枪\n左键散射弹丸(后坐力位移)  |  右键切换玻璃碎片尘云(持续伤害)",
                    eng ? "Gravity Nailer\nLMB gravity well (pulls enemies)  |  RMB black hole grenade (event horizon)"
                        : "引力钉枪\n左键引力钉牵引力场  |  右键切换黑洞榴弹(事件视界秒杀)",
                    eng ? "Infinity Gauntlet\nRMB toggles: TimeStop(TS)/Blink(B)  |  Scroll wheel adjusts blink distance"
                        : "无限手套\n右键切换:时停(TS)/闪现(B)  |  闪现时滚轮调距离",
                    eng ? "Longinus Spear\nLMB thrown spear (piercing+recoil)  |  RMB AT thrust (cone+invuln frames)"
                        : "朗基努斯之枪\n左键投掷穿透+反冲位移  |  右键切换AT推进(锥形+无敌帧)",
                    eng ? "Nano Constructor\nLMB nano blade wave  |  RMB nano platform (standable)  |  Scroll adjusts range"
                        : "纳米构造仪\n左键纳米刀波(类帧伤)  |  右键切换纳米平台(可站立) 滚轮调距离",
                    eng ? "Mystic Staff\nLMB curse orb (homing+DoT)  |  RMB shield  |  Stand still+hold LMB+RMB to summon circle"
                        : "神秘法杖\n左键诅咒法球(追踪+DoT) | 右键切换护盾(S) | 站定时长按左右键召唤法阵",
                };
                ShowTutorialTip(tips[idx]);
                tutorialHintTimer_ = 0.3f;
            }
        }
    }
}
void Game::UpdateShooting(float dt) {
    fireCooldown_ = std::max(0.0f, fireCooldown_ - dt);

    if (fireControlActive_) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            rallyPoint_ = GetFireControlAimPoint();
            rallyPhase_ = RallyPhase::Assembling;
            rallyHoldTimer_ = config_.droneRallyHoldTime;
            fireControlActive_ = false;
            eventText_ = "RALLY SET";
            eventTextTimer_ = 1.4f;
        }
        return;
    }

    if (activeWeapon_ == WeaponType::InfinityGauntlet) {
        if (gauntletMode_ == GauntletMode::TimeStop) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && config_.timeStopEnabled) {
                ToggleTimeStop();
            }
        } else {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && config_.blinkEnabled) {
                Blink();
            }
        }
        return;
    }

    bool laserChord = activeWeapon_ == WeaponType::Laser && IsMouseButtonDown(MOUSE_BUTTON_RIGHT);
    if (laserChord && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        chargingLaser_ = true;
        laserCharge_ = 0.0f;
    }
    if (chargingLaser_) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && laserChord) {
            laserCharge_ = std::min(1.0f, laserCharge_ + dt * config_.laserChargeRate);
            cameraShake_ = std::min(1.0f, cameraShake_ + dt * 0.25f);
        } else {
            FireLaser(laserCharge_);
            chargingLaser_ = false;
            laserCharge_ = 0.0f;
            fireCooldown_ = config_.laserBeamCooldown;
        }
        return;
    }

    // Mystic Staff Magic Circle channel: hold both LMB+RMB while grounded & stationary to summon
    bool magicCircleChord = activeWeapon_ == WeaponType::MysticStaff
        && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)
        && IsMouseButtonDown(MOUSE_BUTTON_LEFT);
    bool playerStationary = Vector3Length(playerVelocity_) < 1.5f;
    if (magicCircleChord && !mysticStaffChanneling_ && grounded_ && playerStationary) {
        mysticStaffChanneling_ = true;
        mysticStaffChannelProgress_ = 0.0f;
        eventText_ = "CHANNELING...";
        eventTextTimer_ = 0.5f;
    }
    if (mysticStaffChanneling_) {
        if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && IsMouseButtonDown(MOUSE_BUTTON_RIGHT)
            && grounded_ && Vector3Length(playerVelocity_) < 1.5f) {
            mysticStaffChannelProgress_ = std::min(1.0f, mysticStaffChannelProgress_ + dt / 3.0f);
            cameraShake_ = std::min(0.35f, cameraShake_ + dt * 0.12f);
            // Spawn channeling particles around the player
            Vector3 up = IsSphericalMap() ? SphericalUpAt(camera_.position) : Vector3{0.0f, 1.0f, 0.0f};
            Vector3 right = PlayerRight();
            Vector3 fwd = PlayerForward();
            float ringR = 1.2f + mysticStaffChannelProgress_ * 0.6f;
            int burstCount = static_cast<int>(mysticStaffChannelProgress_ * 16.0f) + 3;
            for (int pi = 0; pi < burstCount; ++pi) {
                float angle = mysticStaffChannelProgress_ * 12.0f + pi * 0.55f;
                Vector3 offset = Vector3Add(Vector3Scale(right, std::cos(angle) * ringR), Vector3Scale(fwd, std::sin(angle) * ringR));
                Vector3 pos = Vector3Add(camera_.position, offset);
                pos = Vector3Add(pos, Vector3Scale(up, 0.3f + RandomFloat(-0.3f, 0.3f)));
                particles_.push_back(Particle{
                    pos,
                    Vector3Scale(up, RandomFloat(1.5f, 4.5f)),
                    Color{200, 160, 255, 220},
                    RandomFloat(0.25f, 0.65f), RandomFloat(0.25f, 0.65f),
                    RandomFloat(0.04f, 0.1f)
                });
            }
        } else {
            if (mysticStaffChannelProgress_ >= 1.0f) {
                CompleteMagicCircleChannel();
            } else {
                eventText_ = "CANCELLED";
                eventTextTimer_ = 1.0f;
            }
            mysticStaffChanneling_ = false;
            mysticStaffChannelProgress_ = 0.0f;
        }
        return;
    }

    if (laserChord || !IsMouseButtonDown(MOUSE_BUTTON_LEFT) || fireCooldown_ > 0.0f) {
        return;
    }

    Vector3 forward = PlayerForward();
    Vector3 right = PlayerRight();
    Vector3 up = PlayerUp();

    if (activeWeapon_ == WeaponType::Laser) {
        FireProjectile(ProjectileKind::LaserShot, forward, config_.plasmaSpeed, config_.plasmaDamage, config_.plasmaLifetime, config_.plasmaRadius, config_.plasmaRadius, Color{255, 240, 185, 255});
        fireCooldown_ = config_.plasmaCooldown;
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.15f);
    } else if (activeWeapon_ == WeaponType::Flamethrower) {
        if (flamethrowerMode_ == FlamethrowerMode::Heatwave) {
            FireHeatwave(forward);
            fireCooldown_ = 0.16f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.16f);
        } else {
            float side = RandomFloat(-0.05f, 0.05f);
            float lift = RandomFloat(-0.035f, 0.035f);
            Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
            FireProjectile(ProjectileKind::Flame, direction, RandomFloat(19.0f, 23.0f), config_.flameDamage, config_.flameLifetime, 0.12f, config_.flameMaxRadius, Color{255, 112, 28, 235});
            fireCooldown_ = 0.045f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.045f);
        }
    } else if (activeWeapon_ == WeaponType::RocketLauncher) {
        if (rocketLauncherMode_ == RocketLauncherMode::Drone) {
            FireDroneCanister();
            fireCooldown_ = 1.8f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.25f);
        } else {
            FireProjectile(ProjectileKind::Rocket, forward, 34.0f, config_.rocketImpactDamage, 2.8f, 0.34f, 0.34f, Color{230, 235, 210, 255});
            fireCooldown_ = 0.82f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.45f);
        }
    } else if (activeWeapon_ == WeaponType::Shotgun) {
        if (shotgunMode_ == ShotgunMode::GlassShard) {
            for (int i = 0; i < config_.shotgunShardCount; ++i) {
                float side = RandomFloat(-0.09f, 0.09f);
                float lift = RandomFloat(-0.055f, 0.055f);
                Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                FireProjectile(ProjectileKind::GlassShard, direction, config_.glassShardSpeed, config_.glassShardDamage, config_.glassShardLingerTime, 0.13f, 0.13f, Color{190, 245, 255, 255});
            }
            ApplyShotgunRecoil(Vector3Scale(forward, config_.glassShardRecoilScale));
            fireCooldown_ = 0.72f;
        } else {
            for (int i = 0; i < config_.shotgunPelletCount; ++i) {
                float side = RandomFloat(-0.18f, 0.18f);
                float lift = RandomFloat(-0.12f, 0.12f);
                Vector3 direction = Vector3Normalize(Vector3Add(forward, Vector3Add(Vector3Scale(right, side), Vector3Scale(up, lift))));
                FireProjectile(ProjectileKind::Pellet, direction, RandomFloat(48.0f, 58.0f), config_.shotgunPelletDamage, 0.62f, 0.11f, 0.11f, Color{255, 220, 150, 255});
            }
            ApplyShotgunRecoil(forward);
            fireCooldown_ = 0.58f;
        }
        cameraShake_ = std::min(1.0f, cameraShake_ + 0.42f);
    } else if (activeWeapon_ == WeaponType::GravityNailer) {
        if (gravityNailerMode_ == GravityNailerMode::BlackHole) {
            FireProjectile(ProjectileKind::BlackHoleGrenade, forward, 26.0f, config_.blackHoleGrenadeDamage, 1.65f, 0.28f, 0.28f, Color{90, 55, 165, 255});
            fireCooldown_ = 1.15f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.34f);
        } else {
            FireProjectile(ProjectileKind::GravityNail, forward, 76.0f, config_.gravityNailDamage, 1.1f, 0.15f, 0.15f, Color{165, 195, 255, 255});
            fireCooldown_ = 0.72f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.28f);
        }
    } else if (activeWeapon_ == WeaponType::LonginusSpear) {
        if (longinusSpearMode_ == LonginusSpearMode::Thrust) {
            FireSpearThrust(forward);
            fireCooldown_ = 0.68f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.58f);
        } else {
            FireProjectile(ProjectileKind::Lance, forward, config_.longinusSpearSpeed, config_.longinusSpearDamage, 1.15f, 0.28f, 0.28f, Color{255, 160, 50, 255});
            ApplySpearRecoil(forward);
            fireCooldown_ = 0.86f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.5f);
        }
    } else if (activeWeapon_ == WeaponType::NanoConstructor) {
        if (nanoConstructorMode_ == NanoConstructorMode::NanoPlatform) {
            FireNanoPlatform(forward);
            fireCooldown_ = 0.82f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.18f);
        } else {
            FireNanoBlade(forward);
            fireCooldown_ = 0.78f;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.38f);
        }
    } else if (activeWeapon_ == WeaponType::MysticStaff) {
        if (mysticStaffMode_ == MysticStaffMode::CurseOrb) {
            FireCurseOrb(forward);
            fireCooldown_ = config_.curseOrbCooldown;
            cameraShake_ = std::min(1.0f, cameraShake_ + 0.22f);
        } else if (mysticStaffMode_ == MysticStaffMode::Shield) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                DeployMysticStaffShield();
                fireCooldown_ = 0.25f;
            }
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT) && mysticStaffShieldActive_ && fireCooldown_ <= 0.0f) {
                for (const Pickup& p : pickups_) {
                    if (p.type == PickupType::Essence) {
                        SpawnShockwave(p.position, 60.0f, Color{255, 215, 60, 255});
                        SpawnShockwave(p.position, 48.5f, Color{255, 240, 160, 255});
                        SpawnHitBurst(p.position, Color{255, 235, 120, 255}, 16);
                    }
                }
                // Mini essence pulse on the staff muzzle
                Vector3 muzzle = WeaponMuzzlePosition();
                // SpawnShockwave(muzzle, 0.5f, Color{255, 215, 60, 255});
                SpawnHitBurst(muzzle, Color{255, 235, 120, 255}, 6);
                fireCooldown_ = 0.25f;
            }
        }
    }

    SpawnHitBurst(WeaponMuzzlePosition(), Color{255, 230, 180, 255}, 2);
}
void Game::FireProjectile(ProjectileKind kind, Vector3 direction, float speed, float damage, float life, float radius, float maxRadius, Color color) {
    Vector3 spawn = WeaponMuzzlePosition();
    Vector3 intendedVelocity = Vector3{direction.x * speed, direction.y * speed, direction.z * speed};

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = (IsSphericalMap() || playerWorld_ == 1)
        ? 0.0f
        : kind == ProjectileKind::Rocket ? 0.02f : kind == ProjectileKind::BlackHoleGrenade ? 0.45f : kind == ProjectileKind::DroneCanister ? 0.65f : 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(spawn),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, kind, life, life, damage, radius, maxRadius, color, 0, intendedVelocity, ProjectileOwner::Player, timeStopped_, {}, 0.0f, false, playerWorld_});
}
void Game::FireLaser(float charge) {
    float normalizedCharge = std::clamp(charge, 0.18f, 1.0f);
    Vector3 forward = PlayerForward();
    Vector3 start = WeaponMuzzlePosition();
    Vector3 end = Vector3Add(start, Vector3Scale(forward, config_.laserBeamRange));
    float damage = config_.laserBaseDamage + normalizedCharge * config_.laserChargeDamage;
    float beamRadius = config_.laserBeamRadius + normalizedCharge * config_.laserBeamRadiusChargeBonus;

    float beamLife = config_.laserBeamLifetime + normalizedCharge * config_.laserBeamLifetimeChargeBonus;
    beams_.push_back(Beam{
        start,
        end,
        beamLife,
        beamLife,
        beamRadius,
        normalizedCharge,
        Color{120, 220, 255, 255}
    });

    for (size_t i = 0; i < enemies_.size();) {
        Vector3 enemyPosition = BodyPosition(enemies_[i].body);
        float hitDistance = beamRadius + enemies_[i].radius * 0.85f;
        if (DistancePointToSegment(enemyPosition, start, end) <= hitDistance) {
            enemies_[i].health -= damage;
            totalDamageDealt_ += damage;
            SpawnHitBurst(enemyPosition, Color{150, 235, 255, 255}, 18 + static_cast<int>(normalizedCharge * 10.0f));
            if (enemies_[i].health <= 0.0f) {
                score_ += enemies_[i].scoreValue;
                SpawnHitBurst(enemyPosition, Color{255, 255, 255, 255}, 22);
                DestroyEnemy(i);
                continue;
            }
        }
        ++i;
    }

    for (MagicCircle& circle : magicCircles_) {
        if (circle.isWormhole) {
            continue;
        }
        Vector3 circleUp = UpForWorldAt(circle.position, 0);
        Vector3 octaCenter = Vector3Add(circle.position, Vector3Scale(circleUp, 1.0f + circle.radius * 0.4f));
        if (DistancePointToSegment(octaCenter, start, end) > circle.radius + beamRadius) {
            continue;
        }
        circle.activated = true;
        circle.activatedKind = ProjectileKind::LaserShot;
        circle.activatedByLaserBeam = true;
        circle.fireRateMult = config_.magicCircleFireRateMult;
        circle.homingTurnRate = config_.magicCircleHomingTurnRate;
        circle.fireCooldown = 0.0f;
        SpawnShockwave(octaCenter, circle.radius * 1.8f, MagicCircleTint(circle));
        SpawnHitBurst(octaCenter, MagicCircleTint(circle), 30);
        eventText_ = MagicCircleKindName(circle);
        eventTextTimer_ = 1.2f;
    }

    cameraShake_ = std::min(1.0f, cameraShake_ + 0.55f + normalizedCharge * 0.35f);
}
void Game::FireEnemyShot(Vector3 position, Vector3 direction, int world) {
    Vector3 intendedVelocity = Vector3Scale(direction, config_.enemyShotSpeed);
    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(position),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, ProjectileKind::EnemyShot, 3.0f, 3.0f, config_.enemyShotDamage, 0.2f, 0.2f, Color{105, 255, 220, 255}, 0, intendedVelocity, ProjectileOwner::Enemy, timeStopped_, {}, 0.0f, false, world});
}
void Game::FireHomingShot(Vector3 position, Vector3 direction, float speed, float turnRate, float life, float damage, Color color, ProjectileOwner owner, int world) {
    Vector3 intendedVelocity = Vector3Scale(direction, speed);
    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero() : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = 0.0f;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position), JPH::Quat::sIdentity(), projectileConfig);
    Projectile proj{body, ProjectileKind::HomingShot, life, life, damage, 0.22f, 0.22f, color, 0, intendedVelocity, owner, timeStopped_, {}, 0.0f, false, owner == ProjectileOwner::Player ? playerWorld_ : world};
    proj.turnRate = turnRate;
    projectiles_.push_back(proj);
}
void Game::FireCurseOrb(Vector3 direction) {
    Vector3 spawn = WeaponMuzzlePosition();
    float speed = config_.curseOrbSpeed;
    float turnRate = config_.curseOrbTurnRate;
    float life = config_.curseOrbLifetime;
    float damage = config_.curseOrbDirectDamage;
    Color color{180, 100, 255, 255};

    Vector3 intendedVelocity = Vector3Scale(direction, speed);
    PhysicsWorld::BodyConfig projConfig;
    projConfig.motionType = JPH::EMotionType::Dynamic;
    projConfig.layer = Layers::PROJECTILE;
    projConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero()
        : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projConfig.gravityFactor = 0.0f;
    projConfig.linearDamping = 0.0f;
    projConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(spawn),
        JPH::Quat::sIdentity(), projConfig);
    Projectile proj{body, ProjectileKind::CurseOrb, life, life, damage, 0.24f, 0.24f,
        color, 0, intendedVelocity, ProjectileOwner::Player, timeStopped_, {}, 0.0f, false, playerWorld_};
    proj.turnRate = turnRate;
    projectiles_.push_back(proj);
}
void Game::FireSoulOrb(Vector3 position, float damage, Vector3 direction) {
    float speed = config_.soulOrbSpeed;
    float turnRate = config_.soulOrbTurnRate;
    float life = config_.soulOrbLifetime;
    Color color{150, 50, 230, 255};

    Vector3 intendedVelocity = Vector3Scale(direction, speed);
    PhysicsWorld::BodyConfig projConfig;
    projConfig.motionType = JPH::EMotionType::Dynamic;
    projConfig.layer = Layers::PROJECTILE;
    projConfig.linearVelocity = timeStopped_ ? JPH::Vec3::sZero()
        : JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projConfig.gravityFactor = 0.0f;
    projConfig.linearDamping = 0.0f;
    projConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(projectileShape_, ToJoltVector(position),
        JPH::Quat::sIdentity(), projConfig);
    Projectile proj{body, ProjectileKind::SoulOrb, life, life, damage, 0.18f, 0.18f,
        color, 0, intendedVelocity, ProjectileOwner::Player, timeStopped_, {}, 0.0f, false, playerWorld_};
    proj.turnRate = turnRate;
    projectiles_.push_back(proj);
}
void Game::FireHeatwave(Vector3 direction) {
    Vector3 origin = WeaponMuzzlePosition();
    Vector3 forward = Vector3Normalize(direction);
    float range = config_.heatwaveRange;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = enemies_[i];
        Vector3 enemyPosition = BodyPosition(enemy.body);
        Vector3 offset = Vector3Subtract(enemyPosition, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range) {
            ++i;
            continue;
        }
        Vector3 toEnemy = Vector3Scale(offset, 1.0f / distance);
        float facing = Vector3DotProduct(forward, toEnemy);
        if (facing < 0.55f) {
            ++i;
            continue;
        }

        float falloff = 1.0f - distance / range;
        enemy.health -= config_.heatwaveDamage * (0.35f + falloff * 0.65f);
        totalDamageDealt_ += config_.heatwaveDamage * (0.35f + falloff * 0.65f);
        if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
            RecordDummyDamage(enemy, config_.heatwaveDamage * (0.35f + falloff * 0.65f));
        }
        float impulse = config_.heatwaveForce * (0.45f + falloff * 0.9f);
        AddEnemyImpulse(enemy, Vector3{toEnemy.x * impulse, 0.35f + 1.15f * falloff, toEnemy.z * impulse});
        SpawnHitBurst(enemyPosition, Color{255, 155, 75, 255}, 5);
        if (enemy.health <= 0.0f) {
            score_ += enemy.scoreValue;
            SpawnHitBurst(enemyPosition, Color{255, 235, 190, 255}, 18);
            DestroyEnemy(i);
            continue;
        }
        ++i;
    }

    for (Projectile& projectile : projectiles_) {
        if (projectile.kind != ProjectileKind::EnemyShot) {
            continue;
        }
        Vector3 position = BodyPosition(projectile.body);
        Vector3 offset = Vector3Subtract(position, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range) {
            continue;
        }
        Vector3 toProjectile = Vector3Scale(offset, 1.0f / distance);
        if (Vector3DotProduct(forward, toProjectile) < 0.45f) {
            continue;
        }
        float impulse = config_.heatwaveForce * (0.8f + (1.0f - distance / range) * 0.7f);
        AddProjectileImpulse(projectile, Vector3Scale(toProjectile, impulse));
    }

    heatwaves_.push_back(HeatwavePulse{
        origin,
        forward,
        0.18f,
        0.18f,
        range,
        0.95f,
        Color{255, 130, 65, 255}
    });
}
void Game::FireNanoBlade(Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 planeNormal = PlayerRight();
    if (Vector3Length(planeNormal) <= 0.001f) {
        planeNormal = Vector3{1.0f, 0.0f, 0.0f};
    }
    Vector3 up = PlayerUp();
    Vector3 center = Vector3Add(WeaponMuzzlePosition(), Vector3Scale(forward, config_.nanoBladeWaveSpawnDistance));
    nanoBlades_.push_back(NanoBlade{
        center,
        planeNormal,
        forward,
        up,
        Vector3Scale(forward, config_.nanoBladeWaveSpeed),
        config_.nanoBladeDelay,
        config_.nanoBladeLifetime,
        config_.nanoBladeLifetime,
        config_.nanoBladeRadius,
        config_.nanoBladeThickness,
        config_.nanoBladePlaneThickness,
        config_.nanoBladeDamage / config_.nanoBladeLifetime,
        ProjectileOwner::Player,
        playerWorld_
    });
    SpawnHitBurst(WeaponMuzzlePosition(), Color{255, 225, 140, 255}, 8);
    eventText_ = "NANO EDGE";
    eventTextTimer_ = 0.85f;
}
void Game::FireNanoPlatform(Vector3 direction) {
    NanoPlatform platform = MakeNanoPlatformTarget(direction);
    nanoPlatforms_.push_back(platform);
    Vector3 topCenter = IsSphericalMap()
        ? platform.position
        : Vector3{platform.position.x, platform.position.y + platform.scale.y, platform.position.z};
    SpawnHitBurst(topCenter, Color{255, 238, 160, 255}, 8);
    eventText_ = "NANO PLATFORM";
    eventTextTimer_ = 0.95f;
}
void Game::FireSpearThrust(Vector3 direction) {
    Vector3 forward = Vector3Normalize(direction);
    Vector3 origin = WeaponMuzzlePosition();
    float range = config_.longinusSpearThrustRange;
    float halfAngleCos = 0.54f;

    for (size_t i = 0; i < enemies_.size();) {
        Enemy& enemy = enemies_[i];
        Vector3 enemyPosition = BodyPosition(enemy.body);
        Vector3 offset = Vector3Subtract(enemyPosition, origin);
        float distance = Vector3Length(offset);
        if (distance <= 0.05f || distance > range + enemy.radius) {
            ++i;
            continue;
        }
        Vector3 toEnemy = Vector3Scale(offset, 1.0f / distance);
        float facing = Vector3DotProduct(forward, toEnemy);
        if (facing < halfAngleCos) {
            ++i;
            continue;
        }

        float falloff = 1.0f - std::clamp(distance / std::max(0.001f, range), 0.0f, 1.0f);
        enemy.health -= config_.longinusSpearThrustDamage * (0.35f + falloff * 0.65f);
        totalDamageDealt_ += config_.longinusSpearThrustDamage * (0.35f + falloff * 0.65f);
        if (enemy.type == EnemyType::Dummy || enemy.type == EnemyType::DummyBoss) {
            RecordDummyDamage(enemy, config_.longinusSpearThrustDamage * (0.35f + falloff * 0.65f));
        }
        float impulse = config_.longinusSpearThrustForce * (0.45f + falloff * 0.9f);
        AddEnemyImpulse(enemy, Vector3Scale(toEnemy, impulse));
        SpawnHitBurst(enemyPosition, Color{255, 150, 40, 255}, 8);
        if (enemy.health <= 0.0f) {
            score_ += enemy.scoreValue;
            SpawnHitBurst(enemyPosition, Color{255, 200, 80, 255}, 18);
            DestroyEnemy(i);
            continue;
        }
        ++i;
    }

    if (HasWormhole()) {
        const WormholePortal& portal = wormholes_.front();
        Vector3 gates[2] = {portal.frontPosition, portal.backPosition};
        float closeRadius = std::max(config_.wormholeTriggerRadius, config_.wormholeVisualRadius) + 0.75f;
        for (Vector3 gate : gates) {
            Vector3 toGate = Vector3Subtract(gate, origin);
            float along = Vector3DotProduct(toGate, forward);
            Vector3 closest = Vector3Add(origin, Vector3Scale(forward, std::clamp(along, 0.0f, range)));
            if (along >= 0.0f && along <= range && Vector3Distance(gate, closest) <= closeRadius) {
                CloseWormhole(gate);
                break;
            }
        }
    }

    Vector3 up = PlayerUp();
    Vector3 end = Vector3Add(origin, Vector3Scale(forward, range));
    beams_.push_back(Beam{
        origin,
        end,
        0.16f,
        0.16f,
        range * 0.72f,
        0.9f,
        Color{255, 150, 40, 255}
    });
    shockwaves_.push_back(Shockwave{Vector3Add(origin, Vector3Scale(forward, range * 0.55f)), 0.22f, 0.22f, range * 0.55f, Color{255, 150, 40, 255}});
    heatwaves_.push_back(HeatwavePulse{
        origin,
        forward,
        0.18f,
        0.18f,
        range,
        0.52f,
        Color{255, 180, 60, 255}
    });
    SpawnHitBurst(Vector3Add(origin, Vector3Scale(forward, 1.25f)), Color{255, 220, 140, 255}, 14);
    float impulse = config_.longinusSpearThrustImpulse;
    Vector3 thrust = Vector3Scale(forward, impulse);
    thrust = Vector3Add(thrust, Vector3Scale(up, std::max(0.0f, -Vector3DotProduct(forward, up)) * impulse * 0.28f));
    playerVelocity_ = Vector3Add(playerVelocity_, thrust);
    camera_.position = Vector3Add(camera_.position, Vector3Scale(forward, std::clamp(impulse * 0.055f, 0.35f, 1.25f)));
    if (IsSphericalMap()) {
        float surfaceRadius = SphericalSignedRadius(SphericalPlayerAltitude(), playerWorld_);
        bool pushedIntoSurface = IsHollowPhysicsForWorld(playerWorld_)
            ? Vector3Length(camera_.position) > surfaceRadius
            : Vector3Length(camera_.position) < surfaceRadius;
        if (pushedIntoSurface) {
            camera_.position = SphericalSurfacePoint(camera_.position, SphericalPlayerAltitude(), playerWorld_);
        }
        camera_.up = SphericalUpAt(camera_.position, playerWorld_);
    } else {
        camera_.position.y = std::max(playerHeight_, camera_.position.y);
    }
    camera_.target = Vector3Add(camera_.position, PlayerForward());
    longinusSpearThrustInvulnTimer_ = config_.longinusSpearThrustInvuln;
    thrustControlLockTimer_ = 0.18f;
    grounded_ = false;
    eventText_ = "AT THRUST";
    eventTextTimer_ = 0.85f;
}
void Game::FireDroneCanister() {
    if (static_cast<int>(drones_.size()) >= config_.droneMaxCount) {
        eventText_ = "DRONE MAX";
        eventTextTimer_ = 1.2f;
        return;
    }

    // Fires directly forward like the black hole grenade — gravity does the arc.
    Vector3 forward = PlayerForward();
    Vector3 spawn = WeaponMuzzlePosition();
    float speed = config_.droneCanisterSpeed;
    Vector3 intendedVelocity = Vector3{forward.x * speed, forward.y * speed, forward.z * speed};

    PhysicsWorld::BodyConfig projectileConfig;
    projectileConfig.motionType = JPH::EMotionType::Dynamic;
    projectileConfig.layer = Layers::PROJECTILE;
    projectileConfig.linearVelocity = JPH::Vec3(intendedVelocity.x, intendedVelocity.y, intendedVelocity.z);
    projectileConfig.gravityFactor = IsSphericalMap() ? 0.0f : config_.droneCanisterGravity;
    projectileConfig.linearDamping = 0.0f;
    projectileConfig.motionQuality = JPH::EMotionQuality::LinearCast;
    projectileConfig.allowSleeping = false;

    JPH::BodyID body = physics_.CreateBody(
        projectileShape_,
        ToJoltVector(spawn),
        JPH::Quat::sIdentity(),
        projectileConfig);

    projectiles_.push_back(Projectile{body, ProjectileKind::DroneCanister, 4.0f, 4.0f, 0.0f, 0.35f, 0.35f, Color{140, 155, 170, 255}, 0, intendedVelocity, ProjectileOwner::Player, false, {}, 0.0f, false, playerWorld_});
    SpawnHitBurst(spawn, Color{180, 190, 200, 255}, 6);
}
void Game::DeployMysticStaffShield() {
    if (mysticStaffShieldActive_ || mysticStaffShieldCooldown_ > 0.0f) return;
    mysticStaffShieldActive_ = true;
    mysticStaffShieldRadius_ = config_.mysticStaffShieldRadius;
    SpawnShockwave(camera_.position, mysticStaffShieldRadius_ * 0.7f, Color{160, 100, 255, 255});
    eventText_ = "SHIELD DEPLOYED";
    eventTextTimer_ = 1.2f;
}
void Game::BreakMysticStaffShield() {
    if (!mysticStaffShieldActive_) return;
    mysticStaffShieldActive_ = false;
    mysticStaffShieldCooldown_ = config_.mysticStaffShieldCooldown;
    SpawnMysticStaffShockwave(camera_.position);
    eventText_ = "SHIELD BROKEN";
    eventTextTimer_ = 1.5f;
}
void Game::SpawnMysticStaffShockwave(Vector3 position) {
    SpawnShockwave(position, config_.mysticStaffShockwaveRadius, Color{170, 110, 255, 255});
    for (Enemy& enemy : enemies_) {
        Vector3 ep = BodyPosition(enemy.body);
        float dist = Vector3Distance(position, ep);
        if (dist <= config_.mysticStaffShockwaveRadius + enemy.radius) {
            Vector3 pushDir = Vector3Subtract(ep, position);
            float pushDist = Vector3Length(pushDir);
            if (pushDist > 0.01f) {
                pushDir = Vector3Scale(pushDir, 1.0f / pushDist);
                float falloff = 1.0f - std::clamp(dist / config_.mysticStaffShockwaveRadius, 0.0f, 1.0f);
                AddEnemyImpulse(enemy, Vector3Scale(pushDir, config_.mysticStaffShockwaveForce * falloff));
            }
        }
    }
    for (Projectile& proj : projectiles_) {
        if (proj.owner != ProjectileOwner::Enemy && proj.kind != ProjectileKind::EnemyShot) continue;
        Vector3 pp = BodyPosition(proj.body);
        float dist = Vector3Distance(position, pp);
        if (dist <= config_.mysticStaffShockwaveRadius + proj.radius) {
            Vector3 pushDir = Vector3Subtract(pp, position);
            float pushDist = Vector3Length(pushDir);
            if (pushDist > 0.01f) {
                pushDir = Vector3Scale(pushDir, 1.0f / pushDist);
                float falloff = 1.0f - std::clamp(dist / config_.mysticStaffShockwaveRadius, 0.0f, 1.0f);
                AddProjectileImpulse(proj, Vector3Scale(pushDir, config_.mysticStaffShockwaveForce * falloff));
            }
        }
    }
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.55f);
}
void Game::CompleteMagicCircleChannel() {
    Vector3 forward = PlayerForward();
    if (IsSphericalMap()) {
        forward = ProjectOnSphericalTangent(forward, SphericalUpAt(camera_.position, playerWorld_));
    } else {
        forward.y = 0.0f;
    }
    if (Vector3Length(forward) > 0.001f) forward = Vector3Normalize(forward);
    Vector3 spawnPos = Vector3Add(camera_.position, Vector3Scale(forward, 2.0f));
    if (IsSphericalMap()) {
        spawnPos = SphericalSurfacePoint(spawnPos, SphericalAltitudeAt(camera_.position, playerWorld_), playerWorld_);
    } else {
        spawnPos.y = FlatGroundYForWorld(playerWorld_) + FlatUpForWorld(playerWorld_).y * 0.05f;
    }

    MagicCircle circle;
    circle.position = spawnPos;
    circle.life = config_.magicCircleLifetime;
    circle.maxLife = config_.magicCircleLifetime;
    circle.radius = config_.magicCircleRadius;
    circle.fireCooldown = 0.0f;
    circle.fireInterval = config_.magicCircleFireInterval;
    circle.world = playerWorld_;

    magicCircles_.push_back(circle);
    SpawnShockwave(spawnPos, circle.radius * 1.5f, Color{200, 150, 255, 255});
    SpawnHitBurst(spawnPos, Color{220, 180, 255, 255}, 30);
    eventText_ = "MAGIC CIRCLE";
    eventTextTimer_ = 2.0f;
    cameraShake_ = std::min(1.0f, cameraShake_ + 0.65f);
}
const char* Game::WeaponName() const {
    switch (activeWeapon_) {
        case WeaponType::Laser:
            return "LASER";
        case WeaponType::Flamethrower:
            return "FLAME";
        case WeaponType::RocketLauncher:
            return "ROCKET";
        case WeaponType::Shotgun:
            return "SHOTGUN";
        case WeaponType::GravityNailer:
            return "NAIL";
        case WeaponType::InfinityGauntlet:
            return "GAUNT";
        case WeaponType::LonginusSpear:
            return "SPEAR";
        case WeaponType::NanoConstructor:
            return "NANO";
        case WeaponType::MysticStaff:
            return "STAFF";
        default:
            return "UNKNOWN";
    }
}
const char* Game::WeaponModeName() const {
    if (activeWeapon_ == WeaponType::Flamethrower) {
        return flamethrowerMode_ == FlamethrowerMode::Heatwave ? "H" : "F";
    }
    if (activeWeapon_ == WeaponType::RocketLauncher) {
        return rocketLauncherMode_ == RocketLauncherMode::Drone ? "D" : "";
    }
    if (activeWeapon_ == WeaponType::Shotgun) {
        return shotgunMode_ == ShotgunMode::GlassShard ? "G" : "P";
    }
    if (activeWeapon_ == WeaponType::GravityNailer) {
        return gravityNailerMode_ == GravityNailerMode::BlackHole ? "BH" : "N";
    }
    if (activeWeapon_ == WeaponType::InfinityGauntlet) {
        if (timeStopped_) return "T";
        return gauntletMode_ == GauntletMode::Blink ? "B" : "TS";
    }
    if (activeWeapon_ == WeaponType::NanoConstructor) {
        return nanoConstructorMode_ == NanoConstructorMode::NanoPlatform ? "P" : "B";
    }
    if (activeWeapon_ == WeaponType::LonginusSpear) {
        return longinusSpearMode_ == LonginusSpearMode::Thrust ? "T" : "S";
    }
    if (activeWeapon_ == WeaponType::MysticStaff) {
        if (mysticStaffMode_ == MysticStaffMode::CurseOrb) return "C";
        if (mysticStaffMode_ == MysticStaffMode::Shield) {
            return mysticStaffShieldActive_ ? "SA" : (mysticStaffShieldCooldown_ > 0.0f ? "SC" : "S");
        }
        return "";
    }
    return "";
}
