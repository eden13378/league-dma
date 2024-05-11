#include "pch.hpp"

#include "activator/activator.hpp"
#include "../build.hpp"
#include "../menu/menu.hpp"

#if enable_lua
#include "buff_cache.hpp"
#include "entity_list.hpp"
#include "evade.hpp"
#include "orbwalker.hpp"
#include "prediction.hpp"
#include "target_selector/ITargetSelector.hpp"

namespace features {
    auto Evade::initialize_lua( sol::state* state ) -> void{
        state->new_usertype< SpellDetector::SpellInstance >(
            "spell_instance_t",
            "source",
            sol::readonly( &SpellDetector::SpellInstance::source ),
            "source_index",
            sol::readonly( &SpellDetector::SpellInstance::source_index ),
            "type",
            sol::readonly( &SpellDetector::SpellInstance::type ),
            "slot",
            sol::readonly( &SpellDetector::SpellInstance::slot ),
            "missile_obj",
            sol::readonly( &SpellDetector::SpellInstance::missile_obj ),
            "missile_nid",
            sol::readonly( &SpellDetector::SpellInstance::missile_nid ),
            "missile_name",
            sol::readonly( &SpellDetector::SpellInstance::missile_name ),
            "start_pos",
            sol::readonly( &SpellDetector::SpellInstance::start_pos ),
            "current_pos",
            sol::readonly( &SpellDetector::SpellInstance::current_pos ),
            "end_pos",
            sol::readonly( &SpellDetector::SpellInstance::end_pos ),
            "raw_end_pos",
            sol::readonly( &SpellDetector::SpellInstance::raw_end_pos ),
            "speed",
            sol::readonly( &SpellDetector::SpellInstance::speed ),
            "base_speed",
            sol::readonly( &SpellDetector::SpellInstance::base_speed ),
            "range",
            sol::readonly( &SpellDetector::SpellInstance::range ),
            "radius",
            sol::readonly( &SpellDetector::SpellInstance::radius ),
            "windup_time",
            sol::readonly( &SpellDetector::SpellInstance::windup_time ),
            "total_cast_time",
            sol::readonly( &SpellDetector::SpellInstance::total_cast_time ),
            "danger",
            sol::readonly( &SpellDetector::SpellInstance::danger ),
            "has_edge_radius",
            sol::readonly( &SpellDetector::SpellInstance::has_edge_radius ),
            "is_particle_spell",
            sol::readonly( &SpellDetector::SpellInstance::is_particle_spell ),
            "particle_index",
            sol::readonly( &SpellDetector::SpellInstance::particle_index ),
            "particle_nid",
            sol::readonly( &SpellDetector::SpellInstance::particle_nid ),
            "related_particles",
            sol::readonly( &SpellDetector::SpellInstance::related_particles ),
            "particle_type",
            sol::readonly( &SpellDetector::SpellInstance::particle_type ),
            "angle",
            sol::readonly( &SpellDetector::SpellInstance::angle ),
            "allow_dodge",
            sol::readonly( &SpellDetector::SpellInstance::allow_dodge ),
            "ignore_health_threshold",
            sol::readonly( &SpellDetector::SpellInstance::ignore_health_threshold ),
            "stick_during_cast",
            sol::readonly( &SpellDetector::SpellInstance::stick_during_cast ),
            "stick_full_duration",
            sol::readonly( &SpellDetector::SpellInstance::stick_full_duration ),
            "is_direction_locked",
            sol::readonly( &SpellDetector::SpellInstance::is_direction_locked ),
            "direction",
            sol::readonly( &SpellDetector::SpellInstance::direction ),
            "base_position",
            sol::readonly( &SpellDetector::SpellInstance::base_position ),
            "base_distance_to_start",
            sol::readonly( &SpellDetector::SpellInstance::base_distance_to_start ),
            "base_distance_to_end",
            sol::readonly( &SpellDetector::SpellInstance::base_distance_to_end ),
            "is_hitbox_relative",
            sol::readonly( &SpellDetector::SpellInstance::is_hitbox_relative ),
            "update_direction",
            sol::readonly( &SpellDetector::SpellInstance::update_direction ),
            "raw_start_pos",
            sol::readonly( &SpellDetector::SpellInstance::raw_start_pos ),
            "should_run_check",
            sol::readonly( &SpellDetector::SpellInstance::should_run_check ),
            "check_type",
            sol::readonly( &SpellDetector::SpellInstance::check_type ),
            "check_target_index",
            sol::readonly( &SpellDetector::SpellInstance::check_target_index ),
            "special_type",
            sol::readonly( &SpellDetector::SpellInstance::special_type ),
            "start_time",
            sol::readonly( &SpellDetector::SpellInstance::start_time ),
            "server_cast_time",
            sol::readonly( &SpellDetector::SpellInstance::server_cast_time ),
            "end_time",
            sol::readonly( &SpellDetector::SpellInstance::end_time ),
            "spell_name",
            sol::readonly( &SpellDetector::SpellInstance::spell_name ),
            "missile_found",
            sol::readonly( &SpellDetector::SpellInstance::missile_found ),
            "mis_index",
            sol::readonly( &SpellDetector::SpellInstance::mis_index ),
            "mis_start_position",
            sol::readonly( &SpellDetector::SpellInstance::mis_start_position ),
            "mis_end_position",
            sol::readonly( &SpellDetector::SpellInstance::mis_end_position ),
            "mis_last_position",
            sol::readonly( &SpellDetector::SpellInstance::mis_last_position ),
            "collision",
            sol::readonly( &SpellDetector::SpellInstance::collision ),
            "cc",
            sol::readonly( &SpellDetector::SpellInstance::cc ),
            "hitbox_area",
            sol::readonly( &SpellDetector::SpellInstance::hitbox_area ),
            "tether_area",
            sol::readonly( &SpellDetector::SpellInstance::tether_area ),
            "dodge_points",
            sol::readonly( &SpellDetector::SpellInstance::dodge_points ),
            "area_update_time",
            sol::readonly( &SpellDetector::SpellInstance::area_update_time ),
            "update_hitbox",
            sol::resolve( &SpellDetector::SpellInstance::update_hitbox ),
            "get_future_hitbox",
            sol::resolve( &SpellDetector::SpellInstance::get_future_hitbox ),
            "get_current_position",
            sol::resolve( &SpellDetector::SpellInstance::get_current_position ),
            "time_till_impact",
            sol::resolve( &SpellDetector::SpellInstance::time_till_impact ),
            "get_dynamic_line_endpos",
            sol::resolve( &SpellDetector::SpellInstance::get_dynamic_line_endpos ),
            "should_dodge",
            sol::resolve( &SpellDetector::SpellInstance::should_dodge ),
            "is_dangerous",
            sol::resolve( &SpellDetector::SpellInstance::is_dangerous ),
            "manual_end_time",
            sol::readonly( &SpellDetector::SpellInstance::manual_end_time )
        );


        state->new_usertype< SpellDetector::MissileInstance >(
            "missile_instance_t",
            "type",
            sol::readonly( &SpellDetector::MissileInstance::type ),
            "index",
            sol::readonly( &SpellDetector::MissileInstance::index ),
            "network_id",
            sol::readonly( &SpellDetector::MissileInstance::network_id ),
            "name",
            sol::readonly( &SpellDetector::MissileInstance::name ),
            "start_position",
            sol::readonly( &SpellDetector::MissileInstance::start_position ),
            "end_position",
            sol::readonly( &SpellDetector::MissileInstance::end_position ),
            "position",
            sol::readonly( &SpellDetector::MissileInstance::position ),
            "start_time",
            sol::readonly( &SpellDetector::MissileInstance::start_time ),
            "end_time",
            sol::readonly( &SpellDetector::MissileInstance::end_time ),
            "speed",
            sol::readonly( &SpellDetector::MissileInstance::speed ),
            "range",
            sol::readonly( &SpellDetector::MissileInstance::range ),
            "radius",
            sol::readonly( &SpellDetector::MissileInstance::radius ),
            "windup_time",
            sol::readonly( &SpellDetector::MissileInstance::windup_time ),
            "danger",
            sol::readonly( &SpellDetector::MissileInstance::danger ),
            "is_cc",
            sol::readonly( &SpellDetector::MissileInstance::cc ),
            "collision",
            sol::readonly( &SpellDetector::MissileInstance::collision ),
            "has_edge_radius",
            sol::readonly( &SpellDetector::MissileInstance::has_edge_radius ),
            "missile_spawn_time",
            sol::readonly( &SpellDetector::MissileInstance::missile_spawn_time ),
            "distance_to_end",
            sol::readonly( &SpellDetector::MissileInstance::distance_to_end ),
            "is_initialized",
            sol::readonly( &SpellDetector::MissileInstance::is_initialized ),
            "ignore_missile",
            sol::readonly( &SpellDetector::MissileInstance::ignore_missile ),
            "manual_update",
            sol::readonly( &SpellDetector::MissileInstance::manual_update ),
            "allow_dodge",
            sol::readonly( &SpellDetector::MissileInstance::allow_dodge ),
            "ignore_health_threshold",
            sol::readonly( &SpellDetector::MissileInstance::ignore_health_threshold ),
            "special_type",
            sol::readonly( &SpellDetector::MissileInstance::special_type ),
            "get_time_till_impact",
            sol::resolve( &SpellDetector::MissileInstance::time_till_impact ),
            "should_dodge",
            sol::resolve( &SpellDetector::MissileInstance::should_dodge ),
            "is_dangerous",
            sol::resolve( &SpellDetector::MissileInstance::is_dangerous )
        );

        debug_fn_call( )
        state->new_usertype< Evade >(
            "c_evade",
            "is_position_safe",
            sol::overload(
                sol::resolve< bool( const Vec3&, int, float, bool ) const >( &Evade::is_position_safe ),
                sol::resolve< bool( const Vec3&, int, float ) const >( &Evade::is_position_safe ),
                sol::resolve< bool( const Vec3&, int ) const >( &Evade::is_position_safe ),
                sol::resolve< bool( const Vec3& ) const >( &Evade::is_position_safe )
            ),
            "is_active",
            sol::resolve( &Evade::is_active ),
            "on_pre_call",
            sol::resolve( &Evade::push_pre_run_callback ),
            "on_post_call",
            sol::resolve( &Evade::push_post_run_callback ),
            "get_safe_position",
            sol::resolve( &Evade::get_safe_position_lua ),
            "disable_this_tick",
            sol::resolve( &Evade::disable_this_tick_lua ),
            "get_dangerous_spells",
            sol::resolve( &Evade::get_dangerous_spells_table ),
            "get_active_spells",
            sol::resolve( &Evade::get_active_spells_table ),
            "get_active_missiles",
            sol::resolve( &Evade::get_active_missiles_table ),
            "set_enabled",
            sol::resolve( &Evade::set_enabled ),
            "is_enabled",
            sol::resolve( &Evade::is_enabled )
        );
    }

    auto BuffCache::initialize_lua( sol::state* state ) -> void{
        debug_fn_call( )

        state->new_usertype< BuffCache >(
            "c_buff_cache",
            "get_buff",
            sol::resolve( &BuffCache::lua_get_buff ),
            "has_buff_of_type",
            sol::resolve( &BuffCache::lua_has_buff_of_type ),
            "has_hard_cc",
            sol::resolve( &BuffCache::lua_has_hard_cc ),
            "is_immobile",
            sol::resolve( &BuffCache::lua_is_immobile ),
            "can_cast",
            sol::resolve( &BuffCache::lua_can_cast ),
            "get_all_buffs",
            sol::resolve( &BuffCache::lua_get_all_buffs ),
            "on_pre_call",
            sol::resolve( &BuffCache::push_pre_run_callback ),
            "on_post_call",
            sol::resolve( &BuffCache::push_post_run_callback )
        );
    }

    auto EntityList2::initialize_lua( sol::state* state ) -> void{
        debug_fn_call( )

        state->new_enum< EntityList2::EObjectCategory >(
            "e_object_category",
            {
                { "ally_hero", EObjectCategory::ally_hero },
                { "ally_turret", EObjectCategory::ally_turret },
                { "ally_minion", EObjectCategory::ally_minion },
                { "ally_missile", EObjectCategory::ally_missile },
                { "ally_uncategorized", EObjectCategory::ally_uncategorized },
                { "enemy_hero", EObjectCategory::enemy_hero },
                { "enemy_turret", EObjectCategory::enemy_turret },
                { "enemy_minion", EObjectCategory::enemy_minion },
                { "enemy_missile", EObjectCategory::enemy_missile },
                { "enemy_uncategorized", EObjectCategory::enemy_uncategorized },
                { "error", EObjectCategory::error }
            }
        );

        state->new_usertype< EntityList2 >(
            "c_entity_list",
            "get_enemies",
            sol::resolve( &EntityList2::get_enemies_lua ),
            "get_allies",
            sol::resolve( &EntityList2::get_allies_lua ),
            "get_enemy_turrets",
            sol::resolve( &EntityList2::get_enemy_turrets_lua ),
            "get_ally_turrets",
            sol::resolve( &EntityList2::get_ally_turrets_lua ),
            "get_ally_minions",
            sol::resolve( &EntityList2::get_ally_minions_lua ),
            "get_enemy_minions",
            sol::resolve( &EntityList2::get_enemy_minions_lua ),
            "get_by_index",
            sol::resolve( &EntityList2::get_by_index_raw ),
            "get_by_network_id",
            sol::resolve( &EntityList2::get_by_network_id_raw ),
            // "on_pre_call",
            // &EntityList2::push_pre_run_callback,
            // "on_post_call",
            // &EntityList2::push_post_run_callback,
            "get_all",
            sol::resolve( &EntityList2::get_all_table ),
            "get_ally_missiles",
            sol::resolve( &EntityList2::get_ally_missiles_lua ),
            "get_enemy_missiles",
            sol::resolve( &EntityList2::get_enemy_missiles_lua ),
            "get_in_range",
            sol::resolve( &EntityList2::get_in_range_table ),
            "ally_uncategorized",
            sol::resolve( &EntityList2::get_ally_uncategorized_lua ),
            "enemy_uncategorized",
            sol::resolve( &EntityList2::get_enemy_uncategorized_lua ),
            "is_index_valid",
            sol::resolve( &EntityList2::is_index_valid_raw )
        );
    }

    auto Orbwalker::initialize_lua( sol::state* state ) -> void{
        debug_fn_call( )


        state->new_enum(
            "e_orbwalker_mode",
            "none",
            EOrbwalkerMode::none,
            "combo",
            EOrbwalkerMode::combo,
            "harass",
            EOrbwalkerMode::harass,
            "laneclear",
            EOrbwalkerMode::laneclear,
            "lasthit",
            EOrbwalkerMode::lasthit,
            "flee",
            EOrbwalkerMode::flee,
            "recalling",
            EOrbwalkerMode::recalling,
            "freeze",
            EOrbwalkerMode::freeze
        );

        state->new_usertype< OnSpellCastedT >(
            "on_spell_cast_data",
            "target",
            sol::readonly( &OnSpellCastedT::object ),
            "spell_name",
            sol::readonly( &OnSpellCastedT::spell_name ),
            "spell_slot",
            sol::readonly( &OnSpellCastedT::spell_slot )
        );

        state->new_usertype< LastHittableUnit >(
            "last_hittable_unit_t",
            "index",
            sol::readonly( &LastHittableUnit::index ),
            "is_glowing",
            sol::readonly( &LastHittableUnit::is_glowing ),
            "network_id",
            sol::readonly( &LastHittableUnit::network_id )
        );

        state->new_usertype< PingInstance >(
            "ping_instance_t",
            "ping",
            sol::readonly( &PingInstance::ping ),
            "ping_time",
            sol::readonly( &PingInstance::ping_time )
        );
        state->new_usertype< IgnoredMinion >(
            "ignored_minion_t",
            "index",
            sol::readonly( &IgnoredMinion::index ),
            "end_time",
            sol::readonly( &IgnoredMinion::end_time )
        );

        state->new_usertype< Orbwalker >(
            "c_orbwalker",
            "send_attack",
            sol::overload(
                sol::resolve< bool( uintptr_t ) >( &Orbwalker::send_attack ),
                sol::resolve< bool( const Object* ) >( &Orbwalker::send_attack )
            ),
            "is_in_attack",
            sol::resolve( &Orbwalker::in_attack ),
            "send_move_input",
            sol::overload(
                sol::resolve< bool( Vec3, bool ) >( &Orbwalker::lua_send_move_input ),
                sol::resolve< bool( Vec3 ) >( &Orbwalker::lua_send_move_input ),
                sol::resolve< bool( ) >( &Orbwalker::lua_send_move_input )
            ),
            "set_cast_time",
            sol::resolve( &Orbwalker::set_cast_time ),
            "can_move",
            sol::resolve( &Orbwalker::can_move ),
            "is_attackable",
            sol::overload(
                sol::resolve< bool(
                    int16_t,
                    float,
                    bool,
                    bool
                ) const >( &Orbwalker::is_attackable ),
                sol::resolve< bool(
                    int16_t,
                    float,
                    bool
                ) const >( &Orbwalker::is_attackable ),
                sol::resolve< bool(
                    int16_t,
                    float
                ) const >( &Orbwalker::is_attackable ),
                sol::resolve< bool(
                    int16_t
                ) const >( &Orbwalker::is_attackable )
            ),
            "get_ping",
            sol::resolve( &Orbwalker::get_ping ),
            "get_mode",
            sol::resolve( &Orbwalker::get_mode ),
            "reset_aa_timer",
            sol::resolve( &Orbwalker::reset_aa_timer ),
            "can_attack",
            sol::overload(
                sol::resolve< bool( int16_t ) const >( &Orbwalker::can_attack ),
                sol::resolve< bool( const Object* ) const >( &Orbwalker::can_attack ),
                sol::resolve< bool( ) const >( &Orbwalker::can_attack )
            ),
            "should_reset_aa",
            sol::resolve( &Orbwalker::should_reset_aa ),
            "on_pre_call",
            sol::resolve( &Orbwalker::push_pre_run_callback ),
            "on_post_call",
            sol::resolve( &Orbwalker::push_post_run_callback ),
            "get_attack_cast_delay",
            sol::resolve( &Orbwalker::get_attack_cast_delay ),
            "allow_movement",
            sol::resolve( &Orbwalker::allow_movement ),
            "allow_attacks",
            sol::resolve( &Orbwalker::allow_attacks ),
            "get_last_attack_start_time",
            sol::resolve( &Orbwalker::last_attack_time ),
            "get_last_attack_ending_time",
            sol::resolve( &Orbwalker::last_attack_ending_time ),
            "get_last_attack_time",
            sol::resolve( &Orbwalker::last_attack_time ),
            "get_previous_attack_time",
            sol::resolve( &Orbwalker::previous_attack_time ),
            "get_cast_spell_time",
            sol::resolve( &Orbwalker::cast_spell_time ),
            "on_cast",
            sol::resolve( &Orbwalker::on_cast ),
            "is_in_action",
            sol::resolve( &Orbwalker::in_action ),
            "is_ignored",
            sol::resolve( &Orbwalker::is_ignored ),
            "ignore_minion",
            sol::resolve( &Orbwalker::ignore_minion ),
            "ignore_spell_during_attack",
            sol::resolve( &Orbwalker::ignore_spell_during_attack ),
            "get_attack_delay",
            sol::resolve( &Orbwalker::get_attack_delay ),
            "is_winding_down",
            sol::resolve( &Orbwalker::is_winding_down ),
            "get_next_aa_time",
            sol::resolve( &Orbwalker::get_next_aa_time ),
            "get_next_possible_aa_time",
            sol::resolve( &Orbwalker::get_next_possible_aa_time ),
            "allow_fast_move",
            sol::resolve( &Orbwalker::allow_fast_move ),
            "is_movement_disabled",
            sol::resolve( &Orbwalker::is_movement_disabled ),
            "disable_for",
            sol::resolve( &Orbwalker::disable_for ),
            "disable_autoattack_until",
            sol::resolve( &Orbwalker::disable_autoattack_until ),
            "disable_movement_until",
            sol::resolve( &Orbwalker::disable_movement_until ),
            "set_enabled",
            sol::resolve( &Orbwalker::set_enabled ),
            "last_attack_end_time",
            sol::property( &Orbwalker::last_attack_end_time ),
            "last_move_time",
            sol::property( &Orbwalker::last_move_time ),
            "override_target",
            sol::overload(
                sol::resolve< void( int16_t ) >( &Orbwalker::override_target ),
                sol::resolve< void( int16_t, bool ) >( &Orbwalker::override_target )
            ),
            "is_unit_headshottable",
            sol::resolve( &Orbwalker::is_unit_headshottable ),
            "get_target_autospacing_position",
            sol::resolve( &Orbwalker::get_sticky_position ),
            "get_last_target",
            sol::resolve( &Orbwalker::get_last_target ),
            "get_last_target_index",
            sol::resolve( &Orbwalker::get_last_target_index ),
            "is_hard_crowdcontrolled",
            sol::resolve( &Orbwalker::is_hard_crowdcontrolled ),
            "get_support_limiter_active",
            sol::resolve( &Orbwalker::support_limiter_active ),
            "set_last_attack_time",
            sol::resolve( &Orbwalker::set_last_attack_time ),
            "set_last_cast",
            sol::resolve( &Orbwalker::set_last_cast ),
            // "is_autospacing",
            "get_aa_missile_speed",
            sol::resolve( &Orbwalker::get_aa_missile_speed ),
            "get_spellfarm_target_index",
            sol::resolve( &Orbwalker::get_spellfarm_target_index ),
            "should_attack_target",
            sol::resolve( &Orbwalker::should_attack_target ),
            "get_extrapolated_cursor_position",
            sol::resolve( &Orbwalker::get_extrapolated_cursor_position ),
            "get_nearest_turret",
            sol::resolve( &Orbwalker::get_nearest_turret ),
            "get_turret_shot_damage",
            sol::resolve( &Orbwalker::get_turret_shot_damage ),
            "is_position_under_turret",
            sol::resolve( &Orbwalker::is_position_under_turret ),
            "get_turret_next_target",
            sol::overload(
                sol::resolve< Object*( const Object* ) const >( &Orbwalker::get_turret_next_target ),
                sol::resolve< Object*( const Object*, uint32_t ) const >( &Orbwalker::get_turret_next_target )
            ),
            "get_turret_current_target",
            sol::resolve( &Orbwalker::get_turret_current_target ),
            "is_enabled",
            sol::resolve( &Orbwalker::is_enabled ),
            "is_autoattack_allowed",
            sol::resolve( &Orbwalker::is_autoattack_allowed ),
            "is_movement_allowed",
            sol::resolve( &Orbwalker::is_movement_allowed ),
            "get_lasthit_target",
            sol::resolve( &Orbwalker::get_lasthit_target ),
            "get_laneclear_target",
            sol::resolve( &Orbwalker::get_laneclear_target ),
            "get_freeze_target",
            sol::resolve( &Orbwalker::get_freeze_target ),
            "get_senna_soul_target",
            sol::resolve( &Orbwalker::get_senna_soul_target ),
            "get_xayah_passive_harass_target",
            sol::resolve( &Orbwalker::get_xayah_passive_harass_target ),
            "get_special_target",
            sol::resolve( &Orbwalker::get_special_target ),
            "get_special_target_low_priority",
            sol::resolve( &Orbwalker::get_special_target_low_priority ),
            "get_turret_target",
            sol::resolve( &Orbwalker::get_turret_target ),
            "can_attack_turret",
            sol::resolve( &Orbwalker::can_attack_turret ),
            "get_magnet_position",
            sol::resolve( &Orbwalker::get_magnet_position ),
            "get_magnet_position",
            sol::resolve( &Orbwalker::get_magnet_position ),
            "can_override_attack",
            sol::resolve( &Orbwalker::can_override_attack )
        );
    }

    auto Prediction::initialize_lua( sol::state* state ) -> void{
        debug_fn_call( )
        state->new_usertype< PredictionResult >(
            "prediction_result_t",
            "valid",
            sol::readonly( &PredictionResult::valid ),
            "position",
            sol::readonly( &PredictionResult::position ),
            "hitchance",
            sol::readonly( &PredictionResult::hitchance ),
            "reason",
            sol::readonly( &PredictionResult::reason )
        );

        state->globals( ).new_enum< EPredictionFlags >(
            "e_prediction_flags",
            {
                { "include_ping", EPredictionFlags::include_ping },
                { "render_thread", EPredictionFlags::render_thread },
                { "extend_range_with_hitbox", EPredictionFlags::extend_range_with_hitbox },
                { "check_range_from_local_position", EPredictionFlags::check_range_from_local_position },
                { "extend_crowdcontrol", EPredictionFlags::extend_crowdcontrol }
            }
        );


        state->new_usertype< Prediction >(
            "c_prediction",
            "predict",
            sol::overload(
                sol::resolve< Prediction::PredictionResult(
                    int16_t,
                    float,
                    float,
                    float,
                    float,
                    Vec3,
                    bool,
                    int32_t,
                    ESpellType
                ) >( &Prediction::predict ),
                sol::resolve< Prediction::PredictionResult(
                    int16_t,
                    float,
                    float,
                    float,
                    float,
                    Vec3,
                    bool,
                    int32_t
                ) >( &Prediction::predict ),
                sol::resolve< Prediction::PredictionResult(
                    int16_t,
                    float,
                    float,
                    float,
                    float,
                    Vec3,
                    bool
                ) >( &Prediction::predict ),
                sol::resolve< Prediction::PredictionResult(
                    int16_t,
                    float,
                    float,
                    float,
                    float,
                    Vec3
                ) >( &Prediction::predict ),
                sol::resolve< Prediction::PredictionResult(
                    int16_t,
                    float,
                    float,
                    float,
                    float
                ) >( &Prediction::predict )
            ),
            "predict_default",
            sol::overload(
                sol::resolve< std::optional< sdk::math::Vec3 >
                    ( int32_t, float ) const >( &Prediction::predict_default ),
                sol::resolve< std::optional< sdk::math::Vec3 >(
                    const int32_t,
                    float,
                    const bool
                ) const >( &Prediction::predict_default )
            ),
            "predict_health",
            sol::overload(
                sol::resolve< float(
                    const sdk::game::Object* object,
                    float                    time,
                    bool                     multiple_attacks,
                    bool                     ignore_turret,
                    bool                     ignore_minions
                ) >( &Prediction::predict_health ),
                sol::resolve< float(
                    const sdk::game::Object* object,
                    float                    time,
                    bool                     multiple_attacks,
                    bool                     ignore_turret
                ) >( &Prediction::predict_health ),
                sol::resolve< float(
                    const sdk::game::Object* object,
                    float                    time,
                    bool                     multiple_attacks
                ) >( &Prediction::predict_health ),
                sol::resolve< float(
                    const sdk::game::Object* object,
                    float                    time
                ) >( &Prediction::predict_health ),
                sol::resolve< float(
                    const sdk::game::Object* object,
                    float                    time,
                    bool                     multiple_attacks,
                    bool                     ignore_turret,
                    bool                     ignore_minions,
                    bool                     override_delay
                ) >( &Prediction::predict_health )
            ),
            "minion_in_line",
            sol::overload(
                sol::resolve< bool( const sdk::math::Vec3&, const sdk::math::Vec3&, float, unsigned, float ) >(
                    &Prediction::minion_in_line
                ),
                sol::resolve< bool(
                    const sdk::math::Vec3&,
                    const sdk::math::Vec3&,
                    float,
                    unsigned
                ) >( &Prediction::minion_in_line ),
                sol::resolve< bool( const sdk::math::Vec3&, const sdk::math::Vec3&, float ) >(
                    &Prediction::minion_in_line
                )
            ),
            "on_pre_call",
            sol::resolve( &Prediction::push_pre_run_callback ),
            "on_post_call",
            sol::resolve( &Prediction::push_post_run_callback ),
            "count_minions_in_line",
            sol::overload(
                sol::resolve< int32_t( const Vec3&, const Vec3&, float, unsigned ) >(
                    &Prediction::count_minions_in_line
                ),
                sol::resolve< int32_t( const Vec3&, const Vec3&, float ) >( &Prediction::count_minions_in_line )
            ),
            "predict_minion_health",
            sol::overload(
                sol::resolve< float( int16_t, float, bool ) >( &Prediction::predict_minion_health ),
                sol::resolve< float( int16_t, float ) >( &Prediction::predict_minion_health )
            ),
            "set_enabled",
            sol::resolve( &Prediction::set_enabled ),
            "is_enabled",
            sol::resolve( &Prediction::is_enabled )
        );
    }

    auto ITargetSelector::initialize_lua( sol::state* state ) -> void{
    }

    auto ITargetSelector::static_initialize_lua( sol::state* state ) -> void{
        debug_fn_call( )
        state->new_usertype< ITargetSelector >(
            "c_target_selector",
            "get_default_target",
            sol::resolve< Object*( ) >( &ITargetSelector::get_default_target ),
            "get_secondary_target",
            sol::resolve( &ITargetSelector::get_secondary_target ),
            "get_orbwalker_default_target",
            sol::resolve< Object*( ) >( &ITargetSelector::get_orbwalker_default_target ),
            "get_orbwalker_secondary_target",
            sol::resolve< Object*( ) >( &ITargetSelector::get_orbwalker_secondary_target ),
            "get_forced_target",
            sol::resolve( &ITargetSelector::get_forced_target ),
            "is_forced",
            sol::resolve( &ITargetSelector::is_forced ),
            "is_bad_target",
            sol::overload(
                sol::resolve< bool(
                    int16_t,
                    bool,
                    bool
                ) >( &ITargetSelector::is_bad_target ),
                sol::resolve< bool(
                    int16_t,
                    bool
                ) >( &ITargetSelector::is_bad_target ),
                sol::resolve< bool(
                    int16_t
                ) >( &ITargetSelector::is_bad_target )
            ),
            "force_target",
            sol::resolve( &ITargetSelector::lua_force_target ),
            "on_pre_call",
            sol::resolve( &ITargetSelector::push_pre_run_callback ),
            "on_post_call",
            sol::resolve( &ITargetSelector::push_post_run_callback ),
            "get_antigapclose_target",
            sol::resolve( &ITargetSelector::lua_get_antigapclose_target ),
            "set_enabled",
            sol::resolve( &ITargetSelector::set_enabled ),
            "set_forced_target",
            sol::overload(
                sol::resolve< bool( Object* ) >( &ITargetSelector::set_forced_target ),
                sol::resolve< bool( int16_t ) >( &ITargetSelector::set_forced_target )
            ),
            "reset_forced_target",
            sol::resolve( &ITargetSelector::reset_forced_target ),
            "get_spell_specific_target",
            sol::resolve( &ITargetSelector::get_spell_specific_target ),
            "get_killsteal_target",
            sol::resolve( &ITargetSelector::get_killsteal_target ),
            "get_target_priority",
            sol::resolve( &ITargetSelector::get_target_priority )
        );
    }

    void Activator::initialize_lua( sol::state* state ){
        state->new_usertype< Activator >(
            "c_activator",
            // sol::resolve(& Activator::EMapSide),
            //  sol::resolve(& Activator::ELane),
            //  sol::resolve(& Activator::AllyData),
            // sol::resolve(&  Activator::SpellInfo),
            //  sol::resolve(& Activator::EnemySpellData),
            //  sol::resolve(& Activator::SpellCooldownData),
            //  sol::resolve(& Activator::JunglerData),
            //  sol::resolve(& Activator::PingableWard),
            //   sol::resolve(&Activator::PingInstance),
            //  sol::resolve(& Activator::TrackedEnemy),
            //   sol::resolve(&Activator::WardPoint),
            //  sol::resolve(& Activator::WardOccurrence),
            // "run_auto_potion",
            // sol::resolve( &Activator::autopotion ),
            // sol::resolve( &Activator::auto_qss ),
            // sol::resolve( &Activator::auto_everfrost ),
            // sol::resolve( &Activator::auto_everfrost_antigapclose ),
            // sol::resolve( &Activator::auto_goredrinker ),
            // sol::resolve( &Activator::auto_mikaels ),
            // sol::resolve( &Activator::auto_redemption ),
            // sol::resolve( &Activator::autosmite ),
            // sol::resolve( &Activator::autoheal ),
            // sol::resolve( &Activator::autobarrier ),
            // sol::resolve( &Activator::autocleanse ),
            // sol::resolve( &Activator::autoignite ),
            // sol::resolve( &Activator::autoexhaust ),
            // sol::resolve( &Activator::auto_question ),
            // sol::resolve( &Activator::auto_encourage_ally ),
            // sol::resolve( &Activator::auto_say_cooldowns ),
            // sol::resolve( &Activator::autoward ),
            "get_humanized_name",
            sol::resolve( &Activator::get_humanized_name ),
            "is_ally_logged",
            sol::resolve( &Activator::is_ally_logged ),
            "is_support",
            sol::resolve( &Activator::is_support ),
            "is_enemy_tracked",
            sol::resolve( &Activator::is_enemy_tracked ),
            // sol::resolve( &Activator::auto_gank_warn ),
            "get_mapside",
            sol::resolve( &Activator::get_mapside ),
            // sol::resolve( &Activator::auto_cancerping ),
            "can_ping",
            sol::resolve( &Activator::can_ping ),
            "remove_pingable_ward",
            sol::resolve( &Activator::remove_pingable_ward ),
            "on_ping",
            sol::resolve( &Activator::on_ping ),
            "manual_ping_ward",
            sol::resolve( &Activator::manual_ping_ward ),
            "is_enemy_saved",
            sol::resolve( &Activator::is_enemy_saved ),
            "get_lane",
            sol::resolve( &Activator::get_lane ),
            "simulate_ward_vision",
            sol::resolve( &Activator::simulate_ward_vision ),
            "is_ward_tracked",
            sol::resolve( &Activator::is_ward_tracked ),
            "remove_ally_ward",
            sol::resolve( &Activator::remove_ally_ward ),
            "add_pingable_ward",
            sol::resolve( &Activator::add_pingable_ward ),
            "is_ward_pingable",
            sol::resolve( &Activator::is_ward_pingable ),
            "add_ping_instance",
            sol::resolve( &Activator::add_ping_instance ),
            "is_cooldown_logged",
            sol::resolve( &Activator::is_cooldown_logged ),
            "remove_cooldown",
            sol::resolve( &Activator::remove_cooldown ),
            "set_last_camp",
            sol::resolve( &Activator::set_last_camp ),
            "set_enabled",
            sol::resolve( &Activator::set_enabled ),
            "is_enabled",
            sol::resolve( &Activator::is_enabled )
        );
    }
}
#endif
