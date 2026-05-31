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
        {"circle_radius", &config.circleRadius},
        {"gravity", &config.gravity},
        {"space_suit_gravity_scale", &config.spaceSuitGravityScale},
        {"walk_speed", &config.walkSpeed},
        {"run_speed", &config.runSpeed},
        {"ground_acceleration", &config.groundAcceleration},
        {"air_acceleration", &config.airAcceleration},
        {"jump_speed", &config.jumpSpeed},
        {"flight_hover_strength", &config.flightHoverStrength},
        {"flight_hover_damping", &config.flightHoverDamping},
        {"flight_vertical_speed", &config.flightVerticalSpeed},
        {"flight_min_altitude", &config.flightMinAltitude},
        {"flight_max_altitude", &config.flightMaxAltitude},
        {"skates_ground_friction", &config.skatesGroundFriction},
        {"skates_air_control", &config.skatesAirControl},
        {"skates_max_speed_bonus", &config.skatesMaxSpeedBonus},
        {"plasma_damage", &config.plasmaDamage},
        {"plasma_speed", &config.plasmaSpeed},
        {"plasma_lifetime", &config.plasmaLifetime},
        {"plasma_cooldown", &config.plasmaCooldown},
        {"plasma_radius", &config.plasmaRadius},
        {"laser_base_damage", &config.laserBaseDamage},
        {"laser_charge_damage", &config.laserChargeDamage},
        {"laser_beam_radius", &config.laserBeamRadius},
        {"laser_charge_rate", &config.laserChargeRate},
        {"laser_beam_range", &config.laserBeamRange},
        {"laser_beam_lifetime", &config.laserBeamLifetime},
        {"laser_beam_lifetime_charge_bonus", &config.laserBeamLifetimeChargeBonus},
        {"laser_beam_radius_charge_bonus", &config.laserBeamRadiusChargeBonus},
        {"laser_beam_cooldown", &config.laserBeamCooldown},
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
        {"slime_king_spawn_time", &config.slimeKingSpawnTime},
        {"slime_king_health", &config.slimeKingHealth},
        {"slime_king_radius", &config.slimeKingRadius},
        {"slime_king_speed", &config.slimeKingSpeed},
        {"slime_king_long_jump_speed", &config.slimeKingLongJumpSpeed},
        {"slime_king_high_jump_speed", &config.slimeKingHighJumpSpeed},
        {"slime_king_slam_speed", &config.slimeKingSlamSpeed},
        {"slime_king_slam_radius", &config.slimeKingSlamRadius},
        {"slime_king_slam_damage", &config.slimeKingSlamDamage},
        {"slime_king_slam_range", &config.slimeKingSlamRange},
        {"slime_king_shoot_speed", &config.slimeKingShootSpeed},
        {"slime_king_shoot_interval", &config.slimeKingShootInterval},
        {"slime_king_cooldown", &config.slimeKingCooldown},
        {"slime_king_min_health", &config.slimeKingMinHealth},
        {"slime_king_child_scale", &config.slimeKingChildScale},
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
        {"hollow_world_radius", &config.hollowWorldRadius},
        {"hollow_world_player_altitude", &config.hollowWorldPlayerAltitude},
        {"hollow_world_enemy_altitude", &config.hollowWorldEnemyAltitude},
        {"hollow_world_cleanup_distance", &config.hollowWorldCleanupDistance},
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
        {"glass_shard_linger_time", &config.glassShardLingerTime},
        {"glass_shard_drag", &config.glassShardDrag},
        {"glass_shard_linger_height", &config.glassShardLingerHeight},
        {"glass_shard_cloud_radius", &config.glassShardCloudRadius},
        {"glass_shard_separation_radius", &config.glassShardSeparationRadius},
        {"glass_shard_center_force", &config.glassShardCenterForce},
        {"glass_shard_cloud_form_time", &config.glassShardCloudFormTime},
        {"recoil_lance_damage", &config.recoilLanceDamage},
        {"recoil_lance_speed", &config.recoilLanceSpeed},
        {"recoil_lance_impulse", &config.recoilLanceImpulse},
        {"recoil_lance_thrust_damage", &config.recoilLanceThrustDamage},
        {"recoil_lance_thrust_force", &config.recoilLanceThrustForce},
        {"recoil_lance_thrust_range", &config.recoilLanceThrustRange},
        {"recoil_lance_thrust_impulse", &config.recoilLanceThrustImpulse},
        {"recoil_lance_shockwave_damage", &config.recoilLanceShockwaveDamage},
        {"recoil_lance_shockwave_force", &config.recoilLanceShockwaveForce},
        {"recoil_lance_shockwave_radius", &config.recoilLanceShockwaveRadius},
        {"nano_blade_damage", &config.nanoBladeDamage},
        {"nano_blade_range", &config.nanoBladeRange},
        {"nano_blade_width", &config.nanoBladeWidth},
        {"nano_blade_lifetime", &config.nanoBladeLifetime},
        {"nano_blade_delay", &config.nanoBladeDelay},
        {"nano_blade_radius", &config.nanoBladeRadius},
        {"nano_blade_thickness", &config.nanoBladeThickness},
        {"nano_blade_plane_thickness", &config.nanoBladePlaneThickness},
        {"nano_blade_wave_speed", &config.nanoBladeWaveSpeed},
        {"nano_blade_wave_spawn_distance", &config.nanoBladeWaveSpawnDistance},
        {"nano_platform_range", &config.nanoPlatformRange},
        {"nano_platform_delay", &config.nanoPlatformDelay},
        {"nano_platform_lifetime", &config.nanoPlatformLifetime},
        {"nano_platform_length", &config.nanoPlatformLength},
        {"nano_platform_width", &config.nanoPlatformWidth},
        {"nano_platform_height", &config.nanoPlatformHeight},
        {"blink_distance", &config.blinkDistance},
        {"blink_clear_radius", &config.blinkClearRadius},
        {"blink_distance_min", &config.blinkDistanceMin},
        {"blink_distance_max", &config.blinkDistanceMax},
        {"drone_canister_speed", &config.droneCanisterSpeed},
        {"drone_canister_gravity", &config.droneCanisterGravity},
        {"drone_lifetime", &config.droneLifetime},
        {"drone_deploy_time", &config.droneDeployTime},
        {"drone_hover_altitude", &config.droneHoverAltitude},
        {"drone_move_speed", &config.droneMoveSpeed},
        {"drone_bullet_damage", &config.droneBulletDamage},
        {"drone_bullet_speed", &config.droneBulletSpeed},
        {"drone_shoot_interval", &config.droneShootInterval},
        {"drone_shoot_range", &config.droneShootRange},
        {"drone_rocket_interval", &config.droneRocketInterval},
        {"drone_rocket_range", &config.droneRocketRange},
        {"drone_separation_radius", &config.droneSeparationRadius},
        {"drone_separation_force", &config.droneSeparationForce},
        {"drone_flocking_radius", &config.droneFlockingRadius},
        {"drone_flocking_force", &config.droneFlockingForce},
        {"drone_rally_hold_time", &config.droneRallyHoldTime},
        {"drone_rally_marker_altitude", &config.droneRallyMarkerAltitude},
        {"bethlehem_spawn_time", &config.bethlehemSpawnTime},
        {"bethlehem_health", &config.bethlehemHealth},
        {"bethlehem_orbit_radius", &config.bethlehemOrbitRadius},
        {"bethlehem_orbit_period", &config.bethlehemOrbitPeriod},
        {"bethlehem_orbit_altitude", &config.bethlehemOrbitAltitude},
        {"bethlehem_laser_warning_duration", &config.bethlehemLaserWarningDuration},
        {"bethlehem_laser_duration", &config.bethlehemLaserDuration},
        {"bethlehem_laser_cooldown", &config.bethlehemLaserCooldown},
        {"bethlehem_laser_radius", &config.bethlehemLaserRadius},
        {"bethlehem_laser_range", &config.bethlehemLaserRange},
        {"bethlehem_laser_damage", &config.bethlehemLaserDamage},
        {"bethlehem_laser_rotate_speed", &config.bethlehemLaserRotateSpeed},
        {"dummy_health", &config.dummyHealth},
        {"dummy_spawn_interval", &config.dummySpawnInterval},
        {"dummy_boss_spawn_time", &config.dummyBossSpawnTime},
        {"dummy_bethlehem_spawn_time", &config.dummyBethlehemSpawnTime},
        {"boss_homing_turn_rate", &config.bossHomingTurnRate},
        {"boss_homing_burst_interval", &config.bossHomingBurstInterval},
        {"boss_homing_life", &config.bossHomingLife},
        {"boss_homing_speed_scale", &config.bossHomingSpeedScale},
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
        } else if (key == "shotgun_pellet_count") {
            int parsed = 9;
            if (ParseInt(value, parsed)) { config.shotgunPelletCount = parsed; }
        } else if (key == "shotgun_shard_count") {
            int parsed = 5;
            if (ParseInt(value, parsed)) { config.shotgunShardCount = parsed; }
        } else if (key == "drone_max_count") {
            int parsed = 3;
            if (ParseInt(value, parsed)) {
                config.droneMaxCount = parsed;
            }
        } else if (key == "dummy_max_count") {
            int parsed = 6;
            if (ParseInt(value, parsed)) {
                config.dummyMaxCount = parsed;
            }
        } else if (key == "boss_homing_burst_count") {
            int parsed = 3;
            if (ParseInt(value, parsed)) {
                config.bossHomingBurstCount = parsed;
            }
        } else if (key == "slime_king_shoot_count") {
            int parsed = 6;
            if (ParseInt(value, parsed)) { config.slimeKingShootCount = parsed; }
        } else if (key == "slime_king_split_count") {
            int parsed = 4;
            if (ParseInt(value, parsed)) { config.slimeKingSplitCount = parsed; }
        } else if (key == "slime_king_max_generations") {
            int parsed = 2;
            if (ParseInt(value, parsed)) { config.slimeKingMaxGenerations = parsed; }
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
        } else if (key == "boss_rush_mode") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.bossRushMode = parsed;
            }
        }
    }

    config.circleRadius = std::max(6.0f, config.circleRadius);
    config.gravity = std::max(0.0f, config.gravity);
    config.spaceSuitGravityScale = std::max(0.0f, config.spaceSuitGravityScale);
    config.walkSpeed = std::max(0.0f, config.walkSpeed);
    config.runSpeed = std::max(0.0f, config.runSpeed);
    config.groundAcceleration = std::max(0.0f, config.groundAcceleration);
    config.airAcceleration = std::max(0.0f, config.airAcceleration);
    config.jumpSpeed = std::max(0.0f, config.jumpSpeed);
    config.flightHoverStrength = std::max(0.0f, config.flightHoverStrength);
    config.flightHoverDamping = std::max(0.0f, config.flightHoverDamping);
    config.flightVerticalSpeed = std::max(0.0f, config.flightVerticalSpeed);
    config.flightMinAltitude = std::max(0.0f, config.flightMinAltitude);
    config.flightMaxAltitude = std::max(config.flightMinAltitude, config.flightMaxAltitude);
    config.skatesGroundFriction = std::clamp(config.skatesGroundFriction, 0.0f, 1.0f);
    config.skatesAirControl = std::clamp(config.skatesAirControl, 0.0f, 1.0f);
    config.skatesMaxSpeedBonus = std::max(1.0f, config.skatesMaxSpeedBonus);
    config.plasmaDamage = std::max(0.0f, config.plasmaDamage);
    config.plasmaSpeed = std::max(1.0f, config.plasmaSpeed);
    config.plasmaLifetime = std::max(0.1f, config.plasmaLifetime);
    config.plasmaCooldown = std::max(0.01f, config.plasmaCooldown);
    config.plasmaRadius = std::max(0.02f, config.plasmaRadius);
    config.laserBaseDamage = std::max(0.0f, config.laserBaseDamage);
    config.laserChargeDamage = std::max(0.0f, config.laserChargeDamage);
    config.laserBeamRadius = std::max(0.05f, config.laserBeamRadius);
    config.laserChargeRate = std::max(0.1f, config.laserChargeRate);
    config.laserBeamRange = std::max(1.0f, config.laserBeamRange);
    config.laserBeamLifetime = std::max(0.02f, config.laserBeamLifetime);
    config.laserBeamLifetimeChargeBonus = std::max(0.0f, config.laserBeamLifetimeChargeBonus);
    config.laserBeamRadiusChargeBonus = std::max(0.0f, config.laserBeamRadiusChargeBonus);
    config.laserBeamCooldown = std::max(0.01f, config.laserBeamCooldown);
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
    config.slimeKingSpawnTime = std::max(10.0f, config.slimeKingSpawnTime);
    config.slimeKingHealth = std::max(50.0f, config.slimeKingHealth);
    config.slimeKingRadius = std::max(0.5f, config.slimeKingRadius);
    config.slimeKingSpeed = std::max(0.5f, config.slimeKingSpeed);
    config.slimeKingLongJumpSpeed = std::max(1.0f, config.slimeKingLongJumpSpeed);
    config.slimeKingHighJumpSpeed = std::max(1.0f, config.slimeKingHighJumpSpeed);
    config.slimeKingSlamSpeed = std::max(1.0f, config.slimeKingSlamSpeed);
    config.slimeKingSlamRadius = std::max(0.5f, config.slimeKingSlamRadius);
    config.slimeKingSlamDamage = std::max(0.0f, config.slimeKingSlamDamage);
    config.slimeKingSlamRange = std::max(1.0f, config.slimeKingSlamRange);
    config.slimeKingShootCount = std::max(1, config.slimeKingShootCount);
    config.slimeKingShootSpeed = std::max(1.0f, config.slimeKingShootSpeed);
    config.slimeKingShootInterval = std::max(0.05f, config.slimeKingShootInterval);
    config.slimeKingCooldown = std::max(0.1f, config.slimeKingCooldown);
    config.slimeKingSplitCount = std::max(1, config.slimeKingSplitCount);
    config.slimeKingMaxGenerations = std::max(0, config.slimeKingMaxGenerations);
    config.slimeKingMinHealth = std::max(1.0f, config.slimeKingMinHealth);
    config.slimeKingChildScale = std::clamp(config.slimeKingChildScale, 0.1f, 1.0f);
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
    config.hollowWorldRadius = std::max(8.0f, config.hollowWorldRadius);
    config.hollowWorldPlayerAltitude = std::clamp(config.hollowWorldPlayerAltitude, 0.8f, config.hollowWorldRadius - 2.0f);
    config.hollowWorldEnemyAltitude = std::clamp(config.hollowWorldEnemyAltitude, 0.2f, config.hollowWorldRadius - 2.0f);
    config.hollowWorldCleanupDistance = std::max(config.hollowWorldRadius + 8.0f, config.hollowWorldCleanupDistance);
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
    config.glassShardLingerTime = std::max(0.0f, config.glassShardLingerTime);
    config.glassShardDrag = std::clamp(config.glassShardDrag, 0.0f, 20.0f);
    config.glassShardLingerHeight = std::max(0.1f, config.glassShardLingerHeight);
    config.glassShardCloudRadius = std::max(0.3f, config.glassShardCloudRadius);
    config.glassShardSeparationRadius = std::max(0.1f, config.glassShardSeparationRadius);
    config.glassShardCenterForce = std::max(0.0f, config.glassShardCenterForce);
    config.glassShardCloudFormTime = std::max(0.0f, config.glassShardCloudFormTime);
    config.recoilLanceDamage = std::max(0.0f, config.recoilLanceDamage);
    config.recoilLanceSpeed = std::max(0.0f, config.recoilLanceSpeed);
    config.recoilLanceImpulse = std::max(0.0f, config.recoilLanceImpulse);
    config.recoilLanceThrustDamage = std::max(0.0f, config.recoilLanceThrustDamage);
    config.recoilLanceThrustForce = std::max(0.0f, config.recoilLanceThrustForce);
    config.recoilLanceThrustRange = std::max(0.5f, config.recoilLanceThrustRange);
    config.recoilLanceThrustImpulse = std::max(0.0f, config.recoilLanceThrustImpulse);
    config.recoilLanceShockwaveDamage = std::max(0.0f, config.recoilLanceShockwaveDamage);
    config.recoilLanceShockwaveForce = std::max(0.0f, config.recoilLanceShockwaveForce);
    config.recoilLanceShockwaveRadius = std::max(0.1f, config.recoilLanceShockwaveRadius);
    config.nanoBladeDamage = std::max(0.0f, config.nanoBladeDamage);
    config.nanoBladeRange = std::max(0.1f, config.nanoBladeRange);
    config.nanoBladeWidth = std::max(0.05f, config.nanoBladeWidth);
    config.nanoBladeLifetime = std::max(0.05f, config.nanoBladeLifetime);
    config.nanoBladeDelay = std::max(0.0f, config.nanoBladeDelay);
    config.nanoBladeRadius = std::max(0.2f, config.nanoBladeRadius);
    config.nanoBladeThickness = std::max(0.05f, config.nanoBladeThickness);
    config.nanoBladePlaneThickness = std::max(0.05f, config.nanoBladePlaneThickness);
    config.nanoBladeWaveSpeed = std::max(0.0f, config.nanoBladeWaveSpeed);
    config.nanoBladeWaveSpawnDistance = std::max(0.0f, config.nanoBladeWaveSpawnDistance);
    config.nanoPlatformRange = std::max(0.5f, config.nanoPlatformRange);
    config.nanoPlatformDelay = std::max(0.0f, config.nanoPlatformDelay);
    config.nanoPlatformLifetime = std::max(0.1f, config.nanoPlatformLifetime);
    config.nanoPlatformLength = std::max(1.0f, config.nanoPlatformLength);
    config.nanoPlatformWidth = std::max(1.0f, config.nanoPlatformWidth);
    config.nanoPlatformHeight = std::max(0.1f, config.nanoPlatformHeight);
    config.blinkDistance = std::max(0.0f, config.blinkDistance);
    config.blinkClearRadius = std::max(0.1f, config.blinkClearRadius);
    config.blinkDistanceMin = std::max(0.1f, config.blinkDistanceMin);
    config.blinkDistanceMax = std::max(config.blinkDistanceMin, config.blinkDistanceMax);
    config.droneCanisterSpeed = std::max(6.0f, config.droneCanisterSpeed);
    config.droneCanisterGravity = std::clamp(config.droneCanisterGravity, 0.0f, 1.5f);
    config.shotgunPelletCount = std::max(1, config.shotgunPelletCount);
    config.shotgunShardCount = std::max(1, config.shotgunShardCount);
    config.droneMaxCount = std::max(1, config.droneMaxCount);
    config.droneLifetime = std::max(5.0f, config.droneLifetime);
    config.droneDeployTime = std::max(0.2f, config.droneDeployTime);
    config.droneHoverAltitude = std::clamp(config.droneHoverAltitude, 1.0f, 20.0f);
    config.droneMoveSpeed = std::max(1.0f, config.droneMoveSpeed);
    config.droneBulletDamage = std::max(0.0f, config.droneBulletDamage);
    config.droneBulletSpeed = std::max(20.0f, config.droneBulletSpeed);
    config.droneShootInterval = std::max(0.02f, config.droneShootInterval);
    config.droneShootRange = std::max(3.0f, config.droneShootRange);
    config.droneRocketInterval = std::max(0.5f, config.droneRocketInterval);
    config.droneRocketRange = std::max(5.0f, config.droneRocketRange);
    config.droneSeparationRadius = std::max(0.5f, config.droneSeparationRadius);
    config.droneSeparationForce = std::max(0.0f, config.droneSeparationForce);
    config.droneFlockingRadius = std::max(1.0f, config.droneFlockingRadius);
    config.droneFlockingForce = std::max(0.0f, config.droneFlockingForce);
    config.droneRallyHoldTime = std::max(0.5f, config.droneRallyHoldTime);
    config.droneRallyMarkerAltitude = std::clamp(config.droneRallyMarkerAltitude, 0.2f, 20.0f);
    config.bethlehemSpawnTime = std::max(5.0f, config.bethlehemSpawnTime);
    config.bethlehemHealth = std::max(1.0f, config.bethlehemHealth);
    config.bethlehemOrbitRadius = std::max(config.asteroidRadius + 2.0f, config.bethlehemOrbitRadius);
    config.bethlehemOrbitPeriod = std::max(1.0f, config.bethlehemOrbitPeriod);
    config.bethlehemOrbitAltitude = std::max(0.5f, config.bethlehemOrbitAltitude);
    config.bethlehemLaserWarningDuration = std::max(0.5f, config.bethlehemLaserWarningDuration);
    config.bethlehemLaserDuration = std::max(0.5f, config.bethlehemLaserDuration);
    config.bethlehemLaserCooldown = std::max(0.5f, config.bethlehemLaserCooldown);
    config.bethlehemLaserRadius = std::max(0.2f, config.bethlehemLaserRadius);
    config.bethlehemLaserRange = std::max(2.0f, config.bethlehemLaserRange);
    config.bethlehemLaserDamage = std::max(0.0f, config.bethlehemLaserDamage);
    config.bethlehemLaserRotateSpeed = std::max(0.0f, config.bethlehemLaserRotateSpeed);
    config.dummyHealth = std::max(1.0f, config.dummyHealth);
    config.dummySpawnInterval = std::max(0.5f, config.dummySpawnInterval);
    config.dummyBossSpawnTime = std::max(1.0f, config.dummyBossSpawnTime);
    config.dummyBethlehemSpawnTime = std::max(1.0f, config.dummyBethlehemSpawnTime);
    config.dummyMaxCount = std::max(0, config.dummyMaxCount);
    config.bossHomingTurnRate = std::clamp(config.bossHomingTurnRate, 0.5f, 12.0f);
    config.bossHomingBurstCount = std::clamp(config.bossHomingBurstCount, 1, 6);
    config.bossHomingBurstInterval = std::clamp(config.bossHomingBurstInterval, 0.05f, 1.0f);
    config.bossHomingLife = std::max(1.0f, config.bossHomingLife);
    config.bossHomingSpeedScale = std::clamp(config.bossHomingSpeedScale, 0.1f, 2.0f);
    return config;
}
