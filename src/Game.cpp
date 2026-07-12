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
	    20037, 25509, 27704, 37197,
	    23454, 25103, 25171, 25913, 26377, 28216, 31995, 32479, 36755,
	    20462,
	    25152,
};
static constexpr int cjkCodepointCount = sizeof(cjkCodepoints) / sizeof(cjkCodepoints[0]);

}


#ifndef VIONATURE_NO_AUDIO
void Game::PlaySfxAt(Sound& s, Vector3 position, float maxDistance, float volume) {
    if (s.frameCount <= 0) {
        return;
    }
    Vector3 toSound = Vector3Subtract(position, camera_.position);
    float distance = Vector3Length(toSound);
    constexpr float kNearDistance = 2.0f;
    float t = std::clamp((distance - kNearDistance) / std::max(0.001f, maxDistance - kNearDistance), 0.0f, 1.0f);
    float attenuated = volume * std::clamp(config_.sfxVolume, 0.0f, 1.0f) * (1.0f - t);
    if (attenuated <= 0.01f) {
        return;
    }

    float pan = 0.5f;
    if (distance > 0.001f) {
        Vector3 direction = Vector3Scale(toSound, 1.0f / distance);
        pan = std::clamp(0.5f + Vector3DotProduct(direction, PlayerRight()) * 0.48f, 0.0f, 1.0f);
    }
    SetSoundVolume(s, attenuated);
    SetSoundPan(s, pan);
    PlaySound(s);
}
void Game::UpdateLoopingSfxAt(Sound& s, Vector3 position, float maxDistance, float volume) {
    if (s.frameCount <= 0) {
        return;
    }
    Vector3 toSound = Vector3Subtract(position, camera_.position);
    float distance = Vector3Length(toSound);
    constexpr float kNearDistance = 2.0f;
    float t = std::clamp((distance - kNearDistance) / std::max(0.001f, maxDistance - kNearDistance), 0.0f, 1.0f);
    float attenuated = volume * std::clamp(config_.sfxVolume, 0.0f, 1.0f) * (1.0f - t);
    float pan = 0.5f;
    if (distance > 0.001f) {
        Vector3 direction = Vector3Scale(toSound, 1.0f / distance);
        pan = std::clamp(0.5f + Vector3DotProduct(direction, PlayerRight()) * 0.48f, 0.0f, 1.0f);
    }
    SetSoundVolume(s, std::max(0.0f, attenuated));
    SetSoundPan(s, pan);
    if (attenuated > 0.01f && !IsSoundPlaying(s)) {
        PlaySound(s);
    } else if (attenuated <= 0.01f && IsSoundPlaying(s)) {
        StopSound(s);
    }
}

void Game::UpdateBgm(float dt) {
    bool edenApocalypse = IsEdenMap() && edenForbiddenFruit_.claimed;
    if (edenApocalypse) {
        if (ufoHyperspaceBgmLoaded_ && IsMusicStreamPlaying(ufoHyperspaceBgmMusic_)) StopMusicStream(ufoHyperspaceBgmMusic_);
        if (ufoBgmLoaded_ && IsMusicStreamPlaying(ufoBgmMusic_)) StopMusicStream(ufoBgmMusic_);
        if (throneBgmLoaded_ && IsMusicStreamPlaying(throneBgmMusic_)) StopMusicStream(throneBgmMusic_);
        if (seraphBgmLoaded_ && IsMusicStreamPlaying(seraphBgmMusic_)) StopMusicStream(seraphBgmMusic_);
        if (heavenFallsBgmLoaded_ && IsMusicStreamPlaying(heavenFallsBgmMusic_)) StopMusicStream(heavenFallsBgmMusic_);
        if (bgmLoaded_ && IsMusicStreamPlaying(bgmMusic_)) StopMusicStream(bgmMusic_);
        return;
    }

    bool ufoEncounter = ScavengerUfoEncounterActive();
    bool ufoHyperspaceBgmActive = ufoTravelState_ == UfoTravelState::Hyperspace
        || ufoTravelState_ == UfoTravelState::Arriving;
    bool ufoBgmActive = scavengerUfo_.state == ScavengerUfoState::Active
        || scavengerUfo_.state == ScavengerUfoState::Escaping
        || scavengerUfo_.state == ScavengerUfoState::DefeatedFalling;
    bool throneBgmActive = throneAngel_.active || throneAngel_.defeated;
    bool seraphBgmActive = false;
    for (const SeraphBoss& seraph : seraphs_) {
        if (seraph.active && !seraph.edenApocalypse) {
            seraphBgmActive = true;
            break;
        }
    }

    if (ufoHyperspaceBgmLoaded_) {
        if (ufoHyperspaceBgmActive && config_.ufoHyperspaceBgmVolume > 0.001f) {
            SetMusicVolume(ufoHyperspaceBgmMusic_, std::clamp(config_.ufoHyperspaceBgmVolume, 0.0f, 1.0f));
            if (!IsMusicStreamPlaying(ufoHyperspaceBgmMusic_)) {
                PlayMusicStream(ufoHyperspaceBgmMusic_);
            }
            UpdateMusicStream(ufoHyperspaceBgmMusic_);
        } else if (IsMusicStreamPlaying(ufoHyperspaceBgmMusic_)) {
            StopMusicStream(ufoHyperspaceBgmMusic_);
        }
    }

    if (ufoBgmLoaded_) {
        if (!ufoHyperspaceBgmActive && ufoBgmActive && config_.ufoBgmVolume > 0.001f) {
            SetMusicVolume(ufoBgmMusic_, std::clamp(config_.ufoBgmVolume, 0.0f, 1.0f));
            if (!IsMusicStreamPlaying(ufoBgmMusic_)) {
                PlayMusicStream(ufoBgmMusic_);
            }
            UpdateMusicStream(ufoBgmMusic_);
        } else if (IsMusicStreamPlaying(ufoBgmMusic_)) {
            StopMusicStream(ufoBgmMusic_);
        }
    }

    if (throneBgmLoaded_) {
        if (!ufoHyperspaceBgmActive && !ufoBgmActive && throneBgmActive && config_.throneBgmVolume > 0.001f) {
            SetMusicVolume(throneBgmMusic_, std::clamp(config_.throneBgmVolume, 0.0f, 1.0f));
            if (!IsMusicStreamPlaying(throneBgmMusic_)) {
                PlayMusicStream(throneBgmMusic_);
            }
            UpdateMusicStream(throneBgmMusic_);
        } else if (IsMusicStreamPlaying(throneBgmMusic_)) {
            StopMusicStream(throneBgmMusic_);
        }
    }

    if (seraphBgmLoaded_) {
        if (!ufoHyperspaceBgmActive && !ufoBgmActive && !throneBgmActive && seraphBgmActive && config_.seraphBgmVolume > 0.001f) {
            SetMusicVolume(seraphBgmMusic_, std::clamp(config_.seraphBgmVolume, 0.0f, 1.0f));
            if (!IsMusicStreamPlaying(seraphBgmMusic_)) {
                PlayMusicStream(seraphBgmMusic_);
            }
            UpdateMusicStream(seraphBgmMusic_);
        } else if (IsMusicStreamPlaying(seraphBgmMusic_)) {
            StopMusicStream(seraphBgmMusic_);
        }
    }

    bool specialBgmActive = ufoEncounter || ufoHyperspaceBgmActive || ufoBgmActive || throneBgmActive || seraphBgmActive;
    bool heavenFallsBgmActive = config_.heavenFalls && !IsEdenMap() && !specialBgmActive;
    if (heavenFallsBgmLoaded_) {
        if (heavenFallsBgmActive && config_.heavenFallsBgmVolume > 0.001f) {
            SetMusicVolume(heavenFallsBgmMusic_, std::clamp(config_.heavenFallsBgmVolume, 0.0f, 1.0f));
            if (!IsMusicStreamPlaying(heavenFallsBgmMusic_)) {
                PlayMusicStream(heavenFallsBgmMusic_);
            }
            UpdateMusicStream(heavenFallsBgmMusic_);
        } else if (IsMusicStreamPlaying(heavenFallsBgmMusic_)) {
            StopMusicStream(heavenFallsBgmMusic_);
        }
    }

    if (!bgmLoaded_) {
        return;
    }

    float surfaceAltitude = 0.0f;
    if (IsEdenMap()) {
        surfaceAltitude = camera_.position.y - EdenGroundYAt(camera_.position);
    } else if (IsSphericalMap()) {
        surfaceAltitude = SphericalAltitudeAt(camera_.position, playerWorld_);
    } else {
        Vector3 fromGround = Vector3Subtract(camera_.position, Vector3{0.0f, FlatGroundYForWorld(playerWorld_), 0.0f});
        surfaceAltitude = Vector3DotProduct(fromGround, FlatUpForWorld(playerWorld_));
    }
    surfaceAltitude = std::max(0.0f, surfaceAltitude);

    float altitudeFade = 1.0f;
    if (config_.bgmAltitudeFadeEnd > config_.bgmAltitudeFadeStart) {
        float t = std::clamp(
            (surfaceAltitude - config_.bgmAltitudeFadeStart) /
            (config_.bgmAltitudeFadeEnd - config_.bgmAltitudeFadeStart),
            0.0f,
            1.0f);
        t = t * t * (3.0f - 2.0f * t);
        altitudeFade = 1.0f + (config_.bgmAltitudeMinVolume - 1.0f) * t;
    }

    const float baseVolume = std::clamp(config_.bgmVolume, 0.0f, 1.0f);
    const float worldFade = (playerWorld_ == 1) ? config_.bgmBackWorldVolume : 1.0f;
    float volume = (specialBgmActive || heavenFallsBgmActive) ? 0.0f : baseVolume * altitudeFade * worldFade;
    SetMusicVolume(bgmMusic_, volume);
    if (baseVolume <= 0.001f || specialBgmActive || heavenFallsBgmActive) {
        if (IsMusicStreamPlaying(bgmMusic_)) {
            PauseMusicStream(bgmMusic_);
        }
        return;
    }

    if (IsMusicStreamPlaying(bgmMusic_)) {
        UpdateMusicStream(bgmMusic_);
        const float musicLength = GetMusicTimeLength(bgmMusic_);
        if (musicLength > 0.1f && GetMusicTimePlayed(bgmMusic_) >= musicLength - 0.05f) {
            StopMusicStream(bgmMusic_);
            bgmLoopDelayTimer_ = std::max(0.0f, config_.bgmLoopGap);
        }
        return;
    }

    if (bgmLoopDelayTimer_ > 0.0f) {
        bgmLoopDelayTimer_ = std::max(0.0f, bgmLoopDelayTimer_ - dt);
        return;
    }

    PlayMusicStream(bgmMusic_);
    UpdateMusicStream(bgmMusic_);
}
#endif

Game::Game() {
    config_ = LoadGameplayConfig();
    arenaRadius_ = config_.circleRadius;

#ifndef VIONATURE_NO_AUDIO
    InitAudioDevice();
    // Load all SFX (missing files → silent)
    auto loadSfx = [](Sound& s, const char* path) {
        if (FileExists(path)) s = LoadSound(path);
    };
    loadSfx(sfxLaserPlasma_,       "assets/SFX/laser_plasma.wav");
    if (sfxLaserPlasma_.frameCount > 0) {
        for (int i = 0; i < kSfxAliasCount; ++i) {
            sfxLaserPlasmaAlias_[i] = LoadSoundAlias(sfxLaserPlasma_);
        }
    }
    loadSfx(sfxLaserBeam_,         "assets/SFX/laser_beam.wav");
    if (sfxLaserBeam_.frameCount > 0) {
        for (int i = 0; i < 4; ++i) sfxLaserBeamAlias_[i] = LoadSoundAlias(sfxLaserBeam_);
    }
    loadSfx(sfxLaserSuperCharge_,  "assets/SFX/laser_super_charge.wav");
    if (sfxLaserSuperCharge_.frameCount > 0) {
        for (int i = 0; i < 4; ++i) sfxLaserSuperChargeAlias_[i] = LoadSoundAlias(sfxLaserSuperCharge_);
    }
    loadSfx(sfxFlamethrowerFireball_, "assets/SFX/flamethrower_fireball.wav");
    if (sfxFlamethrowerFireball_.frameCount > 0) {
        for (int i = 0; i < kSfxAliasCount; ++i) {
            sfxFlamethrowerFireballAlias_[i] = LoadSoundAlias(sfxFlamethrowerFireball_);
        }
    }
    loadSfx(sfxFlamethrowerNapalm_,   "assets/SFX/flamethrower_napalm.wav");
    loadSfx(sfxRocketLauncher_,    "assets/SFX/rocket_launcher.wav");
    loadSfx(sfxRocketDrone_,       "assets/SFX/rocket_drone.wav");
    loadSfx(sfxShotgunPellet_,     "assets/SFX/shotgun_pellet.wav");
    loadSfx(sfxShotgunGlass_,      "assets/SFX/shotgun_glass.wav");
    loadSfx(sfxGravityNailer_,     "assets/SFX/gravity_nailer.wav");
    loadSfx(sfxGravityBlackHole_,  "assets/SFX/gravity_black_hole.wav");
    loadSfx(sfxGauntletTimeStop_,  "assets/SFX/gauntlet_timestop.wav");
    loadSfx(sfxGauntletTimeStopRelease_, "assets/SFX/gauntlet_timestop_release.wav");
    loadSfx(sfxGauntletBlink_,     "assets/SFX/gauntlet_blink.wav");
    loadSfx(sfxGauntletSnap_,      "assets/SFX/gauntlet_snap.wav");
    loadSfx(sfxSpearThrow_,        "assets/SFX/spear_throw.wav");
    loadSfx(sfxSpearThrust_,       "assets/SFX/spear_thrust.wav");
    loadSfx(sfxSpearJudgment_,     "assets/SFX/spear_judgment.wav");
    loadSfx(sfxNanoCommand_,       "assets/SFX/nano_command.wav");
    loadSfx(sfxNanoBlade_,         "assets/SFX/nano_blade.wav");
    loadSfx(sfxNanoPlatform_,      "assets/SFX/nano_platform.wav");
    loadSfx(sfxNanoWaterDroplet_,  "assets/SFX/nano_water_droplet.wav");
    loadSfx(sfxMysticCurseOrb_,    "assets/SFX/mystic_curse_orb.wav");
    loadSfx(sfxMysticShield_,      "assets/SFX/mystic_shield.wav");
    loadSfx(sfxMysticCircleChannel_, "assets/SFX/mystic_circle_channel.wav");
    loadSfx(sfxMysticCircle_,      "assets/SFX/mystic_circle.wav");
    loadSfx(sfxEssence_,           "assets/SFX/essence.wav");
    loadSfx(sfxWeaponSwitch_,      "assets/SFX/weapon_switch.wav");
    loadSfx(sfxWeaponModeSwitch_,  "assets/SFX/fire_control_mode.wav");
    loadSfx(sfxRocketExplosion_,   "assets/SFX/rocket_explosion.wav");
    loadSfx(sfxNapalmExplosion_,   "assets/SFX/napalm_explosion.wav");
    loadSfx(sfxGravityWellOpen_,   "assets/SFX/gravity_well_open.wav");
    loadSfx(sfxBlackHoleOpen_,     "assets/SFX/black_hole_open.wav");
    loadSfx(sfxDroneDeploy_,       "assets/SFX/drone_deploy.wav");
    loadSfx(sfxSpearImpact_,       "assets/SFX/spear_impact.wav");
    loadSfx(sfxBallLightningExplosion_, "assets/SFX/ball_lightning_explosion.wav");
    loadSfx(sfxBallLightningHum_,  "assets/SFX/ball_lightning_hum.wav");
    loadSfx(sfxWaterDropletBurst_, "assets/SFX/water_droplet_burst.wav");
    loadSfx(sfxEnemyHit_,          "assets/SFX/enemy_hit.wav");
    loadSfx(sfxEnemyKill_,         "assets/SFX/enemy_kill.wav");
    loadSfx(sfxPlayerHit_,         "assets/SFX/player_hit.wav");
    loadSfx(sfxArmorHit_,          "assets/SFX/armor_hit.wav");
    loadSfx(sfxEssenceConsume_,    "assets/SFX/essence_consume.wav");
    loadSfx(sfxMagicCircleActivate_, "assets/SFX/magic_circle_activate.wav");
    loadSfx(sfxMagicCircleClear_,  "assets/SFX/magic_circle_clear.wav");
    loadSfx(sfxWormholeOpen_,      "assets/SFX/wormhole_open.wav");
    loadSfx(sfxWormholeTravel_,    "assets/SFX/wormhole_travel.wav");
    loadSfx(sfxWormholeClose_,     "assets/SFX/wormhole_close.wav");
    loadSfx(sfxBossSpawn_,         "assets/SFX/boss_spawn.wav");
    loadSfx(sfxBossPhase_,         "assets/SFX/boss_phase.wav");
    loadSfx(sfxBossDeath_,         "assets/SFX/boss_death.wav");
    loadSfx(sfxThronePulse_,       "assets/SFX/throne_pulse.wav");
    loadSfx(sfxBethlehemLaserWarn_, "assets/SFX/bethlehem_laser_warn.wav");
    loadSfx(sfxBethlehemLaserFire_, "assets/SFX/bethlehem_laser_fire.wav");
    loadSfx(sfxBossBarrage_,       "assets/SFX/boss_barrage.wav");
    loadSfx(sfxSeraphFireBurst_,   "assets/SFX/seraph_fire_burst.wav");
    loadSfx(sfxWarRiderSpawn_,     "assets/SFX/war_rider_spawn.wav");
    loadSfx(sfxWarRiderCommand_,   "assets/SFX/war_rider_command.wav");
    loadSfx(sfxWarRiderSlash_,     "assets/SFX/war_rider_slash.wav");
    loadSfx(sfxSlimeSlam_,         "assets/SFX/slime_slam.wav");
    loadSfx(sfxUfoHyperspaceCharge_, "assets/SFX/ufo_hyperspace_charge.wav");
    loadSfx(sfxUfoTractor_,        "assets/SFX/ufo_tractor.wav");
    loadSfx(sfxArkFloodCurrent_,    "assets/SFX/ark_flood_current.wav");
    loadSfx(sfxArkFloodSurge_,      "assets/SFX/ark_flood_surge.wav");
    if (!config_.bgmPath.empty() && FileExists(config_.bgmPath.c_str())) {
        bgmMusic_ = LoadMusicStream(config_.bgmPath.c_str());
        bgmMusic_.looping = false;
        bgmLoaded_ = bgmMusic_.ctxData != nullptr;
        if (bgmLoaded_) {
            SetMusicVolume(bgmMusic_, std::clamp(config_.bgmVolume, 0.0f, 1.0f));
        }
    }
    if (!config_.heavenFallsBgmPath.empty() && FileExists(config_.heavenFallsBgmPath.c_str())) {
        heavenFallsBgmMusic_ = LoadMusicStream(config_.heavenFallsBgmPath.c_str());
        heavenFallsBgmMusic_.looping = true;
        heavenFallsBgmLoaded_ = heavenFallsBgmMusic_.ctxData != nullptr;
        if (heavenFallsBgmLoaded_) {
            SetMusicVolume(heavenFallsBgmMusic_, std::clamp(config_.heavenFallsBgmVolume, 0.0f, 1.0f));
        }
    }
    if (!config_.throneBgmPath.empty() && FileExists(config_.throneBgmPath.c_str())) {
        throneBgmMusic_ = LoadMusicStream(config_.throneBgmPath.c_str());
        throneBgmMusic_.looping = true;
        throneBgmLoaded_ = throneBgmMusic_.ctxData != nullptr;
        if (throneBgmLoaded_) {
            SetMusicVolume(throneBgmMusic_, std::clamp(config_.throneBgmVolume, 0.0f, 1.0f));
        }
    }
    if (!config_.seraphBgmPath.empty() && FileExists(config_.seraphBgmPath.c_str())) {
        seraphBgmMusic_ = LoadMusicStream(config_.seraphBgmPath.c_str());
        seraphBgmMusic_.looping = true;
        seraphBgmLoaded_ = seraphBgmMusic_.ctxData != nullptr;
        if (seraphBgmLoaded_) {
            SetMusicVolume(seraphBgmMusic_, std::clamp(config_.seraphBgmVolume, 0.0f, 1.0f));
        }
    }
    if (!config_.ufoBgmPath.empty() && FileExists(config_.ufoBgmPath.c_str())) {
        ufoBgmMusic_ = LoadMusicStream(config_.ufoBgmPath.c_str());
        ufoBgmMusic_.looping = true;
        ufoBgmLoaded_ = ufoBgmMusic_.ctxData != nullptr;
        if (ufoBgmLoaded_) {
            SetMusicVolume(ufoBgmMusic_, std::clamp(config_.ufoBgmVolume, 0.0f, 1.0f));
        }
    }
    if (!config_.ufoHyperspaceBgmPath.empty() && FileExists(config_.ufoHyperspaceBgmPath.c_str())) {
        ufoHyperspaceBgmMusic_ = LoadMusicStream(config_.ufoHyperspaceBgmPath.c_str());
        ufoHyperspaceBgmMusic_.looping = true;
        ufoHyperspaceBgmLoaded_ = ufoHyperspaceBgmMusic_.ctxData != nullptr;
        if (ufoHyperspaceBgmLoaded_) {
            SetMusicVolume(ufoHyperspaceBgmMusic_, std::clamp(config_.ufoHyperspaceBgmVolume, 0.0f, 1.0f));
        }
    }
#endif // VIONATURE_NO_AUDIO

    camera_.position = Vector3{0.0f, playerHeight_, 9.0f};
    camera_.target = Vector3{0.0f, playerHeight_, 0.0f};
    camera_.up = Vector3{0.0f, 1.0f, 0.0f};
    camera_.fovy = 72.0f;
    camera_.projection = CAMERA_PERSPECTIVE;

    float floorHalfExtent = std::max(arenaRadius_, squareHalfExtent_);
    floorShape_ = new JPH::BoxShape(JPH::Vec3(floorHalfExtent, 0.5f, floorHalfExtent));
    projectileShape_ = new JPH::SphereShape(0.18f);
    enemyShape_ = new JPH::SphereShape(0.65f);
    edenArkShape_ = new JPH::BoxShape(JPH::Vec3(8.2f, 4.9f, 17.2f));
    pixelTarget_ = LoadRenderTexture(pixelWidth_, pixelHeight_);
    SetTextureFilter(pixelTarget_.texture, TEXTURE_FILTER_POINT);

    bethlehemModel_ = LoadModel("assets/models/bosses/star_of_bethlehem.obj");
    bethlehemModelLoaded_ = IsModelValid(bethlehemModel_);

    scavengerUfoModel_ = LoadModel("assets/models/bosses/scavenger_ufo.obj");
    scavengerUfoModelLoaded_ = IsModelValid(scavengerUfoModel_);

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
    if (scavengerUfoModelLoaded_) {
        UnloadModel(scavengerUfoModel_);
        scavengerUfoModelLoaded_ = false;
    }
    if (essenceModelLoaded_) {
        UnloadModel(essenceModel_);
        essenceModelLoaded_ = false;
    }
    if (cjkFontLoaded_) {
        UnloadFont(cjkFont_);
        cjkFontLoaded_ = false;
    }
#ifndef VIONATURE_NO_AUDIO
    if (bgmLoaded_) {
        StopMusicStream(bgmMusic_);
        UnloadMusicStream(bgmMusic_);
        bgmLoaded_ = false;
    }
    if (heavenFallsBgmLoaded_) {
        StopMusicStream(heavenFallsBgmMusic_);
        UnloadMusicStream(heavenFallsBgmMusic_);
        heavenFallsBgmLoaded_ = false;
    }
    if (throneBgmLoaded_) {
        StopMusicStream(throneBgmMusic_);
        UnloadMusicStream(throneBgmMusic_);
        throneBgmLoaded_ = false;
    }
    if (seraphBgmLoaded_) {
        StopMusicStream(seraphBgmMusic_);
        UnloadMusicStream(seraphBgmMusic_);
        seraphBgmLoaded_ = false;
    }
    if (ufoBgmLoaded_) {
        StopMusicStream(ufoBgmMusic_);
        UnloadMusicStream(ufoBgmMusic_);
        ufoBgmLoaded_ = false;
    }
    if (ufoHyperspaceBgmLoaded_) {
        StopMusicStream(ufoHyperspaceBgmMusic_);
        UnloadMusicStream(ufoHyperspaceBgmMusic_);
        ufoHyperspaceBgmLoaded_ = false;
    }
    auto unloadSfx = [](Sound& s) { if (s.frameCount > 0) UnloadSound(s); };
    auto unloadSfxAlias = [](Sound& s) { if (s.frameCount > 0) UnloadSoundAlias(s); };
    for (int i = 0; i < kSfxAliasCount; ++i) {
        unloadSfxAlias(sfxLaserPlasmaAlias_[i]);
        unloadSfxAlias(sfxFlamethrowerFireballAlias_[i]);
    }
    for (int i = 0; i < 4; ++i) { unloadSfxAlias(sfxLaserBeamAlias_[i]); unloadSfxAlias(sfxLaserSuperChargeAlias_[i]); }
    unloadSfx(sfxLaserPlasma_);
    unloadSfx(sfxLaserBeam_);
    unloadSfx(sfxLaserSuperCharge_);
    unloadSfx(sfxFlamethrowerFireball_);
    unloadSfx(sfxFlamethrowerNapalm_);
    unloadSfx(sfxRocketLauncher_);
    unloadSfx(sfxRocketDrone_);
    unloadSfx(sfxShotgunPellet_);
    unloadSfx(sfxShotgunGlass_);
    unloadSfx(sfxGravityNailer_);
    unloadSfx(sfxGravityBlackHole_);
    unloadSfx(sfxGauntletTimeStop_);
    unloadSfx(sfxGauntletTimeStopRelease_);
    unloadSfx(sfxGauntletBlink_);
    unloadSfx(sfxGauntletSnap_);
    unloadSfx(sfxSpearThrow_);
    unloadSfx(sfxSpearThrust_);
    unloadSfx(sfxSpearJudgment_);
    unloadSfx(sfxNanoCommand_);
    unloadSfx(sfxNanoBlade_);
    unloadSfx(sfxNanoPlatform_);
    unloadSfx(sfxNanoWaterDroplet_);
    unloadSfx(sfxMysticCurseOrb_);
    unloadSfx(sfxMysticShield_);
    unloadSfx(sfxMysticCircleChannel_);
    unloadSfx(sfxMysticCircle_);
    unloadSfx(sfxEssence_);
    unloadSfx(sfxWeaponSwitch_);
    unloadSfx(sfxWeaponModeSwitch_);
    unloadSfx(sfxRocketExplosion_);
    unloadSfx(sfxNapalmExplosion_);
    unloadSfx(sfxGravityWellOpen_);
    unloadSfx(sfxBlackHoleOpen_);
    unloadSfx(sfxDroneDeploy_);
    unloadSfx(sfxSpearImpact_);
    unloadSfx(sfxBallLightningExplosion_);
    unloadSfx(sfxBallLightningHum_);
    unloadSfx(sfxWaterDropletBurst_);
    unloadSfx(sfxEnemyHit_);
    unloadSfx(sfxEnemyKill_);
    unloadSfx(sfxPlayerHit_);
    unloadSfx(sfxArmorHit_);
    unloadSfx(sfxEssenceConsume_);
    unloadSfx(sfxMagicCircleActivate_);
    unloadSfx(sfxMagicCircleClear_);
    unloadSfx(sfxWormholeOpen_);
    unloadSfx(sfxWormholeTravel_);
    unloadSfx(sfxWormholeClose_);
    unloadSfx(sfxBossSpawn_);
    unloadSfx(sfxBossPhase_);
    unloadSfx(sfxBossDeath_);
    unloadSfx(sfxThronePulse_);
    unloadSfx(sfxBethlehemLaserWarn_);
    unloadSfx(sfxBethlehemLaserFire_);
    unloadSfx(sfxBossBarrage_);
    unloadSfx(sfxSeraphFireBurst_);
    unloadSfx(sfxWarRiderSpawn_);
    unloadSfx(sfxWarRiderCommand_);
    unloadSfx(sfxWarRiderSlash_);
    unloadSfx(sfxSlimeSlam_);
    unloadSfx(sfxUfoHyperspaceCharge_);
    unloadSfx(sfxUfoTractor_);
    unloadSfx(sfxArkFloodCurrent_);
    unloadSfx(sfxArkFloodSurge_);
    CloseAudioDevice();
#endif // VIONATURE_NO_AUDIO
}
void Game::Reset() {
    ClearWorld();
    ufoTravelState_ = UfoTravelState::Inactive;
    ufoHyperspaceObstacles_.clear();
    ufoHyperspaceHoldTimer_ = 0.0f;
    ufoHyperspaceTimer_ = 0.0f;
    ufoHyperspaceObstacleTimer_ = 0.0f;
    ufoHyperspaceAngle_ = 0.0f;
    ufoHyperspaceAltitude_ = config_.ufoHyperspaceMinAltitude;
    ufoHyperspaceFlash_ = 0.0f;
    ufoEssenceTransferTimer_ = 0.0f;

    camera_.position = IsEdenMap() ? RandomEdenSpawnPoint()
        : IsSphericalMap()
            ? SphericalSurfacePoint(Vector3{0.0f, IsHollowWorldMap() ? -1.0f : 1.0f, 0.0f}, SphericalPlayerAltitude())
            : Vector3{0.0f, playerHeight_, 9.0f};
    yaw_ = -90.0f;
    pitch_ = 0.0f;
    playerVelocity_ = Vector3Zero();
    grounded_ = true;
    playerWorld_ = 0;
    coyoteTimer_ = 0.0f;
    jumpBufferTimer_ = 0.0f;
    hasSpaceSuit_ = config_.invincible;
    hasFlightRig_ = config_.invincible;
    hasSkates_ = config_.invincible;
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
    if (!IsSphericalMap() && !IsEdenMap()) {
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
    scavengerUfo_ = {};
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
    famineFireRateDebuffTimer_ = 0.0f;
    chargingLaser_ = false;
    laserCharge_ = 0.0f;
    superCharging_ = false;
    superChargePaused_ = false;
    superCharged_ = false;
    superEssenceConsumed_ = 0;
    superEssenceTimer_ = 0.0f;
    gauntletSnapCharging_ = false;
    gauntletSnapCharge_ = 0.0f;
    longinusJudgmentCharging_ = false;
    longinusJudgmentCharge_ = 0.0f;
    suppressRightClickModeToggle_ = false;
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
    throneAngelSpawned_ = false;
    seraphSpawned_ = false;
    warRiderSpawned_ = false;
    conquestRiderSpawned_ = false;
    famineRiderSpawned_ = false;
    deathRiderSpawned_ = false;
    for (int i = 0; i < 9; ++i) weaponTipShown_[i] = false;
    tutorialTip_[0] = '\0';
    tutorialTipTimer_ = 0.0f;
    tutorialTipDuration_ = 0.0f;
    tutorialHintTimer_ = 8.0f;
    tutorialHintIndex_ = 0;
    configReminderTimer_ = 90.0f;
    configReminderIndex_ = 0;
    for (int i = 0; i < 4; ++i) pickupTipShown_[i] = false;
    StopSfx(sfxBethlehemLaserFire_);
    StopSfx(sfxUfoTractor_);
    StopSfx(sfxLaserBeam_);
    StopSfx(sfxArkFloodCurrent_);
    StopSfx(sfxArkFloodSurge_);
    bethlehem_ = {};
    bethlehem_.laserPhase = BethlehemLaserPhase::Inactive;
    throneAngel_ = {};
    warRider_ = {};
    conquestRider_ = {};
    famineRider_ = {};
    deathRider_ = {};
    cherubs_.clear();
    seraphs_.clear();
    seraphFireballs_.clear();
    plagueArrows_.clear();
    deathSouls_.clear();
    deathSkulls_.clear();
    labyrinthGrid_.clear();
    labyrinthPendingGrid_.clear();
    labyrinthWidth_ = 0;
    labyrinthHeight_ = 0;
    labyrinthRuntimeSeed_ = 0;
    labyrinthShiftTimer_ = 0.0f;
    labyrinthShiftWarning_ = false;
    labyrinthMinotaurSpawned_ = false;
    edenFireRain_.clear();
    edenFireRainTimer_ = 0.0f;
    edenGuardians_.clear();
    edenFireSlashes_.clear();
    duelWon_ = false;
    nextMixedEventTime_ = 104.0f;
    duelArmor_ = DuelMode() ? config_.duelPlayerArmor : 0;
    duelArmorInvulnTimer_ = 0.0f;
    essence_ = config_.startingEssence;
    essenceInvulnTimer_ = 0.0f;
    playerAntigravityTimer_ = 0.0f;
    playerPlagueTimer_ = 0.0f;
    playerPlagueTickTimer_ = 0.0f;
    famineFireRateDebuffTimer_ = 0.0f;
    essenceSpawnTimer_ = 8.0f;  // first essence spawns quickly
    edenEssenceRespawnTimer_ = 0.0f;
    edenRiverBlessing_ = -1;
    edenRiverBlessingTimer_ = 0.0f;
    edenRiverEssenceTimer_ = 0.0f;
    survivalTime_ = 0.0f;
    cameraShake_ = 0.0f;
    damageFlash_ = 0.0f;
    jacobLadderPullFade_ = 0.0f;
    edenExitFade_ = 0.0f;
    edenFallOutTimer_ = 0.0f;
    edenForbiddenFruit_ = {};
    physics_.DestroyBody(edenArk_.body);
    edenArk_ = {};
    score_ = 0;
    totalDamageDealt_ = 0.0f;

    BuildMap();
    if (IsLabyrinthMap()) {
        camera_.position = LabyrinthCellCenter(1, 1);
        camera_.target = Vector3Add(camera_.position, PlayerForward());
        playerVelocity_ = Vector3Zero();
        flightTargetAltitude_ = camera_.position.y;
        eventText_ = "LABYRINTH";
        eventTextTimer_ = 3.0f;
    }
    ResetEdenForbiddenFruit();
    if (!IsEdenMap() && config_.startWithArk) {
        ResetEdenArk();
    }

    if (!IsEdenMap()) {
        SpawnStartingPickups();
    }
    if (IsEdenMap()) {
        eventText_ = "EDEN";
        eventTextTimer_ = 3.0f;
    } else if (TutorialMode()) {
        ShowTutorialTip("Welcome to tutorial!\nYou can change the game mode in gameplay.cfg.");
        tutorialTipTimer_ = 8.0f;
        tutorialTipDuration_ = 8.0f;
        tutorialHintTimer_ = 0.3f;
        eventText_ = "TUTORIAL | No enemies | Test all weapons";
        eventTextTimer_ = 5.0f;
    }
    if (!IsEdenMap() && DuelMode()) {
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
    if (!IsEdenMap() && config_.ufoStartWithVehicle) {
        UfoPreservedState preserved;
        preserved.ufoEssence = config_.ufoPilotEssenceMax;
        preserved.playerEssence = essence_;
        preserved.totalCollected = preserved.ufoEssence;
        preserved.weapon = UfoPilotWeapon::Orb;
        ResetWorldForUfoArrival(config_.gameMode, config_.mapType, preserved);
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
    firePatches_.clear();
    napalmGrenades_.clear();
    gravityWells_.clear();
    nanoBlades_.clear();
    judgmentStigmas_.clear();
    edenGuardians_.clear();
    edenFireSlashes_.clear();
    for (NanoPlatform& platform : nanoPlatforms_) {
        physics_.DestroyBody(platform.platformBody);
    }
    nanoPlatforms_.clear();
    slimeSpawnPods_.clear();
    magicCircles_.clear();
    wormholes_.clear();
    ballLightnings_.clear();
    seraphFireballs_.clear();
    edenFireRain_.clear();
    waterDroplets_.clear();
    waterDropletCrafts_.clear();
    scavengerUfo_ = {};
    throneAngel_ = {};
    warRider_ = {};
    conquestRider_ = {};
    famineRider_ = {};
    deathRider_ = {};
    cherubs_.clear();
    seraphs_.clear();
    plagueArrows_.clear();
    deathSouls_.clear();
    deathSkulls_.clear();
    edenGuardians_.clear();
    edenFireSlashes_.clear();
    edenForbiddenFruit_ = {};
    edenRiverBlessing_ = -1;
    edenRiverBlessingTimer_ = 0.0f;
    edenRiverEssenceTimer_ = 0.0f;
    edenArk_ = {};
    labyrinthGrid_.clear();
    labyrinthPendingGrid_.clear();
    labyrinthWidth_ = 0;
    labyrinthHeight_ = 0;
    labyrinthRuntimeSeed_ = 0;
    labyrinthShiftTimer_ = 0.0f;
    labyrinthShiftWarning_ = false;
    labyrinthMinotaurSpawned_ = false;
    playerAntigravityTimer_ = 0.0f;
    playerPlagueTimer_ = 0.0f;
    playerPlagueTickTimer_ = 0.0f;
    jacobLadderPullFade_ = 0.0f;
    edenFallOutTimer_ = 0.0f;
    StopSfx(sfxUfoTractor_);
    StopSfx(sfxLaserBeam_);
    StopSfx(sfxArkFloodCurrent_);
    StopSfx(sfxArkFloodSurge_);
#ifndef VIONATURE_NO_AUDIO
    if (throneBgmLoaded_ && IsMusicStreamPlaying(throneBgmMusic_)) {
        StopMusicStream(throneBgmMusic_);
    }
    if (seraphBgmLoaded_ && IsMusicStreamPlaying(seraphBgmMusic_)) {
        StopMusicStream(seraphBgmMusic_);
    }
    if (heavenFallsBgmLoaded_ && IsMusicStreamPlaying(heavenFallsBgmMusic_)) {
        StopMusicStream(heavenFallsBgmMusic_);
    }
    if (ufoBgmLoaded_ && IsMusicStreamPlaying(ufoBgmMusic_)) {
        StopMusicStream(ufoBgmMusic_);
    }
    if (ufoHyperspaceBgmLoaded_ && IsMusicStreamPlaying(ufoHyperspaceBgmMusic_)) {
        StopMusicStream(ufoHyperspaceBgmMusic_);
    }
#endif

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
    UpdateBgm(dt);

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

    // Hold ESC to exit (when console is closed)
    if (IsKeyDown(KEY_ESCAPE) && !consoleOpen_) {
        exitHoldTimer_ += GetFrameTime();
        if (exitHoldTimer_ >= kExitHoldDuration) {
            wantsQuit_ = true;
        }
    } else {
        exitHoldTimer_ = 0.0f;
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
    sfxEnemyHitCooldown_ = std::max(0.0f, sfxEnemyHitCooldown_ - dt);

    if (UfoHyperspaceActive()) {
        UpdateUfoHyperspace(dt);
        return;
    }

    UpdateLook(dt);

    if (state_ == State::Playing) {
        bool edenApocalypse = IsEdenMap() && edenForbiddenFruit_.claimed;
        if (edenFallOutTimer_ > 0.0f) {
            edenFallOutTimer_ += dt;
            edenExitFade_ = 0.0f;
            if (edenFallOutTimer_ >= config_.edenFallOutFadeDuration) {
                edenFallOutTimer_ = 0.0f;
                ExitEden(true);
            }
            return;
        }
        if (!timeStopped_ && !IsEdenMap()) {
            survivalTime_ += dt;
        }
        spawnInterval_ = std::max(0.45f, 1.9f - survivalTime_ * 0.025f);

        // Snapshot boss health before damage processing
        float bossHealthBefore[16];
        JPH::BodyID bossBodies[16];
        int bossSnapCount = 0;
        for (const Enemy& enemy : enemies_) {
            if ((enemy.type == EnemyType::Boss || enemy.type == EnemyType::Duelist
                 || enemy.type == EnemyType::DummyBoss || enemy.type == EnemyType::SlimeKing
                 || enemy.type == EnemyType::Minotaur)
                && bossSnapCount < 16) {
                bossBodies[bossSnapCount] = enemy.body;
                bossHealthBefore[bossSnapCount] = enemy.health;
                ++bossSnapCount;
            }
        }

        if (!consoleOpen_) {
            if (UfoPilotActive()) {
                UpdateUfoPilot(dt);
            } else if (edenArk_.piloted) {
                UpdateEdenArk(dt);
                if (IsKeyPressed(KEY_F)) {
                    ExitEdenArk();
                }
            } else if (IsEdenMap()) {
                UpdatePlayer(dt);
                if (EdenArkEnterAvailable() && IsKeyPressed(KEY_F)) {
                    EnterEdenArk();
                } else if (EdenForbiddenFruitInteractAvailable() && IsKeyPressed(KEY_F)) {
                    ClaimEdenForbiddenFruit();
                }
                if (edenForbiddenFruit_.claimed) {
                    UpdateWeaponSwitching();
                    UpdateShooting(dt);
                }
            } else {
                if (EdenArkEnterAvailable() && IsKeyPressed(KEY_F)) {
                    EnterEdenArk();
                } else if (UfoEnterAvailable() && IsKeyPressed(KEY_F)) {
                    EnterScavengerUfo();
                } else {
                    UpdatePlayer(dt);
                    UpdateWeaponSwitching();
                    UpdateShooting(dt);
                }
            }
        }
        if (!IsEdenMap() || edenApocalypse) {
            UpdateBeam(dt);
            UpdateWormholes(dt);
        }
        UpdateShockwaves(dt);
        UpdateHeatwaves(dt);
        if (!timeStopped_ && state_ == State::Playing) {
            UpdatePlayerPlague(dt);
        }
        if (!timeStopped_ && (!IsEdenMap() || edenApocalypse)) {
            UpdateFirePatches(dt);
            UpdateNapalmGrenades(dt);
            UpdateBallLightnings(dt);
            UpdateWaterDroplets(dt);
            UpdateGravityWells(dt);
            UpdateNanoBlades(dt);
            UpdateJudgmentStigmas(dt);
            UpdateEdenFireSlashes(dt);
            UpdateNanoPlatforms(dt);
            if (!IsEdenMap()) {
                UpdateMagicCircles(dt);
            }
        }
        if (!IsEdenMap()) {
            UpdateSlimeSpawnPods(dt);
        }
        if (!timeStopped_ && IsLabyrinthMap()) {
            UpdateLabyrinth(dt);
        }
        if (!timeStopped_ && !IsEdenMap() && !DuelMode() && !TutorialMode()) {
            UpdateWaveDirector(dt);
        }
        if (!timeStopped_ && !IsEdenMap()) {
            UpdateEnemies(dt);
            UpdateDrones(dt);
            UpdateBethlehem(dt);
            UpdateThroneAngel(dt);
            UpdateCherubs(dt);
            UpdateSeraph(dt);
            UpdateSeraphFireballs(dt);
            UpdateWarRider(dt);
            UpdateConquestRider(dt);
            UpdateFamineRider(dt);
            UpdateDeathRider(dt);
            UpdateDeathSouls(dt);
            UpdateDeathSkulls(dt);
            UpdatePlagueArrows(dt);
            UpdateScavengerUfo(dt);
            UpdateProjectiles(dt);
            UpdateCollisions();

            // Update boss health bar priority after all damage is applied
            for (Enemy& enemy : enemies_) {
                if (enemy.type != EnemyType::Boss && enemy.type != EnemyType::Duelist
                    && enemy.type != EnemyType::DummyBoss && enemy.type != EnemyType::SlimeKing
                    && enemy.type != EnemyType::Minotaur) continue;
                for (int si = 0; si < bossSnapCount; ++si) {
                    if (bossBodies[si] == enemy.body && enemy.health < bossHealthBefore[si]) {
                        enemy.lastDamageTime = survivalTime_;
                        break;
                    }
                }
            }
        }
        if (!IsEdenMap()) {
            UpdatePickups(dt);
            UpdateEssenceSpawn(dt);
            UpdateArenaBounds();
        } else {
            UpdateEdenRiverBlessings(dt);
            UpdateEdenEssenceField(dt);
            if (edenForbiddenFruit_.claimed) {
                UpdateEdenFireRain(dt);
                UpdateEdenGuardians(dt);
                UpdateSeraph(dt);
                UpdateSeraphFireballs(dt);
                UpdateProjectiles(dt);
                if (DistanceXZ(camera_.position, Vector3Zero()) > EdenCombatBoundaryRadius()) {
                    edenFallOutTimer_ = 0.001f;
                    eventText_ = "FALLING FROM EDEN";
                    eventTextTimer_ = config_.edenFallOutFadeDuration;
                    StopSfx(sfxFlamethrowerFireball_);
                }
            }
            edenExitFade_ = edenForbiddenFruit_.claimed ? 0.0f : EdenExitFade();
            if (!edenForbiddenFruit_.claimed
                && DistanceXZ(camera_.position, Vector3Zero()) > config_.edenMapRadius) {
                ExitEden();
            }
        }

        if (!timeStopped_ && (!IsEdenMap() || edenApocalypse)) {
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
    float ladderBeamT = 0.0f;
    Vector3 ladderAxisPoint = {};
    float ladderRadialRatio = 0.0f;
    bool inJacobLadder = PlayerInsideThroneLadderBeam(&ladderBeamT, &ladderAxisPoint, &ladderRadialRatio);
    float ladderHeightProgress = 1.0f - std::clamp(ladderBeamT / std::max(0.001f, config_.throneJacobLadderBeamLength), 0.0f, 1.0f);
    ladderHeightProgress = ladderHeightProgress * ladderHeightProgress * (3.0f - 2.0f * ladderHeightProgress);
    float topBloom = std::pow(ladderHeightProgress, 1.35f);
    float targetLadderFade = inJacobLadder
        ? std::clamp(0.12f + topBloom * 0.88f, 0.0f, 1.0f)
        : 0.0f;
    float fadeBlend = std::clamp(dt * (inJacobLadder ? 0.58f : 1.75f), 0.0f, 1.0f);
    jacobLadderPullFade_ += (targetLadderFade - jacobLadderPullFade_) * fadeBlend;

    if (TutorialMode() && state_ == State::Playing) {
        // Periodic combat & movement tips (bottom center, cycles every 18s)
        if (tutorialTipTimer_ <= 0.0f) {
            tutorialHintTimer_ = std::max(0.0f, tutorialHintTimer_ - dt);
            if (tutorialHintTimer_ <= 0.0f) {
                const char* combatTips[] = {
                    T("战斗技巧: 火箭跳\n对脚下发射火箭,利用爆炸冲量获得额外高度",
                      "COMBAT: Rocket Jump\nFire a rocket at your feet for extra height"),
                    T("战斗技巧: 空中位移链\n火箭跳->霰弹反冲->长枪反冲,三段超远位移",
                      "COMBAT: Air Chain\nRocket jump -> shotgun recoil -> spear recoil for 3-stage movement"),
                    T("战斗技巧: 刀波钓鱼\n发射刀波后闪现换位,引诱追逐的敌人撞入刀波",
                      "COMBAT: Blade Bait\nFire a nano blade, then blink to lure enemies into it"),
                    T("战斗技巧: 时停连招\n时停->贴脸全部火箭->黑洞手雷->解冻瞬间爆发",
                      "COMBAT: Time-Stop Combo\nFreeze time -> point-blank rockets -> black hole -> resume"),
                    T("战斗技巧: 无人机交叉火力\n不同位置部署无人机,长按右键设集合点",
                      "COMBAT: Drone Crossfire\nDeploy drones at multiple positions, hold RMB to set rally points"),
                    T("战斗技巧: 角动量保持\n球形地图切线速度不受重力,环绕加速",
                      "COMBAT: Orbital Momentum\nTangent velocity is conserved on spherical maps for extreme speed"),
                };
                const char* configTips[] = {
                    T("系统提示: 开发控制台\n按 ~ 打开控制台, 输入 key=value 实时调整游戏参数",
                      "SYSTEM: Dev Console\nPress ~ to open, type key=value to live-tune parameters"),
                    T("系统提示: 永久保存配置\n编辑 config/gameplay.cfg 可永久保存所有参数修改",
                      "SYSTEM: Permanent Config\nEdit gameplay.cfg to save changes permanently"),
                };
                const char* movementTips[] = {
                    T("移动提示: Shift加速跑动\n空中也保持加速度(Quake风格空中控制)",
                      "MOVEMENT: Shift to Run\nAir acceleration is preserved (Quake-style air control)"),
                    T("移动提示: 太空服(蓝)拾取后按Z开关\n低重力0.24x,可高跳远跃",
                      "MOVEMENT: Space Suit (Blue)\nPress Z to toggle low gravity (0.24x)"),
                    T("移动提示: 飞行装置(青)拾取后按X悬停\n空格升高 / Ctrl降低,悬停瞄准",
                      "MOVEMENT: Flight Rig (Cyan)\nPress X to hover, Space/Ctrl to ascend/descend"),
                    T("移动提示: 滑板(绿)拾取后按C切换滑行\n极低摩擦,保持高动量滑动",
                      "MOVEMENT: Skates (Green)\nPress C to toggle ultra-low friction sliding"),
                };
                const char* welcomeTip = T(
                    "教学模式 | P键隐藏HUD | 自由探索9把武器\n编辑 gameplay.cfg 自定义200+可调参数",
                    "Tutorial Mode | P hides HUD | Explore all 9 weapons\nEdit gameplay.cfg to tune 200+ params");
                const char* allTips[] = {
                    welcomeTip,
                    combatTips[0], movementTips[0], combatTips[1], movementTips[1],
                    combatTips[2], movementTips[2], combatTips[3], movementTips[3],
                    combatTips[4], combatTips[5],
                    configTips[0], configTips[1],
                };
                // When console is open, only rotate config-related tips
                if (consoleOpen_) {
                    ShowTutorialTip(configTips[tutorialHintIndex_ % 2]);
                    tutorialHintIndex_ = (tutorialHintIndex_ + 1) % 2;
                } else {
                    constexpr int totalTips = sizeof(allTips) / sizeof(allTips[0]);
                    ShowTutorialTip(allTips[tutorialHintIndex_ % totalTips]);
                    tutorialHintIndex_ = (tutorialHintIndex_ + 1) % totalTips;
                }
                tutorialHintTimer_ = 1.0f;
            }
        }

        // Config file editing reminders (top-left gold eventText, every 90s)
        configReminderTimer_ = std::max(0.0f, configReminderTimer_ - dt);
        if (configReminderTimer_ <= 0.0f && eventTextTimer_ <= 0.0f) {
            const char* configReminders[] = {
                "TIP: Edit gameplay.cfg to tune 200+ params",
                "TIP: game_mode = survival | duel | tutorial",
                "TIP: map_type = circle | square | asteroid | hollow_world | eden | labyrinth",
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
        if (!config_.heavenFalls && !bethlehemSpawned_ && survivalTime_ >= config_.dummyBethlehemSpawnTime) {
            SpawnBethlehem();
            bethlehemSpawned_ = true;
        }
    }
    timeStopTintTimer_ = std::max(0.0f, timeStopTintTimer_ - dt);
    duelArmorInvulnTimer_ = std::max(0.0f, duelArmorInvulnTimer_ - dt);
    longinusSpearThrustInvulnTimer_ = std::max(0.0f, longinusSpearThrustInvulnTimer_ - dt);
    essenceInvulnTimer_ = std::max(0.0f, essenceInvulnTimer_ - dt);
    playerAntigravityTimer_ = std::max(0.0f, playerAntigravityTimer_ - dt);
    damageFlash_ = std::max(0.0f, damageFlash_ - dt * 4.0f);
    cameraShake_ = std::max(0.0f, cameraShake_ - dt * 5.0f);
}
void Game::Draw() {
    if (ufoTravelState_ == UfoTravelState::Hyperspace || ufoTravelState_ == UfoTravelState::Arriving) {
        DrawUfoHyperspace();
        return;
    }

    BeginTextureMode(pixelTarget_);
    if (IsEdenMap()) {
        ClearBackground(Color{18, 10, 38, 255});
        float apocalypse = edenForbiddenFruit_.claimed ? std::clamp(edenForbiddenFruit_.apocalypse, 0.0f, 1.0f) : 0.0f;
        auto mixChannel = [](float a, float b, float t) -> unsigned char {
            return static_cast<unsigned char>(std::clamp(a + (b - a) * t, 0.0f, 255.0f));
        };
        for (int y = 0; y < pixelHeight_; ++y) {
            float t = static_cast<float>(y) / std::max(1.0f, static_cast<float>(pixelHeight_ - 1));
            Color edenSky{
                static_cast<unsigned char>(18.0f + 98.0f * t),
                static_cast<unsigned char>(10.0f + 72.0f * t),
                static_cast<unsigned char>(38.0f + 72.0f * t),
                255
            };
            Color apocalypseSky{
                static_cast<unsigned char>(62.0f + 132.0f * t),
                static_cast<unsigned char>(8.0f + 30.0f * t),
                static_cast<unsigned char>(18.0f + 16.0f * t),
                255
            };
            Color sky{
                mixChannel(edenSky.r, apocalypseSky.r, apocalypse),
                mixChannel(edenSky.g, apocalypseSky.g, apocalypse),
                mixChannel(edenSky.b, apocalypseSky.b, apocalypse),
                255
            };
            DrawLine(0, y, pixelWidth_, y, sky);
        }
    } else if (config_.heavenFalls) {
        ClearBackground(Color{28, 4, 6, 255});
        for (int y = 0; y < pixelHeight_; ++y) {
            float t = static_cast<float>(y) / std::max(1.0f, static_cast<float>(pixelHeight_ - 1));
            float dusk = t * t * (3.0f - 2.0f * t);
            Color sky{
                static_cast<unsigned char>(26.0f + 84.0f * dusk),
                static_cast<unsigned char>(4.0f + 14.0f * dusk),
                static_cast<unsigned char>(8.0f + 8.0f * dusk),
                255
            };
            DrawLine(0, y, pixelWidth_, y, sky);
        }
    } else {
        ClearBackground(Color{8, 8, 10, 255});
    }

    BeginMode3D(camera_);
    if (IsEdenMap()) {
        DrawEdenSkySphere();
    }
    DrawArena();
    DrawEdenForbiddenFruit();
    if (!IsEdenMap()) DrawEdenArk();
    DrawProps();
    DrawNanoPlatforms();
    DrawSlimeSpawnPods();
    DrawDrones();
    DrawEnemies();
    DrawBethlehem();
    DrawThroneAngel();
    DrawCherubs();
    DrawSeraph();
    DrawWarRider();
    DrawConquestRider();
    DrawFamineRider();
    DrawDeathRider();
    DrawDeathSouls();
    DrawDeathSkulls();
    DrawSeraphFireballs();
    DrawPlagueArrows();
    if (!UfoPilotActive()) DrawScavengerUfo();
    DrawPickups();
    DrawProjectiles();
    DrawBeams();
    DrawBallLightnings();
    DrawWaterDropletCrafts();
    DrawWaterDroplets();
    DrawShockwaves();
    DrawHeatwaves();
    DrawFirePatches();
    DrawNapalmGrenades();
    DrawGravityWells();
    DrawNanoBlades();
    DrawJudgmentStigmas();
    DrawEdenFireSlashes();
    DrawMagicCircles();
    DrawMysticStaffShield();
    DrawParticles();
    DrawRallyMarker();
    DrawBlinkIndicator();
    if (UfoPilotActive()) {
        DrawUfoPilotWeapon();
    } else if (!edenArk_.piloted && (!IsEdenMap() || edenForbiddenFruit_.claimed) && !fireControlActive_ && !hideUI_) {
        DrawWeapon();
    }
    EndMode3D();

    // Second 3D pass: X-ray octahedron markers visible through obstacles
    if (UfoPilotActive()) {
        DrawUfoCockpitOverlay();
    } else if (fireControlActive_) {
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
        if (IsEdenMap() && !edenForbiddenFruit_.claimed) {
            if (!hideUI_) DrawHud();
        } else {
        if (!hideUI_) DrawCrosshair();
        if (!hideUI_) DrawHud();
        }
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
    if (jacobLadderPullFade_ > 0.001f) {
        float fade = std::clamp(jacobLadderPullFade_, 0.0f, 1.0f);
        float whiteBloom = std::pow(fade, 2.25f);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(Color{255, 248, 218, 255}, fade * 0.54f));
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(WHITE, whiteBloom * 0.46f));
    }
    if (IsEdenMap() && edenExitFade_ > 0.001f) {
        float fade = std::clamp(edenExitFade_, 0.0f, 1.0f);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(Color{255, 246, 218, 255}, fade * 0.55f));
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(WHITE, std::pow(fade, 1.8f) * 0.5f));
    }
    if (IsEdenMap() && edenFallOutTimer_ > 0.0f) {
        float fade = std::clamp(edenFallOutTimer_ / std::max(0.001f, config_.edenFallOutFadeDuration), 0.0f, 1.0f);
        fade = fade * fade * (3.0f - 2.0f * fade);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), FadeColor(BLACK, fade));
    }

    // Exit hold overlay
    if (exitHoldTimer_ > 0.0f) {
        float progress = exitHoldTimer_ / kExitHoldDuration;
        unsigned char overlayAlpha = static_cast<unsigned char>(progress * progress * 180.0f);
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), Color{0, 0, 0, overlayAlpha});
        const char* exitText = "EXITING THE GAME";
        int fontSize = 32;
        int tw = MeasureText(exitText, fontSize);
        DrawText(exitText, (GetScreenWidth() - tw) / 2, GetScreenHeight() / 2 - 24, fontSize, WHITE);
        // Progress bar
        int barW = 200, barH = 4;
        int barX = (GetScreenWidth() - barW) / 2, barY = GetScreenHeight() / 2 + 16;
        DrawRectangle(barX, barY, barW, barH, Color{40, 40, 40, 200});
        DrawRectangle(barX, barY, static_cast<int>(barW * progress), barH, Color{200, 60, 40, 240});
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
    if (key == "bgm_path") { c.bgmPath = value; return true; }
    if (key == "heaven_falls_bgm_path") { c.heavenFallsBgmPath = value; return true; }
    if (key == "throne_bgm_path") { c.throneBgmPath = value; return true; }
    if (key == "seraph_bgm_path") { c.seraphBgmPath = value; return true; }
    if (key == "ufo_bgm_path") { c.ufoBgmPath = value; return true; }
    if (key == "ufo_hyperspace_bgm_path") { c.ufoHyperspaceBgmPath = value; return true; }
    if (key == "invincible") { bool b; if (value == "true" || value == "1") { c.invincible = true; return true; } else if (value == "false" || value == "0") { c.invincible = false; return true; } return false; }
    if (key == "boss_rush_mode") { bool b; if (value == "true" || value == "1") { c.bossRushMode = true; return true; } else if (value == "false" || value == "0") { c.bossRushMode = false; return true; } return false; }
    if (key == "heaven_falls" || key == "Heaven_falls") { bool b; if (value == "true" || value == "1") { c.heavenFalls = true; return true; } else if (value == "false" || value == "0") { c.heavenFalls = false; return true; } return false; }
    if (key == "throne_enabled") { bool b; if (value == "true" || value == "1") { c.throneEnabled = true; return true; } else if (value == "false" || value == "0") { c.throneEnabled = false; return true; } return false; }
    if (key == "throne_jacob_ladder_enabled") { bool b; if (value == "true" || value == "1") { c.throneJacobLadderEnabled = true; return true; } else if (value == "false" || value == "0") { c.throneJacobLadderEnabled = false; return true; } return false; }
    if (key == "eden_forbidden_fruit_enabled") { bool b; if (value == "true" || value == "1") { c.edenForbiddenFruitEnabled = true; ResetEdenForbiddenFruit(); return true; } else if (value == "false" || value == "0") { c.edenForbiddenFruitEnabled = false; edenForbiddenFruit_ = {}; return true; } return false; }
    if (key == "start_with_ark") { bool b; if (value == "true" || value == "1") { c.startWithArk = true; ResetEdenArk(); return true; } else if (value == "false" || value == "0") { c.startWithArk = false; physics_.DestroyBody(edenArk_.body); edenArk_ = {}; return true; } return false; }
    if (key == "seraph_enabled") { bool b; if (value == "true" || value == "1") { c.seraphEnabled = true; return true; } else if (value == "false" || value == "0") { c.seraphEnabled = false; return true; } return false; }
    if (key == "war_rider_enabled") { bool b; if (value == "true" || value == "1") { c.warRiderEnabled = true; return true; } else if (value == "false" || value == "0") { c.warRiderEnabled = false; return true; } return false; }
    if (key == "conquest_rider_enabled") { bool b; if (value == "true" || value == "1") { c.conquestRiderEnabled = true; return true; } else if (value == "false" || value == "0") { c.conquestRiderEnabled = false; return true; } return false; }
    if (key == "famine_rider_enabled") { bool b; if (value == "true" || value == "1") { c.famineRiderEnabled = true; return true; } else if (value == "false" || value == "0") { c.famineRiderEnabled = false; return true; } return false; }
    if (key == "ufo_enabled") { bool b; if (value == "true" || value == "1") { c.ufoEnabled = true; return true; } else if (value == "false" || value == "0") { c.ufoEnabled = false; return true; } return false; }
    if (key == "ufo_debug_local_jump_enabled") { bool b; if (value == "true" || value == "1") { c.ufoDebugLocalJumpEnabled = true; return true; } else if (value == "false" || value == "0") { c.ufoDebugLocalJumpEnabled = false; return true; } return false; }
    if (key == "ufo_start_with_vehicle") { bool b; if (value == "true" || value == "1") { c.ufoStartWithVehicle = true; return true; } else if (value == "false" || value == "0") { c.ufoStartWithVehicle = false; return true; } return false; }
    if (key == "ufo_arrival_variant_enabled") { bool b; if (value == "true" || value == "1") { c.ufoArrivalVariantEnabled = true; return true; } else if (value == "false" || value == "0") { c.ufoArrivalVariantEnabled = false; return true; } return false; }
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
    if (key == "eden_essence_target_count") { c.edenEssenceTargetCount = std::clamp(std::atoi(value.c_str()), 0, 256); return true; }
    if (key == "eden_fire_rain_burst_count") { c.edenFireRainBurstCount = std::clamp(std::atoi(value.c_str()), 0, 32); return true; }
    if (key == "eden_apocalypse_seraph_count") { c.edenApocalypseSeraphCount = std::clamp(std::atoi(value.c_str()), 0, 8); return true; }
    if (key == "war_rider_command_max_growth_stacks") { c.warRiderCommandMaxGrowthStacks = std::clamp(std::atoi(value.c_str()), 0, 8); return true; }
    if (key == "dummy_max_count") { c.dummyMaxCount = std::max(0, std::atoi(value.c_str())); return true; }
    if (key == "boss_homing_burst_count") { c.bossHomingBurstCount = std::clamp(std::atoi(value.c_str()), 1, 6); return true; }
    if (key == "throne_summon_count") { c.throneSummonCount = std::clamp(std::atoi(value.c_str()), 0, 64); return true; }
    if (key == "throne_max_cherubs") { c.throneMaxCherubs = std::clamp(std::atoi(value.c_str()), 0, 128); return true; }
    if (key == "seraph_fireball_count") { c.seraphFireballCount = std::clamp(std::atoi(value.c_str()), 1, 80); return true; }
    if (key == "seraph_spawn_count") { c.seraphSpawnCount = std::clamp(std::atoi(value.c_str()), 1, 8); return true; }
    if (key == "conquest_rider_summon_count") { c.conquestRiderSummonCount = std::clamp(std::atoi(value.c_str()), 0, 24); return true; }
    if (key == "ufo_collect_required") { c.ufoCollectRequired = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "ufo_base_essence") { c.ufoBaseEssence = std::max(0, std::atoi(value.c_str())); return true; }
    if (key == "ufo_pilot_essence_max") { c.ufoPilotEssenceMax = std::max(1, std::atoi(value.c_str())); return true; }
    if (key == "ufo_pilot_jump_cost") { c.ufoPilotJumpCost = std::max(1, std::atoi(value.c_str())); return true; }

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
    if (key == "heaven_falls" || key == "Heaven_falls") return c.heavenFalls ? "true" : "false";
    if (key == "throne_enabled") return c.throneEnabled ? "true" : "false";
    if (key == "throne_jacob_ladder_enabled") return c.throneJacobLadderEnabled ? "true" : "false";
    if (key == "seraph_enabled") return c.seraphEnabled ? "true" : "false";
    if (key == "war_rider_enabled") return c.warRiderEnabled ? "true" : "false";
    if (key == "conquest_rider_enabled") return c.conquestRiderEnabled ? "true" : "false";
    if (key == "famine_rider_enabled") return c.famineRiderEnabled ? "true" : "false";
    if (key == "ufo_enabled") return c.ufoEnabled ? "true" : "false";
    if (key == "ufo_debug_local_jump_enabled") return c.ufoDebugLocalJumpEnabled ? "true" : "false";
    if (key == "ufo_start_with_vehicle") return c.ufoStartWithVehicle ? "true" : "false";
    if (key == "ufo_arrival_variant_enabled") return c.ufoArrivalVariantEnabled ? "true" : "false";
    if (key == "start_with_ark") return c.startWithArk ? "true" : "false";
    if (key == "duelist_smart_ai") return c.duelistSmartAi ? "true" : "false";
    if (key == "ufo_enabled") return c.ufoEnabled ? "true" : "false";
    if (key == "eden_forbidden_fruit_enabled") return c.edenForbiddenFruitEnabled ? "true" : "false";
    // Int
    if (key == "shotgun_pellet_count") return std::to_string(c.shotgunPelletCount);
    if (key == "duelist_count") return std::to_string(c.duelistCount);
    if (key == "starting_essence") return std::to_string(c.startingEssence);
    if (key == "duel_player_armor") return std::to_string(c.duelPlayerArmor);
    if (key == "drone_max_count") return std::to_string(c.droneMaxCount);
    if (key == "essence_max_on_map") return std::to_string(c.essenceMaxOnMap);
    if (key == "eden_essence_target_count") return std::to_string(c.edenEssenceTargetCount);
    if (key == "eden_fire_rain_burst_count") return std::to_string(c.edenFireRainBurstCount);
    if (key == "eden_apocalypse_seraph_count") return std::to_string(c.edenApocalypseSeraphCount);
    if (key == "war_rider_command_max_growth_stacks") return std::to_string(c.warRiderCommandMaxGrowthStacks);
    if (key == "throne_summon_count") return std::to_string(c.throneSummonCount);
    if (key == "throne_max_cherubs") return std::to_string(c.throneMaxCherubs);
    if (key == "seraph_fireball_count") return std::to_string(c.seraphFireballCount);
    if (key == "seraph_spawn_count") return std::to_string(c.seraphSpawnCount);
    if (key == "conquest_rider_summon_count") return std::to_string(c.conquestRiderSummonCount);
    if (key == "ufo_collect_required") return std::to_string(c.ufoCollectRequired);
    if (key == "ufo_base_essence") return std::to_string(c.ufoBaseEssence);
    if (key == "ufo_pilot_essence_max") return std::to_string(c.ufoPilotEssenceMax);
    if (key == "ufo_pilot_jump_cost") return std::to_string(c.ufoPilotJumpCost);
    // String
    if (key == "game_mode") return c.gameMode;
    if (key == "map_type") return c.mapType;
    if (key == "bgm_path") return c.bgmPath;
    if (key == "heaven_falls_bgm_path") return c.heavenFallsBgmPath;
    if (key == "throne_bgm_path") return c.throneBgmPath;
    if (key == "seraph_bgm_path") return c.seraphBgmPath;
    if (key == "ufo_bgm_path") return c.ufoBgmPath;
    if (key == "ufo_hyperspace_bgm_path") return c.ufoHyperspaceBgmPath;
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

    // Arrow keys: navigate completions when visible, otherwise history (with hold-to-repeat)
    auto arrowFire = [&](bool isDown)->bool {
        if (IsKeyPressed(isDown ? KEY_DOWN : KEY_UP)) return true;
        consoleArrowTimer_ += GetFrameTime();
        float delay = consoleArrowTimer_ < 0.35f ? 0.35f : 0.04f;
        if (consoleArrowTimer_ >= delay) { consoleArrowTimer_ -= delay; return true; }
        return false;
    };
    bool upHeld = IsKeyDown(KEY_UP) && !IsKeyDown(KEY_DOWN);
    bool downHeld = IsKeyDown(KEY_DOWN) && !IsKeyDown(KEY_UP);
    if (!upHeld && !downHeld) consoleArrowTimer_ = 0.0f;

    if (upHeld && arrowFire(false)) {
        if (!consoleCompletions_.empty()) {
            if (consoleCompletionIdx_ > 0) --consoleCompletionIdx_;
            else consoleCompletionIdx_ = static_cast<int>(consoleCompletions_.size()) - 1;
            const auto& sel = consoleCompletions_[consoleCompletionIdx_];
            strncpy(consoleInput_, sel.c_str(), 126);
            consoleInput_[sel.size()] = '\0';
            consoleCursor_ = static_cast<int>(sel.size());
        } else if (!consoleHistory_.empty()) {
            if (consoleHistoryIdx_ == -1) consoleHistoryIdx_ = static_cast<int>(consoleHistory_.size()) - 1;
            else if (consoleHistoryIdx_ > 0) --consoleHistoryIdx_;
            strncpy(consoleInput_, consoleHistory_[consoleHistoryIdx_].c_str(), 126);
            consoleCursor_ = static_cast<int>(strlen(consoleInput_));
        }
    }
    if (downHeld && arrowFire(true)) {
        if (!consoleCompletions_.empty()) {
            consoleCompletionIdx_ = (consoleCompletionIdx_ + 1) % static_cast<int>(consoleCompletions_.size());
            const auto& sel = consoleCompletions_[consoleCompletionIdx_];
            strncpy(consoleInput_, sel.c_str(), 126);
            consoleInput_[sel.size()] = '\0';
            consoleCursor_ = static_cast<int>(sel.size());
        } else if (!consoleHistory_.empty()) {
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
    int fontSz = 25, hintSz = 25, feedbackSz = 25;
    int h = 30;
    int y = pixelHeight_;
    int screenW = pixelWidth_;

    // Input background
    DrawRectangle(0, y - 4, screenW, h + 8, Color{0, 0, 0, 220});

    // Input line
    DrawText(">", 6, y + 2, fontSz, Color{180, 220, 255, 255});
    DrawText(consoleInput_, 18, y + 2, fontSz, Color{220, 235, 255, 255});

    // Show current value hint when a valid key is typed
    std::string input(consoleInput_, consoleCursor_);
    std::string val = GetConfigValue(input);
    if (val != "?") {
        DrawText(TextFormat("= %s", val.c_str()), 18 + MeasureText(consoleInput_, fontSz) + 4, y + 2, hintSz, Color{120, 130, 150, 255});
    }

    // Cursor blink
    if (static_cast<int>(GetTime() * 2.0f) % 2 == 0) {
        int curX = 18 + MeasureText(TextFormat("%.*s", consoleCursor_, consoleInput_), fontSz);
        DrawLine(curX, y + 3, curX, y + h - 4, Color{255, 255, 255, 200});
    }

    // Completion suggestions (below input line)
    if (consoleCompletions_.size() > 1) {
        int sugY = y + h + 4;
        int sugH = static_cast<int>(consoleCompletions_.size()) * (hintSz + 2);
        DrawRectangle(0, sugY - 2, screenW, sugH + 4 + 4, Color{0, 0, 0, 200});
        for (size_t i = 0; i < consoleCompletions_.size(); ++i) {
            Color c = (static_cast<int>(i) == consoleCompletionIdx_) ? Color{255, 200, 60, 255} : Color{160, 160, 170, 255};
            DrawText(consoleCompletions_[i].c_str(), 8, sugY + static_cast<int>(i) * (hintSz + 2), hintSz, c);
        }
    }

    // Feedback (below everything)
    if (consoleFeedbackTimer_ > 0.0f) {
        int fbY = y + h - 60;
        if (consoleCompletions_.size() > 1) fbY += static_cast<int>(consoleCompletions_.size()) * (hintSz + 2) + 4;
        Color fbColor = consoleFeedback_.find("OK:") == 0 ? Color{120, 255, 140, 255} : Color{255, 140, 120, 255};
        DrawText(consoleFeedback_.c_str(), 8, fbY, feedbackSz, FadeColor(fbColor, std::min(1.0f, consoleFeedbackTimer_)));
    }
}
