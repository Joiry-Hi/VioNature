#include "GameConfig.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <limits>
#include <string>
#include <unordered_map>

namespace {

std::string Trim(std::string value) {
    auto isSpace = [](unsigned char c) { return std::isspace(c) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](unsigned char c) { return !isSpace(c); }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](unsigned char c) { return !isSpace(c); }).base(), value.end());
    return value;
}

std::string Lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}

bool ParseFloat(const std::string& text, float& out) {
    char* end = nullptr;
    float value = std::strtof(text.c_str(), &end);
    if (end == text.c_str()) {
        return false;
    }
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }
    out = value;
    return true;
}

bool ParseInt(const std::string& text, int& out) {
    char* end = nullptr;
    long value = std::strtol(text.c_str(), &end, 10);
    if (end == text.c_str()) {
        return false;
    }
    while (*end != '\0') {
        if (!std::isspace(static_cast<unsigned char>(*end))) {
            return false;
        }
        ++end;
    }
    if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max()) {
        return false;
    }
    out = static_cast<int>(value);
    return true;
}

bool ParseBool(const std::string& text, bool& out) {
    std::string value = Lower(Trim(text));
    if (value == "true" || value == "1" || value == "yes" || value == "on") {
        out = true;
        return true;
    }
    if (value == "false" || value == "0" || value == "no" || value == "off") {
        out = false;
        return true;
    }
    return false;
}

}

GameplayConfig LoadGameplayConfig(const char* path) {
    GameplayConfig config;
    std::ifstream file(path);
    if (!file.is_open()) {
        return config;
    }

    std::unordered_map<std::string, float*> floats = {
        {"gravity", &config.gravity},
        {"space_suit_gravity_scale", &config.spaceSuitGravityScale},
        {"walk_speed", &config.walkSpeed},
        {"run_speed", &config.runSpeed},
        {"ground_acceleration", &config.groundAcceleration},
        {"air_acceleration", &config.airAcceleration},
        {"jump_speed", &config.jumpSpeed},
        {"dagger_damage", &config.daggerDamage},
        {"scatter_damage", &config.scatterDamage},
        {"laser_base_damage", &config.laserBaseDamage},
        {"laser_charge_damage", &config.laserChargeDamage},
        {"flame_damage", &config.flameDamage},
        {"rocket_impact_damage", &config.rocketImpactDamage},
        {"rocket_explosion_damage", &config.rocketExplosionDamage},
        {"rocket_explosion_radius", &config.rocketExplosionRadius},
        {"shotgun_pellet_damage", &config.shotgunPelletDamage},
        {"shotgun_recoil_impulse", &config.shotgunRecoilImpulse},
        {"shotgun_recoil_vertical_bonus", &config.shotgunRecoilVerticalBonus},
        {"rocket_jump_impulse", &config.rocketJumpImpulse},
        {"rocket_jump_radius", &config.rocketJumpRadius},
        {"enemy_shot_damage", &config.enemyShotDamage},
        {"enemy_shot_speed", &config.enemyShotSpeed},
        {"spitter_fire_interval", &config.spitterFireInterval},
        {"pouncer_leap_interval", &config.pouncerLeapInterval},
        {"pouncer_leap_speed", &config.pouncerLeapSpeed},
        {"boss_spawn_time", &config.bossSpawnTime},
        {"boss_health", &config.bossHealth},
        {"duelist_health", &config.duelistHealth},
        {"duelist_weapon_switch_min", &config.duelistWeaponSwitchMin},
        {"duelist_weapon_switch_max", &config.duelistWeaponSwitchMax},
        {"duelist_fire_rate_scale", &config.duelistFireRateScale},
        {"duel_armor_hit_invuln", &config.duelArmorHitInvuln},
        {"harrier_fire_interval", &config.harrierFireInterval},
        {"harrier_speed", &config.harrierSpeed},
        {"harrier_target_height", &config.harrierTargetHeight},
        {"blinker_windup", &config.blinkerWindup},
        {"blinker_cooldown", &config.blinkerCooldown},
        {"blinker_dash_speed", &config.blinkerDashSpeed},
        {"asteroid_radius", &config.asteroidRadius},
        {"asteroid_player_altitude", &config.asteroidPlayerAltitude},
        {"asteroid_enemy_altitude", &config.asteroidEnemyAltitude},
        {"asteroid_cleanup_distance", &config.asteroidCleanupDistance},
        {"asteroid_platform_altitude", &config.asteroidPlatformAltitude},
        {"gravity_nail_damage", &config.gravityNailDamage},
        {"gravity_well_radius", &config.gravityWellRadius},
        {"gravity_well_force", &config.gravityWellForce},
        {"gravity_well_lifetime", &config.gravityWellLifetime},
        {"black_hole_grenade_damage", &config.blackHoleGrenadeDamage},
        {"black_hole_radius", &config.blackHoleRadius},
        {"black_hole_force", &config.blackHoleForce},
        {"black_hole_lifetime", &config.blackHoleLifetime},
        {"black_hole_event_horizon_radius", &config.blackHoleEventHorizonRadius},
        {"heatwave_damage", &config.heatwaveDamage},
        {"heatwave_force", &config.heatwaveForce},
        {"heatwave_range", &config.heatwaveRange},
        {"glass_shard_damage", &config.glassShardDamage},
        {"glass_shard_speed", &config.glassShardSpeed},
        {"glass_shard_recoil_scale", &config.glassShardRecoilScale},
        {"recoil_lance_damage", &config.recoilLanceDamage},
        {"recoil_lance_speed", &config.recoilLanceSpeed},
        {"recoil_lance_impulse", &config.recoilLanceImpulse},
        {"rift_cutter_damage", &config.riftCutterDamage},
        {"rift_cutter_range", &config.riftCutterRange},
        {"rift_cutter_width", &config.riftCutterWidth},
        {"rift_cutter_lifetime", &config.riftCutterLifetime},
        {"rift_cutter_delay", &config.riftCutterDelay},
        {"rift_cutter_radius", &config.riftCutterRadius},
        {"rift_cutter_thickness", &config.riftCutterThickness},
        {"rift_cutter_plane_thickness", &config.riftCutterPlaneThickness},
        {"rift_cutter_wave_speed", &config.riftCutterWaveSpeed},
        {"rift_cutter_wave_spawn_distance", &config.riftCutterWaveSpawnDistance},
        {"rift_platform_range", &config.riftPlatformRange},
        {"rift_platform_delay", &config.riftPlatformDelay},
        {"rift_platform_lifetime", &config.riftPlatformLifetime},
        {"rift_platform_size", &config.riftPlatformSize},
        {"rift_platform_thickness", &config.riftPlatformThickness},
        {"blink_distance", &config.blinkDistance},
        {"blink_clear_radius", &config.blinkClearRadius},
    };

    std::string line;
    while (std::getline(file, line)) {
        size_t comment = line.find('#');
        if (comment != std::string::npos) {
            line = line.substr(0, comment);
        }

        size_t separator = line.find('=');
        if (separator == std::string::npos) {
            continue;
        }

        std::string key = Lower(Trim(line.substr(0, separator)));
        std::string value = Trim(line.substr(separator + 1));
        if (key.empty() || value.empty()) {
            continue;
        }

        auto floatIt = floats.find(key);
        if (floatIt != floats.end()) {
            float parsed = 0.0f;
            if (ParseFloat(value, parsed)) {
                *floatIt->second = parsed;
            }
            continue;
        }

        if (key == "game_mode") {
            config.gameMode = Lower(Trim(value));
        } else if (key == "map_type") {
            config.mapType = Lower(Trim(value));
        } else if (key == "duel_player_armor") {
            int parsed = 0;
            if (ParseInt(value, parsed)) {
                config.duelPlayerArmor = parsed;
            }
        } else if (key == "invincible") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.invincible = parsed;
            }
        } else if (key == "time_stop_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.timeStopEnabled = parsed;
            }
        } else if (key == "blink_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.blinkEnabled = parsed;
            }
        }
    }

    config.gravity = std::max(0.0f, config.gravity);
    config.spaceSuitGravityScale = std::max(0.0f, config.spaceSuitGravityScale);
    config.walkSpeed = std::max(0.0f, config.walkSpeed);
    config.runSpeed = std::max(0.0f, config.runSpeed);
    config.groundAcceleration = std::max(0.0f, config.groundAcceleration);
    config.airAcceleration = std::max(0.0f, config.airAcceleration);
    config.jumpSpeed = std::max(0.0f, config.jumpSpeed);
    config.daggerDamage = std::max(0.0f, config.daggerDamage);
    config.scatterDamage = std::max(0.0f, config.scatterDamage);
    config.laserBaseDamage = std::max(0.0f, config.laserBaseDamage);
    config.laserChargeDamage = std::max(0.0f, config.laserChargeDamage);
    config.flameDamage = std::max(0.0f, config.flameDamage);
    config.rocketImpactDamage = std::max(0.0f, config.rocketImpactDamage);
    config.rocketExplosionDamage = std::max(0.0f, config.rocketExplosionDamage);
    config.rocketExplosionRadius = std::max(0.0f, config.rocketExplosionRadius);
    config.shotgunPelletDamage = std::max(0.0f, config.shotgunPelletDamage);
    config.shotgunRecoilImpulse = std::max(0.0f, config.shotgunRecoilImpulse);
    config.shotgunRecoilVerticalBonus = std::max(0.0f, config.shotgunRecoilVerticalBonus);
    config.rocketJumpImpulse = std::max(0.0f, config.rocketJumpImpulse);
    config.rocketJumpRadius = std::max(0.0f, config.rocketJumpRadius);
    config.enemyShotDamage = std::max(0.0f, config.enemyShotDamage);
    config.enemyShotSpeed = std::max(0.0f, config.enemyShotSpeed);
    config.spitterFireInterval = std::max(0.1f, config.spitterFireInterval);
    config.pouncerLeapInterval = std::max(0.2f, config.pouncerLeapInterval);
    config.pouncerLeapSpeed = std::max(0.0f, config.pouncerLeapSpeed);
    config.bossSpawnTime = std::max(5.0f, config.bossSpawnTime);
    config.bossHealth = std::max(1.0f, config.bossHealth);
    config.duelistHealth = std::max(1.0f, config.duelistHealth);
    config.duelistWeaponSwitchMin = std::max(0.4f, config.duelistWeaponSwitchMin);
    config.duelistWeaponSwitchMax = std::max(config.duelistWeaponSwitchMin, config.duelistWeaponSwitchMax);
    config.duelistFireRateScale = std::max(0.1f, config.duelistFireRateScale);
    config.duelPlayerArmor = std::max(0, config.duelPlayerArmor);
    config.duelArmorHitInvuln = std::max(0.0f, config.duelArmorHitInvuln);
    config.harrierFireInterval = std::max(0.15f, config.harrierFireInterval);
    config.harrierSpeed = std::max(0.0f, config.harrierSpeed);
    config.harrierTargetHeight = std::max(1.0f, config.harrierTargetHeight);
    config.blinkerWindup = std::max(0.05f, config.blinkerWindup);
    config.blinkerCooldown = std::max(0.2f, config.blinkerCooldown);
    config.blinkerDashSpeed = std::max(0.0f, config.blinkerDashSpeed);
    config.asteroidRadius = std::max(8.0f, config.asteroidRadius);
    config.asteroidPlayerAltitude = std::max(0.8f, config.asteroidPlayerAltitude);
    config.asteroidEnemyAltitude = std::max(0.2f, config.asteroidEnemyAltitude);
    config.asteroidCleanupDistance = std::max(config.asteroidRadius + 8.0f, config.asteroidCleanupDistance);
    config.asteroidPlatformAltitude = std::max(0.5f, config.asteroidPlatformAltitude);
    config.gravityNailDamage = std::max(0.0f, config.gravityNailDamage);
    config.gravityWellRadius = std::max(0.1f, config.gravityWellRadius);
    config.gravityWellForce = std::max(0.0f, config.gravityWellForce);
    config.gravityWellLifetime = std::max(0.1f, config.gravityWellLifetime);
    config.blackHoleGrenadeDamage = std::max(0.0f, config.blackHoleGrenadeDamage);
    config.blackHoleRadius = std::max(0.1f, config.blackHoleRadius);
    config.blackHoleForce = std::max(0.0f, config.blackHoleForce);
    config.blackHoleLifetime = std::max(0.1f, config.blackHoleLifetime);
    config.blackHoleEventHorizonRadius = std::max(0.1f, config.blackHoleEventHorizonRadius);
    config.heatwaveDamage = std::max(0.0f, config.heatwaveDamage);
    config.heatwaveForce = std::max(0.0f, config.heatwaveForce);
    config.heatwaveRange = std::max(0.1f, config.heatwaveRange);
    config.glassShardDamage = std::max(0.0f, config.glassShardDamage);
    config.glassShardSpeed = std::max(0.0f, config.glassShardSpeed);
    config.glassShardRecoilScale = std::max(0.0f, config.glassShardRecoilScale);
    config.recoilLanceDamage = std::max(0.0f, config.recoilLanceDamage);
    config.recoilLanceSpeed = std::max(0.0f, config.recoilLanceSpeed);
    config.recoilLanceImpulse = std::max(0.0f, config.recoilLanceImpulse);
    config.riftCutterDamage = std::max(0.0f, config.riftCutterDamage);
    config.riftCutterRange = std::max(0.1f, config.riftCutterRange);
    config.riftCutterWidth = std::max(0.05f, config.riftCutterWidth);
    config.riftCutterLifetime = std::max(0.05f, config.riftCutterLifetime);
    config.riftCutterDelay = std::max(0.0f, config.riftCutterDelay);
    config.riftCutterRadius = std::max(0.2f, config.riftCutterRadius);
    config.riftCutterThickness = std::max(0.05f, config.riftCutterThickness);
    config.riftCutterPlaneThickness = std::max(0.05f, config.riftCutterPlaneThickness);
    config.riftCutterWaveSpeed = std::max(0.0f, config.riftCutterWaveSpeed);
    config.riftCutterWaveSpawnDistance = std::max(0.0f, config.riftCutterWaveSpawnDistance);
    config.riftPlatformRange = std::max(0.5f, config.riftPlatformRange);
    config.riftPlatformDelay = std::max(0.0f, config.riftPlatformDelay);
    config.riftPlatformLifetime = std::max(0.1f, config.riftPlatformLifetime);
    config.riftPlatformSize = std::max(1.0f, config.riftPlatformSize);
    config.riftPlatformThickness = std::max(0.1f, config.riftPlatformThickness);
    config.blinkDistance = std::max(0.0f, config.blinkDistance);
    config.blinkClearRadius = std::max(0.1f, config.blinkClearRadius);
    return config;
}
