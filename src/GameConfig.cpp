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

std::unordered_map<std::string, float*> GameplayConfig::FloatMap() {
    return std::unordered_map<std::string, float*>{
        {"circle_radius", &circleRadius},
        {"gravity", &gravity},
        {"space_suit_gravity_scale", &spaceSuitGravityScale},
        {"walk_speed", &walkSpeed},
        {"run_speed", &runSpeed},
        {"ground_acceleration", &groundAcceleration},
        {"air_acceleration", &airAcceleration},
        {"jump_speed", &jumpSpeed},
        {"flight_hover_strength", &flightHoverStrength},
        {"flight_hover_damping", &flightHoverDamping},
        {"flight_vertical_speed", &flightVerticalSpeed},
        {"flight_min_altitude", &flightMinAltitude},
        {"flight_max_altitude", &flightMaxAltitude},
        {"skates_ground_friction", &skatesGroundFriction},
        {"skates_air_control", &skatesAirControl},
        {"skates_max_speed_bonus", &skatesMaxSpeedBonus},
        {"plasma_damage", &plasmaDamage},
        {"plasma_speed", &plasmaSpeed},
        {"plasma_lifetime", &plasmaLifetime},
        {"plasma_cooldown", &plasmaCooldown},
        {"plasma_radius", &plasmaRadius},
        {"laser_base_damage", &laserBaseDamage},
        {"laser_charge_damage", &laserChargeDamage},
        {"laser_beam_radius", &laserBeamRadius},
        {"laser_charge_rate", &laserChargeRate},
        {"laser_beam_range", &laserBeamRange},
        {"laser_beam_lifetime", &laserBeamLifetime},
        {"laser_beam_lifetime_charge_bonus", &laserBeamLifetimeChargeBonus},
        {"laser_beam_radius_charge_bonus", &laserBeamRadiusChargeBonus},
        {"laser_beam_cooldown", &laserBeamCooldown},
        {"flame_damage", &flameDamage},
        {"flame_lifetime", &flameLifetime},
        {"flame_max_radius", &flameMaxRadius},
        {"rocket_impact_damage", &rocketImpactDamage},
        {"rocket_explosion_damage", &rocketExplosionDamage},
        {"rocket_explosion_radius", &rocketExplosionRadius},
        {"shotgun_pellet_damage", &shotgunPelletDamage},
        {"shotgun_recoil_impulse", &shotgunRecoilImpulse},
        {"shotgun_recoil_vertical_bonus", &shotgunRecoilVerticalBonus},
        {"rocket_jump_impulse", &rocketJumpImpulse},
        {"rocket_jump_radius", &rocketJumpRadius},
        {"enemy_shot_damage", &enemyShotDamage},
        {"enemy_shot_speed", &enemyShotSpeed},
        {"spitter_fire_interval", &spitterFireInterval},
        {"pouncer_leap_interval", &pouncerLeapInterval},
        {"pouncer_leap_speed", &pouncerLeapSpeed},
        {"boss_spawn_time", &bossSpawnTime},
        {"boss_health", &bossHealth},
        {"slime_king_spawn_time", &slimeKingSpawnTime},
        {"slime_king_health", &slimeKingHealth},
        {"slime_king_radius", &slimeKingRadius},
        {"slime_king_speed", &slimeKingSpeed},
        {"slime_king_long_jump_speed", &slimeKingLongJumpSpeed},
        {"slime_king_high_jump_speed", &slimeKingHighJumpSpeed},
        {"slime_king_slam_speed", &slimeKingSlamSpeed},
        {"slime_king_slam_radius", &slimeKingSlamRadius},
        {"slime_king_slam_damage", &slimeKingSlamDamage},
        {"slime_king_slam_range", &slimeKingSlamRange},
        {"slime_king_shoot_speed", &slimeKingShootSpeed},
        {"slime_king_shoot_interval", &slimeKingShootInterval},
        {"slime_king_cooldown", &slimeKingCooldown},
        {"slime_king_min_health", &slimeKingMinHealth},
        {"slime_king_child_scale", &slimeKingChildScale},
        {"slime_king_spherical_gravity", &slimeKingSphericalGravity},
        {"slime_king_surface_damping", &slimeKingSurfaceDamping},
        {"duelist_health", &duelistHealth},
        {"duelist_weapon_switch_min", &duelistWeaponSwitchMin},
        {"duelist_weapon_switch_max", &duelistWeaponSwitchMax},
        {"duelist_fire_rate_scale", &duelistFireRateScale},
        {"duel_armor_hit_invuln", &duelArmorHitInvuln},
        {"harrier_fire_interval", &harrierFireInterval},
        {"harrier_speed", &harrierSpeed},
        {"harrier_target_height", &harrierTargetHeight},
        {"blinker_windup", &blinkerWindup},
        {"blinker_cooldown", &blinkerCooldown},
        {"blinker_dash_speed", &blinkerDashSpeed},
        {"asteroid_radius", &asteroidRadius},
        {"asteroid_player_altitude", &asteroidPlayerAltitude},
        {"asteroid_enemy_altitude", &asteroidEnemyAltitude},
        {"asteroid_cleanup_distance", &asteroidCleanupDistance},
        {"hollow_world_radius", &hollowWorldRadius},
        {"hollow_world_player_altitude", &hollowWorldPlayerAltitude},
        {"hollow_world_enemy_altitude", &hollowWorldEnemyAltitude},
        {"hollow_world_cleanup_distance", &hollowWorldCleanupDistance},
        {"gravity_nail_damage", &gravityNailDamage},
        {"gravity_well_radius", &gravityWellRadius},
        {"gravity_well_force", &gravityWellForce},
        {"gravity_well_lifetime", &gravityWellLifetime},
        {"black_hole_grenade_damage", &blackHoleGrenadeDamage},
        {"black_hole_radius", &blackHoleRadius},
        {"black_hole_force", &blackHoleForce},
        {"black_hole_lifetime", &blackHoleLifetime},
        {"black_hole_event_horizon_radius", &blackHoleEventHorizonRadius},
        {"heatwave_damage", &heatwaveDamage},
        {"heatwave_force", &heatwaveForce},
        {"heatwave_range", &heatwaveRange},
        {"glass_shard_damage", &glassShardDamage},
        {"glass_shard_speed", &glassShardSpeed},
        {"glass_shard_recoil_scale", &glassShardRecoilScale},
        {"glass_shard_linger_time", &glassShardLingerTime},
        {"glass_shard_drag", &glassShardDrag},
        {"glass_shard_linger_height", &glassShardLingerHeight},
        {"glass_shard_cloud_radius", &glassShardCloudRadius},
        {"glass_shard_separation_radius", &glassShardSeparationRadius},
        {"glass_shard_center_force", &glassShardCenterForce},
        {"glass_shard_cloud_form_time", &glassShardCloudFormTime},
        {"longinus_spear_damage", &longinusSpearDamage},
        {"longinus_spear_speed", &longinusSpearSpeed},
        {"longinus_spear_impulse", &longinusSpearImpulse},
        {"longinus_spear_thrust_damage", &longinusSpearThrustDamage},
        {"longinus_spear_thrust_force", &longinusSpearThrustForce},
        {"longinus_spear_thrust_range", &longinusSpearThrustRange},
        {"longinus_spear_thrust_impulse", &longinusSpearThrustImpulse},
        {"longinus_spear_shockwave_damage", &longinusSpearShockwaveDamage},
        {"longinus_spear_shockwave_force", &longinusSpearShockwaveForce},
        {"longinus_spear_shockwave_radius", &longinusSpearShockwaveRadius},
        {"longinus_spear_thrust_invuln", &longinusSpearThrustInvuln},
        {"nano_blade_damage", &nanoBladeDamage},
        {"nano_blade_range", &nanoBladeRange},
        {"nano_blade_width", &nanoBladeWidth},
        {"nano_blade_lifetime", &nanoBladeLifetime},
        {"nano_blade_delay", &nanoBladeDelay},
        {"nano_blade_radius", &nanoBladeRadius},
        {"nano_blade_thickness", &nanoBladeThickness},
        {"nano_blade_plane_thickness", &nanoBladePlaneThickness},
        {"nano_blade_wave_speed", &nanoBladeWaveSpeed},
        {"nano_blade_wave_spawn_distance", &nanoBladeWaveSpawnDistance},
        {"nano_platform_range", &nanoPlatformRange},
        {"nano_platform_delay", &nanoPlatformDelay},
        {"nano_platform_lifetime", &nanoPlatformLifetime},
        {"nano_platform_length", &nanoPlatformLength},
        {"nano_platform_width", &nanoPlatformWidth},
        {"nano_platform_height", &nanoPlatformHeight},
        {"blink_distance", &blinkDistance},
        {"blink_clear_radius", &blinkClearRadius},
        {"blink_distance_min", &blinkDistanceMin},
        {"blink_distance_max", &blinkDistanceMax},
        {"drone_canister_speed", &droneCanisterSpeed},
        {"drone_canister_gravity", &droneCanisterGravity},
        {"drone_lifetime", &droneLifetime},
        {"drone_deploy_time", &droneDeployTime},
        {"drone_hover_altitude", &droneHoverAltitude},
        {"drone_move_speed", &droneMoveSpeed},
        {"drone_bullet_damage", &droneBulletDamage},
        {"drone_bullet_speed", &droneBulletSpeed},
        {"drone_shoot_interval", &droneShootInterval},
        {"drone_shoot_range", &droneShootRange},
        {"drone_rocket_interval", &droneRocketInterval},
        {"drone_rocket_range", &droneRocketRange},
        {"drone_separation_radius", &droneSeparationRadius},
        {"drone_separation_force", &droneSeparationForce},
        {"drone_flocking_radius", &droneFlockingRadius},
        {"drone_flocking_force", &droneFlockingForce},
        {"drone_rally_hold_time", &droneRallyHoldTime},
        {"drone_rally_marker_altitude", &droneRallyMarkerAltitude},
        {"bethlehem_spawn_time", &bethlehemSpawnTime},
        {"bethlehem_health", &bethlehemHealth},
        {"bethlehem_orbit_radius", &bethlehemOrbitRadius},
        {"bethlehem_orbit_period", &bethlehemOrbitPeriod},
        {"bethlehem_orbit_altitude", &bethlehemOrbitAltitude},
        {"bethlehem_laser_warning_duration", &bethlehemLaserWarningDuration},
        {"bethlehem_laser_duration", &bethlehemLaserDuration},
        {"bethlehem_laser_cooldown", &bethlehemLaserCooldown},
        {"bethlehem_laser_radius", &bethlehemLaserRadius},
        {"bethlehem_laser_range", &bethlehemLaserRange},
        {"bethlehem_laser_damage", &bethlehemLaserDamage},
        {"bethlehem_laser_rotate_speed", &bethlehemLaserRotateSpeed},
        {"dummy_health", &dummyHealth},
        {"dummy_spawn_interval", &dummySpawnInterval},
        {"essence_hit_invuln", &essenceHitInvuln},
        {"essence_respawn_time", &essenceRespawnTime},
        {"dummy_boss_spawn_time", &dummyBossSpawnTime},
        {"dummy_bethlehem_spawn_time", &dummyBethlehemSpawnTime},
        {"boss_homing_turn_rate", &bossHomingTurnRate},
        {"boss_homing_burst_interval", &bossHomingBurstInterval},
        {"boss_homing_life", &bossHomingLife},
        {"boss_homing_speed_scale", &bossHomingSpeedScale},
        {"curse_orb_direct_damage", &curseOrbDirectDamage},
        {"curse_orb_dps", &curseOrbDps},
        {"curse_orb_max_stack_mult", &curseOrbMaxStackMult},
        {"curse_orb_speed", &curseOrbSpeed},
        {"curse_orb_turn_rate", &curseOrbTurnRate},
        {"curse_orb_lifetime", &curseOrbLifetime},
        {"curse_orb_cooldown", &curseOrbCooldown},
        {"soul_orb_count", &soulOrbCount},
        {"soul_orb_damage_scale", &soulOrbDamageScale},
        {"soul_orb_speed", &soulOrbSpeed},
        {"soul_orb_turn_rate", &soulOrbTurnRate},
        {"soul_orb_lifetime", &soulOrbLifetime},
        {"mystic_staff_shield_radius", &mysticStaffShieldRadius},
        {"mystic_staff_shield_cooldown", &mysticStaffShieldCooldown},
        {"mystic_staff_shockwave_radius", &mysticStaffShockwaveRadius},
        {"mystic_staff_shockwave_force", &mysticStaffShockwaveForce},
        {"magic_circle_lifetime", &magicCircleLifetime},
        {"magic_circle_radius", &magicCircleRadius},
        {"magic_circle_fire_interval", &magicCircleFireInterval},
        {"magic_circle_fire_rate_mult", &magicCircleFireRateMult},
        {"magic_circle_homing_turn_rate", &magicCircleHomingTurnRate},
        {"wormhole_player_cooldown", &wormholePlayerCooldown},
        {"wormhole_enemy_cooldown", &wormholeEnemyCooldown},
        {"wormhole_trigger_radius", &wormholeTriggerRadius},
        {"wormhole_visual_radius", &wormholeVisualRadius},
    };
}

GameplayConfig LoadGameplayConfig(const char* path) {
    GameplayConfig config;
    std::ifstream file(path);
    if (!file.is_open()) return config;

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

        auto floats = config.FloatMap();
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
        } else if (key == "duelist_count") {
            int parsed = 1;
            if (ParseInt(value, parsed)) { config.duelistCount = parsed; }
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
        } else if (key == "starting_essence") {
            int parsed = 3;
            if (ParseInt(value, parsed)) { config.startingEssence = parsed; }
        } else if (key == "essence_max_on_map") {
            int parsed = 1;
            if (ParseInt(value, parsed)) { config.essenceMaxOnMap = parsed; }
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
        } else if (key == "duelist_smart_ai") {
            bool parsed = false;
            if (ParseBool(value, parsed)) { config.duelistSmartAi = parsed; }
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
    config.flameLifetime = std::max(0.05f, config.flameLifetime);
    config.flameMaxRadius = std::max(0.05f, config.flameMaxRadius);
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
    config.slimeKingSphericalGravity = std::clamp(config.slimeKingSphericalGravity, 0.0f, 60.0f);
    config.slimeKingSurfaceDamping = std::clamp(config.slimeKingSurfaceDamping, 0.3f, 0.99f);
    config.duelistHealth = std::max(1.0f, config.duelistHealth);
    config.duelistWeaponSwitchMin = std::max(0.4f, config.duelistWeaponSwitchMin);
    config.duelistWeaponSwitchMax = std::max(config.duelistWeaponSwitchMin, config.duelistWeaponSwitchMax);
    config.duelistFireRateScale = std::max(0.1f, config.duelistFireRateScale);
    config.duelistCount = std::clamp(config.duelistCount, 1, 10);
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
    config.longinusSpearDamage = std::max(0.0f, config.longinusSpearDamage);
    config.longinusSpearSpeed = std::max(0.0f, config.longinusSpearSpeed);
    config.longinusSpearImpulse = std::max(0.0f, config.longinusSpearImpulse);
    config.longinusSpearThrustDamage = std::max(0.0f, config.longinusSpearThrustDamage);
    config.longinusSpearThrustForce = std::max(0.0f, config.longinusSpearThrustForce);
    config.longinusSpearThrustRange = std::max(0.5f, config.longinusSpearThrustRange);
    config.longinusSpearThrustImpulse = std::max(0.0f, config.longinusSpearThrustImpulse);
    config.longinusSpearShockwaveDamage = std::max(0.0f, config.longinusSpearShockwaveDamage);
    config.longinusSpearShockwaveForce = std::max(0.0f, config.longinusSpearShockwaveForce);
    config.longinusSpearShockwaveRadius = std::max(0.1f, config.longinusSpearShockwaveRadius);
    config.longinusSpearThrustInvuln = std::max(0.0f, config.longinusSpearThrustInvuln);
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
    config.curseOrbDirectDamage = std::max(0.0f, config.curseOrbDirectDamage);
    config.curseOrbDps = std::max(0.0f, config.curseOrbDps);
    config.curseOrbMaxStackMult = std::max(1.0f, config.curseOrbMaxStackMult);
    config.curseOrbSpeed = std::max(1.0f, config.curseOrbSpeed);
    config.curseOrbTurnRate = std::clamp(config.curseOrbTurnRate, 0.5f, 12.0f);
    config.curseOrbLifetime = std::max(0.5f, config.curseOrbLifetime);
    config.curseOrbCooldown = std::max(0.02f, config.curseOrbCooldown);
    config.soulOrbCount = std::max(1.0f, config.soulOrbCount);
    config.soulOrbDamageScale = std::max(0.01f, config.soulOrbDamageScale);
    config.soulOrbSpeed = std::max(1.0f, config.soulOrbSpeed);
    config.soulOrbTurnRate = std::clamp(config.soulOrbTurnRate, 0.5f, 12.0f);
    config.soulOrbLifetime = std::max(0.5f, config.soulOrbLifetime);
    config.mysticStaffShieldRadius = std::max(0.5f, config.mysticStaffShieldRadius);
    config.mysticStaffShieldCooldown = std::max(0.1f, config.mysticStaffShieldCooldown);
    config.mysticStaffShockwaveRadius = std::max(0.3f, config.mysticStaffShockwaveRadius);
    config.mysticStaffShockwaveForce = std::max(0.0f, config.mysticStaffShockwaveForce);
    config.magicCircleLifetime = std::max(1.0f, config.magicCircleLifetime);
    config.magicCircleRadius = std::max(0.5f, config.magicCircleRadius);
    config.magicCircleFireInterval = std::max(0.02f, config.magicCircleFireInterval);
    config.magicCircleFireRateMult = std::max(0.1f, config.magicCircleFireRateMult);
    config.magicCircleHomingTurnRate = std::max(0.5f, config.magicCircleHomingTurnRate);
    config.wormholePlayerCooldown = std::max(0.1f, config.wormholePlayerCooldown);
    config.wormholeEnemyCooldown = std::max(0.1f, config.wormholeEnemyCooldown);
    config.wormholeTriggerRadius = std::max(0.25f, config.wormholeTriggerRadius);
    config.wormholeVisualRadius = std::max(0.25f, config.wormholeVisualRadius);
    config.startingEssence = std::max(0, config.startingEssence);
    config.essenceHitInvuln = std::max(0.1f, config.essenceHitInvuln);
    config.essenceRespawnTime = std::max(10.0f, config.essenceRespawnTime);
    config.essenceMaxOnMap = std::clamp(config.essenceMaxOnMap, 1, 10);
    return config;
}
