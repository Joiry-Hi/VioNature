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
        {"sfx_volume", &sfxVolume},
        {"bgm_volume", &bgmVolume},
        {"bgm_loop_gap", &bgmLoopGap},
        {"bgm_altitude_fade_start", &bgmAltitudeFadeStart},
        {"bgm_altitude_fade_end", &bgmAltitudeFadeEnd},
        {"bgm_altitude_min_volume", &bgmAltitudeMinVolume},
        {"bgm_back_world_volume", &bgmBackWorldVolume},
        {"heaven_falls_bgm_volume", &heavenFallsBgmVolume},
        {"throne_bgm_volume", &throneBgmVolume},
        {"seraph_bgm_volume", &seraphBgmVolume},
        {"ufo_bgm_volume", &ufoBgmVolume},
        {"ufo_hyperspace_bgm_volume", &ufoHyperspaceBgmVolume},
        {"ufo_arrival_altitude", &ufoArrivalAltitude},
        {"ufo_arrival_world_variance", &ufoArrivalWorldVariance},
        {"ufo_arrival_enemy_variance", &ufoArrivalEnemyVariance},
        {"circle_radius", &circleRadius},
        {"eden_play_radius", &edenPlayRadius},
        {"eden_map_radius", &edenMapRadius},
        {"eden_corrupted_fall_radius", &edenCorruptedFallRadius},
        {"eden_height_scale", &edenHeightScale},
        {"eden_height_epsilon", &edenHeightEpsilon},
        {"eden_spawn_min_radius", &edenSpawnMinRadius},
        {"eden_spawn_max_radius", &edenSpawnMaxRadius},
        {"eden_exit_fade_power", &edenExitFadePower},
        {"eden_center_gravity_min_scale", &edenCenterGravityMinScale},
        {"eden_center_gravity_radius", &edenCenterGravityRadius},
        {"eden_essence_respawn_interval", &edenEssenceRespawnInterval},
        {"eden_essence_altitude_min", &edenEssenceAltitudeMin},
        {"eden_essence_altitude_max", &edenEssenceAltitudeMax},
        {"ark_shift_speed_mult", &arkShiftSpeedMult},
        {"labyrinth_cell_size", &labyrinthCellSize},
        {"labyrinth_wall_height", &labyrinthWallHeight},
        {"labyrinth_wall_thickness", &labyrinthWallThickness},
        {"labyrinth_shift_interval", &labyrinthShiftInterval},
        {"labyrinth_shift_warning_time", &labyrinthShiftWarningTime},
        {"labyrinth_minotaur_spawn_delay", &labyrinthMinotaurSpawnDelay},
        {"labyrinth_minotaur_health", &labyrinthMinotaurHealth},
        {"labyrinth_minotaur_speed", &labyrinthMinotaurSpeed},
        {"labyrinth_minotaur_charge_interval", &labyrinthMinotaurChargeInterval},
        {"labyrinth_minotaur_charge_speed", &labyrinthMinotaurChargeSpeed},
        {"labyrinth_minotaur_contact_damage", &labyrinthMinotaurContactDamage},
        {"eden_forbidden_fruit_height_offset", &edenForbiddenFruitHeightOffset},
        {"eden_forbidden_fruit_radius", &edenForbiddenFruitRadius},
        {"eden_forbidden_fruit_absorb_range", &edenForbiddenFruitAbsorbRange},
        {"eden_forbidden_fruit_absorb_speed", &edenForbiddenFruitAbsorbSpeed},
        {"eden_forbidden_fruit_interact_range", &edenForbiddenFruitInteractRange},
        {"eden_forbidden_fruit_exile_impulse", &edenForbiddenFruitExileImpulse},
        {"eden_forbidden_fruit_exile_force_scale", &edenForbiddenFruitExileForceScale},
        {"eden_forbidden_fruit_shockwave_radius", &edenForbiddenFruitShockwaveRadius},
        {"eden_forbidden_fruit_apocalypse_speed", &edenForbiddenFruitApocalypseSpeed},
        {"eden_fall_out_fade_duration", &edenFallOutFadeDuration},
        {"eden_fire_rain_interval", &edenFireRainInterval},
        {"eden_fire_rain_spawn_height", &edenFireRainSpawnHeight},
        {"eden_fire_rain_speed", &edenFireRainSpeed},
        {"eden_fire_rain_radius", &edenFireRainRadius},
        {"eden_fire_rain_lifetime", &edenFireRainLifetime},
        {"eden_fire_rain_damage", &edenFireRainDamage},
        {"eden_fire_rain_fire_radius", &edenFireRainFireRadius},
        {"eden_fire_rain_fire_duration", &edenFireRainFireDuration},
        {"eden_fire_rain_fire_dps", &edenFireRainFireDps},
        {"eden_apocalypse_hit_invuln", &edenApocalypseHitInvuln},
        {"eden_guardian_approach_speed", &edenGuardianApproachSpeed},
        {"eden_guardian_attack_interval", &edenGuardianAttackInterval},
        {"eden_guardian_slash_speed", &edenGuardianSlashSpeed},
        {"eden_guardian_slash_lifetime", &edenGuardianSlashLifetime},
        {"eden_guardian_slash_radius", &edenGuardianSlashRadius},
        {"eden_guardian_slash_thickness", &edenGuardianSlashThickness},
        {"eden_guardian_slash_plane_thickness", &edenGuardianSlashPlaneThickness},
        {"war_rider_spawn_time", &warRiderSpawnTime},
        {"war_rider_health", &warRiderHealth},
        {"war_rider_move_speed", &warRiderMoveSpeed},
        {"war_rider_charge_speed", &warRiderChargeSpeed},
        {"war_rider_charge_interval", &warRiderChargeInterval},
        {"war_rider_charge_duration", &warRiderChargeDuration},
        {"war_rider_overhead_charge_altitude", &warRiderOverheadChargeAltitude},
        {"war_rider_charge_fireball_interval", &warRiderChargeFireballInterval},
        {"war_rider_charge_fireball_speed", &warRiderChargeFireballSpeed},
        {"war_rider_charge_fire_patch_radius", &warRiderChargeFirePatchRadius},
        {"war_rider_charge_fire_patch_duration", &warRiderChargeFirePatchDuration},
        {"war_rider_slash_interval", &warRiderSlashInterval},
        {"war_rider_slash_speed", &warRiderSlashSpeed},
        {"war_rider_slash_radius", &warRiderSlashRadius},
        {"war_rider_contact_damage", &warRiderContactDamage},
        {"war_rider_slash_damage", &warRiderSlashDamage},
        {"war_rider_hover_altitude", &warRiderHoverAltitude},
        {"war_rider_orbit_radius_factor", &warRiderOrbitRadiusFactor},
        {"war_rider_command_interval", &warRiderCommandInterval},
        {"war_rider_command_radius", &warRiderCommandRadius},
        {"war_rider_command_duration", &warRiderCommandDuration},
        {"war_rider_command_speed_mult", &warRiderCommandSpeedMult},
        {"war_rider_command_fire_rate_mult", &warRiderCommandFireRateMult},
        {"war_rider_global_enrage_duration", &warRiderGlobalEnrageDuration},
        {"war_rider_global_enrage_speed_mult", &warRiderGlobalEnrageSpeedMult},
        {"war_rider_command_growth_mult", &warRiderCommandGrowthMult},
        {"war_rider_command_size_growth_mult", &warRiderCommandSizeGrowthMult},
        {"conquest_rider_spawn_time", &conquestRiderSpawnTime},
        {"conquest_rider_health", &conquestRiderHealth},
        {"conquest_rider_move_speed", &conquestRiderMoveSpeed},
        {"conquest_rider_hover_altitude", &conquestRiderHoverAltitude},
        {"conquest_rider_orbit_radius_factor", &conquestRiderOrbitRadiusFactor},
        {"conquest_rider_arrow_interval", &conquestRiderArrowInterval},
        {"conquest_rider_arrow_speed", &conquestRiderArrowSpeed},
        {"conquest_rider_arrow_lifetime", &conquestRiderArrowLifetime},
        {"conquest_rider_arrow_radius", &conquestRiderArrowRadius},
        {"conquest_rider_arrow_damage", &conquestRiderArrowDamage},
        {"conquest_rider_plague_radius", &conquestRiderPlagueRadius},
        {"conquest_rider_plague_duration", &conquestRiderPlagueDuration},
        {"conquest_rider_plague_dps", &conquestRiderPlagueDps},
        {"conquest_rider_plague_dot_duration", &conquestRiderPlagueDotDuration},
        {"conquest_rider_plague_dot_interval", &conquestRiderPlagueDotInterval},
        {"conquest_rider_plague_move_speed_mult", &conquestRiderPlagueMoveSpeedMult},
        {"conquest_rider_plague_infect_duration", &conquestRiderPlagueInfectDuration},
        {"conquest_rider_plague_small_radius", &conquestRiderPlagueSmallRadius},
        {"conquest_rider_plague_small_duration", &conquestRiderPlagueSmallDuration},
        {"conquest_rider_summon_interval", &conquestRiderSummonInterval},
        {"famine_rider_spawn_time", &famineRiderSpawnTime},
        {"famine_rider_health", &famineRiderHealth},
        {"famine_rider_move_speed", &famineRiderMoveSpeed},
        {"famine_rider_hover_altitude", &famineRiderHoverAltitude},
        {"famine_rider_orbit_radius_factor", &famineRiderOrbitRadiusFactor},
        {"famine_rider_wither_interval", &famineRiderWitherInterval},
        {"famine_rider_wither_radius", &famineRiderWitherRadius},
        {"famine_rider_wither_age_boost", &famineRiderWitherAgeBoost},
        {"famine_rider_aura_wither_rate", &famineRiderAuraWitherRate},
        {"famine_rider_essence_seek_range", &famineRiderEssenceSeekRange},
        {"famine_rider_wander_jitter", &famineRiderWanderJitter},
        {"famine_rider_radius_gain_per_essence", &famineRiderRadiusGainPerEssence},
        {"famine_rider_max_radius_bonus", &famineRiderMaxRadiusBonus},
        {"famine_rider_fire_rate_debuff_duration", &famineRiderFireRateDebuffDuration},
        {"famine_rider_fire_rate_debuff_mult", &famineRiderFireRateDebuffMult},
        {"death_rider_spawn_time", &deathRiderSpawnTime},
        {"death_rider_health", &deathRiderHealth},
        {"death_rider_move_speed", &deathRiderMoveSpeed},
        {"death_rider_hover_altitude", &deathRiderHoverAltitude},
        {"death_rider_orbit_radius_factor", &deathRiderOrbitRadiusFactor},
        {"death_rider_soul_speed", &deathRiderSoulSpeed},
        {"death_rider_skull_interval", &deathRiderSkullInterval},
        {"death_rider_skull_health", &deathRiderSkullHealth},
        {"death_rider_skull_speed", &deathRiderSkullSpeed},
        {"death_rider_skull_turn_rate", &deathRiderSkullTurnRate},
        {"death_rider_skull_life", &deathRiderSkullLife},
        {"death_rider_skull_swarm_radius", &deathRiderSkullSwarmRadius},
        {"death_rider_skull_swarm_delay", &deathRiderSkullSwarmDelay},
        {"survival_spawn_interval_scale", &survivalSpawnIntervalScale},
        {"heaven_falls_spawn_interval_scale", &heavenFallsSpawnIntervalScale},
        {"survival_event_spawn_scale", &survivalEventSpawnScale},
        {"heaven_falls_event_spawn_scale", &heavenFallsEventSpawnScale},
        {"normal_enemy_essence_drop_chance", &normalEnemyEssenceDropChance},
        {"famine_rider_essence_drop_chance_mult", &famineRiderEssenceDropChanceMult},
        {"dropped_essence_lifetime", &droppedEssenceLifetime},
        {"dropped_essence_fade_start", &droppedEssenceFadeStart},
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
        {"super_essence_interval", &superEssenceInterval},
        {"super_ball_speed", &superBallSpeed},
        {"super_ball_lifetime", &superBallLifetime},
        {"super_ball_radius", &superBallRadius},
        {"super_ball_fire_interval", &superBallFireInterval},
        {"super_ball_beam_damage", &superBallBeamDamage},
        {"super_ball_beam_range", &superBallBeamRange},
        {"super_ball_explosion_radius", &superBallExplosionRadius},
        {"super_ball_explosion_damage", &superBallExplosionDamage},
        {"super_rainbow_beam_base_life", &superRainbowBeamBaseLife},
        {"super_rainbow_beam_life_per_essence", &superRainbowBeamLifePerEssence},
        {"super_rainbow_beam_width_base", &superRainbowBeamWidthBase},
        {"super_rainbow_beam_width_per_essence", &superRainbowBeamWidthPerEssence},
        {"super_ball_beam_width", &superBallBeamWidth},
        {"water_droplet_speed", &waterDropletSpeed},
        {"water_droplet_damage", &waterDropletDamage},
        {"water_droplet_lifetime", &waterDropletLifetime},
        {"water_droplet_radius", &waterDropletRadius},
        {"water_droplet_hover_altitude", &waterDropletHoverAltitude},
        {"water_droplet_craft_interval", &waterDropletCraftInterval},
        {"water_droplet_resume_range", &waterDropletResumeRange},
        {"water_droplet_turn_rate", &waterDropletTurnRate},
        {"water_droplet_separation_mult", &waterDropletSeparationMult},
        {"water_droplet_repulsion_strength", &waterDropletRepulsionStrength},
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
        {"heatwave_fire_patch_lifetime", &heatwaveFirePatchLifetime},
        {"heatwave_fire_patch_radius", &heatwaveFirePatchRadius},
        {"heatwave_fire_patch_damage", &heatwaveFirePatchDamage},
        {"heatwave_fire_patch_height", &heatwaveFirePatchHeight},
        {"napalm_speed", &napalmSpeed},
        {"napalm_fuse", &napalmFuse},
        {"napalm_bounce_restitution", &napalmBounceRestitution},
        {"napalm_explosion_radius", &napalmExplosionRadius},
        {"napalm_explosion_damage", &napalmExplosionDamage},
        {"napalm_ignite_duration", &napalmIgniteDuration},
        {"napalm_ignite_dps", &napalmIgniteDps},
        {"napalm_spread_radius", &napalmSpreadRadius},
        {"napalm_spread_interval", &napalmSpreadInterval},
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
        {"longinus_judgment_charge_time", &longinusJudgmentChargeTime},
        {"longinus_judgment_length", &longinusJudgmentLength},
        {"longinus_judgment_radius", &longinusJudgmentRadius},
        {"longinus_judgment_lifetime", &longinusJudgmentLifetime},
        {"longinus_judgment_dps", &longinusJudgmentDps},
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
        {"gauntlet_snap_charge_time", &gauntletSnapChargeTime},
        {"gauntlet_snap_kill_chance", &gauntletSnapKillChance},
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
        {"bethlehem_essence_interval_min", &bethlehemEssenceIntervalMin},
        {"bethlehem_essence_interval_max", &bethlehemEssenceIntervalMax},
        {"bethlehem_essence_launch_speed", &bethlehemEssenceLaunchSpeed},
        {"bethlehem_essence_launch_lift", &bethlehemEssenceLaunchLift},
        {"bethlehem_essence_fall_gravity", &bethlehemEssenceFallGravity},
        {"bethlehem_essence_fall_drag", &bethlehemEssenceFallDrag},
        {"bethlehem_essence_death_speed", &bethlehemEssenceDeathSpeed},
        {"bethlehem_essence_death_lift", &bethlehemEssenceDeathLift},
        {"bethlehem_essence_death_gravity", &bethlehemEssenceDeathGravity},
        {"bethlehem_essence_death_drag", &bethlehemEssenceDeathDrag},
        {"throne_spawn_time", &throneSpawnTime},
        {"throne_health", &throneHealth},
        {"throne_hover_altitude", &throneHoverAltitude},
        {"throne_wander_radius", &throneWanderRadius},
        {"throne_move_speed", &throneMoveSpeed},
        {"throne_summon_interval", &throneSummonInterval},
        {"cherub_health", &cherubHealth},
        {"cherub_speed", &cherubSpeed},
        {"cherub_separation", &cherubSeparation},
        {"cherub_attack_range", &cherubAttackRange},
        {"cherub_shot_interval", &cherubShotInterval},
        {"cherub_shot_speed", &cherubShotSpeed},
        {"cherub_shot_turn_rate", &cherubShotTurnRate},
        {"cherub_shot_damage", &cherubShotDamage},
        {"throne_pulse_interval", &thronePulseInterval},
        {"throne_pulse_radius", &thronePulseRadius},
        {"throne_pulse_force", &thronePulseForce},
        {"throne_defeated_pulse_interval_scale", &throneDefeatedPulseIntervalScale},
        {"throne_defeated_pulse_damage", &throneDefeatedPulseDamage},
        {"throne_antigravity_duration", &throneAntigravityDuration},
        {"throne_antigravity_scale", &throneAntigravityScale},
        {"throne_jacob_ladder_open_time", &throneJacobLadderOpenTime},
        {"throne_jacob_ladder_beam_length", &throneJacobLadderBeamLength},
        {"throne_jacob_ladder_top_radius", &throneJacobLadderTopRadius},
        {"throne_jacob_ladder_bottom_radius", &throneJacobLadderBottomRadius},
        {"throne_jacob_ladder_lift_speed", &throneJacobLadderLiftSpeed},
        {"throne_jacob_ladder_axis_pull", &throneJacobLadderAxisPull},
        {"throne_jacob_ladder_contact_radius", &throneJacobLadderContactRadius},
        {"seraph_spawn_time", &seraphSpawnTime},
        {"seraph_health", &seraphHealth},
        {"seraph_hover_altitude", &seraphHoverAltitude},
        {"seraph_wander_radius", &seraphWanderRadius},
        {"seraph_move_speed", &seraphMoveSpeed},
        {"seraph_separation_radius", &seraphSeparationRadius},
        {"seraph_separation_force", &seraphSeparationForce},
        {"seraph_attack_stagger", &seraphAttackStagger},
        {"seraph_attack_interval", &seraphAttackInterval},
        {"seraph_fireball_speed", &seraphFireballSpeed},
        {"seraph_fireball_spread", &seraphFireballSpread},
        {"seraph_fireball_radius", &seraphFireballRadius},
        {"seraph_fireball_lifetime", &seraphFireballLifetime},
        {"seraph_fireball_damage", &seraphFireballDamage},
        {"seraph_fire_layer_radius", &seraphFireLayerRadius},
        {"seraph_fire_layer_duration", &seraphFireLayerDuration},
        {"seraph_fire_layer_dps", &seraphFireLayerDps},
        {"ufo_spawn_delay", &ufoSpawnDelay},
        {"ufo_health", &ufoHealth},
        {"ufo_hover_altitude_min", &ufoHoverAltitudeMin},
        {"ufo_hover_altitude_max", &ufoHoverAltitudeMax},
        {"ufo_move_speed", &ufoMoveSpeed},
        {"ufo_collect_range", &ufoCollectRange},
        {"ufo_tractor_range", &ufoTractorRange},
        {"ufo_tractor_strength", &ufoTractorStrength},
        {"ufo_tractor_altitude", &ufoTractorAltitude},
        {"ufo_attack_range", &ufoAttackRange},
        {"ufo_attack_interval", &ufoAttackInterval},
        {"ufo_orb_speed", &ufoOrbSpeed},
        {"ufo_orb_turn_rate", &ufoOrbTurnRate},
        {"ufo_orb_lifetime", &ufoOrbLifetime},
        {"ufo_orb_damage", &ufoOrbDamage},
        {"ufo_orb_explosion_damage", &ufoOrbExplosionDamage},
        {"ufo_orb_airburst_altitude", &ufoOrbAirburstAltitude},
        {"ufo_orb_explosion_radius", &ufoOrbExplosionRadius},
        {"ufo_orb_damage_radius_mult", &ufoOrbDamageRadiusMult},
        {"ufo_enter_range", &ufoEnterRange},
        {"ufo_pilot_hover_altitude", &ufoPilotHoverAltitude},
        {"ufo_pilot_move_speed", &ufoPilotMoveSpeed},
        {"ufo_pilot_sprint_mult", &ufoPilotSprintMult},
        {"ufo_pilot_vertical_speed", &ufoPilotVerticalSpeed},
        {"ufo_pilot_hover_strength", &ufoPilotHoverStrength},
        {"ufo_pilot_hover_damping", &ufoPilotHoverDamping},
        {"ufo_pilot_jump_cooldown", &ufoPilotJumpCooldown},
        {"ufo_pilot_orb_interval", &ufoPilotOrbInterval},
        {"ufo_pilot_orb_damage", &ufoPilotOrbDamage},
        {"ufo_pilot_orb_explosion_damage", &ufoPilotOrbExplosionDamage},
        {"ufo_pilot_orb_laser_damage", &ufoPilotOrbLaserDamage},
        {"ufo_pilot_orb_laser_range", &ufoPilotOrbLaserRange},
        {"ufo_pilot_orb_laser_width", &ufoPilotOrbLaserWidth},
        {"ufo_pilot_tractor_range", &ufoPilotTractorRange},
        {"ufo_pilot_tractor_strength", &ufoPilotTractorStrength},
        {"ufo_hyperspace_hold_time", &ufoHyperspaceHoldTime},
        {"ufo_hyperspace_duration", &ufoHyperspaceDuration},
        {"ufo_hyperspace_tunnel_radius", &ufoHyperspaceTunnelRadius},
        {"ufo_hyperspace_min_altitude", &ufoHyperspaceMinAltitude},
        {"ufo_hyperspace_max_altitude", &ufoHyperspaceMaxAltitude},
        {"ufo_hyperspace_angular_speed", &ufoHyperspaceAngularSpeed},
        {"ufo_hyperspace_vertical_speed", &ufoHyperspaceVerticalSpeed},
        {"ufo_hyperspace_obstacle_interval", &ufoHyperspaceObstacleInterval},
        {"ufo_hyperspace_obstacle_speed", &ufoHyperspaceObstacleSpeed},
        {"ufo_hyperspace_obstacle_damage", &ufoHyperspaceObstacleDamage},
        {"ufo_hyperspace_obstacle_radius", &ufoHyperspaceObstacleRadius},
        {"ufo_hyperspace_obstacle_spawn_distance", &ufoHyperspaceObstacleSpawnDistance},
        {"ufo_hyperspace_tunnel_draw_distance", &ufoHyperspaceTunnelDrawDistance},
        {"ufo_hyperspace_tunnel_ring_spacing", &ufoHyperspaceTunnelRingSpacing},
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
        } else if (key == "bgm_path") {
            config.bgmPath = Trim(value);
        } else if (key == "heaven_falls_bgm_path") {
            config.heavenFallsBgmPath = Trim(value);
        } else if (key == "throne_bgm_path") {
            config.throneBgmPath = Trim(value);
        } else if (key == "seraph_bgm_path") {
            config.seraphBgmPath = Trim(value);
        } else if (key == "ufo_bgm_path") {
            config.ufoBgmPath = Trim(value);
        } else if (key == "ufo_hyperspace_bgm_path") {
            config.ufoHyperspaceBgmPath = Trim(value);
        } else if (key == "tutorial_language") {
            std::string v = Lower(Trim(value));
            if (v == "english") config.tutorialLanguage = "english";
            else config.tutorialLanguage = "chinese";
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
        } else if (key == "super_essence_threshold") {
            int parsed = 5;
            if (ParseInt(value, parsed)) { config.superEssenceThreshold = parsed; }
        } else if (key == "water_droplet_essence_cost") {
            int parsed = 10;
            if (ParseInt(value, parsed)) { config.waterDropletEssenceCost = parsed; }
        } else if (key == "gauntlet_snap_essence_cost") {
            int parsed = 6;
            if (ParseInt(value, parsed)) { config.gauntletSnapEssenceCost = parsed; }
        } else if (key == "longinus_judgment_essence_cost") {
            int parsed = 8;
            if (ParseInt(value, parsed)) { config.longinusJudgmentEssenceCost = parsed; }
        } else if (key == "starting_essence") {
            int parsed = 3;
            if (ParseInt(value, parsed)) { config.startingEssence = parsed; }
        } else if (key == "essence_max_on_map") {
            int parsed = 1;
            if (ParseInt(value, parsed)) { config.essenceMaxOnMap = parsed; }
        } else if (key == "eden_essence_target_count") {
            int parsed = 36;
            if (ParseInt(value, parsed)) { config.edenEssenceTargetCount = parsed; }
        } else if (key == "labyrinth_width") {
            int parsed = 17;
            if (ParseInt(value, parsed)) { config.labyrinthWidth = parsed; }
        } else if (key == "labyrinth_height") {
            int parsed = 17;
            if (ParseInt(value, parsed)) { config.labyrinthHeight = parsed; }
        } else if (key == "labyrinth_seed") {
            int parsed = 0;
            if (ParseInt(value, parsed)) { config.labyrinthSeed = parsed; }
        } else if (key == "eden_fire_rain_burst_count") {
            int parsed = 3;
            if (ParseInt(value, parsed)) { config.edenFireRainBurstCount = parsed; }
        } else if (key == "eden_apocalypse_seraph_count") {
            int parsed = 3;
            if (ParseInt(value, parsed)) { config.edenApocalypseSeraphCount = parsed; }
        } else if (key == "bethlehem_essence_death_count") {
            int parsed = 4;
            if (ParseInt(value, parsed)) { config.bethlehemEssenceDeathCount = parsed; }
        } else if (key == "throne_summon_count") {
            int parsed = 8;
            if (ParseInt(value, parsed)) { config.throneSummonCount = parsed; }
        } else if (key == "throne_max_cherubs") {
            int parsed = 32;
            if (ParseInt(value, parsed)) { config.throneMaxCherubs = parsed; }
        } else if (key == "seraph_fireball_count") {
            int parsed = 18;
            if (ParseInt(value, parsed)) { config.seraphFireballCount = parsed; }
        } else if (key == "seraph_spawn_count") {
            int parsed = 1;
            if (ParseInt(value, parsed)) { config.seraphSpawnCount = parsed; }
        } else if (key == "ufo_collect_required") {
            int parsed = 10;
            if (ParseInt(value, parsed)) { config.ufoCollectRequired = parsed; }
        } else if (key == "ufo_base_essence") {
            int parsed = 10;
            if (ParseInt(value, parsed)) { config.ufoBaseEssence = parsed; }
        } else if (key == "ufo_pilot_essence_max") {
            int parsed = 30;
            if (ParseInt(value, parsed)) { config.ufoPilotEssenceMax = parsed; }
        } else if (key == "ufo_pilot_jump_cost") {
            int parsed = 10;
            if (ParseInt(value, parsed)) { config.ufoPilotJumpCost = parsed; }
        } else if (key == "conquest_rider_summon_count") {
            int parsed = 4;
            if (ParseInt(value, parsed)) { config.conquestRiderSummonCount = parsed; }
        } else if (key == "war_rider_command_max_growth_stacks") {
            int parsed = 3;
            if (ParseInt(value, parsed)) { config.warRiderCommandMaxGrowthStacks = parsed; }
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
        } else if (key == "eden_forbidden_fruit_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.edenForbiddenFruitEnabled = parsed;
            }
        } else if (key == "start_with_ark") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.startWithArk = parsed;
            }
        } else if (key == "labyrinth_minotaur_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.labyrinthMinotaurEnabled = parsed;
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
        } else if (key == "heaven_falls" || key == "Heaven_falls") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.heavenFalls = parsed;
            }
        } else if (key == "throne_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.throneEnabled = parsed;
            }
        } else if (key == "throne_jacob_ladder_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.throneJacobLadderEnabled = parsed;
            }
        } else if (key == "seraph_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.seraphEnabled = parsed;
            }
        } else if (key == "war_rider_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.warRiderEnabled = parsed;
            }
        } else if (key == "conquest_rider_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.conquestRiderEnabled = parsed;
            }
        } else if (key == "famine_rider_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.famineRiderEnabled = parsed;
            }
        } else if (key == "death_rider_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.deathRiderEnabled = parsed;
            }
        } else if (key == "death_rider_soul_threshold") {
            int parsed = config.deathRiderSoulThreshold;
            if (ParseInt(value, parsed)) {
                config.deathRiderSoulThreshold = parsed;
            }
        } else if (key == "death_rider_skull_swarm_count") {
            int parsed = config.deathRiderSkullSwarmCount;
            if (ParseInt(value, parsed)) {
                config.deathRiderSkullSwarmCount = parsed;
            }
        } else if (key == "ufo_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.ufoEnabled = parsed;
            }
        } else if (key == "ufo_debug_local_jump_enabled") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.ufoDebugLocalJumpEnabled = parsed;
            }
        } else if (key == "ufo_start_with_vehicle") {
            bool parsed = false;
            if (ParseBool(value, parsed)) {
                config.ufoStartWithVehicle = parsed;
            }
        } else if (key == "ufo_arrival_variant_enabled") {
            bool parsed = true;
            if (ParseBool(value, parsed)) {
                config.ufoArrivalVariantEnabled = parsed;
            }
        }
    }

    config.sfxVolume = std::clamp(config.sfxVolume, 0.0f, 1.0f);
    config.bgmVolume = std::clamp(config.bgmVolume, 0.0f, 1.0f);
    config.bgmLoopGap = std::max(0.0f, config.bgmLoopGap);
    config.bgmAltitudeFadeStart = std::max(0.0f, config.bgmAltitudeFadeStart);
    config.bgmAltitudeFadeEnd = std::max(config.bgmAltitudeFadeStart + 0.1f, config.bgmAltitudeFadeEnd);
    config.bgmAltitudeMinVolume = std::clamp(config.bgmAltitudeMinVolume, 0.0f, 1.0f);
    config.bgmBackWorldVolume = std::clamp(config.bgmBackWorldVolume, 0.0f, 1.0f);
    config.heavenFallsBgmVolume = std::clamp(config.heavenFallsBgmVolume, 0.0f, 1.0f);
    config.throneBgmVolume = std::clamp(config.throneBgmVolume, 0.0f, 1.0f);
    config.seraphBgmVolume = std::clamp(config.seraphBgmVolume, 0.0f, 1.0f);
    config.ufoBgmVolume = std::clamp(config.ufoBgmVolume, 0.0f, 1.0f);
    config.ufoHyperspaceBgmVolume = std::clamp(config.ufoHyperspaceBgmVolume, 0.0f, 1.0f);
    config.ufoArrivalAltitude = std::max(1.0f, config.ufoArrivalAltitude);
    config.ufoArrivalWorldVariance = std::clamp(config.ufoArrivalWorldVariance, 0.0f, 0.5f);
    config.ufoArrivalEnemyVariance = std::clamp(config.ufoArrivalEnemyVariance, 0.0f, 0.5f);
    config.circleRadius = std::max(6.0f, config.circleRadius);
    config.edenPlayRadius = std::max(8.0f, config.edenPlayRadius);
    config.edenMapRadius = std::max(config.edenPlayRadius + 2.0f, config.edenMapRadius);
    config.edenCorruptedFallRadius = std::max(config.edenMapRadius, config.edenCorruptedFallRadius);
    config.edenHeightScale = std::max(0.0f, config.edenHeightScale);
    config.edenHeightEpsilon = std::max(0.1f, config.edenHeightEpsilon);
    config.edenSpawnMinRadius = std::clamp(config.edenSpawnMinRadius, 0.0f, config.edenPlayRadius);
    config.edenSpawnMaxRadius = std::clamp(config.edenSpawnMaxRadius, config.edenSpawnMinRadius, config.edenPlayRadius);
    config.edenExitFadePower = std::max(0.1f, config.edenExitFadePower);
    config.edenCenterGravityMinScale = std::clamp(config.edenCenterGravityMinScale, 0.0f, 1.0f);
    config.edenCenterGravityRadius = std::max(0.0f, config.edenCenterGravityRadius);
    config.edenEssenceTargetCount = std::max(0, config.edenEssenceTargetCount);
    config.edenEssenceRespawnInterval = std::max(0.05f, config.edenEssenceRespawnInterval);
    config.edenEssenceAltitudeMin = std::max(0.0f, config.edenEssenceAltitudeMin);
    config.edenEssenceAltitudeMax = std::max(config.edenEssenceAltitudeMin, config.edenEssenceAltitudeMax);
    config.arkShiftSpeedMult = std::max(1.0f, config.arkShiftSpeedMult);
    auto clampOdd = [](int value) {
        value = std::clamp(value, 7, 51);
        if ((value & 1) == 0) ++value;
        return std::clamp(value, 7, 51);
    };
    config.labyrinthWidth = clampOdd(config.labyrinthWidth);
    config.labyrinthHeight = clampOdd(config.labyrinthHeight);
    config.labyrinthSeed = std::max(0, config.labyrinthSeed);
    config.labyrinthCellSize = std::max(2.5f, config.labyrinthCellSize);
    config.labyrinthWallHeight = std::max(1.0f, config.labyrinthWallHeight);
    config.labyrinthWallThickness = std::clamp(config.labyrinthWallThickness, 0.2f, config.labyrinthCellSize * 0.45f);
    config.labyrinthShiftInterval = std::max(4.0f, config.labyrinthShiftInterval);
    config.labyrinthShiftWarningTime = std::clamp(config.labyrinthShiftWarningTime, 0.5f, config.labyrinthShiftInterval - 0.2f);
    config.labyrinthMinotaurSpawnDelay = std::max(0.0f, config.labyrinthMinotaurSpawnDelay);
    config.labyrinthMinotaurHealth = std::max(1.0f, config.labyrinthMinotaurHealth);
    config.labyrinthMinotaurSpeed = std::max(0.5f, config.labyrinthMinotaurSpeed);
    config.labyrinthMinotaurChargeInterval = std::max(0.5f, config.labyrinthMinotaurChargeInterval);
    config.labyrinthMinotaurChargeSpeed = std::max(config.labyrinthMinotaurSpeed, config.labyrinthMinotaurChargeSpeed);
    config.labyrinthMinotaurContactDamage = std::max(0.0f, config.labyrinthMinotaurContactDamage);
    config.edenForbiddenFruitHeightOffset = std::max(0.0f, config.edenForbiddenFruitHeightOffset);
    config.edenForbiddenFruitRadius = std::max(0.25f, config.edenForbiddenFruitRadius);
    config.edenForbiddenFruitAbsorbRange = std::max(0.0f, config.edenForbiddenFruitAbsorbRange);
    config.edenForbiddenFruitAbsorbSpeed = std::max(0.0f, config.edenForbiddenFruitAbsorbSpeed);
    config.edenForbiddenFruitInteractRange = std::max(0.5f, config.edenForbiddenFruitInteractRange);
    config.edenForbiddenFruitExileImpulse = std::max(0.0f, config.edenForbiddenFruitExileImpulse);
    config.edenForbiddenFruitExileForceScale = std::max(0.0f, config.edenForbiddenFruitExileForceScale);
    config.edenForbiddenFruitShockwaveRadius = std::max(1.0f, config.edenForbiddenFruitShockwaveRadius);
    config.edenForbiddenFruitApocalypseSpeed = std::max(0.0f, config.edenForbiddenFruitApocalypseSpeed);
    config.edenFallOutFadeDuration = std::max(0.2f, config.edenFallOutFadeDuration);
    config.edenFireRainInterval = std::max(0.05f, config.edenFireRainInterval);
    config.edenFireRainBurstCount = std::clamp(config.edenFireRainBurstCount, 0, 32);
    config.edenFireRainSpawnHeight = std::max(100.0f, config.edenFireRainSpawnHeight);
    config.edenFireRainSpeed = std::max(1.0f, config.edenFireRainSpeed);
    config.edenFireRainRadius = std::max(0.05f, config.edenFireRainRadius);
    config.edenFireRainLifetime = std::max(0.5f, config.edenFireRainLifetime);
    config.edenFireRainDamage = std::max(0.0f, config.edenFireRainDamage);
    config.edenFireRainFireRadius = std::max(0.3f, config.edenFireRainFireRadius);
    config.edenFireRainFireDuration = std::max(0.1f, config.edenFireRainFireDuration);
    config.edenFireRainFireDps = std::max(0.0f, config.edenFireRainFireDps);
    config.edenApocalypseSeraphCount = std::clamp(config.edenApocalypseSeraphCount, 0, 8);
    config.edenApocalypseHitInvuln = std::clamp(config.edenApocalypseHitInvuln, 0.0f, config.essenceHitInvuln);
    config.edenGuardianApproachSpeed = std::max(0.0f, config.edenGuardianApproachSpeed);
    config.edenGuardianAttackInterval = std::max(0.35f, config.edenGuardianAttackInterval);
    config.edenGuardianSlashSpeed = std::max(1.0f, config.edenGuardianSlashSpeed);
    config.edenGuardianSlashLifetime = std::max(0.2f, config.edenGuardianSlashLifetime);
    config.edenGuardianSlashRadius = std::max(1.0f, config.edenGuardianSlashRadius);
    config.edenGuardianSlashThickness = std::max(0.2f, config.edenGuardianSlashThickness);
    config.edenGuardianSlashPlaneThickness = std::max(0.2f, config.edenGuardianSlashPlaneThickness);
    config.warRiderSpawnTime = std::max(0.0f, config.warRiderSpawnTime);
    config.warRiderHealth = std::max(1.0f, config.warRiderHealth);
    config.warRiderMoveSpeed = std::max(1.0f, config.warRiderMoveSpeed);
    config.warRiderChargeSpeed = std::max(config.warRiderMoveSpeed, config.warRiderChargeSpeed);
    config.warRiderChargeInterval = std::max(0.5f, config.warRiderChargeInterval);
    config.warRiderChargeDuration = std::max(0.2f, config.warRiderChargeDuration);
    config.warRiderOverheadChargeAltitude = std::max(0.0f, config.warRiderOverheadChargeAltitude);
    config.warRiderChargeFireballInterval = std::max(0.05f, config.warRiderChargeFireballInterval);
    config.warRiderChargeFireballSpeed = std::max(1.0f, config.warRiderChargeFireballSpeed);
    config.warRiderChargeFirePatchRadius = std::max(0.5f, config.warRiderChargeFirePatchRadius);
    config.warRiderChargeFirePatchDuration = std::max(0.2f, config.warRiderChargeFirePatchDuration);
    config.warRiderSlashInterval = std::max(0.5f, config.warRiderSlashInterval);
    config.warRiderSlashSpeed = std::max(1.0f, config.warRiderSlashSpeed);
    config.warRiderSlashRadius = std::max(1.0f, config.warRiderSlashRadius);
    config.warRiderContactDamage = std::max(0.0f, config.warRiderContactDamage);
    config.warRiderSlashDamage = std::max(0.0f, config.warRiderSlashDamage);
    config.warRiderHoverAltitude = std::max(1.5f, config.warRiderHoverAltitude);
    config.warRiderOrbitRadiusFactor = std::clamp(config.warRiderOrbitRadiusFactor, 0.15f, 0.95f);
    config.warRiderCommandInterval = std::max(0.5f, config.warRiderCommandInterval);
    config.warRiderCommandRadius = std::max(2.0f, config.warRiderCommandRadius);
    config.warRiderCommandDuration = std::max(0.25f, config.warRiderCommandDuration);
    config.warRiderCommandSpeedMult = std::max(1.0f, config.warRiderCommandSpeedMult);
    config.warRiderCommandFireRateMult = std::max(1.0f, config.warRiderCommandFireRateMult);
    config.warRiderGlobalEnrageDuration = std::max(0.1f, config.warRiderGlobalEnrageDuration);
    config.warRiderGlobalEnrageSpeedMult = std::max(1.0f, config.warRiderGlobalEnrageSpeedMult);
    config.warRiderCommandGrowthMult = std::max(1.0f, config.warRiderCommandGrowthMult);
    config.warRiderCommandSizeGrowthMult = std::max(1.0f, config.warRiderCommandSizeGrowthMult);
    config.warRiderCommandMaxGrowthStacks = std::clamp(config.warRiderCommandMaxGrowthStacks, 0, 8);
    config.conquestRiderSpawnTime = std::max(0.0f, config.conquestRiderSpawnTime);
    config.conquestRiderHealth = std::max(1.0f, config.conquestRiderHealth);
    config.conquestRiderMoveSpeed = std::max(1.0f, config.conquestRiderMoveSpeed);
    config.conquestRiderHoverAltitude = std::max(1.5f, config.conquestRiderHoverAltitude);
    config.conquestRiderOrbitRadiusFactor = std::clamp(config.conquestRiderOrbitRadiusFactor, 0.15f, 0.95f);
    config.conquestRiderArrowInterval = std::max(0.5f, config.conquestRiderArrowInterval);
    config.conquestRiderArrowSpeed = std::max(1.0f, config.conquestRiderArrowSpeed);
    config.conquestRiderArrowLifetime = std::max(0.2f, config.conquestRiderArrowLifetime);
    config.conquestRiderArrowRadius = std::max(0.05f, config.conquestRiderArrowRadius);
    config.conquestRiderArrowDamage = std::max(0.0f, config.conquestRiderArrowDamage);
    config.conquestRiderPlagueRadius = std::max(0.5f, config.conquestRiderPlagueRadius);
    config.conquestRiderPlagueDuration = std::max(0.2f, config.conquestRiderPlagueDuration);
    config.conquestRiderPlagueDps = std::max(0.0f, config.conquestRiderPlagueDps);
    config.conquestRiderPlagueDotDuration = std::max(0.0f, config.conquestRiderPlagueDotDuration);
    config.conquestRiderPlagueDotInterval = std::max(0.15f, config.conquestRiderPlagueDotInterval);
    config.conquestRiderPlagueMoveSpeedMult = std::clamp(config.conquestRiderPlagueMoveSpeedMult, 0.1f, 1.0f);
    config.conquestRiderPlagueInfectDuration = std::max(0.0f, config.conquestRiderPlagueInfectDuration);
    config.conquestRiderPlagueSmallRadius = std::max(0.3f, config.conquestRiderPlagueSmallRadius);
    config.conquestRiderPlagueSmallDuration = std::max(0.2f, config.conquestRiderPlagueSmallDuration);
    config.conquestRiderSummonInterval = std::max(0.8f, config.conquestRiderSummonInterval);
    config.conquestRiderSummonCount = std::clamp(config.conquestRiderSummonCount, 0, 24);
    config.famineRiderSpawnTime = std::max(0.0f, config.famineRiderSpawnTime);
    config.famineRiderHealth = std::max(1.0f, config.famineRiderHealth);
    config.famineRiderMoveSpeed = std::max(1.0f, config.famineRiderMoveSpeed);
    config.famineRiderHoverAltitude = std::max(1.5f, config.famineRiderHoverAltitude);
    config.famineRiderOrbitRadiusFactor = std::clamp(config.famineRiderOrbitRadiusFactor, 0.15f, 0.98f);
    config.famineRiderWitherInterval = std::max(0.5f, config.famineRiderWitherInterval);
    config.famineRiderWitherRadius = std::max(2.0f, config.famineRiderWitherRadius);
    config.famineRiderWitherAgeBoost = std::max(0.0f, config.famineRiderWitherAgeBoost);
    config.famineRiderAuraWitherRate = std::max(0.0f, config.famineRiderAuraWitherRate);
    config.famineRiderEssenceSeekRange = std::max(1.0f, config.famineRiderEssenceSeekRange);
    config.famineRiderWanderJitter = std::clamp(config.famineRiderWanderJitter, 0.0f, 1.0f);
    config.famineRiderRadiusGainPerEssence = std::max(0.0f, config.famineRiderRadiusGainPerEssence);
    config.famineRiderMaxRadiusBonus = std::max(0.0f, config.famineRiderMaxRadiusBonus);
    config.famineRiderFireRateDebuffDuration = std::max(0.0f, config.famineRiderFireRateDebuffDuration);
    config.famineRiderFireRateDebuffMult = std::clamp(config.famineRiderFireRateDebuffMult, 0.05f, 1.0f);
    config.deathRiderSpawnTime = std::max(0.0f, config.deathRiderSpawnTime);
    config.deathRiderHealth = std::max(1.0f, config.deathRiderHealth);
    config.deathRiderMoveSpeed = std::max(1.0f, config.deathRiderMoveSpeed);
    config.deathRiderHoverAltitude = std::max(1.5f, config.deathRiderHoverAltitude);
    config.deathRiderOrbitRadiusFactor = std::clamp(config.deathRiderOrbitRadiusFactor, 0.15f, 0.98f);
    config.deathRiderSoulThreshold = std::clamp(config.deathRiderSoulThreshold, 1, 200);
    config.deathRiderSoulSpeed = std::max(1.0f, config.deathRiderSoulSpeed);
    config.deathRiderSkullInterval = std::max(0.25f, config.deathRiderSkullInterval);
    config.deathRiderSkullHealth = std::max(0.1f, config.deathRiderSkullHealth);
    config.deathRiderSkullSpeed = std::max(1.0f, config.deathRiderSkullSpeed);
    config.deathRiderSkullTurnRate = std::max(0.0f, config.deathRiderSkullTurnRate);
    config.deathRiderSkullLife = std::max(0.3f, config.deathRiderSkullLife);
    config.deathRiderSkullSwarmCount = std::clamp(config.deathRiderSkullSwarmCount, 1, 80);
    config.deathRiderSkullSwarmRadius = std::max(0.5f, config.deathRiderSkullSwarmRadius);
    config.deathRiderSkullSwarmDelay = std::max(0.0f, config.deathRiderSkullSwarmDelay);
    config.survivalSpawnIntervalScale = std::max(0.2f, config.survivalSpawnIntervalScale);
    config.heavenFallsSpawnIntervalScale = std::max(0.2f, config.heavenFallsSpawnIntervalScale);
    config.survivalEventSpawnScale = std::max(0.0f, config.survivalEventSpawnScale);
    config.heavenFallsEventSpawnScale = std::max(0.0f, config.heavenFallsEventSpawnScale);
    config.normalEnemyEssenceDropChance = std::clamp(config.normalEnemyEssenceDropChance, 0.0f, 1.0f);
    config.famineRiderEssenceDropChanceMult = std::clamp(config.famineRiderEssenceDropChanceMult, 0.0f, 1.0f);
    config.droppedEssenceLifetime = std::max(1.0f, config.droppedEssenceLifetime);
    config.droppedEssenceFadeStart = std::clamp(config.droppedEssenceFadeStart, 0.0f, config.droppedEssenceLifetime);
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
    config.superEssenceInterval = std::max(0.1f, config.superEssenceInterval);
    config.superEssenceThreshold = std::clamp(config.superEssenceThreshold, 1, 50);
    config.superBallSpeed = std::max(1.0f, config.superBallSpeed);
    config.superBallLifetime = std::max(0.5f, config.superBallLifetime);
    config.superBallRadius = std::max(0.3f, config.superBallRadius);
    config.superBallFireInterval = std::max(0.05f, config.superBallFireInterval);
    config.superBallBeamDamage = std::max(0.0f, config.superBallBeamDamage);
    config.superBallBeamRange = std::max(5.0f, config.superBallBeamRange);
    config.superBallExplosionRadius = std::max(1.0f, config.superBallExplosionRadius);
    config.superBallExplosionDamage = std::max(0.0f, config.superBallExplosionDamage);
    config.superRainbowBeamBaseLife = std::max(0.05f, config.superRainbowBeamBaseLife);
    config.superRainbowBeamLifePerEssence = std::max(0.0f, config.superRainbowBeamLifePerEssence);
    config.superRainbowBeamWidthBase = std::max(0.1f, config.superRainbowBeamWidthBase);
    config.superRainbowBeamWidthPerEssence = std::max(0.0f, config.superRainbowBeamWidthPerEssence);
    config.superBallBeamWidth = std::max(0.05f, config.superBallBeamWidth);
    config.waterDropletEssenceCost = std::clamp(config.waterDropletEssenceCost, 1, 100);
    config.waterDropletSpeed = std::max(10.0f, config.waterDropletSpeed);
    config.waterDropletDamage = std::max(0.0f, config.waterDropletDamage);
    config.waterDropletLifetime = std::max(1.0f, config.waterDropletLifetime);
    config.waterDropletRadius = std::max(0.1f, config.waterDropletRadius);
    config.waterDropletHoverAltitude = std::max(0.5f, config.waterDropletHoverAltitude);
    config.waterDropletCraftInterval = std::max(0.1f, config.waterDropletCraftInterval);
    config.waterDropletResumeRange = std::max(1.0f, config.waterDropletResumeRange);
    config.waterDropletTurnRate = std::clamp(config.waterDropletTurnRate, 0.5f, 40.0f);
    config.waterDropletSeparationMult = std::max(0.0f, config.waterDropletSeparationMult);
    config.waterDropletRepulsionStrength = std::clamp(config.waterDropletRepulsionStrength, 0.0f, 1.0f);
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
    config.heatwaveFirePatchLifetime = std::max(0.2f, config.heatwaveFirePatchLifetime);
    config.heatwaveFirePatchRadius = std::max(0.3f, config.heatwaveFirePatchRadius);
    config.heatwaveFirePatchDamage = std::max(0.0f, config.heatwaveFirePatchDamage);
    config.heatwaveFirePatchHeight = std::clamp(config.heatwaveFirePatchHeight, 0.0f, 0.5f);
    config.napalmSpeed = std::max(8.0f, config.napalmSpeed);
    config.napalmFuse = std::max(0.2f, config.napalmFuse);
    config.napalmBounceRestitution = std::clamp(config.napalmBounceRestitution, 0.1f, 0.9f);
    config.napalmExplosionRadius = std::max(1.0f, config.napalmExplosionRadius);
    config.napalmExplosionDamage = std::max(0.0f, config.napalmExplosionDamage);
    config.napalmIgniteDuration = std::max(0.5f, config.napalmIgniteDuration);
    config.napalmIgniteDps = std::max(0.0f, config.napalmIgniteDps);
    config.napalmSpreadRadius = std::max(0.5f, config.napalmSpreadRadius);
    config.napalmSpreadInterval = std::max(0.1f, config.napalmSpreadInterval);
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
    config.longinusJudgmentEssenceCost = std::max(1, config.longinusJudgmentEssenceCost);
    config.longinusJudgmentChargeTime = std::max(0.1f, config.longinusJudgmentChargeTime);
    config.longinusJudgmentLength = std::max(4.0f, config.longinusJudgmentLength);
    config.longinusJudgmentRadius = std::max(0.25f, config.longinusJudgmentRadius);
    config.longinusJudgmentLifetime = std::max(0.1f, config.longinusJudgmentLifetime);
    config.longinusJudgmentDps = std::max(0.0f, config.longinusJudgmentDps);
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
    config.gauntletSnapEssenceCost = std::max(1, config.gauntletSnapEssenceCost);
    config.gauntletSnapChargeTime = std::max(0.1f, config.gauntletSnapChargeTime);
    config.gauntletSnapKillChance = std::clamp(config.gauntletSnapKillChance, 0.0f, 1.0f);
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
    config.bethlehemEssenceIntervalMin = std::max(5.0f, config.bethlehemEssenceIntervalMin);
    config.bethlehemEssenceIntervalMax = std::max(config.bethlehemEssenceIntervalMin, config.bethlehemEssenceIntervalMax);
    config.bethlehemEssenceLaunchSpeed = std::max(1.0f, config.bethlehemEssenceLaunchSpeed);
    config.bethlehemEssenceLaunchLift = std::max(0.0f, config.bethlehemEssenceLaunchLift);
    config.bethlehemEssenceFallGravity = std::max(0.05f, config.bethlehemEssenceFallGravity);
    config.bethlehemEssenceFallDrag = std::max(0.0f, config.bethlehemEssenceFallDrag);
    config.bethlehemEssenceDeathCount = std::max(0, config.bethlehemEssenceDeathCount);
    config.bethlehemEssenceDeathSpeed = std::max(1.0f, config.bethlehemEssenceDeathSpeed);
    config.bethlehemEssenceDeathLift = std::max(0.0f, config.bethlehemEssenceDeathLift);
    config.bethlehemEssenceDeathGravity = std::max(0.05f, config.bethlehemEssenceDeathGravity);
    config.bethlehemEssenceDeathDrag = std::max(0.0f, config.bethlehemEssenceDeathDrag);
    config.throneSpawnTime = std::max(1.0f, config.throneSpawnTime);
    config.throneHealth = std::max(1.0f, config.throneHealth);
    config.throneHoverAltitude = std::max(2.0f, config.throneHoverAltitude);
    config.throneWanderRadius = std::max(2.0f, config.throneWanderRadius);
    config.throneMoveSpeed = std::max(0.5f, config.throneMoveSpeed);
    config.throneSummonInterval = std::max(1.0f, config.throneSummonInterval);
    config.throneSummonCount = std::clamp(config.throneSummonCount, 0, 64);
    config.throneMaxCherubs = std::clamp(config.throneMaxCherubs, 0, 128);
    config.cherubHealth = std::max(0.1f, config.cherubHealth);
    config.cherubSpeed = std::max(0.5f, config.cherubSpeed);
    config.cherubSeparation = std::max(0.1f, config.cherubSeparation);
    config.cherubAttackRange = std::max(1.0f, config.cherubAttackRange);
    config.cherubShotInterval = std::max(0.1f, config.cherubShotInterval);
    config.cherubShotSpeed = std::max(1.0f, config.cherubShotSpeed);
    config.cherubShotTurnRate = std::clamp(config.cherubShotTurnRate, 0.1f, 20.0f);
    config.cherubShotDamage = std::max(0.0f, config.cherubShotDamage);
    config.thronePulseInterval = std::max(1.0f, config.thronePulseInterval);
    config.thronePulseRadius = std::max(1.0f, config.thronePulseRadius);
    config.thronePulseForce = std::max(0.0f, config.thronePulseForce);
    config.throneDefeatedPulseIntervalScale = std::clamp(config.throneDefeatedPulseIntervalScale, 0.05f, 1.0f);
    config.throneDefeatedPulseDamage = std::max(0.0f, config.throneDefeatedPulseDamage);
    config.throneAntigravityDuration = std::max(0.0f, config.throneAntigravityDuration);
    config.throneAntigravityScale = std::clamp(config.throneAntigravityScale, 0.0f, 1.0f);
    config.throneJacobLadderOpenTime = std::max(0.1f, config.throneJacobLadderOpenTime);
    config.throneJacobLadderBeamLength = std::max(4.0f, config.throneJacobLadderBeamLength);
    config.throneJacobLadderTopRadius = std::max(0.2f, config.throneJacobLadderTopRadius);
    config.throneJacobLadderBottomRadius = std::max(config.throneJacobLadderTopRadius, config.throneJacobLadderBottomRadius);
    config.throneJacobLadderLiftSpeed = std::max(0.0f, config.throneJacobLadderLiftSpeed);
    config.throneJacobLadderAxisPull = std::max(0.0f, config.throneJacobLadderAxisPull);
    config.throneJacobLadderContactRadius = std::max(0.5f, config.throneJacobLadderContactRadius);
    config.seraphSpawnTime = std::max(1.0f, config.seraphSpawnTime);
    config.seraphHealth = std::max(1.0f, config.seraphHealth);
    config.seraphHoverAltitude = std::max(2.0f, config.seraphHoverAltitude);
    config.seraphWanderRadius = std::max(2.0f, config.seraphWanderRadius);
    config.seraphMoveSpeed = std::max(0.5f, config.seraphMoveSpeed);
    config.seraphSpawnCount = std::clamp(config.seraphSpawnCount, 1, 8);
    config.seraphSeparationRadius = std::max(0.0f, config.seraphSeparationRadius);
    config.seraphSeparationForce = std::max(0.0f, config.seraphSeparationForce);
    config.seraphAttackStagger = std::max(0.0f, config.seraphAttackStagger);
    config.seraphAttackInterval = std::max(0.25f, config.seraphAttackInterval);
    config.seraphFireballCount = std::clamp(config.seraphFireballCount, 1, 80);
    config.seraphFireballSpeed = std::max(1.0f, config.seraphFireballSpeed);
    config.seraphFireballSpread = std::clamp(config.seraphFireballSpread, 0.0f, 1.2f);
    config.seraphFireballRadius = std::max(0.05f, config.seraphFireballRadius);
    config.seraphFireballLifetime = std::max(0.2f, config.seraphFireballLifetime);
    config.seraphFireballDamage = std::max(0.0f, config.seraphFireballDamage);
    config.seraphFireLayerRadius = std::max(0.5f, config.seraphFireLayerRadius);
    config.seraphFireLayerDuration = std::max(0.2f, config.seraphFireLayerDuration);
    config.seraphFireLayerDps = std::max(0.0f, config.seraphFireLayerDps);
    config.ufoSpawnDelay = std::max(0.0f, config.ufoSpawnDelay);
    config.ufoHealth = std::max(1.0f, config.ufoHealth);
    config.ufoHoverAltitudeMin = std::max(1.0f, config.ufoHoverAltitudeMin);
    config.ufoHoverAltitudeMax = std::max(config.ufoHoverAltitudeMin, config.ufoHoverAltitudeMax);
    config.ufoMoveSpeed = std::max(1.0f, config.ufoMoveSpeed);
    config.ufoCollectRequired = std::max(1, config.ufoCollectRequired);
    config.ufoBaseEssence = std::max(0, config.ufoBaseEssence);
    config.ufoCollectRange = std::max(0.4f, config.ufoCollectRange);
    config.ufoTractorRange = std::max(config.ufoCollectRange, config.ufoTractorRange);
    config.ufoTractorStrength = std::max(0.0f, config.ufoTractorStrength);
    config.ufoTractorAltitude = std::clamp(config.ufoTractorAltitude, 0.5f, config.ufoHoverAltitudeMax);
    config.ufoAttackRange = std::max(2.0f, config.ufoAttackRange);
    config.ufoAttackInterval = std::max(0.1f, config.ufoAttackInterval);
    config.ufoOrbSpeed = std::max(1.0f, config.ufoOrbSpeed);
    config.ufoOrbTurnRate = std::clamp(config.ufoOrbTurnRate, 0.1f, 20.0f);
    config.ufoOrbLifetime = std::max(0.5f, config.ufoOrbLifetime);
    config.ufoOrbDamage = std::max(0.0f, config.ufoOrbDamage);
    config.ufoOrbExplosionDamage = std::max(0.0f, config.ufoOrbExplosionDamage);
    config.ufoOrbAirburstAltitude = std::max(0.2f, config.ufoOrbAirburstAltitude);
    config.ufoOrbExplosionRadius = std::max(0.5f, config.ufoOrbExplosionRadius);
    config.ufoOrbDamageRadiusMult = std::max(0.1f, config.ufoOrbDamageRadiusMult);
    config.ufoEnterRange = std::max(1.0f, config.ufoEnterRange);
    config.ufoPilotHoverAltitude = std::max(1.0f, config.ufoPilotHoverAltitude);
    config.ufoPilotMoveSpeed = std::max(1.0f, config.ufoPilotMoveSpeed);
    config.ufoPilotSprintMult = std::max(1.0f, config.ufoPilotSprintMult);
    config.ufoPilotVerticalSpeed = std::max(0.5f, config.ufoPilotVerticalSpeed);
    config.ufoPilotHoverStrength = std::max(0.0f, config.ufoPilotHoverStrength);
    config.ufoPilotHoverDamping = std::max(0.0f, config.ufoPilotHoverDamping);
    config.ufoPilotEssenceMax = std::max(1, config.ufoPilotEssenceMax);
    config.ufoPilotJumpCost = std::clamp(config.ufoPilotJumpCost, 1, config.ufoPilotEssenceMax);
    config.ufoPilotJumpCooldown = std::max(0.0f, config.ufoPilotJumpCooldown);
    config.ufoPilotOrbInterval = std::max(0.05f, config.ufoPilotOrbInterval);
    config.ufoPilotOrbDamage = std::max(0.0f, config.ufoPilotOrbDamage);
    config.ufoPilotOrbExplosionDamage = std::max(0.0f, config.ufoPilotOrbExplosionDamage);
    config.ufoPilotOrbLaserDamage = std::max(0.0f, config.ufoPilotOrbLaserDamage);
    config.ufoPilotOrbLaserRange = std::max(2.0f, config.ufoPilotOrbLaserRange);
    config.ufoPilotOrbLaserWidth = std::max(0.1f, config.ufoPilotOrbLaserWidth);
    config.ufoPilotTractorRange = std::max(2.0f, config.ufoPilotTractorRange);
    config.ufoPilotTractorStrength = std::max(0.0f, config.ufoPilotTractorStrength);
    config.ufoHyperspaceHoldTime = std::max(0.1f, config.ufoHyperspaceHoldTime);
    config.ufoHyperspaceDuration = std::max(1.0f, config.ufoHyperspaceDuration);
    config.ufoHyperspaceTunnelRadius = std::max(3.0f, config.ufoHyperspaceTunnelRadius);
    config.ufoHyperspaceMinAltitude = std::max(0.0f, config.ufoHyperspaceMinAltitude);
    config.ufoHyperspaceMaxAltitude = std::max(config.ufoHyperspaceMinAltitude + 0.1f, config.ufoHyperspaceMaxAltitude);
    config.ufoHyperspaceAngularSpeed = std::max(0.1f, config.ufoHyperspaceAngularSpeed);
    config.ufoHyperspaceVerticalSpeed = std::max(0.1f, config.ufoHyperspaceVerticalSpeed);
    config.ufoHyperspaceObstacleInterval = std::max(0.15f, config.ufoHyperspaceObstacleInterval);
    config.ufoHyperspaceObstacleSpeed = std::max(1.0f, config.ufoHyperspaceObstacleSpeed);
    config.ufoHyperspaceObstacleDamage = std::max(0.0f, config.ufoHyperspaceObstacleDamage);
    config.ufoHyperspaceObstacleRadius = std::max(0.2f, config.ufoHyperspaceObstacleRadius);
    config.ufoHyperspaceObstacleSpawnDistance = std::max(24.0f, config.ufoHyperspaceObstacleSpawnDistance);
    config.ufoHyperspaceTunnelDrawDistance = std::max(config.ufoHyperspaceObstacleSpawnDistance + 24.0f, config.ufoHyperspaceTunnelDrawDistance);
    config.ufoHyperspaceTunnelRingSpacing = std::max(2.0f, config.ufoHyperspaceTunnelRingSpacing);
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
