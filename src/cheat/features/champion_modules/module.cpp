#include "pch.hpp"

#include "module.hpp"

#include "../entity_list.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#if enable_new_lua
#include "../../lua-v2/custom_structs.hpp"
#endif
#include "../target_selector/target_selector.hpp"
#include "../../utils/input.hpp"
#include "../../sdk/math/geometry.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"

#include "../../sdk/game/spell_cast_info.hpp"
#include "../../sdk/game/spell_info.hpp"
#include "../../sdk/game/spell_data.hpp"

namespace features::champion_modules {
    auto IModule::run( ) -> void{
        if ( !g_features->buff_cache->can_cast( g_local->index ) ) return;

        initialize_spell_slots( );

        if ( !m_slot_q || !m_slot_w || !m_slot_e || !m_slot_r ) return;

        for ( const auto element : m_priority_list ) {
            switch ( element ) {
            case q_spell:
            {
                // debug_log( "q_spell {}", !m_slot_q );
                if ( !m_slot_q || !m_slot_q->is_ready( true ) || !should_run( m_disable_q ) ) continue;

                // debug_log( "q_spell" );

                if ( spell_q( ) ) return;
            }
            break;
            case w_spell:
            {
                if ( !m_slot_w || !m_slot_w->is_ready( true ) || !should_run( m_disable_w ) ) continue;

                if ( spell_w( ) ) return;
            }
            break;
            case e_spell:
            {
                if ( !m_slot_e || !m_slot_e->is_ready( true ) || !should_run( m_disable_e ) ) continue;

                if ( spell_e( ) ) return;
            }
            break;
            case r_spell:
            {
                if ( !m_slot_r || !m_slot_r->is_ready( true ) || !should_run( m_disable_r ) ) continue;

                if ( spell_r( ) ) return;
            }
            break;
            default: ;
            }
        }
    }

    auto IModule::initialize_spell_slots( ) -> void{
        if ( m_last_spell_update_time != *g_time ) {
            m_slot_q                 = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            m_slot_w                 = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            m_slot_e                 = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            m_slot_r                 = g_local->spell_book.get_spell_slot( ESpellSlot::r );
            m_last_spell_update_time = *g_time;
        }
    }

    auto IModule::should_run( const std::vector< spell_callback_t >& checks ) -> bool{
        for ( auto check : checks ) if ( check( ) ) return false;

        return true;
    }

    auto IModule::cast_spell( const ESpellSlot slot ) -> bool{ return g_input->cast_spell( slot ); }

    auto IModule::cast_spell( const ESpellSlot slot, const Vec3& position ) -> bool{
        if ( position.length( ) == 0.f ) return false;
        return g_input->cast_spell( slot, position );
    }

    auto IModule::cast_spell(
        const ESpellSlot slot,
        const Vec3&      start_position,
        const Vec3&      end_position
    ) -> bool{
        if ( start_position.length( ) == 0.f || end_position.length( ) == 0.f ) return false;
        return g_input->cast_spell( slot, start_position, end_position );
    }

    auto IModule::cast_spell( const ESpellSlot slot, const unsigned nid ) -> bool{
        return g_input->cast_spell( slot, nid );
    }

    auto IModule::release_chargeable( const ESpellSlot slot, const Vec3& position, const bool release ) -> bool{
        return g_input->release_chargeable( slot, position, release );
    }

    auto IModule::get_zeri_laneclear_target(
        const std::function< float( Object* unit ) >& get_damage,
        const std::function< float( Object* unit ) >& get_traveltime,
        const float                                   range,
        float                                         width,
        const bool                                    has_collision
    ) -> std::optional< IModule::LaneclearInfo >{
        LaneclearInfo info{ };

        std::vector< Object* > candidates{ };

        for ( auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                minion->dist_to_local( ) > range + 100.f ||
                minion->is_invisible( ) && minion->get_ward_type( ) != Object::EWardType::control && minion->
                get_ward_type( ) != Object::EWardType::blue ||
                !minion->is_normal_minion( ) && minion->get_ward_type( ) != Object::EWardType::control && minion->
                get_ward_type( ) != Object::EWardType::blue ||
                minion->get_minion_type( ) == Object::EMinionType::plant ||
                g_features->orbwalker->is_ignored( minion->index ) )
                continue;

            if ( minion->is_lane_minion( ) ) {
                auto pred = g_features->prediction->predict_default( minion->index, get_traveltime( minion ) );
                if ( !pred || pred.value( ).dist_to( g_local->position ) > range || has_collision && g_features->
                    prediction->minion_in_line( g_local->position, pred.value( ), 40.f, minion->network_id ) )
                    continue;
            } else if ( has_collision && g_features->prediction->minion_in_line(
                g_local->position,
                minion->position,
                40.f,
                minion->network_id
            ) )
                continue;

            candidates.push_back( minion );
        }

        const Object*       target{ };
        Object::EMinionType target_type{ };
        bool                target_found{ };
        bool                is_lasthit{ };
        float               tt{ };
        float               dealt_damage{ };
        Vec3                cast_position;
        int16_t             target_index{ };

        const auto is_fastclear = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::laneclear &&
            GetAsyncKeyState( VK_CONTROL );

        for ( const auto minion : candidates ) {
            const auto damage      = get_damage( minion );
            const auto travel_time = get_traveltime( minion );

            if ( minion->get_minion_type( ) == Object::EMinionType::jungle || minion->get_minion_type( ) ==
                Object::EMinionType::misc ) {
                auto pred = g_features->prediction->predict_default( minion->index, travel_time );
                if ( !pred || g_local->position.dist_to( pred.value( ) ) > range ) continue;

                if ( !target_found || target_type < minion->get_minion_type( ) ) {
                    target        = minion;
                    target_type   = minion->get_minion_type( );
                    cast_position = !pred ? minion->position : pred.value( );
                    target_found  = true;
                    is_lasthit    = damage >= minion->health;
                    target_index  = minion->index;
                }
            } else {
                auto pred = g_features->prediction->predict_default( minion->index, travel_time );
                if ( !pred || g_local->position.dist_to( *pred ) > range ) continue;

                const auto future_health = g_features->prediction->predict_health( minion, travel_time );
                const auto pre_aa_health = g_features->prediction->predict_health(
                    minion,
                    travel_time + g_features->orbwalker->get_attack_cast_delay( ) + g_features->orbwalker->get_ping( )
                );

                if ( future_health <= damage && ( !is_lasthit || target_type < minion->get_minion_type( ) ) ) {
                    target_found  = true;
                    is_lasthit    = true;
                    target        = minion;
                    target_type   = minion->get_minion_type( );
                    dealt_damage  = damage;
                    tt            = travel_time;
                    cast_position = pred.value( );
                    target_index  = minion->index;
                    continue;
                }

                if ( is_lasthit ) continue;

                const auto is_aa_ready = g_features->orbwalker->is_attackable( minion->index ) && ( g_features->
                    orbwalker->
                    can_attack( minion->index )
                    || g_features->orbwalker->get_next_aa_time( ) <= *g_time + g_features->orbwalker->
                    get_attack_cast_delay( ) + g_features->orbwalker->get_ping( ) * 2.f );

                const auto aa_damage = helper::get_aa_damage( minion->index, false ) >
                                       helper::zeri_execute_threshold( )
                                           ? helper::get_aa_damage( minion->index, false )
                                           : helper::zeri_execute_threshold( );


                if ( ( is_fastclear || !g_features->prediction->is_minion_in_danger( minion->index ) || is_aa_ready &&
                        pre_aa_health - damage > aa_damage / 2.f || !is_aa_ready && pre_aa_health - damage >= damage )
                    &&
                    ( !target || target_type < minion->get_minion_type( ) || target_type == minion->get_minion_type( )
                        && minion->health < target->health ) ) {
                    target_found = true;
                    target       = minion;
                    target_type  = minion->get_minion_type( );
                    dealt_damage = damage;

                    tt            = travel_time;
                    cast_position = pred.value( );
                    target_index  = minion->index;
                }
            }
        }


        bool is_turret{ };

        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret || turret->is_dead( ) || turret->dist_to_local( ) > range
                || target_found && g_features->prediction->minion_in_line( g_local->position, turret->position, 40.f ) )
                continue;

            target_index  = turret->index;
            cast_position = turret->position;
            is_turret     = true;
            break;
        }

        if ( target_index == 0 ) return std::nullopt;


        info.travel_time   = tt;
        info.damage        = dealt_damage;
        info.target_index  = target_index;
        info.cast_position = cast_position;
        info.will_execute  = !is_turret && is_lasthit;

        return std::make_optional( info );
    }

    auto IModule::get_zeri_lasthit_target(
        const std::function< float( Object* unit ) >& get_damage,
        const std::function< float( Object* unit ) >& get_traveltime,
        const float                                   range,
        const float                                   width,
        const bool                                    collision
    ) -> std::optional< LasthitInfo >{
        LasthitInfo info{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                g_local->position.dist_to( minion->position ) > range + 100.f ||
                minion->is_invisible( ) && minion->get_ward_type( ) != Object::EWardType::control && minion->
                get_ward_type( ) != Object::EWardType::blue ||
                !minion->is_normal_minion( ) ||
                minion->is_untargetable_minion( ) ||
                g_features->orbwalker->is_ignored( minion->index ) )
                continue;

            const auto travel_time   = get_traveltime( minion );
            const auto health_on_hit = g_features->prediction->predict_health( minion, travel_time );
            const auto damage        = get_damage( minion );

            if ( health_on_hit <= 0.f || health_on_hit >= damage ) continue;

            auto pred = g_features->prediction->predict_default( minion->index, travel_time );
            if ( !pred || g_local->position.dist_to( *pred ) > range ||
                collision && g_features->prediction->minion_in_line(
                    g_local->position,
                    *pred,
                    width,
                    minion->network_id
                ) )
                continue;

            if ( info.index != 0 && info.travel_time < travel_time ) continue;

            info.cast_position = pred.value( );
            info.damage        = damage;
            info.travel_time   = travel_time;
            info.index         = minion->index;
        }

        if ( info.index == 0 ) return std::nullopt;

        return info;
    }

    auto IModule::get_line_lasthit_target_advanced(
        const std::function< float( Object* unit ) >& get_damage,
        const std::function< float( Object* unit ) >& get_traveltime,
        const float                                   range,
        const float                                   radius,
        const float                                   cast_delay,
        const int                                     collision_limit,
        const bool                                    conserve_mana
    ) -> std::optional< LasthitInfo >{
        LasthitInfo info{ };
        bool        found_target{ };

        const auto next_aa_time = g_features->orbwalker->get_next_aa_time( );

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > range * 1.25f
                || !minion->is_normal_minion( ) || minion->is_minion_only_autoattackable( ) ||
                g_features->orbwalker->is_ignored( minion->index ) )
                continue;

            const auto travel_time = get_traveltime( minion );
            const auto damage      = get_damage( minion );

            if ( conserve_mana && g_features->orbwalker->is_attackable( minion->index ) ) {
                const auto next_aa_health = g_features->prediction->predict_minion_health(
                    minion->index,
                    next_aa_time > *g_time ? next_aa_time - *g_time : 0.f
                );
                const auto aa_damage = helper::get_aa_damage( minion->index, true );

                if ( next_aa_health > aa_damage * 0.75f || !g_features->prediction->
                                                                        is_minion_in_danger( minion->index ) )
                    continue;
            }

            const auto health_on_hit      = g_features->prediction->predict_minion_health( minion->index, travel_time );
            const auto late_health_on_hit = g_features->prediction->predict_minion_health(
                minion->index,
                travel_time + g_features->orbwalker->get_ping( ),
                true
            );

            if ( health_on_hit <= 0.f || late_health_on_hit <= 0.f ||
                health_on_hit >= damage || late_health_on_hit >= damage )
                continue;

            auto pred = g_features->prediction->predict_default( minion->index, travel_time );
            if ( !pred || pred.value( ).dist_to( g_local->position ) >= range ||
                collision_limit > 0 && g_features->prediction->minions_in_line(
                    g_local->position,
                    *pred,
                    radius,
                    minion->network_id,
                    cast_delay,
                    travel_time
                ) > collision_limit )
                continue;

            if ( found_target && info.travel_time < travel_time ) continue;

            info.cast_position = pred.value( );
            info.damage        = damage;
            info.travel_time   = travel_time;
            info.index         = minion->index;
            found_target       = true;
        }

        if ( !found_target ) return std::nullopt;

        return std::make_optional( info );
    }


    auto IModule::get_line_lasthit_target(
        const std::function< float( Object* unit ) >& get_damage,
        const float                                   range,
        const float                                   speed,
        const float                                   delay,
        const float                                   width,
        const bool                                    has_collision
    ) -> std::optional< IModule::LasthitInfo >{
        LasthitInfo info{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                g_local->position.dist_to( minion->position ) > range ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                !minion->is_normal_minion( ) ||
                minion->is_minion_only_autoattackable( ) ||
                g_features->orbwalker->is_ignored( minion->index )
            )
                continue;

            auto travel_time = delay + speed > 0.f ? g_local->position.dist_to( minion->position ) / speed : 0.f;

            auto collision = has_collision;

            switch ( helper::get_current_hero( ) ) {
            case EHeroes::samira:
                if ( minion->dist_to_local( ) < 340.f ) {
                    travel_time = delay;
                    collision   = false;
                }

                break;
            default:
                break;
            }

            const auto health_on_hit = g_features->prediction->predict_health(
                minion,
                travel_time - g_features->orbwalker->get_ping( )
            );
            const auto late_health_on_hit =
                g_features->prediction->predict_health( minion, travel_time + g_features->orbwalker->get_ping( ) );
            const auto damage = get_damage( minion );

            if ( health_on_hit <= 0.f || late_health_on_hit <= 0.f || health_on_hit >= damage ) continue;

            auto pred = g_features->prediction->predict_default( minion->index, travel_time );
            if ( !pred || pred.value( ).dist_to( g_local->position ) >= range ||
                collision && g_features->prediction->minion_in_line(
                    g_local->position,
                    *pred,
                    width,
                    minion->network_id
                ) )
                continue;

            if ( info.index != 0 && info.travel_time < travel_time ) continue;

            info.cast_position = pred.value( );
            info.damage        = damage;
            info.travel_time   = travel_time;
            info.index         = minion->index;
        }

        if ( info.index == 0 ) return std::nullopt;

        return info;
    }

    auto IModule::get_line_laneclear_target(
        const std::function< float( Object* unit ) >& get_damage,
        const std::function< float( Object* unit ) >& get_travel_time,
        const float                                   range,
        const float                                   width,
        const bool                                    has_collision,
        const bool                                    only_jungle_monsters,
        const bool                                    fast
    ) -> std::optional< IModule::LaneclearInfo >{
        Object::EMinionType target_type{ };
        bool                target_found{ };
        Vec3                cast_position{ };
        Object*             target{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                g_features->orbwalker->is_ignored( minion->index ) ||
                minion->dist_to_local( ) > range * 1.1f ||
                !minion->is_normal_minion( ) ||
                only_jungle_monsters && !minion->is_jungle_monster( ) ||
                minion->is_minion_only_autoattackable( )
            )
                continue;

            const auto health_on_hit = g_features->prediction->predict_minion_health(
                minion->index,
                get_travel_time( minion )
            );
            const auto health_on_next_aa = g_features->prediction->predict_minion_health(
                minion->index,
                get_travel_time( minion ) + g_features->orbwalker->get_attack_cast_delay( ) * 2
            );
            const auto damage = get_damage( minion );

            if ( health_on_hit <= damage ) continue;

            auto collision = has_collision;

            switch ( helper::get_current_hero( ) ) {
            case EHeroes::samira:
                if ( minion->dist_to_local( ) < 340.f ) collision = false;
                break;
            default:
                break;
            }

            auto pred = g_features->prediction->predict_default( minion->index, get_travel_time( minion ) );
            if ( !pred || pred.value( ).dist_to( g_local->position ) >= range || collision && g_features->prediction->
                minion_in_line( g_local->position, pred.value( ), width, minion->network_id ) )
                continue;

            if ( ( health_on_next_aa - damage > damage || fast || !g_features->prediction->is_minion_in_danger(
                    minion->index
                ) ) &&
                ( !target_found || target_type < minion->get_minion_type( ) )
            ) {
                target_found  = true;
                target_type   = minion->get_minion_type( );
                target        = minion;
                cast_position = pred.value( );
            }
        }

        if ( !target ) return { };

        LaneclearInfo data;

        data.cast_position = cast_position;
        data.target_index  = target->index;
        data.travel_time   = get_travel_time( target );
        data.damage        = get_damage( target );

        return data;
    }

    auto IModule::get_targetable_lasthit_target(
        const std::function< float( Object* unit ) >& get_damage,
        const float                                   range,
        const std::function< float( Object* unit ) >& get_travel_time
    ) -> std::optional< IModule::TargetableLasthitInfo >{
        TargetableLasthitInfo info{ };

        const auto current_hero = helper::get_current_hero( );

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                g_local->position.dist_to( minion->position ) > range ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                !minion->is_lane_minion( ) &&
                !minion->is_jungle_monster( ) ||
                g_features->orbwalker->is_ignored( minion->index ) )
                continue;

            switch ( current_hero ) {
            case EHeroes::yasuo:
                if ( g_features->buff_cache->get_buff( minion->index, ct_hash( "YasuoE" ) ) ) continue;
                break;
            default:
                break;
            }

            const auto travel_time   = get_travel_time( minion );
            const auto health_on_hit = g_features->prediction->predict_minion_health( minion->index, travel_time );
            const auto damage        = get_damage( minion );

            if ( health_on_hit <= 0 || health_on_hit >= damage ) continue;

            if ( info.network_id != 0 && info.type > minion->get_minion_type( ) ) continue;

            info.damage      = damage;
            info.travel_time = travel_time;
            info.network_id  = minion->network_id;
            info.index       = minion->index;
            info.type        = minion->get_minion_type( );
        }

        if ( !info.network_id ) return { };

        return info;
    }

    auto IModule::get_circle_laneclear_target(
        const std::function< float( Object* unit ) >& get_damage,
        const float                                   range,
        const float                                   speed,
        const float                                   delay,
        const float                                   width
    ) -> IModule::FarmPosition{
        Vec3                      result;
        auto                      result_points = 0;
        std::vector< DamageData > result_data;

        const auto start  = g_local->position;
        const auto units  = g_entity_list.get_enemy_minions( );
        const auto widthc = width * 2;

        for ( const auto minion : units ) {
            if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->is_minion_only_autoattackable( ) ||
                minion->is_untargetable_minion( ) || !minion->is_normal_minion( ) || g_features->orbwalker->is_ignored(
                    minion->index
                ) )
                continue;

            auto pos = minion->position;
            if ( pos.dist_to( start ) > range ) continue;

            std::vector< DamageData > current_data{ };
            auto                      tt     = delay + speed != 0 ? minion->dist_to_local( ) / speed : 0;
            auto                      points = 0;
            const auto                damage = get_damage( minion );

            current_data.push_back( { tt, get_damage( minion ), minion->index } );

            if ( get_damage( minion ) > g_features->prediction->predict_health( minion, tt ) ) {
                points += minion->get_minion_type( ) == Object::EMinionType::siege ? 4 : 2;
            }

            for ( const auto second_minion : units ) {
                if ( !second_minion ||
                    second_minion->index == minion->index ||
                    second_minion->is_dead( ) ||
                    second_minion->is_invisible( ) ||
                    !second_minion->is_normal_minion( ) ||
                    g_features->orbwalker->is_ignored( minion->index )
                )
                    continue;

                if ( pos.dist_to( second_minion->position ) <= widthc ) {
                    tt                = delay + speed != 0 ? second_minion->dist_to_local( ) / speed : 0;
                    const auto damage = get_damage( second_minion );

                    if ( damage > g_features->prediction->predict_health( second_minion, tt ) ) points += 2;
                    else ++points;

                    current_data.push_back( { tt, get_damage( second_minion ), second_minion->index } );
                }

                if ( points < result_points ) continue;

                result        = pos;
                result_points = points;
                result_data   = current_data;
            }
        }

        return FarmPosition{ result_points, result, result_data };
    }

    auto IModule::get_circle_lasthit_target(
        const std::function< float( Object* unit ) >& get_damage,
        const float                                   range,
        float                                         speed,
        const float                                   delay,
        float                                         width
    ) -> IModule::FarmPosition{
        auto       result          = Vec3( 0, 0, 0 );
        auto       result_priority = 0;
        DamageData result_data{ };

        const auto start = g_local->position;
        const auto units = g_entity_list.get_enemy_minions( );

        for ( const auto minion : units ) {
            if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->is_minion_only_autoattackable( )
                || minion->is_untargetable_minion( ) || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) ||
                g_features->orbwalker->is_ignored( minion->index ) )
                continue;

            auto pos = minion->position;
            if ( pos.dist_to( start ) > range ) continue;

            const auto in_aa_range    = g_features->orbwalker->is_attackable( minion->index );
            const auto health_next_aa = g_features->prediction->predict_health(
                minion,
                g_features->orbwalker->get_next_aa_time( ) - *g_time
            );
            auto priority = 0;

            if ( !in_aa_range ) priority++;
            if ( health_next_aa < 20.f ) priority++;
            if ( minion->get_minion_type( ) == Object::EMinionType::siege ) priority++;

            const auto tt            = delay;
            const auto damage        = get_damage( minion );
            const auto health_on_hit = g_features->prediction->predict_health( minion, tt );

            if ( damage < health_on_hit || result_priority >= priority || health_on_hit <= 0.f ) continue;

            Vec3 cast_pos;
            auto pred = g_features->prediction->predict_default( minion->index, tt );
            if ( !pred || pred.value( ).dist_to( g_local->position ) > range ) cast_pos = minion->position;
            else cast_pos                                                               = pred.value( );

            result          = cast_pos;
            result_priority = priority;
            result_data     = DamageData{ tt, get_damage( minion ), minion->index };
        }

        const std::vector payload{ result_data };

        return FarmPosition{ result_priority, result, payload };
    }

    auto IModule::get_best_laneclear_position(
        const float range,
        const float width,
        const bool  allow_lane_minion,
        const bool  allow_jungle_monster,
        const float delay
    ) -> IModule::SimpleFarmPosition{
        Vec3       result{ };
        int        result_count{ };
        const auto start = g_local->position;

        const auto widthc = width * 2;

        const auto current_hero = helper::get_current_hero( );

        const auto units = g_entity_list.get_enemy_minions( );
        for ( const auto minion : units ) {
            if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->is_untargetable_minion( ) || minion
                ->is_ward( ) || !allow_lane_minion && minion->is_lane_minion( ) || !allow_jungle_monster && minion->
                is_jungle_monster( ) || minion->is_turret( ) || minion->is_plant( ) )
                continue;

            auto pos = minion->position;

            if ( delay > 0.f ) {
                auto pred = g_features->prediction->predict_default( minion->index, delay );
                if ( pred.has_value( ) ) pos = pred.value( );
            }

            if ( pos.dist_to( start ) > range ) continue;

            auto count = 0;
            for ( const auto second_minion : units ) {
                if ( !second_minion || second_minion->is_dead( ) || second_minion->is_invisible( ) || !second_minion->
                    is_normal_minion( ) || second_minion->is_ward( ) || second_minion->is_turret( ) )
                    continue;

                auto temp_pos = second_minion->position;
                if ( delay > 0.f ) {
                    auto pred = g_features->prediction->predict_default( second_minion->index, delay );
                    if ( pred.has_value( ) ) temp_pos = pred.value( );
                }

                if ( pos.dist_to( temp_pos ) <= widthc ) count++;
            }


            if ( current_hero == EHeroes::cassiopeia ) {
                if ( minion->get_monster_priority( ) == 2 ) count += 10;
                else if ( minion->get_monster_priority( ) == 1 ) count += 5;
            }

            if ( count < result_count ) continue;

            result       = pos;
            result_count = count;
        }

        return SimpleFarmPosition{ result_count, result };
    }

    auto IModule::get_multihit_position(
        float range,
        float speed,
        float radius,
        float delay,
        bool  linear
    ) -> IModule::CollisionPosition{
        int  hit_count{ };
        Vec3 position;

        auto enemies = g_entity_list.get_enemies( );

        if ( linear ) {
            for ( auto enemy : enemies ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > range *
                    1.5f ) continue;

                float                 travel_time{ };
                auto                  compensation = radius / enemy->movement_speed;
                std::optional< Vec3 > pred{ };


                if ( speed == 0 ) {
                    travel_time = delay;
                    pred        = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
                    if ( !pred ) continue;
                } else {
                    travel_time = delay + enemy->dist_to_local( ) / speed;
                    pred        = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
                    if ( !pred ) continue;
                }

                if ( pred.value( ).dist_to( g_local->position ) > range ) continue;

                auto rect = sdk::math::Rectangle( g_local->position, g_local->position.extend( *pred, range ), radius );
                auto polygon = rect.to_polygon( );

                int count{ };

                for ( const auto second_enemy : enemies ) {
                    std::optional< Vec3 > predict{ };
                    if ( g_features->target_selector->is_bad_target( second_enemy->index ) ) continue;

                    if ( speed == 0 ) {
                        travel_time = delay;
                        predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                        if ( !predict ) continue;
                    } else {
                        travel_time = delay + second_enemy->dist_to_local( ) / speed;
                        predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                        if ( !predict ) continue;
                    }

                    if ( g_local->position.dist_to( predict.value( ) ) > range || polygon.
                        is_outside( *predict ) )
                        continue;

                    count++;
                }

                if ( count <= hit_count ) continue;

                hit_count = count;
                position  = *pred;
            }

            // no compensation
            if ( hit_count < 2 ) {
                for ( auto enemy : enemies ) {
                    if ( enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

                    float                 travel_time{ };
                    std::optional< Vec3 > pred{ };


                    if ( speed == 0 ) {
                        travel_time = delay;
                        pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                        if ( !pred ) continue;
                    } else {
                        travel_time = delay + enemy->dist_to_local( ) / speed;
                        pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                        if ( !pred ) continue;
                    }

                    auto rect = sdk::math::Rectangle(
                        g_local->position,
                        g_local->position.extend( *pred, range ),
                        radius
                    );
                    auto polygon = rect.to_polygon( );

                    int count{ };

                    for ( const auto second_enemy : enemies ) {
                        std::optional< Vec3 > predict{ };
                        if ( g_features->target_selector->is_bad_target( second_enemy->index ) ) continue;

                        if ( speed == 0 ) {
                            travel_time = delay;
                            predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                            if ( !predict ) continue;
                        } else {
                            travel_time = delay + second_enemy->dist_to_local( ) / speed;
                            predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                            if ( !predict ) continue;
                        }

                        if ( g_local->position.dist_to( predict.value( ) ) > range || polygon.
                            is_outside( *predict ) )
                            continue;

                        count++;
                    }

                    if ( count <= hit_count ) continue;

                    hit_count = count;
                    position  = *pred;
                }
            }
        } else {
            for ( auto enemy : enemies ) {
                if ( enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > range * 1.25f ) continue;

                float                 travel_time{ };
                auto                  compensation = radius / enemy->movement_speed;
                std::optional< Vec3 > pred{ };


                if ( speed == 0 ) {
                    travel_time = delay;
                    pred        = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
                    if ( !pred ) continue;
                } else {
                    travel_time = delay + enemy->dist_to_local( ) / speed;
                    pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                    if ( !pred ) continue;

                    travel_time = delay + g_local->position.dist_to( *pred ) / speed;
                    pred        = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
                    if ( !pred ) continue;
                }

                if ( pred.value( ).dist_to( g_local->position ) > range ) continue;

                auto start = *pred;
                int  count{ };

                for ( const auto second_enemy : enemies ) {
                    if ( g_features->target_selector->is_bad_target( second_enemy->index ) || second_enemy->
                        dist_to_local( ) > range * 1.25f )
                        continue;
                    std::optional< Vec3 > predict{ };

                    if ( speed == 0 ) {
                        travel_time = delay;
                        predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                        if ( !predict ) continue;
                    } else {
                        travel_time = delay + second_enemy->dist_to_local( ) / speed;
                        predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                        if ( !predict ) continue;

                        travel_time = delay + g_local->position.dist_to( *predict ) / speed;
                        predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                        if ( !predict ) continue;
                    }

                    if ( g_local->position.dist_to( *predict ) > range + radius || start.dist_to( *predict ) >= radius *
                        2 )
                        continue;

                    count++;
                }

                if ( count <= hit_count ) continue;

                hit_count = count;
                position  = *pred;
            }

            // no compensation
            if ( hit_count < 2 ) {
                for ( auto enemy : enemies ) {
                    if ( enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > range *
                        1.25f )
                        continue;

                    float                 travel_time{ };
                    std::optional< Vec3 > pred{ };

                    if ( speed == 0 ) {
                        travel_time = delay;
                        pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                        if ( !pred ) continue;
                    } else {
                        travel_time = delay + enemy->dist_to_local( ) / speed;
                        pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                        if ( !pred ) continue;

                        travel_time = delay + g_local->position.dist_to( *pred ) / speed;
                        pred        = g_features->prediction->predict_default( enemy->index, travel_time );
                        if ( !pred ) continue;
                    }

                    if ( pred.value( ).dist_to( g_local->position ) > range ) continue;

                    auto start = *pred;
                    int  count{ };

                    for ( const auto second_enemy : enemies ) {
                        if ( g_features->target_selector->is_bad_target( second_enemy->index ) || second_enemy->
                            dist_to_local( ) > range * 1.25f )
                            continue;
                        std::optional< Vec3 > predict{ };

                        if ( speed == 0 ) {
                            travel_time = delay;
                            predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                            if ( !predict ) continue;
                        } else {
                            travel_time = delay + second_enemy->dist_to_local( ) / speed;
                            predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                            if ( !predict ) continue;

                            travel_time = delay + g_local->position.dist_to( *predict ) / speed;
                            predict     = g_features->prediction->predict_default( second_enemy->index, travel_time );
                            if ( !predict ) continue;
                        }

                        if ( g_local->position.dist_to( *predict ) > range + radius || start.dist_to( *predict ) >
                            radius * 2 )
                            continue;

                        count++;
                    }

                    if ( count <= hit_count ) continue;

                    hit_count = count;
                    position  = *pred;
                }
            }
        }

        return CollisionPosition{ hit_count, position };
    }

    auto IModule::get_circle_multihit(
        const std::function< float( Object* unit ) >& get_travel_time,
        const float                                   range,
        const float                                   radius
    ) -> CollisionPosition{
        auto hit_count{ 1 };
        Vec3 position;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->dist_to_local( ) > range + radius || g_features->target_selector->is_bad_target(
                enemy->index
            ) )
                continue;

            const auto travel_time = get_travel_time( enemy );

            const auto predict = g_features->prediction->predict( enemy->index, range, 0.f, radius / 2.f, travel_time );
            if ( !predict.valid ) continue;

            auto cast_position = predict.position;

            auto count{ 1 };
            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero || hero->network_id == enemy->network_id || hero->dist_to_local( ) > range * 1.5f ||
                    g_features->target_selector->is_bad_target( hero->index ) )
                    continue;

                auto pred = g_features->prediction->predict( hero->index, range, 0.f, 0.f, travel_time );
                if ( !pred.valid || pred.position.dist_to( cast_position ) > radius ) continue;


                ++count;
            }

            if ( count <= hit_count ) continue;

            position  = cast_position;
            hit_count = count;
        }


        return { hit_count, position };
    }


    auto IModule::get_viktor_multihit_position(
        const Vec3&    source_position,
        const float    speed,
        const float    radius,
        const unsigned ignored_nid
    ) -> std::optional< CollisionPosition >{
        int  hitcount{ };
        Vec3 position{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->position.dist_to( source_position ) > 500.f || enemy->network_id == ignored_nid ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            const auto compensation = enemy->movement_speed / radius;
            auto       travel_time  = enemy->position.dist_to( source_position ) / speed;
            auto       pred         = g_features->prediction->predict_default( enemy->index, travel_time );
            if ( !pred ) continue;

            travel_time = source_position.dist_to( *pred ) / speed;
            pred        = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
            if ( !pred || pred.value( ).dist_to( source_position ) > 500.f ) continue;

            auto rect    = sdk::math::Rectangle( source_position, source_position.extend( *pred, 500.f ), radius );
            auto polygon = rect.to_polygon( );

            auto count{ 1 };

            for ( const auto second_enemy : g_entity_list.get_enemies( ) ) {
                if ( !second_enemy || second_enemy->network_id == ignored_nid || second_enemy->position.
                    dist_to( source_position ) > 600.f || g_features->target_selector->is_bad_target(
                        second_enemy->index
                    ) )
                    continue;

                const auto comp    = second_enemy->movement_speed / radius;
                auto       tt      = second_enemy->position.dist_to( source_position ) / speed;
                auto       predict = g_features->prediction->predict_default( second_enemy->index, tt );
                if ( !predict ) continue;

                tt      = source_position.dist_to( *pred ) / speed;
                predict = g_features->prediction->predict_default( second_enemy->index, tt - comp );
                if ( !predict || predict.value( ).dist_to( source_position ) > 500.f || !polygon.
                    is_inside( *predict ) )
                    continue;

                count++;
            }

            if ( count <= hitcount ) continue;

            hitcount = count;
            position = *pred;
        }

        return CollisionPosition{ hitcount, position };
    }

    auto IModule::get_viktor_spellclear_position( ) -> std::optional< ViktorFarmPosition >{
        Vec3 cast_start{ };
        Vec3 cast_end{ };
        int  hitcount{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion || minion->is_dead( ) || minion->is_invisible( )
                || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) || minion->dist_to_local( ) > 510.f )
                continue;

            auto count{ 1 };

            auto start = minion->position;
            Vec3 best_end{ };

            for ( const auto unit : g_entity_list.get_enemy_minions( ) ) {
                if ( !unit || unit->is_dead( ) || unit->is_invisible( ) ||
                    !unit->is_lane_minion( ) && !unit->is_jungle_monster( ) ||
                    unit->position.dist_to( start ) > 700.f )
                    continue;

                auto candidate = start.extend( unit->position, 700.f );

                const auto minion_count = g_features->prediction->count_minions_in_line( start, candidate, 45.f );
                if ( minion_count < count ) continue;

                best_end = candidate;
                count    = minion_count;
            }

            if ( count < hitcount ) continue;

            cast_start = start;
            cast_end   = best_end;
            hitcount   = count;
        }

        if ( hitcount < 2 ) return std::nullopt;

        return std::make_optional( ViktorFarmPosition( cast_start, cast_end, hitcount ) );
    }

    auto IModule::get_antigapclose_position_weight(
        const Vec3& position,
        const Vec3& dash_end,
        const bool  passes_through_walls
    ) -> float{
        float      weight{ };
        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        const auto v1            = position - g_local->position;
        const auto v2            = cursor - g_local->position;
        const auto dot           = v1.normalize( ).dot_product( v2.normalize( ) );
        const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

        // angle weight
        weight += std::min( 1.f - current_angle / 180.f, 0.5f );

        // turret weight
        if ( helper::is_position_under_turret( position ) ) weight -= 1.f;

        if ( helper::is_position_under_turret( position, true ) ) weight += 1.f;
        else if ( helper::is_position_near_turret( position, true ) ) weight += 0.5f;

        // distance weight
        weight += std::min( 1.f - dash_end.dist_to( position ) / 500.f, 1.f );

        // ally weight
        const auto nearby_enemies = helper::get_nearby_champions_count( position, false );
        const auto nearby_allies  = helper::get_nearby_champions_count( position, true );

        const auto enemy_weight{ nearby_enemies / 5.f };
        const auto ally_weight{ nearby_allies / 5.f };

        if ( helper::is_wall_in_line( g_local->position, position ) && !passes_through_walls ) weight -= 1.f;

        // wall weight
        if ( helper::is_wall_in_line( position, dash_end ) &&
            helper::is_wall_in_line( g_local->position, position ) )
            weight += 1.f;

        if ( ally_weight > enemy_weight ) weight += ally_weight - enemy_weight;
        else if ( enemy_weight > ally_weight ) weight -= enemy_weight - ally_weight;

        return weight;
    }

    auto IModule::will_kill( Object* target, const std::function< float( Object* unit ) >& get_damage ) -> bool{
        if ( !target ) return false;

        return get_damage( target ) > target->health + target->total_health_regen * 2.f;
    }

    auto IModule::get_antigapclose_target( const float danger_distance ) -> Object*{
        // Function is used in LUA, message @tore if you change args
        Object* target{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy ||
                enemy->is_dead( ) ||
                enemy->is_invisible( ) ||
                enemy->dist_to_local( ) > 4000.f ||
                g_features->target_selector->is_bad_target( enemy->index ) ||
                rt_hash( enemy->champion_name.text ) == ct_hash( "Kalista" ) )
                continue;

            auto aimgr = enemy->get_ai_manager( );
            if ( !aimgr || !aimgr->is_moving || !aimgr->is_dashing ) continue;

            auto path = aimgr->get_path( );
            if ( path.size( ) <= 1 || path.size( ) == aimgr->next_path_node ) continue;

            auto dash_end = path[ path.size( ) - 1 ];
            if ( g_local->position.dist_to( dash_end ) > danger_distance ) continue;

            target = enemy;
            break;
        }

        return target;
    }

    auto IModule::get_stasis_target(
        const float                                   range,
        const std::function< float( Object* unit ) >& get_traveltime,
        const float                                   hitbox_width,
        Vec3                                          source_position
    ) -> Object*{
        if ( source_position.length( ) <= 0.f ) source_position = g_local->position;

        const std::vector< hash_t > stasis_buffs = { ct_hash( "ChronoRevive" ), ct_hash( "ZhonyasRingShield" ) };

        Object* target{ };
        int     target_priority{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->position.dist_to( source_position ) >
                range )
                continue;

            const auto priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );
            if ( priority < target_priority ) continue;

            const auto travel_time       = get_traveltime( enemy );
            const auto time_compensation = hitbox_width / enemy->movement_speed;
            bool       found_buff{ };

            for ( const auto buff_hash : stasis_buffs ) {
                const auto buff = g_features->buff_cache->get_buff( enemy->index, buff_hash );
                if ( !buff || buff->buff_data->end_time - *g_time < travel_time + time_compensation ) continue;

                found_buff = true;
                break;
            }

            if ( !found_buff ) continue;

            target          = enemy;
            target_priority = priority;
            break;
        }

        return target;
    }


    auto IModule::get_interruptable_target( const float range, Vec3 source_position ) const -> Object*{
        Object* target{ };

        if ( source_position.length( ) <= 0.f ) source_position = g_local->position;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->position.dist_to( source_position ) > range ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            auto sci = enemy->spell_book.get_spell_cast_info( );
            if ( !sci || sci->slot < 0 ) continue;

            auto info = sci->get_spell_info( );
            if ( !info ) continue;

            auto data = info->get_spell_data( );
            if ( !data ) continue;

            auto spell_name = data->get_name( );
            bool found_interruptable{ };

            for ( const auto name : m_interruptable_list ) {
                if ( spell_name == name ) {
                    found_interruptable = true;
                    break;
                }
            }

            if ( !found_interruptable ) continue;

            target = enemy;
            break;
        }

        return target;
    }

    auto IModule::get_advanced_antigapclose_target(
        const float range,
        const float speed,
        const float width,
        const float delay,
        const bool  edge_range,
        Vec3        source_position
    ) -> Object*{
        if ( source_position.length( ) <= 0.f ) source_position = g_local->position;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->dist_to_local( ) > ( range * 1.5f > 1000.f ? range * 1.5f : 1000.f ) ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            Vec3       cast_position{ };
            const auto bounding_radius = edge_range
                                             ? g_features->prediction->get_champion_radius(
                                                 rt_hash( enemy->champion_name.text )
                                             )
                                             : 0.f;

            const auto dash_pred = g_features->prediction->predict_dash(
                enemy->index,
                range,
                speed,
                width + bounding_radius,
                delay,
                source_position
            );

            if ( !dash_pred.valid ) {
                const auto dash_spell_pred = g_features->prediction->predict_dash_from_spell(
                    enemy->index,
                    range,
                    speed,
                    width + bounding_radius,
                    delay,
                    source_position
                );

                if ( !dash_spell_pred.valid ) {
                    const auto blink_pred = g_features->prediction->predict_blink(
                        enemy->index,
                        range,
                        speed,
                        width + bounding_radius,
                        delay,
                        source_position
                    );

                    if ( !blink_pred.valid ) continue;

                    cast_position = blink_pred.position;
                } else cast_position = dash_spell_pred.position;
            } else cast_position = dash_pred.position;

            if ( source_position.dist_to( cast_position ) >= range ) continue;

            return enemy;
        }
        return { };
    }


    auto IModule::should_spellshield_ally( Object* hero ) -> bool{
        if ( !hero || hero->is_dead( ) ) return false;

        bool should_shield{ };

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

            auto sci = enemy->spell_book.get_spell_cast_info( );
            if ( !sci || sci->server_cast_time <= *g_time ) continue;

            const auto is_hero_targeted = sci->get_target_index( ) == hero->index;

            switch ( rt_hash( enemy->champion_name.text ) ) {
            case ct_hash( "Udyr" ):
            case ct_hash( "Leona" ):
            case ct_hash( "Malphite" ):
            case ct_hash( "Fiddlesticks" ):
            case ct_hash( "Kassadin" ):
            case ct_hash( "Teemo" ):
            case ct_hash( "Vladimir" ):
                if ( sci->slot == 0 && is_hero_targeted ) should_shield = true; // Q
                break;
            case ct_hash( "Renekton" ):
            case ct_hash( "Nasus" ):
            case ct_hash( "Ryze" ):
            case ct_hash( "Janna" ):
            case ct_hash( "TwistedFate" ):
                if ( sci->slot == 1 && is_hero_targeted ) should_shield = true; // W
                break;
            case ct_hash( "Rammus" ):
            case ct_hash( "Blitzcrank" ):
                if ( sci->slot == 2 && is_hero_targeted ) should_shield = true; // E
                break;
            case ct_hash( "Darius" ):
            case ct_hash( "Chogath" ):
            case ct_hash( "Veigar" ):
            case ct_hash( "Trundle" ):
            case ct_hash( "Tristana" ):
                if ( sci->slot == 3 && is_hero_targeted ) should_shield = true; // R
                break;
            case ct_hash( "Garen" ):
                if ( ( sci->slot == 0 || sci->slot == 3 ) && is_hero_targeted ) should_shield = true; // R
                break;
            case ct_hash( "Morgana" ): // SPECIAL
                if ( sci->slot == 3 && enemy->position.dist_to( hero->position ) < 625.f ) should_shield = true;
                break;
            case ct_hash( "Diana" ): // SPECIAL
                if ( sci->slot == 3 && enemy->position.dist_to( hero->position ) < 250.f ) should_shield = true;
                break;
            case ct_hash( "Nunu" ): // SPECIAL
                if ( sci->slot == 3 && enemy->position.dist_to( hero->position ) < 550.f ) should_shield = true;
                break;
            case ct_hash( "Twitch" ):
                if ( sci->slot == 2 && enemy->position.dist_to( hero->position ) <= 1200.f ) {
                    const auto buff = g_features->buff_cache->get_buff( hero->index, ct_hash( "TwitchDeadlyVenom" ) );
                    if ( !buff || buff->stacks( ) < 3 ) break;

                    should_shield = true;
                }
                break;
            default:
                break;
            }

            if ( should_shield ) break;
        }

        return should_shield;
    }

    auto IModule::get_colliding_skillshot( int16_t index ) -> std::optional< CollidingSkillshot >{
        auto hero = g_entity_list.get_by_index( index );
        if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ) return std::nullopt;

        hero.update( );

        CollidingSkillshot instance{ };
        bool               found_collision{ };

        const auto bounding_radius  = hero->get_bounding_radius( );
        const auto dangerous_spells = g_features->evade->get_dangerous_spells( hero->position, false, bounding_radius );

        for ( auto spell : dangerous_spells ) {
            auto time_till_impact = spell.time_till_impact( hero->position );
            if ( time_till_impact <= 0.f ) continue;

            auto pred = g_features->prediction->predict_default( hero->index, time_till_impact, true );
            if ( !pred ) continue;

            sdk::math::Polygon hitbox{ };

            switch ( spell.type ) {
            case SpellDetector::ESpellType::line:
            case SpellDetector::ESpellType::circle:
                hitbox = spell.hitbox_area;
                break;
            case SpellDetector::ESpellType::cone:
            {
                auto adjusted_start = spell.start_pos.extend( spell.end_pos, -140.f );

                auto cone = sdk::math::Sector( adjusted_start, spell.end_pos, spell.angle, spell.range );
                hitbox    = cone.to_polygon_new( 200 );
                break;
            }
            default:
                continue;
            }

            if ( hitbox.is_outside( pred.value( ) ) || found_collision &&
                ( spell.danger < instance.danger || spell.danger == instance.danger && instance.time_until_collision <
                    time_till_impact ) )
                continue;

            instance.danger               = spell.danger;
            instance.time_until_collision = time_till_impact;
            instance.name                 = spell.spell_name;
            found_collision               = true;
        }

        if ( found_collision ) return std::make_optional( instance );

        const auto dangerous_missiles = g_features->evade->get_dangerous_missiles(
            hero->position,
            false,
            bounding_radius
        );

        for ( auto missile : dangerous_missiles ) {
            auto time_till_impact = missile.time_till_impact( hero->position );
            if ( time_till_impact <= 0.f ) continue;

            auto pred = g_features->prediction->predict_default( hero->index, time_till_impact, false );
            if ( !pred ) continue;

            sdk::math::Polygon hitbox{ };

            switch ( missile.type ) {
            case SpellDetector::ESpellType::line:
                hitbox = sdk::math::Rectangle( missile.position, missile.get_dynamic_line_endpos( ), missile.radius )
                    .to_polygon( static_cast< int >( bounding_radius ) );
                break;
            case SpellDetector::ESpellType::circle:
                hitbox = sdk::math::Circle( missile.end_position, missile.radius )
                    .to_polygon( static_cast< int >( bounding_radius ) );
                break;
            default:
                continue;
            }

            if ( hitbox.is_outside( pred.value( ) ) || missile.danger < instance.danger ||
                missile.danger == instance.danger && instance.time_until_collision < time_till_impact )
                continue;

            instance.danger               = missile.danger;
            instance.time_until_collision = time_till_impact;
            instance.name                 = missile.name;
            found_collision               = true;
        }

        return found_collision ? std::make_optional( instance ) : std::nullopt;
    }

    auto IModule::get_damage_overtime_data(
        const int16_t index,
        const bool    include_abilities,
        const bool    include_summoners,
        const bool    include_items,
        const bool    include_neutral_buffs
    ) const -> std::optional< std::vector< EDamageOvertimeInstance > >{
        const auto unit = g_entity_list.get_by_index( index );
        if ( !unit ) return std::nullopt;

        int shield_buff{ };

        std::vector< EDamageOvertimeInstance > active_instances{ };

        auto buff = g_features->buff_cache->get_buff( index, ct_hash( "SummonerDot" ) );
        if ( buff && include_summoners ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::summoner,
                    EDamageOvertimeType::ignite,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Ignite" )
                }
            );
        }

        if ( include_items ) {
            buff = g_features->buff_cache->get_buff( index, ct_hash( "6653burn" ) );
            if ( buff && include_items ) {
                active_instances.push_back(
                    {
                        EDamageOvertimeForm::item,
                        EDamageOvertimeType::liandrys_torment_burn,
                        buff->stacks( ),
                        buff->buff_data->end_time,
                        _( "Liandrys torment" )
                    }
                );
            }

            buff = g_features->buff_cache->get_buff( index, ct_hash( "4637debuff" ) );
            if ( buff ) {
                active_instances.push_back(
                    {
                        EDamageOvertimeForm::item,
                        EDamageOvertimeType::demonic_embrace_burn,
                        buff->stacks( ),
                        buff->buff_data->end_time,
                        _( "Demonic embrace" )
                    }
                );
            }
        }

        if ( include_neutral_buffs ) {
            buff = g_features->buff_cache->get_buff( index, ct_hash( "Burning" ) );
            if ( buff ) {
                active_instances.push_back(
                    {
                        EDamageOvertimeForm::neutral_buff,
                        EDamageOvertimeType::red_buff_passive,
                        buff->stacks( ),
                        buff->buff_data->end_time,
                        _( "Crest of Cinders" )
                    }
                );
            }
        }

        if ( !include_abilities ) {
            return active_instances.empty( ) ? std::nullopt : std::make_optional( active_instances );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "BrandAblaze" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::brand_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Brand passive" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "cassiopeiaqdebuff" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::cassiopeia_q,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Cassiopeia Q" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "cassiopeiawpoison" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::cassiopeia_w,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Cassiopeia W" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "DariusHemo" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::darius_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Darius passive" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "fiddlestickswdrain" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::fiddlesticks_w,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Fiddlesticks W" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "fizzwdot" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::fizz_w,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Fizz W" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "gangplankpassiveattackdot" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::gangplank_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Gangplank passive" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "LilliaPDoT" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::lillia_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Lillia passive" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "MalzaharE" ) ); // next
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::malzahar_e,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Malzahar E" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "maokaiemark" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::maokai_e,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Maokai E" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "NautilusWDoT" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::nautilus_w,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Nautilus W" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "bushwhackdamage" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::nidalee_w,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Nidalee W" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "rumblecarpetbombslow" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::rumble_r,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Rumble R" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "TalonPassiveBleed" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::talon_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Talon passive" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "ToxicShotParticle" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::teemo_e,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Teemo E" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "bantamtraptarget" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::teemo_r,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Teemo R" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "TrundlePain" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::trundle_r,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Trundle R" )
                }
            );
        }

        buff = g_features->buff_cache->get_buff( index, ct_hash( "TwitchDeadlyVenom" ) );
        if ( buff ) {
            active_instances.push_back(
                {
                    EDamageOvertimeForm::ability,
                    EDamageOvertimeType::twitch_passive,
                    buff->stacks( ),
                    buff->buff_data->end_time,
                    _( "Twitch passive" )
                }
            );
        }


        return active_instances.empty( ) ? std::nullopt : std::make_optional( active_instances );
    }

    // maokaiemark
    // ToxicShotParticle
    // bantamtraptarget
    //  DariusHemo
    //  LilliaPDoT
    //  poisontrailtarget
    //  MalzaharE
    //  rumblecarpetbombslow
    //  gangplankpassiveattackdot
    //  NautilusWDoT
    //  TrundlePain
    //  fizzwdot
    //  bushwhackdamage
    //  TalonPassiveBleed
    //  fiddlestickswdrain
    //  TwitchDeadlyVenom

    // Burning - red buff burn

    // auto c_module::get_multihit_position( sdk::math::circle circle, float range, float speed, float delay, c_prediction::e_multihit_logic multihit_logic ) -> collision_position_t {
    //
    //     int hit_count{ };
    //     vec3 position;
    //
    //     std::map< int16_t, vec3 > predicted_positions{ };
    //     std::vector< std::pair< float, collision_position_t > > multihits{ };
    //
    //     auto enemies = g_entity_list->get_enemies( );
    //     for ( auto enemy : enemies ) {
    //
    //         if ( enemy->is_dead( ) || enemy->is_invisible( ) ) continue;
    //
    //         float ms = enemy->movement_speed;
    //         float travel_time{ };
    //         float compensation = circle.radius / ms > 0.f ?
    //                                  ms :
    //                                  1.f;
    //         std::optional< vec3 > pred{ };
    //
    //         if ( speed == 0 ) {
    //             travel_time = delay;
    //             pred = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
    //             if ( !pred ) continue;
    //         } else {
    //             travel_time = delay + enemy->dist_to_local( ) / speed;
    //             pred = g_features->prediction->predict_default( enemy->index, travel_time );
    //             if ( !pred ) continue;
    //
    //             travel_time = delay + g_local->position.dist_to( *pred ) / speed;
    //             pred = g_features->prediction->predict_default( enemy->index, travel_time - compensation );
    //             if ( !pred ) continue;
    //         }
    //
    //         predicted_positions.insert( { enemy->index, pred.value( ) } );
    //     }
    //
    //     for ( const auto& entry : predicted_positions ) {
    //
    //         float dist_to_local = entry.second.dist_to( g_local->position );
    //
    //         if ( dist_to_local > range + circle.radius ) continue;
    //
    //         std::vector< std::pair< int16_t, vec3 > > preds_hit{ entry };
    //
    //         float x_total = entry.second.x;
    //         float z_total = entry.second.z;
    //
    //         vec3 attack_position = entry.second;
    //
    //         vec3 good_attack_position;
    //         if ( dist_to_local > range ) {
    //             vec3 good_attack_position = vec3{ 0.f, 0.f, 0.f };
    //         } else {
    //             good_attack_position = attack_position;
    //         }
    //
    //         bool added = false;
    //         do {
    //             added = false;
    //             for ( const auto& entry_other : predicted_positions ) {
    //
    //                 bool contains = false;
    //
    //                 // if we already have this pred as being hit
    //                 for ( auto& pred_hit : preds_hit ) {
    //                     if ( pred_hit.first == entry_other.first ) {
    //                         contains = true;
    //                         break;
    //                     }
    //                 }
    //
    //                 if ( contains ) continue;
    //
    //                 //create a new attack position toward the next potential target
    //                 vec3 attack_position_temp = { attack_position.x + ( entry_other.second.x - attack_position.x ) / 2, attack_position.y, attack_position.z + ( entry_other.second.z - attack_position.z ) / 2 };
    //
    //                 bool accepted = true;
    //
    //                 //check to see if all our original preds are still beinh hit
    //                 for ( auto& pred_hit : preds_hit ) {
    //
    //                     // if we missed one of our original preds
    //                     if ( pred_hit.second.dist_to( attack_position_temp ) > circle.radius ) {
    //                         accepted = false;
    //                         break;
    //                     }
    //
    //                     // if we missed, go on to the next
    //                     if ( !accepted ) continue;
    //
    //                     //we can hit all our preds even if we offset to allow for this other new one
    //                     preds_hit.push_back( entry_other );
    //                     x_total += entry_other.second.x;
    //                     z_total += entry_other.second.z;
    //                     attack_position = attack_position_temp;
    //
    //                     if ( attack_position.dist_to( g_local->position ) <= range ) {
    //                         good_attack_position = attack_position;
    //                     }
    //
    //                     added = true;
    //                 }
    //             }
    //         } while ( added );
    //
    //
    //         std::vector< std::pair< int16_t, vec3 > > good_pred_hits{ };
    //
    //         for ( const auto& pred : preds_hit ) {
    //             if ( good_attack_position.dist_to( pred.second ) < circle.radius ) good_pred_hits.push_back( pred );
    //         }
    //
    //         if ( good_pred_hits.empty( ) ) continue;
    //
    //         int hits = good_pred_hits.size( );
    //
    //         switch ( multihit_logic ) {
    //         case c_prediction::e_multihit_logic::tightest_angle: {
    //             vec3 our_pos = g_local->position;
    //
    //             std::sort(
    //                 good_pred_hits.begin( ),
    //                 good_pred_hits.end( ),
    //                 [our_pos]( const std::pair< int16_t, vec3 >& l, const std::pair< int16_t, vec3 >& r ){
    //                     return our_pos.angle_between_degrees( l.second ) < our_pos.angle_between_degrees( r.second );
    //                 }
    //             );
    //             float min_angle = g_local->position.angle_between_degrees( good_pred_hits.at( 0 ).second );
    //
    //             float max_angle = g_local->position.angle_between_degrees( good_pred_hits.at( good_pred_hits.size( ) - 1 ).second );
    //
    //             debug_log( "Adding a multihit with hits {}, rating {} at [{}, {}, {}]", hits, 360.f - (max_angle - min_angle), good_attack_position.x, good_attack_position.y, good_attack_position.z );
    //             multihits.push_back( { 360.f - ( max_angle - min_angle ), { hits, good_attack_position } } );
    //             break;
    //         }
    //
    //         case c_prediction::e_multihit_logic::max_average_path_length:
    //         case c_prediction::e_multihit_logic::max_average_path_time:
    //
    //             std::vector< vec3 > pred_hit_pos{ };
    //             for ( const auto& pred_hit : good_pred_hits ) {
    //                 pred_hit_pos.push_back( pred_hit.second );
    //             }
    //             float path_tally{ };
    //             for ( const auto& pred_hit : good_pred_hits ) {
    //                 auto ent = g_entity_list->get_by_index( pred_hit.first );
    //                 if ( !ent ) continue;
    //                 auto copy = ent->create_updated_copy( );
    //
    //                 float exit_path_length = circle.radius - good_attack_position.dist_to( copy->position );
    //                 //preds_hit
    //                 path_tally += multihit_logic == c_prediction::e_multihit_logic::max_average_path_length ?
    //                                   exit_path_length :
    //                                   exit_path_length /= copy->movement_speed;
    //             }
    //
    //             hits = good_pred_hits.size( );
    //             debug_log( "Adding a multihit with hits {}, rating {} at [{}, {}, {}]", hits, path_tally / hits, good_attack_position.x, good_attack_position.y, good_attack_position.z );
    //             multihits.push_back( { path_tally / hits, { hits, good_attack_position } } );
    //             break;
    //         }
    //     }
    //
    //     std::pair best_multi{ 0.f, collision_position_t{ 0, { 0, 0, 0 } } };
    //
    //     for ( const auto& multihit : multihits ) {
    //         if ( multihit.second.hit_count > best_multi.second.hit_count ) {
    //             best_multi = multihit;
    //         } else if ( multihit.second.hit_count == best_multi.second.hit_count ) // if we are hitting the same amount of enemies
    //         {
    //             if ( multihit.first > best_multi.first ) best_multi = multihit; // check if we have a tighter grouping for better hitchance
    //         }
    //     }
    //     return best_multi.second;
    // }
    //
    // auto c_module::get_multihit_position( sdk::math::sector sector, float delay, c_prediction::e_multihit_logic multihit_logic ) -> collision_position_t {
    //     int hit_count{ };
    //     vec3 position;
    //
    //     float radius = sector.radius + 55.f; // THIS IS TO ACCOUNT FOR OUR PLAYER RADIUS
    //
    //     std::vector< std::pair< float, collision_position_t > > multihits{ }; // stored as angle between widest targets as the key
    //     std::map< int16_t, float > predicted_angles{ };
    //
    //     auto enemies = g_entity_list->get_enemies( );
    //     for ( auto enemy : enemies ) {
    //         if ( enemy->is_dead( ) || enemy->is_invisible( ) ) continue;
    //
    //         if ( enemy->dist_to_local( ) > radius ) continue;
    //
    //
    //         float move_speed = enemy->movement_speed;
    //         float compensation = radius / move_speed > 0 ?
    //                                  move_speed :
    //                                  1;
    //         std::optional< vec3 > pred{ };
    //
    //         pred = g_features->prediction->predict_default( enemy->index, delay - compensation );
    //         if ( !pred ) continue;
    //
    //         auto angle = g_local->position.angle_between_degrees( pred.value( ) );
    //
    //         predicted_angles.insert( std::pair( enemy->index, angle ) );
    //
    //     }
    //
    //     if ( predicted_angles.empty( ) ) return { 0, { 0, 0, 0 } };
    //
    //     std::vector< std::pair< int16_t, float > > vec{ };
    //     // copy key-value pairs from the map to the vector
    //     std::copy(
    //         predicted_angles.begin( ),
    //         predicted_angles.end( ),
    //         std::back_inserter< std::vector< std::pair< int16_t, float > > >( vec )
    //     );
    //
    //     std::sort(
    //         vec.begin( ),
    //         vec.end( ),
    //         []( const std::pair< int16_t, float >& l, const std::pair< int16_t, float >& r ){
    //             if ( l.second != r.second ) {
    //                 return l.second < r.second;
    //             }
    //             return l.first < r.first;
    //         }
    //     );
    //
    //
    //     for ( auto enemy : enemies ) {
    //         if ( !enemy ) continue;
    //
    //         auto our_entry = predicted_angles.find( enemy->index );
    //
    //         if ( our_entry == predicted_angles.end( ) ) continue;
    //
    //         if ( our_entry->first != enemy->index ) continue;
    //
    //         std::vector< std::pair< int16_t, float > > enemy_possible_angles{ *our_entry };
    //
    //         for ( auto val : vec ) {
    //             if ( val.first == enemy->index ) continue;
    //
    //             if ( our_entry->second + sector.angle > val.second && our_entry->second - sector.angle < val.second ) {
    //                 enemy_possible_angles.push_back( val );
    //             }
    //         }
    //
    //         std::sort(
    //             enemy_possible_angles.begin( ),
    //             enemy_possible_angles.end( ),
    //             []( const std::pair< int16_t, float >& l, const std::pair< int16_t, float >& r ){
    //                 if ( l.second != r.second ) {
    //                     return l.second < r.second;
    //                 }
    //                 return l.first < r.first;
    //             }
    //         );
    //
    //         float lowest = enemy_possible_angles.at( 0 ).second;
    //         float highest = enemy_possible_angles.at( enemy_possible_angles.size( ) - 1 ).second;
    //
    //         while ( true ) {
    //             if ( enemy_possible_angles.size( ) <= 1 ) break;
    //
    //             if ( highest - lowest < sector.angle ) break;
    //
    //             float upper_difference = enemy_possible_angles.at( enemy_possible_angles.size( ) - 2 ).second - highest;
    //             float lower_difference = lowest - enemy_possible_angles.at( 1 ).second;
    //
    //             if ( upper_difference > lower_difference ) {
    //                 enemy_possible_angles.pop_back( );
    //                 highest = enemy_possible_angles.at( enemy_possible_angles.size( ) - 1 ).second;
    //             } else {
    //                 enemy_possible_angles.erase( enemy_possible_angles.begin( ) );
    //                 lowest = enemy_possible_angles.at( 0 ).second;
    //             }
    //         }
    //
    //         if ( enemy_possible_angles.empty( ) ) continue;
    //
    //         float degrees = lowest + ( highest - lowest ) / 2;
    //
    //
    //         float angle_rad = degrees * ( m_pi / 180.f );
    //         auto attack_point = vec3( sector.center.x + sin( angle_rad ) * radius, sector.center.y, sector.center.z + cos( angle_rad ) * radius );
    //         int32_t hits = enemy_possible_angles.size( );
    //
    //
    //         switch ( multihit_logic ) {
    //
    //         case c_prediction::e_multihit_logic::tightest_angle:
    //             multihits.push_back( { sector.angle - ( highest - lowest ), { hits, attack_point } } );
    //             break;
    //         case c_prediction::e_multihit_logic::max_average_path_length:
    //         case c_prediction::e_multihit_logic::max_average_path_time:
    //             float path_tally{ };
    //             for ( auto enemy_entry : enemy_possible_angles ) {
    //                 auto ent = g_entity_list->get_by_index( enemy_entry.first );
    //                 if ( !ent ) continue;
    //                 auto copy = ent->create_updated_copy( );
    //
    //                 float dist_local = copy->dist_to_local( );
    //
    //                 float dist_to_back = radius - dist_local;
    //
    //                 float upper_edge = degrees + ( sector.radius * 2 );
    //                 float lower_edge = degrees - ( sector.radius * 2 );
    //
    //                 float player_direction = g_local->position.angle_between_degrees( copy->position );
    //                 float perpindicular_tri_angle_deg = player_direction >= degrees ?
    //                                                         upper_edge - player_direction :
    //                                                         player_direction - lower_edge;
    //                 float perpindicular_tri_angle_rad = perpindicular_tri_angle_deg * ( m_pi / 180.f );
    //
    //                 float dist_to_edge = tan( perpindicular_tri_angle_rad ) * dist_local;
    //
    //                 float shortest_path = ( dist_to_edge > dist_to_back ) ?
    //                                           dist_to_back :
    //                                           dist_to_edge;
    //                 path_tally += multihit_logic == c_prediction::e_multihit_logic::max_average_path_length ?
    //                                   shortest_path :
    //                                   shortest_path /= copy->movement_speed;
    //                 //   O
    //                 //  T A
    //
    //             }
    //
    //             multihits.push_back( { path_tally / hits, { hits, attack_point } } );
    //         }
    //     }
    //
    //     std::pair best_multi{ 0.f, collision_position_t{ 0, { 0, 0, 0 } } };
    //
    //     for ( auto multihit : multihits ) {
    //         if ( multihit.second.hit_count > best_multi.second.hit_count ) {
    //             best_multi = multihit;
    //         } else if ( multihit.second.hit_count == best_multi.second.hit_count ) // if we are hitting the same amount of enemies
    //         {
    //
    //             if ( multihit.first > best_multi.first ) best_multi = multihit; // check if we have a tighter grouping for better hitchance
    //         }
    //     }
    //     return best_multi.second;
    // }


    auto IModule::get_multihit_count(
        const Vec3& end_position,
        float       range,
        const float speed,
        const float radius,
        const float delay,
        const bool  linear
    ) -> int32_t{
        int hit_count{ };

        if ( linear ) {
            const auto rect    = sdk::math::Rectangle( g_local->position, end_position, radius );
            const auto polygon = rect.to_polygon( );

            for ( const auto obj : g_entity_list.get_enemies( ) ) {
                if ( !obj ) continue;
                if ( g_features->target_selector->is_bad_target( obj->index ) ) continue;

                float                 travel_time{ };
                std::optional< Vec3 > pred{ };

                if ( speed == 0 ) {
                    travel_time = delay;
                    pred        = g_features->prediction->predict_default( obj->index, travel_time );
                    if ( !pred ) continue;
                } else {
                    travel_time = delay + obj->dist_to_local( ) / speed;
                    pred        = g_features->prediction->predict_default( obj->index, travel_time );
                    if ( !pred ) continue;

                    travel_time = delay + g_local->position.dist_to( *pred ) / speed;
                    pred        = g_features->prediction->predict_default( obj->index, travel_time );
                    if ( !pred ) continue;
                }

                if ( polygon.is_outside( *pred ) ) continue;

                hit_count++;
            }
        }

        return hit_count;
    }

    auto IModule::is_position_in_turret_range( Vec3 position, const bool ally_turret ) -> bool{
        if ( g_local->is_dead( ) ) return false;

        if ( position.length( ) <= 0.f ) position = g_local->position;

        if ( ally_turret ) {
            for ( const auto turret : g_entity_list.get_ally_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 905.f ) continue;

                return true;
            }
        } else {
            for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
                if ( !turret || turret->is_dead( ) || position.dist_to( turret->position ) > 905.f ) continue;

                return true;
            }
        }

        return false;
    }

#if enable_new_lua
    auto IModule::get_slot_q_lua( ) const -> lua::LuaSpellSlot{ return lua::LuaSpellSlot( m_slot_q.get( ) ); }
    auto IModule::get_slot_w_lua( ) const -> lua::LuaSpellSlot{ return lua::LuaSpellSlot( m_slot_w.get( ) ); }
    auto IModule::get_slot_e_lua( ) const -> lua::LuaSpellSlot{ return lua::LuaSpellSlot( m_slot_e.get( ) ); }

    auto IModule::get_slot_r_lua( ) const -> lua::LuaSpellSlot{ return lua::LuaSpellSlot( m_slot_r.get( ) ); }
#endif
}
