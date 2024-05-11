#include "pch.hpp"

#include "prediction.hpp"
#include "entity_list.hpp"
#include "orbwalker.hpp"
#include "buff_cache.hpp"
#include "evade.hpp"
#include "target_selector/target_selector.hpp"
#include "tracker.hpp"
#include "../lua-v2/state.hpp"
// #include "../renderer/c_fonts.hpp"
#include "../sdk/game/ai_manager.hpp"

#include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../sdk/game/spell_data.hpp"
#include "../sdk/game/unit_info_component.hpp"

#include "../sdk/game/buff_info.hpp"
// #include "../sdk/game/render_manager.hpp"

namespace features {
    constexpr float RANGED_MISSILE_SPEED{ 650.f };
    constexpr float SIEGE_MISSILE_SPEED{ 1200.f };
    constexpr float EXTRA_DURATION{ 0.033f };


    auto Prediction::run( ) -> void{
#if enable_lua
        if ( g_lua2 ) {
            g_lua2->execute_locked(
                []( ) -> void{ g_lua2->run_callback( ct_hash( "features.prediction" ) ); }
            );
        }
#endif

        //update_pattern_data();
        update_prediction_data( );
        update_minion_attacks( );
        update_special_attacks( );
        update_windwall( );
        update_minions( );

        update_hero_data( );
        update_hero_attacks( );
        update_champion_spells( );
    }

    auto Prediction::update_prediction_data( ) -> void{
        remove_invalid_paths( );

        for ( const auto obj : g_entity_list.get_enemies( ) ) {
            if ( !obj || obj->is_dead( ) ) continue;

            update_prediction( obj->index );
        }
    }

    auto Prediction::update_prediction( const int16_t index ) -> void{
        auto object = g_entity_list.get_by_index( index );
        if ( !object ) return;

        object.update( );

        const auto data_index = get_data_index( object->network_id );
        if ( data_index < 0 ) return;

        auto& data = m_prediction_data[ data_index ];
        if ( data.network_id == 0 ) return;

        if ( data.index == 0 ) data.index = index;

        if ( object->is_invisible( ) ) {
            data.was_pathing       = false;
            data.is_pathing        = false;
            data.idle_time         = 0.f;
            data.cast_detected     = false;
            data.is_standing_still = false;
            return;
        }

        auto       ai_manager = object->get_ai_manager( );
        const auto sci        = object->spell_book.get_spell_cast_info( );

        if ( !ai_manager ) return;

        ai_manager.update( );

        const auto path     = ai_manager->get_path( );
        const auto path_end = ai_manager->path_end;

        if ( !data.cast_detected && sci ) {
            data.is_standing_still = *g_time - data.last_cast_time <= 0.5f && !data.events.empty( ) &&
                data.events[ data.events.size( ) - 1 ].type == EInputType::spellcast;

            data.cast_detected      = true;
            data.paths_after_cast   = 0;
            data.server_cast_time   = sci->server_cast_time;
            data.idle_time          = 0.f;
            data.last_cast_position = object->position;

            data.last_action_time = *g_time;
            data.was_pathing      = false;
            data.is_pathing       = false;

            data.events.push_back( { EInputType::spellcast, sci->start_time } );
        } else if ( data.cast_detected && ( !sci || data.server_cast_time < sci->server_cast_time ) ) {
            data.cast_detected     = false;
            data.is_standing_still = false;
            data.last_cast_time    = *g_time;
            data.idle_time         = *g_time;
        }

        if ( sci ) {
            if ( data.animation_server_cast_time != sci->server_cast_time ) {
                const auto animation_lock_time = get_animation_lock_duration( object->index );
                if ( animation_lock_time > 0.f ) {
                    data.lock_start_time            = sci->start_time;
                    data.lock_end_time              = animation_lock_time;
                    data.animation_server_cast_time = sci->server_cast_time;
                    data.animation_position         = sci->end_position;
                }
            }
        } else if ( data.is_animation_locked( ) )
            if ( data.animation_position.dist_to( object->position ) > 10.f )
                data
                    .lock_end_time = *g_time;


        // bool cc_found{ };
        // bool slow_found{ };
        // bool haste_found{ };

        /* if ( *g_time - data.last_cc_update_time >= 0.75f || data.is_slowed || data.is_hasted ) {
             for ( auto buff : g_features->buff_cache->get_all_buffs( object->index ) ) {
                 if ( !buff || !buff->buff_data ) continue;

                 auto type = static_cast< e_buff_type >( buff->buff_data->type );

                 switch ( type ) {
                 case e_buff_type::stun:
                 case e_buff_type::charm:
                 case e_buff_type::fear:
                 case e_buff_type::knockup:
                 case e_buff_type::knockback:
                 case e_buff_type::asleep:
                 case e_buff_type::snare:
                 case e_buff_type::suppression:
                 case e_buff_type::taunt:

                     cc_found = true;
                     data.is_crowdcontrolled = true;

                     if ( data.idle_time > 0.f ) data.idle_time = 0.f;

                     if ( data.last_cc_time < buff->buff_data->end_time )
                         data.last_cc_time = buff->buff_data->
                                                   end_time;
                     if ( data.has_new_speed ) data.has_new_speed = false;
                     break;
                 case e_buff_type::slow:
                     slow_found = true;
                     data.is_slowed = true;

                     if ( data.last_slow_time < buff->buff_data->end_time )
                         data.last_slow_time = buff->buff_data->
                                                     end_time;
                     if ( data.has_new_speed ) data.has_new_speed = false;
                     break;
                 case e_buff_type::haste:
                     haste_found = true;

                     data.is_hasted = true;

                     if ( data.last_haste_time < buff->buff_data->end_time )
                         data.last_haste_time = buff->buff_data->
                                                      end_time;
                     if ( data.has_new_speed ) data.has_new_speed = false;
                     break;
                 default:
                     break;
                 }
             }

             if ( !cc_found && data.is_crowdcontrolled ) {
                 if ( *g_time - data.last_cc_time < 1.f ) data.last_cc_time = *g_time;
                 data.is_crowdcontrolled = false;
             }

             if ( !slow_found && data.is_slowed ) {
                 if ( *g_time - data.last_slow_time < 1.f ) data.last_slow_time = *g_time;
                 data.is_slowed = false;
             }

             if ( !haste_found && data.is_hasted ) {
                 if ( *g_time - data.last_haste_time < 1.f ) data.last_haste_time = *g_time;
                 data.is_hasted = false;
             }

             data.last_cc_update_time = *g_time;
         }

         if ( static_cast< int >( data.base_movement_speed ) != static_cast< int >( object->movement_speed ) &&
             !haste_found && !data.is_hasted && *g_time - data.last_haste_time > 3.f &&
             !slow_found && !data.is_slowed && *g_time - data.last_slow_time > 3.f ) {
             if ( static_cast< int >( data.last_move_speed ) != static_cast< int >( object->movement_speed ) ) {
                 data.last_movespeed_change_time = *g_time;
                 data.last_move_speed            = object->movement_speed;
                 data.has_new_speed              = true;
             }

             if ( data.has_new_speed && *g_time - data.last_movespeed_change_time > 8.f ) {
                 data.base_movement_speed = data.last_move_speed;
                 data.has_new_speed       = false;
             }
         }*/

        if ( !ai_manager->is_moving || static_cast< int >( path.size( ) ) <= 1 ||
            static_cast< int >( path.size( ) ) == ai_manager->next_path_node ) {
            if ( data.cast_detected ||
                !data.events.empty( ) && data.events[ data.events.size( ) - 1 ].type == EInputType::spellcast &&
                *g_time - data.last_cast_time <= 0.1f )
                return;

            if ( data.idle_time == 0.f ) {
                data.events.push_back( { EInputType::order_stop, *g_time } );
                data.idle_time = *g_time;
            }

            data.was_pathing = false;
            data.is_pathing  = false;
            return;
        }

        const auto max_path = static_cast< int >( path.size( ) ) - 1;

        data.idle_time = 0.f;

        if ( max_path == -1 || path_end == data.last_path_end_position ) return;

        process_path( index, data_index );
    }

    auto Prediction::process_path( const int16_t index, const int data_index ) -> void{
        if ( data_index < 0 || data_index > m_prediction_data.size( ) - 1 ) return;

        auto& data = m_prediction_data[ data_index ];

        auto object = g_entity_list.get_by_index( index );
        if ( !object ) return;

        object.update( );

        if ( object->is_dead( ) || object->is_invisible( ) ) return;

        if ( data.network_id == 0 ) {
            // debug_log( "Process path error: invalid nid [ {} ]", object->champion_name.text );
            return;
        }

        auto aimgr = object->get_ai_manager( );
        if ( !aimgr ) {
            // debug_log( "Process path error: invalid aimgr [ {} ]", object->champion_name.text );
            return;
        }

        aimgr.update( );

        const auto path = aimgr->get_path( );
        if ( path.empty( ) || path.size( ) <= 1 ) {
            data.is_pathing = false;
            return;
        }

        const auto max_path = path.size( ) - 1;
        const auto path_end = aimgr->get_path_end_position( );

        if ( path_end == data.last_path_end_position ) {
            // debug_log( "Process path error: old path detected [ {} ]", object->champion_name.text );
            return;
        }

        if ( aimgr->is_dashing && aimgr->is_moving && data.dash_start_time == 0.f ) {
            data.paths_after_dash = 0;
            data.dash_start_time  = *g_time;
            data.is_pathing       = false;
            data.was_pathing      = false;
        } else if ( data.dash_start_time > 0.f && ( !aimgr->is_moving || !aimgr->is_dashing ) ) {
            data.dash_start_time =
                0.f;
        }

        if ( data.is_pathing ) {
            const auto current_pos        = path[ 0 ];
            const auto previous_direction = current_pos.extend( current_pos + data.path_direction, 100.f );

            const auto v1         = path[ max_path ] - current_pos;
            const auto v2         = previous_direction - current_pos;
            const auto dot        = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto last_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

            if ( data.path_angle > 0.f ) data.last_path_angle = data.path_angle;
            data.path_angle = last_angle;

            if ( last_angle <= 20.f ) data.identical_path_count++;
            else data.identical_path_count = 0;


            data.path_duration = *g_time - data.path_change_time;
            data.was_pathing   = true;
        } else {
            data.last_path_angle = 0.f;
            data.path_angle      = 0.f;

            if ( *g_time - data.path_change_time > 2.f ) data.path_duration = 0.f;
            data.path_distance = 0.f;

            data.was_pathing          = false;
            data.is_fresh_path        = false;
            data.identical_path_count = 0;
        }

        data.last_path_end_position   = path_end;
        data.last_path_start_position = path[ 0 ];

        data.paths.push_back( *g_time );

        data.paths_after_dash++;
        data.paths_after_cast++;
        data.path_length = path[ 0 ].dist_to( path_end );

        data.path_direction = ( path_end - path[ 0 ] ).normalize( );

        data.is_pathing = aimgr->is_moving && !aimgr->is_dashing;

        data.path_change_time = *g_time; // - get_time_on_current_path( index );
        data.last_action_time = *g_time;

        if ( data.was_pathing ) {
            data.events.push_back(
                {
                    data.is_pathing
                        ? EInputType::order_move
                        : aimgr->is_dashing
                              ? EInputType::dash
                              : EInputType::unknown,
                    *g_time,
                    data.path_length,
                    data.path_angle,
                    data.last_path_start_position,
                    data.last_path_end_position
                }
            );
        } else {
            data.events.push_back(
                {
                    data.is_pathing
                        ? EInputType::order_move
                        : aimgr->is_dashing
                              ? EInputType::dash
                              : EInputType::unknown,
                    *g_time,
                    data.is_pathing ? data.path_length : -1.f
                }
            );
        }
    }

    auto Prediction::predict(
        int16_t    target_index,
        float      projectile_range,
        float      projectile_speed,
        float      projectile_width,
        float      delay,
        Vec3       source_position,
        bool       extend,
        int32_t    flags,
        ESpellType spell_type
    ) -> PredictionResult{
        // Function is used in LUA, message @tore if you change args


        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return { };
        target.update( );

        Vec3 local_position{ };
        bool range_check_from_local{ };

        if ( source_position.length( ) <= 0.f ) source_position = get_server_position( g_local->index );
        else if ( flags & check_range_from_local_position ) {
            local_position         = get_server_position(g_local->index);
            range_check_from_local = true;
        }

        PredictionResult result{ };
        EHitchanceReason hc_reason{ };
        auto             hitchance{ 0 };

        if ( target->is_hero( ) ) update_prediction( target_index );
        const auto data_index          = get_data_index( target->network_id );
        auto       data                = data_index >= 0 ? m_prediction_data[ data_index ] : PredictionInstance{ };
        auto       has_prediction_data = data_index >= 0;

        auto aimgr = target->get_ai_manager( );
        if ( !aimgr ) return result;

        aimgr.update( );

        auto path = aimgr->get_path( );
        if ( path.empty( ) ) return result;

        auto bounding_radius{ target->get_bounding_radius( ) };
        auto prediction_width{ projectile_width - 5.f };

        /*switch ( g_config->prediction.hitbox_compensation_mode->get< int >( ) ) {
        case 1:
            prediction_width *= 0.95f;
            break;
        case 2:
            prediction_width *= 0.9f;
            break;
        case 3:
            prediction_width *= 0.85f;
            break;
        case 4:
            prediction_width *= 0.8f;
            break;
        case 5:
            prediction_width *= 0.75f;
            break;
        default:
            //if ( prediction_width > 5.f ) prediction_width -= 5.f;
            break;
        }*/

        float travel_time{ };
        float travel_time_raw{ };
        bool  bad_prediction{ };

        std::optional< Vec3 > pred{ };
        std::optional< Vec3 > raw_pred{ };
        float                 time_slowed{ };

        auto radius_compensation = prediction_width / target->movement_speed;
        auto compensation_amount = projectile_width;

        if ( aimgr->is_moving && aimgr->is_dashing ) {
            if ( extend ) radius_compensation = ( prediction_width + bounding_radius ) / aimgr->dash_speed;
            else radius_compensation          = prediction_width / aimgr->dash_speed;

            compensation_amount = extend ? projectile_width + bounding_radius : projectile_width;
        } else if ( extend ) {
            radius_compensation = ( prediction_width + bounding_radius ) / target->movement_speed;

            compensation_amount = projectile_width + bounding_radius;
        }

        auto           ping_value = 0.f;
        constexpr auto cast_delay = 0.075f;
        if ( flags & include_ping ) {
            ping_value = g_features->orbwalker->get_ping( ) * ( 1.f + 0.25f * g_config->prediction.ping_compensation->get<int >( ) );
        }

        auto origin{ ( flags & check_range_from_local_position ? local_position : source_position ) };

        // debug | spellcast packet delay
        //if ( g_config->prediction.include_cast_delay->get< bool >( ) ) delay += cast_delay;

        // calculate default travel time
        if ( projectile_speed == 0.f ) {
            travel_time_raw = delay + ping_value;
            travel_time     = travel_time_raw - radius_compensation;

            //std::cout << "Traveltime(raw): " << travel_time_raw << " | compensation: " << compensation_amount
            //          << " | TT: " << travel_time << std::endl;

            pred     = predict_movement( target_index, travel_time_raw, compensation_amount );
            raw_pred = predict_movement( target_index, travel_time_raw );
            if ( !pred || !raw_pred ) return result;
        } else {
            if ( flags & extend_range_with_hitbox ) {
                auto travel_distance = source_position.dist_to( target->position );
                travel_time          = delay + travel_distance / projectile_speed;
                pred                 = predict_movement( target_index, travel_time + ping_value - radius_compensation );
                raw_pred             = predict_movement(target_index, travel_time + ping_value);
                if ( !pred || !raw_pred ) return result;

                // auto target_position{ *pred };

                if ( origin.dist_to( raw_pred.value( ) ) > projectile_range ) {
                    Vec3 safe_point{ };
                    Vec3 unknown_point{ };

                    const auto max_extend_range = projectile_range;

                    for ( auto i = 1; i <= 10; i++ ) {
                        auto temp = origin.extend( *pred, origin.dist_to( *pred ) / 10.f * i );

                        temp.y = g_navgrid->get_height( temp );

                        safe_point    = i == 1 ? origin : unknown_point;
                        unknown_point = temp;

                        if ( temp.dist_to( origin ) > max_extend_range ) break;
                    }

                    // auto cast_point{ safe_point };

                    for ( auto i = 1; i <= 20; i++ ) {
                        auto temp = safe_point.extend(
                            unknown_point,
                            safe_point.dist_to( unknown_point ) / 20.f * static_cast< float >( i )
                        );
                        temp.y = g_navgrid->get_height( temp );


                        if ( temp.dist_to( origin ) >= max_extend_range ) break;

                        // cast_point = temp;
                    }

                    // target_position = cast_point;
                }
            } else {
                // calc travel time again with predicted position
                const auto missile_size =
                    g_config->prediction.travel_reduction->get<int>() == 0 || spell_type != ESpellType::linear
                    ? 0.f : prediction_width;

                travel_time = ping_value + delay + source_position.dist_to( target->position ) / projectile_speed;
                pred        = predict_movement( target_index, travel_time );
                if ( !pred ) {
                    result.error_reason = EInvalidReason::invalid_prediction;
                    return result;
                }

                travel_time_raw = ping_value + delay + ( source_position.dist_to( pred.value( ) ) - missile_size ) /
                    projectile_speed;
                travel_time = travel_time_raw - radius_compensation;

                pred     = predict_movement( target_index, travel_time_raw, compensation_amount );
                raw_pred = predict_movement( target_index, travel_time_raw );
                if ( !pred || !raw_pred ) {
                    result.error_reason = EInvalidReason::invalid_prediction;
                    return result;
                }
            }
        }

        //if ( g_config->prediction.include_cast_delay->get< bool >( ) ) {
        //    travel_time_raw -= cast_delay;
        //    travel_time -= cast_delay;
        //}

        bool is_decided{ };

        // crowd control hitchance check
        if ( target->is_hero( ) && !( flags & render_thread ) ) {
            bool      stun_found{ };
            EBuffType stun_type{ };
            float     stun_end_time{ };
            float     stun_start_time{ };

            float time_until_expire{ };
            int   speed_buff_count{ };

            bool  stasis_found{ };
            float stasis_end_time{ };

            for ( auto buff : g_features->buff_cache->get_all_buffs( target_index ) ) {
                if ( !buff || !buff->buff_data || buff->buff_data->end_time >= 25000.f ) continue;

                auto type = static_cast< EBuffType >( buff->buff_data->type );
                if ( type == EBuffType::stun ||
                    type == EBuffType::charm ||
                    type == EBuffType::fear ||
                    type == EBuffType::knockup ||
                    type == EBuffType::knockback ||
                    type == EBuffType::asleep ||
                    type == EBuffType::snare ||
                    type == EBuffType::suppression ||
                    type == EBuffType::taunt
                ) {
                    if ( buff->buff_data->end_time > stun_end_time ) {
                        time_until_expire = buff->buff_data->end_time - *g_time;
                        data.idle_time    = 0.f;

                        stun_type       = type;
                        stun_end_time   = buff->buff_data->end_time;
                        stun_start_time = buff->buff_data->start_time;
                        stun_found      = true;
                    }
                } else if ( type == EBuffType::slow ) {
                    speed_buff_count++;

                    if ( buff->buff_data->end_time - *g_time > time_slowed ) {
                        time_slowed = buff->buff_data->end_time - *
                            g_time;
                    }
                } else if ( rt_hash( buff->buff_info->get_name().c_str(  ) ) == ct_hash( "ZhonyasRingShield" ) ||
                    rt_hash( buff->buff_info->get_name().c_str(  ) ) == ct_hash( "ChronoRevive" ) ) {
                    stasis_found    = true;
                    stasis_end_time = buff->buff_data->end_time;
                }
            }

            if ( stasis_found ) {
                const auto time_left = stasis_end_time - *g_time;
                if ( time_left >= travel_time_raw ) {
                    if ( flags & extend_crowdcontrol ) {
                        auto extra_delay = stasis_end_time - travel_time_raw;
                        if ( extra_delay > 0.1f ) return { };
                    }

                    hitchance = 4;
                } else {
                    auto free_move_time = travel_time_raw - time_left;
                    auto movable_units  = free_move_time * target->movement_speed;

                    if ( movable_units < compensation_amount ) hitchance = 3;
                    else if ( movable_units <= compensation_amount * 2.f ) hitchance = 1;
                    else hitchance                                                   = 0;
                }


                *pred      = target->position;
                *raw_pred  = target->position;
                is_decided = true;
                hc_reason  = EHitchanceReason::crowd_control;
            } else if ( stun_found ) {
                data.type_of_stun  = static_cast< int >( stun_type );
                data.stun_end_time = stun_end_time;
                data.is_stunned    = true;

                const auto time_left = time_until_expire;
                if ( time_left >= travel_time_raw ) {
                    if ( flags & extend_crowdcontrol ) {
                        auto extra_delay = time_until_expire - travel_time_raw;
                        if ( extra_delay > 0.1f ) return { };
                    }

                    if ( stun_type == EBuffType::knockup && path.size( ) == 2 ) {
                        *pred = path[ path.size( ) - 1 ];
                        //std::cout << "Airborne pred fix | " << *g_time << std::endl;
                    } else if ( data.path_change_time < stun_start_time &&
                        data.last_path_end_position == path[ path.size( ) - 1 ] )
                        *pred = target->position;

                    hitchance = 4;
                    hc_reason = EHitchanceReason::crowd_control;
                } else {
                    auto free_move_time = travel_time_raw - time_left;
                    auto movable_units  = free_move_time * target->movement_speed;

                    if ( movable_units < compensation_amount ) hitchance = 3;
                    else if ( movable_units <= compensation_amount * 2.f ) hitchance = 1;
                    else hitchance                                                   = 0;

                    if ( stun_type == EBuffType::knockup && path.size( ) == 2 ) {
                        *pred = path[ path.size( ) - 1 ];
                        //std::cout << "Airborne pred fix | " << *g_time << std::endl;
                    } else if ( data.path_change_time < stun_start_time
                        && data.last_path_end_position == path[ path.size( ) - 1 ] )
                        *pred = target->position;
                }

                is_decided = true;
            } else if ( data.is_stunned ) {
                data.stun_end_time = 0.f;
                data.type_of_stun  = 0;
                data.is_stunned    = false;
            }

            /* if ( g_config->prediction.speed_calculation_mode->get< int >( ) == 0 || speed_buff_count > 1 ) {
                if ( time_slowed > 0.f && time_slowed + 0.016f < travel_time_raw || time_slowed == 0.f && *g_time - data
                    .last_slow_time <= 0.5f &&
                    static_cast< int >( data.base_movement_speed ) != static_cast< int >( target->movement_speed ) ) {
                    fluctuating_movespeed = true;
                    result.error_reason   = e_invalid_reason::bad_slow;
                } else if ( time_hasted > 0.f && time_hasted + 0.016f < travel_time_raw || time_hasted == 0.f && *g_time
                    - data.last_haste_time <= 0.5f &&
                    static_cast< int >( data.base_movement_speed ) != static_cast< int >( target->movement_speed ) ) {
                    fluctuating_movespeed = true;
                    result.error_reason   = e_invalid_reason::bad_haste;
                }
            }*/
        }

        if ( flags & extend_crowdcontrol && hitchance != 4 ) return { };

        // process dash
        if ( aimgr->is_dashing && aimgr->is_moving ) {
            auto info = predict_dash(
                target->index,
                projectile_range,
                projectile_speed,
                compensation_amount,
                delay,
                source_position
            );

            return {
                source_position.dist_to( info.position ) <= projectile_range,
                info.position,
                info.position,
                info.hitchance,
                EInvalidReason::unknown,
                EHitchanceReason::dash
            };
        }

        // process spell cast / idle
        if ( hitchance == 0 && !is_decided ) {
            auto sci = target->spell_book.get_spell_cast_info( );

            if ( sci ) {
                auto info = sci->get_spell_info( );
                if ( !info ) return result;

                auto spell_data = info->get_spell_data( );
                if ( !spell_data ) return result;

                auto name_hash = rt_hash( spell_data->get_name().data() );

                if ( !sci->is_autoattack && sci->server_cast_time > *g_time && is_blink_spell( name_hash ) ) {
                    const auto blink_result = predict_blink(
                        target->index,
                        projectile_range,
                        projectile_speed,
                        compensation_amount,
                        delay,
                        source_position
                    );

                    result.position = blink_result.position;
                    result.default_position = blink_result.position;
                    result.hitchance = blink_result.hitchance;
                    result.reason = EHitchanceReason::spellcast;
                    result.valid = source_position.dist_to( blink_result.position ) <= projectile_range && blink_result.
                        valid;
                    return result;
                }

                if ( !sci->is_autoattack && sci->server_cast_time > *g_time && is_dash_spell( name_hash ) ) {
                    auto dash_result = predict_dash_from_spell(
                        target->index,
                        projectile_range,
                        projectile_speed,
                        compensation_amount,
                        delay,
                        source_position
                    );

                    result.position = dash_result.position;
                    result.default_position = dash_result.position;
                    result.hitchance = dash_result.hitchance;
                    result.reason = EHitchanceReason::spellcast;
                    result.valid = source_position.dist_to( dash_result.position ) <= projectile_range && dash_result.
                        valid;
                    return result;
                }

                if ( !sci->is_autoattack && is_channel_spell( name_hash ) ) {
                    auto duration_left = sci->end_time - *g_time;

                    auto can_reach = source_position.dist_to( *pred ) <= projectile_range;

                    if ( can_reach && duration_left > travel_time_raw + g_features->orbwalker->get_ping( ) )
                    //&& duration_left < travel_time_raw + g_features->orbwalker->get_ping() + 0.025f) this will make it cast at last millisecond
                    {
                        hitchance = 4;
                        hc_reason = EHitchanceReason::spellcast;
                    }

                    if ( hitchance == 0 && is_channel_blink( name_hash ) ) {
                        auto blink_end = sci->end_position;

                        if ( source_position.dist_to( blink_end ) > projectile_range ) {
                            result.error_reason = EInvalidReason::out_of_range;
                            return result;
                        }

                        auto time_until_blink = duration_left;

                        if ( projectile_speed == 0.f ) {
                            travel_time     = delay - radius_compensation;
                            travel_time_raw = delay;
                        } else {
                            travel_time = delay + source_position.dist_to( blink_end ) / projectile_speed -
                                radius_compensation;
                            travel_time_raw = delay + source_position.dist_to( blink_end ) / projectile_speed;
                        }

                        travel_time = travel_time_raw - 0.1f;

                        if ( time_until_blink >= travel_time &&
                            time_until_blink <= travel_time_raw ) {
                            pred      = blink_end;
                            hitchance = 4;
                            hc_reason = EHitchanceReason::spellcast;
                        }
                    }

                    is_decided = true;
                }

                if ( has_prediction_data && data.is_animation_locked( ) ) {
                    auto       animation_end = data.lock_end_time;
                    const auto time_left     = animation_end - *g_time;

                    auto movable_time  = std::max( travel_time_raw - time_left, 0.f );
                    auto movable_units = target->movement_speed * movable_time;

                    if ( movable_units <= 0.f ) hitchance = 4;
                    else if ( movable_units <= compensation_amount ) hitchance = 3;
                    else if ( movable_units <= compensation_amount * 1.15f ) hitchance = 2;
                    else if ( movable_units <= compensation_amount * 2.f ) hitchance = 1;

                    *pred = target->position;

                    hc_reason  = EHitchanceReason::spellcast;
                    is_decided = true;
                    // std::cout << "[ Pred: " << target->champion_name.text << " ] Animation locked: " << time_left
                    //           << " | hitchance: " << hitchance << " | movable: " << movable_units << " | compensation: " << compensation_amount << std::endl;
                } else if ( sci->server_cast_time > *g_time && ( !aimgr->is_moving || path.size( ) <= 1 || path.size( )
                    == aimgr->next_path_node ) ) {
                    auto time_left = fix_server_cast_time( name_hash, sci->server_cast_time ) - *g_time;

                    if ( time_left >= travel_time_raw ) {
                        hitchance = 4;
                        hc_reason = EHitchanceReason::spellcast;
                    } else {
                        auto movable_time  = travel_time_raw - time_left;
                        auto movable_units = target->movement_speed * movable_time;


                        if ( movable_units <= compensation_amount ) {
                            hitchance = 3;
                            hc_reason = EHitchanceReason::spellcast;
                        } else if ( movable_units <= compensation_amount * 1.1f ) {
                            hitchance = 2;
                            hc_reason = EHitchanceReason::spellcast;
                        } else if ( movable_units <= compensation_amount * 2.f ) {
                            hitchance = 1;
                            hc_reason = EHitchanceReason::spellcast;
                        }

                        /* std::cout << "[ " << target->champion_name.text << " HC: " << hitchance
                                  << " ] cast ending in " << time_left
                                  << "s | movable: " << movable_units
                                  << " | comp: " << compensation_amount << std::endl;*/
                    }

                    *pred      = target->position;
                    is_decided = true;
                }
            } else if ( has_prediction_data && data.idle_time > 0.f ) {
                if ( *g_time - data.idle_time > 0.8f ) {
                    hitchance = 2;
                    hc_reason = EHitchanceReason::idle;

                    //std::cout << "IDLING: HITCHANCE 2\n";

                    is_decided = true;
                } else if ( *g_time - data.idle_time > 0.5f ) {
                    hitchance = 1;
                    hc_reason = EHitchanceReason::idle;

                    is_decided = true;

                    //std::cout << "IDLING: HITCHANCE 1\n";
                }

                if ( travel_time_raw <= radius_compensation ) {
                    is_decided = true;
                    hitchance  = 4;
                }
            }
        }

        auto default_pred{ raw_pred.value( ) };

        if ( flags & extend_range_with_hitbox ) {
            if ( origin.dist_to( default_pred ) >= projectile_range + projectile_width - 5.f ) {
                result.error_reason = EInvalidReason::out_of_range;
                result.position     = *pred;

                return result;
            }

            if ( origin.dist_to( default_pred ) > projectile_range - 5.f ) {
                Vec3 safe_point{ };
                Vec3 unknown_point{ };

                const auto max_extend_range = projectile_range - 5.f;

                auto extend_point{ default_pred };

                for ( auto i = 1; i <= 10; i++ ) {
                    auto temp = origin.extend( extend_point, origin.dist_to( extend_point ) / 10.f * i );

                    temp.y = g_navgrid->get_height( temp );


                    safe_point    = i == 1 ? origin : unknown_point;
                    unknown_point = temp;

                    if ( temp.dist_to( origin ) > max_extend_range ) break;
                }

                auto cast_point{ safe_point };

                for ( auto i = 1; i <= 20; i++ ) {
                    auto temp = safe_point.extend(
                        unknown_point,
                        safe_point.dist_to( unknown_point ) / 20.f * static_cast
                        < float >( i )
                    );
                    temp.y = g_navgrid->get_height( temp );


                    if ( temp.dist_to( origin ) >= max_extend_range ) break;

                    cast_point = temp;
                }


                //std::cout << "[ PRED ] Extended cast properly | old range: " << origin.dist_to( default_pred )
                //          << " | new range: " << origin.dist_to( cast_point );

                *pred = cast_point;

                if ( origin.dist_to( *pred ) >= projectile_range + projectile_width ) {
                    result.error_reason = EInvalidReason::out_of_range;
                    result.position     = *pred;

                    //std::cout << "[ PRED ] #2 Cast position too far: " << origin.dist_to( *pred )
                    //          << " | max range: " << projectile_range + projectile_width << std::endl;

                    return result;
                }
            }
        } else {
            if ( spell_type == ESpellType::circle && projectile_speed <= 0.f ) {
                if ( origin.dist_to( *pred ) >= projectile_range &&
                    origin.dist_to( default_pred ) >= projectile_range ) {
                    result.error_reason = EInvalidReason::out_of_range;
                    result.position     = *pred;
                    result.valid        = false;
                    return result;
                }
            } else if ( origin.dist_to( *pred ) >= projectile_range || origin.dist_to( default_pred ) >=
                projectile_range ||
                range_check_from_local && ( local_position.dist_to( *pred ) >= projectile_range || local_position.
                    dist_to( default_pred ) >= projectile_range ) ) {
                result.error_reason = EInvalidReason::out_of_range;
                result.position     = *pred;
                result.valid        = false;
                return result;
            }
        }

        if ( projectile_speed > 0 && pred.has_value( ) && windwall_in_line( source_position, *pred ) ) {
            bad_prediction      = true;
            result.error_reason = EInvalidReason::windwall_collision;
        }

        // process normal movement
        if ( hitchance == 0 && !is_decided && ( !has_prediction_data || data.idle_time == 0.f ) ) {
            bool is_valid_path{ };

            const auto path_end = aimgr->path_end;

            /* if ( path_end.dist_to( data.last_path_end_position ) > 5.f && m_new_path_time == 0.f ) {
                process_path( target_index, data_index, true );
            }*/

            /* if ( g_config->prediction.wall_prediction->get< bool >( ) && target->is_hero( ) &&
                      spell_type != e_spell_type::none ) {

                bool calculated_wall_position{ };

                switch ( spell_type ) {
                case e_spell_type::circle: {

                    std::vector< Vec3 > candidates{ };

                    auto base_position = target->position;
                    bool wall_found{ };

                    for ( int i = 1; i <= 5; i++ ) {
                        const auto points =
                            g_render->get_3d_circle_points( base_position, compensation_amount / 5.f * i, 10 );

                        for ( const auto point : points ) {

                            if(!wall_found && g_navgrid->is_wall( point )) {
                                wall_found = true;
                            }

                            candidates.push_back( point );
                        }
                    }

                    if ( !wall_found ) break;

                    m_wall_polygon.points = candidates;
                    m_last_wall_calculation_time = *g_time;

                    Vec3 cast_point{ };
                    float cast_safe_distance{ };

                    float lowest_target_dist{ };
                    float lowest_pred_dist{ };
                    float cast_weight{ };

                    for ( auto candidate : candidates ) {
                        if ( candidate.dist_to( local_position ) > projectile_range )
                            continue;

                        auto hitbox = sdk::math::Circle( candidate, compensation_amount ).to_polygon( 0, -1, 20 );
                        if ( hitbox.is_outside( *pred ) || hitbox.is_outside( target->position ) || hitbox.is_outside( *raw_pred ) ) continue;


                         auto safe_points =
                            g_features->evade->get_circle_segment_points( candidate, compensation_amount, 10, true )
                                .points;

                        float target_lowest_distance{ 1000.f };
                        Vec3  target_safepoint{ };
                        Vec3  pred_safepoint{ };

                        for ( auto point : safe_points ) {

                            if ( g_navgrid->is_wall( point ) )
                                continue;

                            auto distance = point.dist_to(  *pred );
                            if ( distance > target_lowest_distance ) continue;

                            target_lowest_distance = distance;
                            target_safepoint       = point;
                        }

                        float pred_lowest_distance{ 1000.f };
                        for ( auto point : safe_points ) {
                            if ( g_navgrid->is_wall( point ) )
                                continue;

                            auto distance = point.dist_to( *raw_pred );
                            if ( distance > pred_lowest_distance ) continue;

                            pred_lowest_distance = distance;
                            pred_safepoint       = point;
                        }

                        float raw_lowest_distance{ 1000.f };
                        for ( auto point : safe_points ) {
                            if ( g_navgrid->is_wall( point ) )
                                continue;

                            auto distance = point.dist_to( target->position );
                            if ( distance > raw_lowest_distance ) continue;

                            raw_lowest_distance = distance;
                        }

                        float target_weight = std::clamp( target_lowest_distance / compensation_amount, 0.f, 1.f );
                        float pred_weight   = std::clamp( pred_lowest_distance / compensation_amount, 0.f, 1.f );
                        float raw_weight    = std::clamp( raw_lowest_distance / compensation_amount, 0.f, 1.f );

                        
                        if ( target_weight + pred_weight + raw_weight < cast_weight ) continue;

                        cast_point = candidate;
                        cast_weight        = target_weight + pred_weight + raw_weight;
                        lowest_target_dist = raw_lowest_distance;
                        lowest_pred_dist   = pred_lowest_distance;

                            m_base_position = target_safepoint;
                        m_closest_safe_point = pred_safepoint;
                    }

                    if ( cast_point.length(  ) > 0.f) {
                        //std::cout << "[ prediction: wall point found ] target safe dist: " << lowest_target_dist
                        //          << " | pred safe dist: " << lowest_pred_dist << " | cast weight: " << cast_weight
                        //          << std::endl;


                        if ( cast_weight >= 2.975f ) hitchance = 4;
                        else if ( cast_weight >= 2.8f ) hitchance = 3;
                        else if ( cast_weight >= 2.6f) hitchance = 2;
                        else if ( cast_weight >= 2.2f ) hitchance = 1;

                        *pred = cast_point;
                    }


                    calculated_wall_position = true;
                    break;
                }
                case e_spell_type::linear: {

                    std::vector< Vec3 > candidates{ };

                    auto base_position = target->position;
                    bool wall_found{ };

                    const auto points = g_render->get_3d_circle_points( base_position, compensation_amount * 1.5f, 30 );
                    for ( const auto point : points ) {

                        if ( g_navgrid->is_wall( point ) ) {

                            if ( !wall_found ) wall_found = true;

                            continue;
                        }

                        candidates.push_back( point );
                    }

                    if ( !wall_found ) break;

                    m_wall_polygon.points        = candidates;
                    m_last_wall_calculation_time = *g_time;
                    Vec3  cast_point{ };
                    float cast_weight{ };

                    for ( auto candidate : candidates ) {
                        //if ( candidate.dist_to( local_position ) > projectile_range ) continue;

                        auto hitbox = sdk::math::Rectangle( source_position, source_position.extend( candidate, projectile_range ), compensation_amount ).to_polygon( );
                        if ( hitbox.is_outside( *pred ) || hitbox.is_outside( target->position ) || hitbox.is_outside( *raw_pred ) )
                            continue;

                        auto safe_points =
                            g_features->evade->get_line_segment_points( source_position, source_position.extend( candidate, projectile_range ), compensation_amount,
                            true ).points;

                        std::vector< Vec3 > valid_points{ };
                        for ( auto safe_point : safe_points ) {
                            if ( g_navgrid->is_wall( safe_point ) ) continue;

                            valid_points.push_back( safe_point );
                        }

                        if ( valid_points.empty( ) ) continue;

                        float target_lowest_distance{ 1000.f };
                        Vec3  target_safepoint{ };
                        Vec3  pred_safepoint{ };

                        for ( auto point : valid_points ) {
                            auto distance = point.dist_to( *pred );
                            if ( distance > target_lowest_distance ) continue;

                            target_lowest_distance = distance;
                            target_safepoint       = point;
                        }

                        float pred_lowest_distance{ 1000.f };
                        for ( auto point : valid_points ) {
                            auto distance = point.dist_to( *raw_pred );
                            if ( distance > pred_lowest_distance ) continue;

                            pred_lowest_distance = distance;
                            pred_safepoint       = point;
                        }

                        float raw_lowest_distance{ 1000.f };
                        for ( auto point : valid_points ) {
                            auto distance = point.dist_to( target->position );
                            if ( distance > raw_lowest_distance ) continue;

                     
                            raw_lowest_distance = distance;
                        }

                        float target_weight = std::clamp( target_lowest_distance / compensation_amount, 0.f, 1.f );
                        float pred_weight   = std::clamp( pred_lowest_distance / compensation_amount, 0.f, 1.f );
                        float raw_weight    = std::clamp( raw_lowest_distance / compensation_amount, 0.f, 1.f );


                        if ( target_weight + pred_weight + raw_weight < cast_weight ) continue;

                        cast_point         = candidate;
                        cast_weight        = target_weight + pred_weight + raw_weight;

                        m_base_position      = target_safepoint;
                        m_closest_safe_point = pred_safepoint;
                    }

                    if ( cast_point.length( ) > 0.f ) {

                        if ( cast_weight >= 2.95f ) hitchance = 4;
                        else if ( cast_weight >= 2.75f )
                            hitchance = 3;
                        else if ( cast_weight >= 2.5f )
                            hitchance = 2;
                        else if ( cast_weight >= 2.f )
                            hitchance = 1;

                        std::cout << "[ Prediction: Linear wall pred ] Hitchance" << hitchance
                                  << " | weight: " << cast_weight << std::endl;

                        *pred = cast_point;
                        calculated_wall_position = true;
                    }

                    break;
                }
                default:
                    break;
                }
            }
            */

            const auto time_on_path = *g_time - data.path_change_time;

            if ( travel_time_raw <= radius_compensation ) {
                //std::cout << target->get_name( ) << " | Guaranteed hit | T: " << *g_time << std::endl;
                hitchance = 4;
            }

            // walking hitchance rating logic
            if ( hitchance <= 1 ) {
                switch ( g_config->prediction.hitchance_rating_logic->get< int >( ) ) {
                case 0:
                {
                    auto high_threshold = 0.055f;
                    is_valid_path       = g_config->prediction.slower_prediction->get< bool >( )
                                              ? target->position.dist_to( path_end ) / target->movement_speed >
                                              travel_time_raw
                                              + 0.25f
                                              : true;

                    if ( !is_valid_path ) {
                        hitchance = 0;
                        break;
                    }

                    if ( time_on_path <= high_threshold ) hitchance = 2;
                    else if ( time_on_path <= 0.1f ) hitchance = 1;
                    else hitchance                             = 0;

                    if ( hitchance == 1 ) {
                        // dbg normal is hitchance == 2 or hitchance > 0

                        // if enemys first move command after casting aa/ability, big hitchance improvement
                        if ( data.paths_after_cast == 1 && *g_time - data.last_cast_time <= 0.2f ) {
                            hc_reason = EHitchanceReason::post_spell;
                            hitchance++;
                        }

                        // if slowed and slow will last until spell hits
                        if ( time_slowed > 0.f && travel_time_raw <= time_slowed ) {
                            hc_reason = EHitchanceReason::slowed;
                            hitchance++;
                        }

                        /* if ( data.is_path_secure( ) &&
                                  ( data.identical_path_count > 0 || data.is_path_confirmed( ) ) ) {
                            hitchance++;
                            hc_reason = e_hitchance_reason::similiar_path_angle;
                        }*/

                        // if long path
                        // if ( target->position.dist_to( path_end ) > 1200.f ) hitchance++;

                        // if enemies path changed big time
                        if ( data.path_angle > 80.f ) {
                            hc_reason = EHitchanceReason::fresh_path_angle;
                            hitchance++;
                        }

                        if ( hitchance > 2 ) hitchance = 2;
                    }

                    if ( !is_valid_path ) hitchance = 0;

                    if ( travel_time_raw <= radius_compensation ) hitchance = 4;

                    break;
                }
                case 1:
                {
                    auto high_threshold = 0.055f;
                    is_valid_path       = g_config->prediction.slower_prediction->get< bool >( )
                                              ? target->position.dist_to( path_end ) / target->movement_speed >
                                              travel_time_raw
                                              + 0.25f
                                              : true;

                    if ( !is_valid_path ) {
                        hitchance = 0;
                        break;
                    }

                    if ( time_on_path <= high_threshold ) hitchance = 2;
                    else if ( time_on_path <= 0.1f ) hitchance = 1;
                    else hitchance                             = 0;

                    if ( hitchance > 0 ) {
                        auto best_possible_hitchance = 2;
                        // if enemys first move command after casting aa/ability, big hitchance improvement
                        if ( data.paths_after_cast == 1 && *g_time - data.last_cast_time <= 0.2f ) {
                            hc_reason = EHitchanceReason::post_spell;
                            hitchance++;
                        }

                        if ( !data.is_path_secure( ) ) best_possible_hitchance = 1;
                        else if ( data.identical_path_count > 2 || data.is_path_confirmed( ) ) {
                            hitchance++;
                            hc_reason = EHitchanceReason::similar_path_angle;
                        }

                        // if slowed and slow will last until spell hits
                        /* if ( time_slowed > 0.f && travel_time_raw <= time_slowed ) {
                            hc_reason = e_hitchance_reason::slowed;
                            hitchance++;
                        }*/

                        // if long path
                        //if ( target->position.dist_to( path_end ) > 1200.f ) hitchance++;

                        //if ( data.identical_path_count > 0 && data.identical_path_count <= 3 )
                        //     best_possible_hitchance = 0;

                        // if enemies path changed big time
                        if ( data.path_angle >= 50.f ) {
                            hc_reason = EHitchanceReason::fresh_path_angle;
                            hitchance++;
                        }

                        if ( hitchance > best_possible_hitchance ) hitchance = best_possible_hitchance;
                    }

                    break;
                }
                default:
                {
                    auto high_threshold = 0.075f;
                    is_valid_path       = g_config->prediction.slower_prediction->get< bool >( )
                                              ? target->position.dist_to( path_end ) / target->movement_speed >
                                              travel_time_raw
                                              + 0.25f + g_features->orbwalker->get_ping( )
                                              : true;
                    if ( !is_valid_path ) {
                        hitchance = 0;
                        break;
                    }

                    if ( time_on_path <= high_threshold ) hitchance = 2;
                    else if ( time_on_path <= 0.125f ) hitchance = 1;
                    else hitchance                               = 0;

                    if ( hitchance == 1 ) {
                        bool is_good_path{ };

                        if ( data.paths_after_cast == 1 && *g_time - data.last_cast_time <= 0.2f ) {
                            hc_reason    = EHitchanceReason::post_spell;
                            is_good_path = true;
                        }

                        if ( data.path_angle > 50.f ) {
                            is_good_path = true;
                            hc_reason    = EHitchanceReason::fresh_path_angle;
                        }

                        if ( is_good_path ) hitchance = 2;
                    }

                    break;
                }
                }
            }
        }

        if ( target->movement_speed == 0.f ) {
            result.position         = pred.value( );
            result.default_position = result.position;
            result.valid            = origin.dist_to( *pred ) <= projectile_range;
            if ( !result.valid ) result.error_reason = EInvalidReason::out_of_range;
            result.hitchance = ( EHitchance )hitchance;

            return result;
        }

        result.position         = pred.value( );
        result.default_position = default_pred;
        result.hitchance        = static_cast< EHitchance >( hitchance );
        result.reason           = hc_reason;
        result.valid            = !bad_prediction;

        return result;
    }

    //PykeQRange

    auto Prediction::get_animation_lock_duration( const int16_t index ) -> float{
        const auto target = g_entity_list.get_by_index( index );
        if ( !target ) return 0.f;

        auto sci = target->spell_book.get_spell_cast_info( );
        if ( !sci ) return 0.f;

        const auto name      = sci->get_spell_name( );
        const auto name_hash = rt_hash( name.data( ) );

        switch ( name_hash ) {
        case ct_hash( "PykeQ" ):
            return 0.f;
        case ct_hash( "PykeQRange" ):
            return sci->server_cast_time + sci->start_position.dist_to( sci->end_position ) / 2000.f;
        case ct_hash( "RocketGrab" ):
            return sci->server_cast_time + 1115.f / 1800.f;
        case ct_hash( "ThreshQInternal" ):
            return sci->server_cast_time + 1100.f / 1900.f;
        case ct_hash( "LeonaZenithBlade" ):
            return sci->server_cast_time + 900.f / 2000.f;
        case ct_hash( "NautilusAnchorDragMissile" ):
            return sci->server_cast_time + 1100.f / 2000.f;
        case ct_hash( "ThreshE" ):
        case ct_hash( "ZyraE" ):
        case ct_hash( "KaynW" ):
            return sci->server_cast_time + 0.15f;
        default:
            return sci->server_cast_time;
        }
    }

    auto Prediction::predict_dash(
        int16_t target_index,
        float   range,
        float   speed,
        float   width,
        float   delay,
        Vec3    source_position
    ) -> PredictionResult{
        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return { false };

        target.update( );

        PredictionResult result{ };
        auto             hitchance{ 0 };

        auto aimgr = target->get_ai_manager( );
        if ( !aimgr || !aimgr->is_moving || !aimgr->is_dashing ) return result;

        auto path = aimgr->get_path( );
        if ( path.empty( ) ) return result;

        auto dash_start = target->position;
        auto dash_end   = path[ path.size( ) - 1 ];

        auto dash_range = dash_start.dist_to( dash_end );
        auto dash_speed = aimgr->dash_speed;
        auto extra_lock_time{ 0.f };

        bool ignore_movable_distance{ };

        switch ( rt_hash( target->champion_name.text ) ) {
        case ct_hash( "Kayn" ):
        {
            auto spell_slot = target->spell_book.get_spell_slot( ESpellSlot::q );
            if ( spell_slot && !spell_slot->is_ready( false ) ) {
                auto current_cd = spell_slot->cooldown_expire - *g_time;
                auto total_cd   = spell_slot->cooldown;

                if ( current_cd > total_cd - 0.4f ) extra_lock_time = 0.3f;
            }
            break;
        }
        case ct_hash( "Yuumi" ):
        {
            auto spell_slot = target->spell_book.get_spell_slot( ESpellSlot::w );
            if ( spell_slot && spell_slot->is_ready( ) ) ignore_movable_distance = true;
            break;
        }
        default:
            break;
        }

        auto  travel_time{ delay };
        float raw_travel_time;

        if ( speed > 0.f ) travel_time = delay + source_position.dist_to( target->position ) / speed;

        raw_travel_time = travel_time;

        auto dash_time     = travel_time;
        auto dash_distance = dash_time * dash_speed;
        if ( dash_distance > dash_range ) dash_distance = dash_range;

        auto predicted = dash_start.extend( dash_end, dash_distance );
        predicted.y    = 0.f;

        if ( speed > 0.f ) {
            travel_time   = delay + source_position.dist_to( predicted ) / speed;
            dash_distance = travel_time * dash_speed;
            if ( dash_distance > dash_range ) dash_distance = dash_range;

            predicted   = dash_start.extend( dash_end, dash_distance );
            predicted.y = 0.f;

            travel_time     = delay + source_position.dist_to( predicted ) / speed + g_features->orbwalker->get_ping( );
            raw_travel_time = delay + source_position.dist_to( predicted ) / speed;

            dash_distance = travel_time * dash_speed;
            if ( dash_distance > dash_range ) dash_distance = dash_range;

            predicted   = dash_start.extend( dash_end, dash_distance );
            predicted.y = g_navgrid->get_height( predicted );
        }

        if ( static_cast< int >( dash_distance ) >= static_cast< int >( dash_range ) ) {
            auto after_dash_time  = raw_travel_time - dash_range / dash_speed;
            auto movable_distance = std::max( after_dash_time - extra_lock_time, 0.f ) * target->movement_speed;

            if ( movable_distance <= 0.f ) hitchance = 4;
            else if ( movable_distance <= width ) hitchance = 3;
            else if ( movable_distance <= width * 1.25f ) hitchance = 2;
            else if ( movable_distance <= width * 1.75f ) hitchance = 1;
            else hitchance                                          = 0;

            if ( ignore_movable_distance ) hitchance = 0;

            result.position  = { dash_end.x, g_navgrid->get_height( dash_end ), dash_end.z };
            result.hitchance = static_cast< EHitchance >( hitchance );
            result.valid     = source_position.dist_to( result.position ) <= range;
            return result;
        }

        predicted.y      = g_navgrid->get_height( predicted );
        result.position  = predicted;
        result.hitchance = EHitchance::immobile;
        result.valid     = source_position.dist_to( result.position ) <= range;
        return result;
    }

    auto Prediction::predict_dash_from_spell(
        int16_t target_index,
        float   range,
        float   speed,
        float   width,
        float   delay,
        Vec3    source_position
    ) -> PredictionResult{
        auto& target = g_entity_list.get_by_index( target_index );
        if ( !target ) return { false };

        target.update( );

        PredictionResult result{ };
        auto             hitchance{ 0 };

        auto sci = target->spell_book.get_spell_cast_info( );
        if ( !sci ) return result;

        auto info = sci->get_spell_info( );
        if ( !info ) return result;

        auto data = info->get_spell_data( );
        if ( !data ) return result;

        auto name_hash = rt_hash( data->get_name( ).data( ) );
        if ( !is_dash_spell( name_hash ) ) return result;

        Vec3 predicted{ };

        Vec3 start{ sci->start_position.x, g_navgrid->get_height( sci->start_position ), sci->start_position.z };
        Vec3 end{ sci->end_position.x, g_navgrid->get_height( sci->end_position ), sci->end_position.z };

        Vec3 start_raw{ sci->start_position.x, 0.f, sci->start_position.z };
        Vec3 end_raw{ sci->end_position.x, 0.f, sci->end_position.z };

        bool is_processed{ };

        auto server_cast_time{ sci->server_cast_time };

        if ( name_hash == ct_hash( "AkaliE" ) ) {
            server_cast_time -= 0.15f;

            auto pred = g_features->prediction->predict_default(
                target->index,
                std::max( server_cast_time - *g_time, 0.f )
            );
            if ( !pred ) return result;

            auto dash_start_position{ pred.value( ) };

            auto direction     = sci->end_position - sci->start_position;
            auto direction_vec = dash_start_position + direction;

            auto simulated = dash_start_position.extend( direction_vec, 400.f );

            start     = dash_start_position;
            start_raw = { start.x, 0.f, start.z };

            end     = simulated;
            end_raw = { simulated.x, 0.f, simulated.z };

            auto time_on_path = get_time_on_current_path( target->index );
            if ( *g_time - time_on_path > sci->start_time ) {
                auto aimgr = target->get_ai_manager( );
                if ( aimgr && aimgr->is_moving && aimgr->is_dashing ) {
                    return predict_dash(
                        target_index,
                        range,
                        speed,
                        width,
                        delay,
                        source_position
                    );
                }
            }

            is_processed = true;
        } else if ( name_hash == ct_hash( "GalioE" ) ) {
            if ( server_cast_time - 0.3f > *g_time ) return result;

            start     = target->position;
            start_raw = { start.x, 0.f, start.z };

            auto rect = sdk::math::Rectangle(
                target->position,
                target->position.extend( sci->end_position, start.dist_to( sci->start_position ) + 380.f ),
                320.f
            );
            auto polygon = rect.to_polygon( );


            Vec3 end_position{ };
            bool is_calculated{ };
            for ( auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id == g_local->network_id || ally->is_dead( ) || ally->is_invisible( )
                    || ally->get_selectable_flag( ) != 1 || ally->position.dist_to( target->position ) > 1000.f )
                    continue;

                auto start_pred = g_features->prediction->predict_default(
                    ally->index,
                    std::max( server_cast_time - *g_time, 0.f )
                );
                if ( !start_pred ) continue;

                if ( polygon.is_inside( *start_pred ) && polygon.is_inside( ally->position ) ) {
                    auto nearest = g_features->evade->get_closest_line_point(
                        target->position,
                        sci->end_position,
                        *start_pred
                    );

                    end_position  = nearest.extend( start, 80.f );
                    is_calculated = true;
                    break;
                }

                // champions.push_back( ally->index );
            }

            if ( !is_calculated ) {
                for ( auto i = 1; i <= 10; i++ ) {
                    auto temp =
                        start.extend( sci->end_position, ( start.dist_to( sci->start_position ) + 650.f ) / 10 * i );
                    if ( g_navgrid->is_wall( temp ) ) break;

                    //rect    = sdk::math::Rectangle( temp, temp.extend( sci->end_position, 100.f ), 320.f );
                    //polygon = rect.to_polygon( );

                    //bool should_break{ };

                    // if ( should_break ) break;

                    end_position = temp;
                }
            }

            end     = { end_position.x, g_navgrid->get_height( end_position ), end_position.z };
            end_raw = { end.x, 0.f, end.z };

            is_processed = true;
        } else if ( name_hash == ct_hash( "YuumiWCast" ) ) {
            server_cast_time += 0.25f;

            auto  index = sci->get_target_index( );
            auto& unit  = g_entity_list.get_by_index( index );
            if ( !unit ) return result;

            unit.update( );
            if ( unit->is_dead( ) || unit->is_invisible( ) ) return result;

            auto pred = g_features->prediction->predict_default(
                unit->index,
                std::max( sci->server_cast_time - *g_time, 0.f ),
                false
            );
            if ( !pred ) return result;

            end          = *pred;
            end_raw      = { end.x, 0.f, end.z };
            is_processed = true;
        }

        auto dash_end = end;

        if ( !is_processed && get_fixed_range( name_hash ) ) {
            auto dash_range = get_fixed_range( name_hash ).value( );

            dash_end   = start_raw.extend( end_raw, dash_range );
            dash_end.y = g_navgrid->get_height( dash_end );

            if ( static_cast< int >( start.dist_to( dash_end ) ) != static_cast< int >( dash_range ) ) {
                auto difference = start.dist_to( dash_end ) - dash_range;
                auto extend     = start_raw.extend( end_raw, dash_range - difference );
                extend.y        = g_navgrid->get_height( extend );

                std::cout << "[ DASH PRED ] Fixed end position, final distance: " << start.dist_to( extend )
                    << " | original distance: " << start.dist_to( dash_end ) << " | goal: " << dash_range
                    << std::endl;

                dash_end = extend;
            }
        } else if ( !is_processed && get_spell_max_range( name_hash ) ) {
            const auto max_range = get_spell_max_range( name_hash ).value( );

            if ( start.dist_to( end ) > max_range ) {
                dash_end   = start.extend( end, max_range );
                dash_end.y = g_navgrid->get_height( dash_end );
            }
        }

        end     = dash_end;
        end_raw = { dash_end.x, 0.f, dash_end.z };

        auto dash_range  = start.dist_to( dash_end );
        auto dash_speed  = get_dash_speed( name_hash, target->movement_speed );
        auto windup_time = std::max( server_cast_time - *g_time, 0.f );

        switch ( name_hash ) {
        case ct_hash( "YuumiWCast" ):
        {
            auto spell = target->spell_book.get_spell_slot( ESpellSlot::w );
            if ( !spell ) return result;

            dash_speed = 1100.f + 100.f * spell->level;
            dash_range = start.dist_to( end );
            break;
        }
        default:
            break;
        }

        float travel_time{ };
        float raw_travel_time{ };
        float radius_compensation{ 0 };

        if ( speed > 0.f ) travel_time = delay + source_position.dist_to( target->position ) / speed;
        else travel_time               = delay;

        travel_time += g_features->orbwalker->get_ping( ) * 2.f;

        raw_travel_time = travel_time;

        if ( windup_time + radius_compensation >= travel_time ) { // if can hit before dash even begins
            predicted = target->position;
            hitchance = 4;

            result.position  = predicted;
            result.hitchance = static_cast< EHitchance >( hitchance );
            result.valid     = source_position.dist_to( result.position ) <= range;

            return result;
        }

        auto dash_time     = travel_time - windup_time;
        auto dash_distance = dash_time * dash_speed;
        if ( dash_distance > dash_range ) dash_distance = dash_range;

        predicted   = start_raw.extend( end_raw, dash_distance );
        predicted.y = 0.f;

        if ( speed > 0.f ) {
            travel_time =
                delay + source_position.dist_to( predicted ) / speed;
            dash_distance = ( travel_time - windup_time ) * dash_speed;
            if ( dash_distance > dash_range ) dash_distance = dash_range;

            predicted   = start_raw.extend( end_raw, dash_distance );
            predicted.y = 0.f;

            travel_time = delay + source_position.dist_to( predicted ) / speed + g_features->orbwalker->get_ping( ) *
                2.f;
            raw_travel_time = delay + source_position.dist_to( predicted ) / speed + g_features->orbwalker->get_ping( )
                * 2.f;

            dash_distance = ( travel_time - windup_time ) * dash_speed;
            if ( dash_distance > dash_range ) dash_distance = dash_range;

            predicted   = start_raw.extend( end_raw, dash_distance );
            predicted.y = g_navgrid->get_height( predicted );
        }

        if ( static_cast< int >( dash_distance ) == static_cast< int >( dash_range ) ) {
            auto after_dash_time  = raw_travel_time - ( windup_time + dash_range / dash_speed );
            auto movable_distance = after_dash_time * target->movement_speed;

            if ( movable_distance <= width ) hitchance = 3;
            else if ( movable_distance <= width * 1.15f ) hitchance = 2;
            else if ( movable_distance <= width * 1.25f ) hitchance = 1;
            else hitchance                                          = 0;

            if ( name_hash == ct_hash( "YuumiWCast" ) ) hitchance = 0;


            result.position  = { dash_end.x, g_navgrid->get_height( dash_end ), dash_end.z };
            result.hitchance = static_cast< EHitchance >( hitchance );
            result.valid     = source_position.dist_to( result.position ) <= range;

            return result;
        }

        predicted.y      = g_navgrid->get_height( predicted );
        result.position  = predicted;
        result.hitchance = EHitchance::immobile;
        result.valid     = source_position.dist_to( result.position ) <= range;

        return result;
    }

    auto Prediction::predict_blink(
        const int16_t target_index,
        const float   range,
        const float   speed,
        const float   width,
        const float   delay,
        const Vec3    source_position
    ) -> PredictionResult{
        auto& target = g_entity_list.get_by_index( target_index );
        if ( !target ) return { false };

        target.update( );

        PredictionResult result{ };
        int32_t          hitchance;

        auto sci = target->spell_book.get_spell_cast_info( );
        if ( !sci ) return result;

        auto info = sci->get_spell_info( );
        if ( !info ) return result;

        auto data = info->get_spell_data( );
        if ( !data ) return result;

        const auto name_hash = rt_hash( data->get_name( ).data( ) );
        if ( !is_blink_spell( name_hash ) ) return result;

        auto blink_end = sci->end_position;

        if ( get_spell_max_range( name_hash ) &&
            sci->start_position.dist_to( sci->end_position ) > get_spell_max_range( name_hash ).value( ) )
            blink_end = sci->start_position.extend( sci->end_position, get_spell_max_range( name_hash ).value( ) );

        switch ( name_hash ) {
        case ct_hash( "EkkoEAttack" ):
        {
            const auto index = sci->get_target_index( );
            auto&      unit  = g_entity_list.get_by_index( index );
            if ( !unit ) return result;

            unit.update( );
            if ( unit->is_dead( ) || unit->is_invisible( ) ) return result;

            const auto pred = g_features->prediction->predict_default(
                unit->index,
                std::max( sci->server_cast_time - *g_time, 0.f ),
                false
            );
            if ( !pred ) return result;

            const auto target_radius = unit->is_hero( )
                                           ? get_champion_radius( rt_hash( unit->champion_name.text ) )
                                           : unit->get_bounding_radius( );

            blink_end = target->position.extend(
                *pred,
                target->position.dist_to( *pred ) - target_radius - get_champion_radius(
                    rt_hash( target->champion_name.text )
                )
            );
            if ( g_navgrid->is_wall( blink_end ) ) blink_end = *pred;

            break;
        }
        default:
            break;
        }

        blink_end.y = g_navgrid->get_height( blink_end );

        const auto time_to_blink = sci->server_cast_time - *g_time;
        auto       travel_time{ delay };

        const auto pred = g_features->prediction->predict_default( target->index, time_to_blink, false );
        if ( !pred ) return result;

        if ( speed > 0.f ) travel_time = delay + source_position.dist_to( pred.value( ) ) / speed;

        if ( time_to_blink >= travel_time ) {
            return {
                source_position.dist_to( pred.value( ) ) <= range,
                pred.value( ),
                pred.value( ),
                EHitchance::immobile
            };
        }

        if ( g_navgrid->is_wall( blink_end ) || source_position.dist_to( blink_end ) > range ) return result;

        if ( speed > 0.f ) {
            travel_time = delay + source_position.dist_to( blink_end ) / speed;

            if ( time_to_blink >= travel_time ) return { true, blink_end, blink_end, EHitchance::immobile };
        }

        const auto after_blink_time = travel_time - time_to_blink;
        const auto movable_units    = after_blink_time * target->movement_speed;

        if ( movable_units <= width ) hitchance = 3;
        else if ( movable_units <= width * 1.1f ) hitchance = 2;
        else hitchance                                      = 0;

        result.position         = blink_end;
        result.default_position = blink_end;
        result.hitchance        = static_cast< EHitchance >( hitchance );
        result.valid            = source_position.dist_to( result.position ) <= range;

        return result;
    }

    auto Prediction::get_time_on_current_path( const int16_t index ) -> float{
        auto& target = g_entity_list.get_by_index( index );
        if ( !target ) return 0.f;

        target.update( );

        if ( target->is_dead( ) || target->is_invisible( ) ) return 0.f;

        auto mgr = target->get_ai_manager( );
        if ( !mgr || !mgr->is_moving ) return 0.f;

        const auto path            = mgr->get_path( );
        const auto last_node_index = mgr->next_path_node - 1;
        float      distance_moved{ };

        const auto movespeed = mgr->is_dashing ? mgr->dash_speed : target->movement_speed;

        if ( mgr->next_path_node >= static_cast< int32_t >( path.size( ) ) || path.size( ) <= 1u ||
            last_node_index < 0 )
            return 0.f;

        if ( last_node_index > 0 ) {
            Vec3 last_node{ };

            for ( auto i = last_node_index; i >= 0; i-- ) {
                last_node = path[ i ];

                distance_moved += i == last_node_index
                                      ? target->position.dist_to( path[ last_node_index ] )
                                      : last_node.dist_to( path[ i + 1 ] );

                if ( i == 0 ) break;
            }
        } else {
            const auto last_next_delta   = path[ last_node_index ].dist_to( path[ last_node_index + 1 ] );
            const auto target_next_delta = target->position.dist_to( path[ last_node_index + 1 ] );

            if ( last_node_index == 0 && target_next_delta >= last_next_delta ) return 0.f;

            distance_moved = target->position.dist_to( path[ 0 ] );
        }

        return distance_moved / movespeed;
    }

    auto Prediction::is_object_turret_target( Object* object ) const -> bool{
        for ( auto& shot : m_turret_attacks ) if ( shot.target_nid == object->network_id ) return true;

        return false;
    }

    auto Prediction::get_average_path_time( const unsigned network_id ) -> float{
        const auto o = get_data_index( network_id );
        if ( o < 0 ) return 0.25f;

        int   sample_count{ };
        float last_path{ };
        float path_sum{ };

        if ( m_prediction_data[ o ].paths.empty( ) ) return 0.25f;

        for ( const auto inst : m_prediction_data[ o ].paths ) {
            if ( *g_time - inst >= 15.f ) continue;

            if ( last_path == 0.f ) {
                last_path = inst;
                continue;
            }

            const auto path_time = inst - last_path;

            if ( path_time >= 2.f ) {
                last_path = inst;
                continue;
            }

            last_path = inst;
            path_sum += path_time;
            sample_count++;
        }

        if ( sample_count < 10 ) return 0.25f;

        const auto avg_time = path_sum / static_cast< float >( sample_count );
        if ( isnan( avg_time ) || isinf( avg_time ) ) return 0.25f;

        return avg_time;
    }

    auto Prediction::predict_default(
        const int32_t target_index,
        float         time,
        const bool    include_extra_tick
    ) const
        -> std::optional< Vec3 >{
        // Function is used in LUA, message @tore if you change args
        auto target = g_entity_list.get_by_index( target_index );
        if ( !target ) return std::nullopt;

        //if ( target->is_hero( ) && !target->is_local() ) return predict_movement( target_index, time );

        target.update( );

        if ( !target || target->is_dead( ) ) return std::nullopt;
        if ( target->is_ward( ) || time <= 0.f || target->movement_speed == 0.f ) {
            return std::make_optional(
                target->position
            );
        }
        if ( target->is_invisible( ) ) return std::nullopt;

        auto mgr = target->get_ai_manager( );
        if ( !mgr || !mgr->is_moving ) return std::make_optional( target->position );

        Vec3       predicted_position{ };
        const auto waypoint_index = mgr->next_path_node;
        const auto path           = mgr->get_path( );

        if ( path.empty( ) || waypoint_index == path.size( ) || path.size( ) == 1 || static_cast< size_t >(
            waypoint_index ) >= path.size( ) )
            return std::make_optional( target->position );

        auto current_waypoint = path[ waypoint_index ];
        current_waypoint.y    = g_navgrid->get_height( current_waypoint );

        auto move_speed = target->movement_speed;
        if ( mgr->is_dashing ) move_speed = mgr->dash_speed;

        if ( include_extra_tick ) time += 0.016f;

        auto       movable_units     = move_speed * time;
        const auto waypoint_distance = target->position.dist_to( current_waypoint );

        if ( !mgr->is_dashing && target->is_hero( ) && g_config->prediction.speed_calculation_mode->get< int >( ) == 1
            && g_threading->is_feature_thread( ) ) {
            int   speed_modifying_buff_count{ };
            float buff_duration{ };
            bool  should_skip{ };

            for ( const auto buff : g_features->buff_cache->get_all_buffs( target_index ) ) {
                if ( !buff || !buff->buff_data ) continue;

                switch ( static_cast< EBuffType >( buff->buff_data->type ) ) {
                case EBuffType::slow:
                case EBuffType::haste:
                    if ( buff->buff_data->end_time >= 25000.f ) break; // if permanent buff

                    speed_modifying_buff_count++;
                    buff_duration = buff->buff_data->end_time - *g_time;
                    should_skip   = speed_modifying_buff_count > 1;
                    break;
                default:
                    break;
                }

                if ( should_skip ) break;
            }

            if ( speed_modifying_buff_count == 1 && buff_duration < time ) {
                const auto normal_speed     = get_normal_move_speed( target->index );
                const auto normal_time_left = time - buff_duration;

                if ( normal_speed > 0.f ) {
                    movable_units = target->movement_speed * buff_duration + normal_speed *
                        normal_time_left;
                }
            }
        }

        if ( movable_units > waypoint_distance && static_cast< int32_t >( path.size( ) ) > waypoint_index + 1 ) {
            movable_units -= waypoint_distance;
            auto current_node = current_waypoint;

            for ( auto i = waypoint_index + 1u; i < path.size( ); i++ ) {
                auto next_node = path[ i ];
                current_node   = i == waypoint_index + 1 ? current_waypoint : path[ i - 1 ];

                current_node.y = g_navgrid->
                    get_height( current_node ); // setting y for waypoints to get correct distance between them
                next_node.y = g_navgrid->get_height( next_node );


                const auto node_distance = current_node.dist_to( next_node );

                // if next simulated node is too far to walk to, calculate position between current and next node which target will reach
                if ( node_distance >= movable_units || i == path.size( ) - 1 ) {
                    predicted_position = current_node.extend(
                        next_node,
                        node_distance > movable_units ? movable_units : node_distance
                    );
                    break;
                }

                movable_units -= node_distance;
            }
        } else {
            predicted_position = target->position.extend(
                current_waypoint,
                movable_units > waypoint_distance ? waypoint_distance : movable_units
            );
        }

        return std::make_optional(
            Vec3{ predicted_position.x, g_navgrid->get_height( predicted_position ), predicted_position.z }
        );
    }

    auto Prediction::predict_health(
        const Object* object,
        const float   time,
        const bool    multiple_attacks,
        const bool    ignore_turret,
        const bool    ignore_minions,
        const bool    override_delay
    ) -> float{
        // Function is used in LUA, message @tore if you change args
        auto predicted_health = object->health + object->total_health_regen;

        if ( object->is_minion( ) ) return predict_minion_health( object->index, time, multiple_attacks );

        update_minion_attacks( );

        if ( !ignore_minions ) {
            for ( const auto& attack : m_attacks ) {
                if ( attack.target_index != object->index ) continue;

                const auto land_time      = attack.land_time + ( override_delay ? EXTRA_DURATION : 0.f );
                const auto next_land_time = attack.end_time + attack.attack_time;
                //const float third_land_time = attack.end_time + attack.total_cast_time + attack.attack_time;

                if ( land_time > time + *g_time ) continue;

                if ( land_time > *g_time && !attack.landed ) predicted_health -= attack.damage;

                if ( next_land_time > time + *g_time || !multiple_attacks ) continue;

                predicted_health -= attack.damage;
            }
        }

        if ( !ignore_turret && predicted_health > 0.f ) {
            for ( const auto& shot : m_turret_attacks ) {
                if ( shot.target_nid != object->network_id || object->is_hero( ) && !shot.missile_found ) continue;

                const auto land_time = shot.land_time;

                if ( land_time > time + *g_time ) continue;

                if ( land_time > *g_time && !shot.landed ) predicted_health -= shot.damage;

                if ( predicted_health <= 0.f ) break;
            }
        }

        if ( predicted_health > 0.f ) {
            for ( const auto& inst : m_hero_attacks ) {
                if ( inst.target_index != object->index || inst.type == EAttackType::missile && inst.server_cast_time
                    >= *g_time
                    || inst.land_time > time + *g_time || inst.land_time <= *g_time )
                    continue;

                predicted_health -= inst.damage;

                if ( predicted_health <= 0.f ) break;
            }
        }

        if ( predicted_health > 0.f ) {
            remove_invalid_special_attacks( );

            for ( const auto& inst : m_special_attacks ) {
                if ( inst.land_time > *g_time + time || inst.land_time <= *g_time || inst.has_condition ) continue;

                predicted_health -= inst.damage;

                if ( predicted_health <= 0.f ) break;
            }
        }

        return predicted_health;
    }

    auto Prediction::predict_minion_health(
        const int16_t index,
        const float   delay,
        const bool    predict_multiple_attacks
    ) -> float{
        update_existing_attacks( );

        auto object = g_entity_list.get_by_index( index );
        if ( !object ) return -1.f;

        object.update( );

        if ( !object ) return -1.f;

        auto calculated_health = object->health;

        for ( const auto attack : m_attacks ) {
            if ( attack.target_index != index ) continue;

            const auto land_time = attack.land_time;
            if ( land_time > *g_time + delay ) continue;

            if ( !attack.landed ) calculated_health -= attack.damage;

            const auto next_land_time = attack.end_time + attack.attack_time + EXTRA_DURATION;
            if ( next_land_time > *g_time + delay || !predict_multiple_attacks ) continue;

            calculated_health -= attack.damage;
        }

        if ( calculated_health <= 0.f ) return -1.f;

        for ( const auto shot : m_turret_attacks ) {
            if ( shot.target_nid != object->network_id ) continue;

            const auto land_time = shot.land_time;

            if ( land_time > *g_time + delay ) continue;

            if ( !shot.landed ) calculated_health -= shot.damage;

            if ( calculated_health <= 0.f ) break;
        }

        if ( calculated_health <= 0.f ) return -1.f;

        remove_invalid_special_attacks( );

        for ( const auto& inst : m_special_attacks ) {
            if ( inst.target_index != index || inst.land_time > *g_time + delay || inst.land_time <= *g_time + 0.05f ||
                inst.has_condition )
                continue;

            calculated_health -= inst.damage;

            if ( calculated_health <= 0.f ) break;
        }

        if ( calculated_health <= 0.f ) return -1.f;

        for ( const auto& inst : m_hero_attacks ) {
            if ( inst.target_index != index || inst.type == EAttackType::missile && inst.server_cast_time >= *g_time
                || inst.land_time > *g_time + delay || inst.land_time <= *g_time + 0.05f )
                continue;

            calculated_health -= inst.damage;

            if ( calculated_health <= 0.f ) break;
        }

        return calculated_health;
    }

    auto Prediction::predict_possible_minion_health(
        const int16_t index,
        const float   delay,
        const bool    predict_multiple_attacks
    ) const -> float{
        auto object = g_entity_list.get_by_index( index );
        if ( !object ) return -1.f;

        object.update( );

        if ( !object ) return -1.f;

        auto calculated_health = object->health;

        for ( const auto& attack : m_attacks ) {
            if ( attack.target_index != index ) continue;

            const auto land_time = attack.land_time;
            if ( land_time > *g_time + delay ) continue;

            if ( !attack.landed || land_time >= *g_time ) calculated_health -= attack.damage;

            const auto next_land_time = attack.end_time + attack.attack_time + EXTRA_DURATION;
            if ( next_land_time > *g_time + delay || !predict_multiple_attacks ) continue;

            calculated_health -= attack.damage;

            continue;

            const auto third_land_time = attack.end_time + attack.total_cast_time + attack.attack_time + EXTRA_DURATION;
            if (third_land_time > *g_time + delay || !predict_multiple_attacks) continue;

             calculated_health -= attack.damage;
        }

        if ( calculated_health <= 0.f ) return -1.f;

        for ( auto& shot : m_turret_attacks ) {
            if ( shot.target_nid != object->network_id ) continue;

            const auto land_time = shot.land_time;

            if ( land_time > *g_time + delay ) continue;

            if ( land_time > *g_time ) calculated_health -= shot.damage;

            if ( calculated_health <= 0.f ) break;
        }

        return calculated_health;
    }

    auto Prediction::update_hero_data( ) -> void{
        for ( const auto hero : g_entity_list.get_allies( ) ) {
            if ( !hero || hero->is_dead( ) || is_hero_tracked( hero->index ) || hero->network_id == g_local->
                network_id )
                continue;

            HeroData data{ hero->index, true };

            m_hero_data.push_back( data );
        }

        for ( const auto hero : g_entity_list.get_enemies( ) ) {
            if ( !hero || hero->is_dead( ) || is_hero_tracked( hero->index ) ) continue;

            HeroData data{ hero->index, false };

            m_hero_data.push_back( data );
        }

        for ( auto& inst : m_hero_data ) {
            if ( inst.ready ) continue;

            const auto& unit = g_entity_list.get_by_index( inst.index );
            if ( !unit || unit->is_dead( ) || unit->is_invisible( ) ) continue;

            if ( *g_time - inst.last_attack_time > 0.5f ) {
                if ( inst.tries > 10 ) {
                    inst.ready = true;
                    continue;
                }

                auto sci = unit->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack ) continue;

                inst.last_attack_time = sci->server_cast_time;
                inst.last_missile_nid = sci->missile_nid;
                inst.tries++;
            } else if ( *g_time >= inst.last_attack_time ) {
                auto missiles = inst.is_ally ? g_entity_list.get_ally_missiles( ) : g_entity_list.get_enemy_missiles( );
                for ( const auto missile : missiles ) {
                    if ( !missile || missile->network_id != inst.last_missile_nid ) continue;

                    auto info = missile->missile_spell_info( );
                    if ( !info ) continue;

                    auto data = info->get_spell_data( );
                    if ( !data ) continue;

                    inst.ready         = true;
                    inst.has_missile   = data->get_missile_speed( ) > 1000.f;
                    inst.missile_speed = inst.has_missile ? data->get_missile_speed( ) : 0.f;
                    //std::cout << "[ " << unit->champion_name.text << " ] AA Missile found! speed: " << inst.missile_speed << std::endl;

                    break;
                }
            }
        }
    }

    auto Prediction::update_hero_attacks( ) -> void{
        update_existing_hero_attacks( );

        for ( const auto hero : g_entity_list.get_enemies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ) continue;

            auto sci = hero->spell_book.get_spell_cast_info( );
            if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                sci->server_cast_time < *g_time || is_hero_attack_active( hero->index, sci->server_cast_time ) )
                continue;

            auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target ) continue;

            target.update( );

            HeroAttack instance{ };

            instance.source_index = hero->index;
            instance.target_index = sci->get_target_index( );

            const auto attack_name = sci->get_spell_name( );

            instance.damage = helper::get_champion_autoattack_damage(
                hero->index,
                target->index,
                sci->server_cast_time,
                instance.missile_speed <= 0.f,
                helper::get_champion_hero( rt_hash( hero->champion_name.text ) ) != EHeroes::zeri,
                attack_name.find( _( "Crit" ) ) != std::string::npos
            );

            instance.missile_speed = get_hero_missile_speed( hero->index );
            instance.type          = instance.missile_speed > 0.f ? EAttackType::missile : EAttackType::instant;
            instance.missile_nid   = sci->missile_nid;

            instance.is_teammate      = false;
            instance.start_time       = sci->start_time;
            instance.server_cast_time = sci->server_cast_time;
            instance.land_time        = sci->server_cast_time + ( instance.type == EAttackType::instant
                                                                      ? 0.f
                                                                      : hero->position.dist_to(
                                                                          target->position
                                                                      ) / instance.
                                                                      missile_speed ) + EXTRA_DURATION;
            instance.end_time = sci->end_time;

            m_hero_attacks.push_back( instance );
        }

        for ( const auto hero : g_entity_list.get_allies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->network_id == g_local->network_id ) continue;

            auto sci = hero->spell_book.get_spell_cast_info( );
            if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time <= *g_time ||
                is_hero_attack_active(
                    hero->index,
                    sci->server_cast_time
                ) )
                continue;

            auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target ) continue;

            target.update( );

            HeroAttack instance{ };

            instance.source_index = hero->index;
            instance.target_index = sci->get_target_index( );

            const auto attack_name = sci->get_spell_name( );

            instance.damage = helper::get_champion_autoattack_damage(
                hero->index,
                target->index,
                sci->server_cast_time + EXTRA_DURATION,
                instance.missile_speed <= 0.f,
                helper::get_champion_hero( rt_hash( hero->champion_name.text ) ) != EHeroes::zeri,
                attack_name.find( _( "Crit" ) ) != std::string::npos
            );

            instance.missile_speed = get_hero_missile_speed( hero->index );
            instance.type          = instance.missile_speed > 0.f ? EAttackType::missile : EAttackType::instant;
            instance.missile_nid   = sci->missile_nid;

            instance.is_teammate      = true;
            instance.start_time       = sci->start_time;
            instance.server_cast_time = sci->server_cast_time;
            instance.land_time        = sci->server_cast_time + ( instance.type == EAttackType::instant
                                                                      ? 0.f
                                                                      : hero->position.dist_to(
                                                                          target->position
                                                                      ) / instance.
                                                                      missile_speed ) + EXTRA_DURATION;
            instance.end_time = sci->end_time;

            m_hero_attacks.push_back( instance );
        }
    }

    auto Prediction::update_existing_hero_attacks( ) -> void{
        if ( *g_time == m_last_hero_attacks_update_time ) return;

        for ( auto& attack : m_hero_attacks ) {
            if ( attack.end_time <= *g_time ) {
                remove_hero_attack( attack.source_index, attack.server_cast_time );
                continue;
            }

            auto source = g_entity_list.get_by_index( attack.source_index );
            if ( !source ) {
                remove_hero_attack( attack.source_index, attack.server_cast_time );
                continue;
            }

            source.update( );

            auto sci = source->spell_book.get_spell_cast_info( );
            if ( !sci && *g_time < attack.server_cast_time || sci && sci->server_cast_time != attack.server_cast_time &&
                sci->start_time < attack.server_cast_time ) {
                remove_hero_attack( attack.source_index, attack.server_cast_time );
                continue;
            }

            if ( ( attack.type == EAttackType::instant || !attack.missile_found ) &&
                *g_time + g_features->orbwalker->get_ping( ) >= attack.land_time ) {
                remove_hero_attack( attack.source_index, attack.server_cast_time );
                continue;
            }

            if ( attack.type == EAttackType::missile && !attack.missile_found ) {
                if ( attack.is_teammate ) {
                    for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                        if ( !missile || missile->network_id != attack.missile_nid ) continue;

                        attack.missile_found = true;
                        attack.missile_index = missile->index;
                        break;
                    }
                } else {
                    for ( const auto missile : g_entity_list.get_enemy_missiles( ) ) {
                        if ( !missile || missile->network_id != attack.missile_nid ) continue;

                        attack.missile_found = true;
                        attack.missile_index = missile->index;
                        break;
                    }
                }
            } else if ( attack.missile_found ) {
                auto missile = g_entity_list.get_by_index( attack.missile_index );
                if ( !missile ) {
                    remove_hero_attack( attack.source_index, attack.server_cast_time );
                    continue;
                }

                const auto target = g_entity_list.get_by_index( attack.target_index );
                if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                    remove_hero_attack( attack.source_index, attack.server_cast_time );
                    continue;
                }

                missile.update( );

                if ( missile->position.dist_to( target->position ) <= 100.f ) {
                    remove_hero_attack( attack.source_index, attack.server_cast_time );
                    continue;
                }

                const auto travel_time = missile->position.dist_to( target->position ) / attack.missile_speed;
                if ( travel_time <= EXTRA_DURATION ) {
                    remove_hero_attack( attack.source_index, attack.server_cast_time );
                    continue;
                }

                attack.land_time = *g_time + travel_time + EXTRA_DURATION;
            }
        }

        m_last_hero_attacks_update_time = *g_time;
    }

    auto Prediction::update_champion_spells( ) -> void{
        update_existing_champion_spells( );

        for ( const auto hero : g_entity_list.get_allies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ) continue;

            auto sci = hero->spell_book.get_spell_cast_info( );
            if ( !sci || sci->is_autoattack || sci->is_special_attack || sci->server_cast_time < *g_time
                || is_champion_spell_active( hero->index, sci->slot, sci->server_cast_time ) )
                continue;

            const auto name      = sci->get_spell_name( );
            const auto name_hash = rt_hash( name.data( ) );
            const auto attribute = get_targetable_spell_attributes( name_hash );
            if ( !attribute ) continue;

            auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target ) continue;

            target.update( );

            HeroSpell instance{ };

            instance.source_index = hero->index;
            instance.target_index = target->index;

            instance.attributes    = attribute.value( );
            instance.type          = attribute->spell_type;
            instance.missile_speed = attribute->missile_speed;
            instance.missile_nid   = sci->missile_nid;
            instance.delayed_cast  = attribute->delayed_cast;

            instance.name      = name;
            instance.name_hash = name_hash;

            instance.start_time           = sci->start_time;
            instance.raw_server_cast_time = sci->server_cast_time;
            instance.server_cast_time     = instance.delayed_cast ? sci->end_time : sci->server_cast_time;

            switch ( instance.type ) {
            case EAttackType::missile:
                instance.land_time = instance.server_cast_time +
                    hero->position.dist_to( target->position ) / instance.missile_speed + EXTRA_DURATION;
                break;
            default:
                instance.land_time = instance.server_cast_time + EXTRA_DURATION;
                break;
            }

            instance.damage = attribute->get_damage(
                name_hash,
                instance.source_index,
                instance.target_index,
                instance.server_cast_time,
                instance.land_time - EXTRA_DURATION
            );
            instance.is_teammate = hero->team == g_local->team;
            instance.slot        = sci->slot;

            m_hero_spells.push_back( instance );
        }

        for ( const auto hero : g_entity_list.get_enemies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ) continue;

            auto sci = hero->spell_book.get_spell_cast_info( );
            if ( !sci || sci->is_autoattack || sci->is_special_attack || sci->server_cast_time < *g_time ||
                is_champion_spell_active( hero->index, sci->slot, sci->server_cast_time ) )
                continue;

            const auto name      = sci->get_spell_name( );
            const auto name_hash = rt_hash( name.data( ) );
            const auto attribute = get_targetable_spell_attributes( name_hash );
            if ( !attribute ) continue;

            auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target ) continue;

            target.update( );

            HeroSpell instance{ };

            instance.source_index = hero->index;
            instance.target_index = target->index;

            instance.attributes    = attribute.value( );
            instance.type          = attribute->spell_type;
            instance.missile_speed = attribute->missile_speed;
            instance.missile_nid   = sci->missile_nid;
            instance.delayed_cast  = attribute->delayed_cast;

            instance.name      = name;
            instance.name_hash = name_hash;

            instance.start_time           = sci->start_time;
            instance.raw_server_cast_time = sci->server_cast_time;
            instance.server_cast_time     = instance.delayed_cast ? sci->end_time : sci->server_cast_time;

            switch ( instance.type ) {
            case EAttackType::missile:
                instance.land_time = instance.server_cast_time +
                    hero->position.dist_to( target->position ) / instance.missile_speed + EXTRA_DURATION;
                break;
            default:
                instance.land_time = instance.server_cast_time + EXTRA_DURATION;
                break;
            }

            instance.damage = attribute->get_damage(
                name_hash,
                instance.source_index,
                instance.target_index,
                instance.server_cast_time,
                instance.land_time - EXTRA_DURATION
            );
            instance.is_teammate = hero->team == g_local->team;
            instance.slot        = sci->slot;

            m_hero_spells.push_back( instance );
        }
    }

    auto Prediction::update_existing_champion_spells( ) -> void{
        if ( *g_time == m_last_champion_spells_update_time ) return;

        for ( auto& spell : m_hero_spells ) {
            if ( spell.land_time <= *g_time + g_features->orbwalker->get_ping( ) ) {
                remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                continue;
            }

            auto source = g_entity_list.get_by_index( spell.source_index );
            if ( !source ) {
                remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                continue;
            }

            source.update( );

            auto sci = source->spell_book.get_spell_cast_info( );
            if ( !sci && *g_time < spell.raw_server_cast_time ||
                sci && ( sci->server_cast_time != spell.raw_server_cast_time || sci->slot != spell.slot ) && sci->
                start_time < spell.raw_server_cast_time ) {
                remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                continue;
            }

            if ( ( spell.type == EAttackType::instant || !spell.missile_found ) &&
                *g_time + g_features->orbwalker->get_ping( ) >= spell.land_time ) {
                remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                continue;
            }

            spell.damage = spell.attributes.get_damage(
                spell.name_hash,
                spell.source_index,
                spell.target_index,
                spell.server_cast_time,
                spell.land_time - EXTRA_DURATION
            );

            if ( spell.type == EAttackType::missile && spell.server_cast_time <= *g_time && !spell.missile_found ) {
                if ( spell.is_teammate ) {
                    for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                        if ( !missile || missile->network_id != spell.missile_nid ) continue;

                        spell.missile_found = true;
                        spell.missile_index = missile->index;
                        break;
                    }
                } else {
                    for ( const auto missile : g_entity_list.get_enemy_missiles( ) ) {
                        if ( !missile || missile->network_id != spell.missile_nid ) continue;

                        spell.missile_found = true;
                        spell.missile_index = missile->index;
                        break;
                    }
                }
            } else if ( spell.missile_found ) {
                auto missile = g_entity_list.get_by_index( spell.missile_index );
                if ( !missile ) {
                    remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                    continue;
                }

                const auto target = g_entity_list.get_by_index( spell.target_index );
                if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                    remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                    continue;
                }

                missile.update( );

                if ( missile->position.dist_to( target->position ) <= 150.f ) {
                    remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                    continue;
                }

                const auto travel_time = missile->position.dist_to( target->position ) / spell.missile_speed;
                if ( travel_time <= EXTRA_DURATION + g_features->orbwalker->get_ping( ) ) {
                    remove_champion_spell( spell.source_index, spell.slot, spell.raw_server_cast_time );
                    continue;
                }

                spell.land_time = *g_time + travel_time + EXTRA_DURATION;
            }
        }

        m_last_champion_spells_update_time = *g_time;
    }


    auto Prediction::update_minion_attacks( ) -> void{
        update_existing_attacks( );

        const auto minions = g_entity_list.get_ally_minions( );
        for ( const auto minion : minions ) {
            if ( !minion || g_local->position.dist_to( minion->position ) > 1500.f ||
                !minion->is_lane_minion( ) )
                continue;

            if ( minion->is_dead( ) ) {
                remove_attack( minion->index );
                continue;
            }

            auto sci = minion->spell_book.get_spell_cast_info( );

            if ( !sci ) {
                const auto attack = get_attack( minion->index );
                if ( !attack || attack->server_cast_time < *g_time ) continue;

                remove_attack( minion->index );
                continue;
            }

            if ( is_attack_active( minion->index ) ) {
                const auto attack = get_attack( minion->index );
                if ( attack && attack->server_cast_time != sci->server_cast_time ) remove_attack( minion->index );
                else if ( attack ) continue;
            }

            auto obj = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !obj ) continue;

            auto target{ obj.get( ) };

            if ( !target ) continue;

            AttackInstance attack{ };

            attack.source = minion;
            attack.target = target;

            attack.source_index = minion->index;
            attack.target_index = target->index;

            attack.start_time       = sci->start_time;
            attack.server_cast_time = sci->server_cast_time;
            attack.end_time         = sci->end_time;
            attack.windup_time      = sci->windup_time;
            attack.total_cast_time  = sci->total_cast_time;

            attack.damage        = minion->attack_damage( );
            attack.missile_speed = 0;
            attack.missile_nid   = 0;

            auto bounding_radius = 0.f; // minion->get_bounding_radius( );

            switch ( minion->get_minion_type( ) ) {
            case Object::EMinionType::melee:
                attack.attack_time = sci->windup_time;
                attack.land_time = sci->server_cast_time;
                attack.type      = Object::EMinionType::melee;
                break;
            case Object::EMinionType::ranged:
                attack.attack_time = sci->windup_time + minion->position.dist_to( target->position ) / RANGED_MISSILE_SPEED;
                attack.land_time = sci->server_cast_time + minion->position.dist_to( target->position ) / RANGED_MISSILE_SPEED + EXTRA_DURATION;
                attack.missile_speed = RANGED_MISSILE_SPEED;
                attack.type          = Object::EMinionType::ranged;
                attack.missile_nid   = sci->missile_nid;
                break;
            case Object::EMinionType::siege:
                attack.attack_time = sci->windup_time + minion->position.dist_to( target->position ) / SIEGE_MISSILE_SPEED;
                attack.land_time = sci->server_cast_time + minion->position.dist_to( target->position )  / SIEGE_MISSILE_SPEED + EXTRA_DURATION;
                attack.missile_speed = SIEGE_MISSILE_SPEED;
                attack.type          = Object::EMinionType::siege;
                attack.missile_nid   = sci->missile_nid;
                break;
            case Object::EMinionType::super:
                attack.attack_time = sci->windup_time + 0.2f;
                attack.land_time = sci->server_cast_time + 0.2f;
                attack.type      = Object::EMinionType::super;
                break;
            default:
                break;
            }

            if ( false && is_minion_attacking( minion->index, target->index ) ) {

                for ( auto inst : m_minions ) {

                    if (inst.index != minion->index) continue;

                    float simulated_time = sci->start_time < inst.second_attack_register_time
                        ? inst.second_attack_register_time
                        : inst.third_attack_register_time;

                    if (sci->start_time > inst.third_attack_register_time) break;

                    std::cout << "[ sim-attack diff: " << sci->server_cast_time - simulated_time << " ]\n";

                    break;
                }
                
            }

            m_attacks.push_back( attack );
        }

        for ( const auto turret : g_entity_list.get_ally_turrets( ) ) {
            if ( !turret || turret->is_dead( ) || turret->dist_to_local( ) > 1800.f ) continue;

            auto sci = turret->spell_book.get_spell_cast_info( );

            if ( !sci ) {
                remove_turret_shot( turret->network_id );
                continue;
            }

            if ( is_turret_attack_active( turret->network_id ) || sci->server_cast_time <= 1.f ) continue;

            auto& obj = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !obj ) continue;

            auto target{ obj.get( ) };

            if ( !target ) continue;

            TurretShot shot{ };

            shot.source       = turret;
            shot.target       = target;
            shot.target_index = target->index;

            shot.target_position = target->position;

            auto type = target->get_minion_type( );

            if ( type == Object::EMinionType::melee ) shot.damage = target->max_health * 0.45f;
            else if ( type == Object::EMinionType::ranged ) shot.damage = target->max_health * 0.7f;
            else if ( type == Object::EMinionType::siege ) shot.damage = target->max_health * 0.14f;
            else if ( type == Object::EMinionType::super ) shot.damage = target->max_health * 0.07f;
            else if ( target->is_hero( ) ) shot.damage = turret->attack_damage( );
            else continue;

            // turret missile speed is always 1200

            shot.server_cast_time = sci->server_cast_time;
            shot.end_time         = sci->end_time;
            shot.missile_nid      = sci->missile_nid;
            shot.land_time        = sci->server_cast_time + turret->position.dist_to( target->position ) / 1200.f;
            shot.windup_time      = sci->windup_time;
            shot.missile_speed    = 1200.f;
            shot.source_nid       = turret->network_id;
            shot.target_nid       = target->network_id;

            m_turret_attacks.push_back( shot );
        }

        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret || turret->is_dead( ) || turret->dist_to_local( ) > 1800.f ) continue;

            auto sci = turret->spell_book.get_spell_cast_info( );

            if ( !sci ) {
                remove_turret_shot( turret->network_id );
                continue;
            }

            if ( is_turret_attack_active( turret->network_id ) || sci->server_cast_time <= 1.f ) continue;

            auto& obj = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !obj ) continue;

            auto target{ obj.get( ) };

            if ( !target ) continue;

            TurretShot shot{ };

            shot.source       = turret;
            shot.target       = target;
            shot.target_index = target->index;

            shot.target_position = target->position;

            auto type = target->get_minion_type( );

            if ( type == Object::EMinionType::melee ) shot.damage = target->max_health * 0.45f;
            else if ( type == Object::EMinionType::ranged ) shot.damage = target->max_health * 0.7f;
            else if ( type == Object::EMinionType::siege ) shot.damage = target->max_health * 0.14f;
            else if ( type == Object::EMinionType::super ) shot.damage = target->max_health * 0.07f;
            else if ( target->is_hero( ) ) shot.damage = turret->attack_damage( );
            else continue;

            // turret missile speed is always 1200

            shot.server_cast_time = sci->server_cast_time;
            shot.end_time         = sci->end_time;
            shot.missile_nid      = sci->missile_nid;
            shot.land_time        = sci->server_cast_time + turret->position.dist_to( target->position ) / 1200.f + EXTRA_DURATION;
            shot.windup_time      = sci->windup_time;
            shot.missile_speed    = 1200.f;
            shot.source_nid       = turret->network_id;
            shot.target_nid       = target->network_id;

            m_turret_attacks.push_back( shot );
        }

        update_ignored_missiles( );
    }

    auto Prediction::update_special_attacks( ) -> void{
        remove_invalid_special_attacks( );

        for ( auto& inst : m_special_attacks ) {
            if ( !inst.has_condition ) {
                if ( inst.land_time <= *g_time ) {
                    //g_features->tracker->add_text_popup(g_local->index, "Q Attack ended", color(76, 196, 230));

                    // std::cout << "Removed Q special attack " << std::endl;
                    remove_special_attack( inst.target_index, inst.damage, inst.creation_time );
                }

                continue;
            }

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci || sci->start_time < inst.creation_time ) continue;

            if ( sci->slot != static_cast< int >( inst.condition_slot ) ) {
                //std::cout << "Removed conditional SAttack due to invalid slot\n";
                remove_special_attack( inst.target_index, inst.damage, inst.creation_time );
                continue;
            }

            const auto& minion = g_entity_list.get_by_index( inst.target_index );
            if ( !minion ) continue;

            const auto minion_radius = minion->get_bounding_radius( );
            const auto travel_time   = ( minion->dist_to_local( ) - minion_radius ) / 2600.f;

            inst.has_condition = false;
            //inst.damage = inst.damage / 7.f * 6.f;
            inst.land_time = sci->start_time + travel_time;
        }
    }

    auto Prediction::update_minions() -> void {

        for (auto minion : g_entity_list.get_ally_minions()) {

            if (!minion || minion->dist_to_local() > 2000.f || minion->is_dead() ||
                is_minion_instance_active(minion->network_id) || !minion->is_lane_minion())
                continue;

            m_minions.push_back( MinionAI{ minion, minion->index, minion->network_id,           minion->get_minion_type(), *g_time,
                      500.f,  false,         MinionTargetingPriority::None } );

        }

        for (auto& inst : m_minions) {

            auto unit = g_entity_list.get_by_index(inst.index);
            if (!unit || unit->is_dead())
            {
                remove_minion_instance(inst.network_id);
                continue;
            }

            auto aimgr = unit->get_ai_manager();
            if (aimgr && aimgr->is_moving && unit->position.dist_to(aimgr->path_end) > 50.f)
            {

                if (inst.state == MinionState::Moving) continue;

                inst.state = MinionState::Moving;
                inst.state_change_time = *g_time;
                /*if (inst.last_target_index > 0)
                {
                    auto target = g_entity_list.get_by_index(inst.last_target_index);
                    if (target && target->is_alive())
                        std::cout << "Target priority swapped, duration: " << *g_time - inst.last_targeting_time << std::endl;
                }*/

                continue;
            }

            auto sci = unit->spell_book.get_spell_cast_info();
            if (sci) {

                const auto target_index = sci->get_target_index( );
                if (target_index == inst.last_target_index && inst.state == MinionState::TargetingUnit) continue;

                auto target = g_entity_list.get_by_index(target_index);
                if (!target || target->is_dead()) continue;

                auto attack = get_attack(inst.index);
                if (!attack) continue;

                 if (inst.state != MinionState::TargetingUnit) inst.state_change_time = *g_time;

                inst.last_targeting_time = sci->start_time;
                inst.last_target_index   = target_index;
                inst.state               = MinionState::TargetingUnit;
                inst.targeting_position  = unit->position;

                inst.attack_register_time = attack->server_cast_time;
                inst.second_attack_register_time = attack->end_time + attack->windup_time;
                inst.third_attack_register_time  = attack->end_time + attack->total_cast_time + attack->windup_time;

                //std::cout << "Updated state for " << unit->get_name() << " | atkcastdelay: " << sci->windup_time << " | atkdelay: " << sci->total_cast_time
                //          << std::endl;
            }
            else {

                inst.state = MinionState::Idle;
                //inst.state_change_time = *g_time;

            }
           
        }
    }


    auto Prediction::remove_invalid_special_attacks( ) -> void{
        if ( m_special_attacks.empty( ) ) return;

        const auto to_remove = std::ranges::remove_if(
            m_special_attacks,
            [&]( const SpecialAttackInstance& instance ) -> bool{
                return instance.land_time <= *g_time && ( !instance.has_condition || *g_time - instance.creation_time >
                    0.5f );
            }
        );

        if ( to_remove.empty( ) ) return;

        m_special_attacks.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Prediction::remove_invalid_paths( ) -> void{
        for ( auto& inst : m_prediction_data ) {
            const auto to_remove = std::ranges::remove_if(
                inst.paths,
                [&]( const float& path_time ) -> bool{ return *g_time - path_time > 120.f; }
            );

            if ( to_remove.empty( ) ) return;

            inst.paths.erase( to_remove.begin( ), to_remove.end( ) );
        }
    }

    auto Prediction::remove_special_attack(
        const int16_t target_index,
        const float   damage,
        const float   creation_time
    ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_special_attacks,
            [&]( const SpecialAttackInstance& instance ) -> bool{
                return instance.target_index == target_index && instance.damage - damage == 0.f && instance.
                    creation_time - creation_time == 0.f;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_special_attacks.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Prediction::is_minion_instance_active(unsigned network_id) -> bool {

        for (auto inst : m_minions)
        {
            if (inst.network_id != network_id) continue;

            return true;
        }

        return false;
    }


    auto Prediction::remove_minion_instance(unsigned network_id) -> void
    {
        const auto to_remove = std::ranges::remove_if(m_minions,
                                                      [&](const MinionAI &instance) -> bool
                                                      {
                                                          return instance.network_id == network_id;
                                                      });

        if (to_remove.empty()) return;

        m_minions.erase(to_remove.begin(), to_remove.end());
    }


    auto Prediction::is_minion_in_danger( const int16_t index ) const -> bool{
        if ( get_attack_count( index ) > 0 ) return true;

        auto& unit = g_entity_list.get_by_index( index );
        if ( !unit ) return false;
        unit.update( );

        const auto bounding_radius = unit->get_bounding_radius( );

        for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
            if ( !minion || minion->is_dead( ) || !minion->is_lane_minion( ) || minion->position.dist_to(
                unit->position
            ) > 1000.f )
                continue;

            auto sci = minion->spell_book.get_spell_cast_info( );
            if ( sci ) {
                const auto target_index = sci->get_target_index( );
                if ( target_index != index ) continue;

                return true;
            }

            const auto minion_radius = minion->get_bounding_radius( );

            // is in ally minions attack range
            if ( unit->position.dist_to( minion->position ) < minion->attack_range + minion_radius +
                bounding_radius )
                return true;
        }

        return false;
    }


    /* New minion in line code, beautiful, sexy, fast, efficient */
    auto Prediction::minion_in_line(
        const Vec3&    start_pos,
        const Vec3&    end_pos,
        const float    projectile_width,
        const unsigned ignored_network_id,
        const float    damage_override
    ) -> bool{
        // Function is used in LUA, message @tore if you change args
        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj ) continue;

            if ( obj->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) ||
                obj->is_dead( ) ||
                obj->is_invisible( ) ||
                !obj->is_normal_minion( ) ||
                obj->is_minion_only_autoattackable( ) ||
                obj->network_id == ignored_network_id ||
                obj->health <= damage_override )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );

            auto rect    = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( minion_radius ) );

            // make a box around the minion
            // see if they overlap

            if ( polygon.is_inside( obj->position ) ) return true;

            auto pred = g_features->prediction->predict_default( obj->index, 0.25f );
            if ( !pred ) continue;

            if ( polygon.is_inside( *pred ) ) return true;
        }

        return false;
    }

    auto Prediction::turret_in_line(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float projectile_width
    ) -> bool{
        for ( const auto turret : g_entity_list.get_enemy_turrets( ) ) {
            if ( !turret || turret->is_dead( ) || turret->position.dist_to( start_pos ) > start_pos.
                dist_to( end_pos ) )
                continue;

            constexpr auto turret_radius = 88.4f;

            auto rect    = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( turret_radius ) );

            // make a box around the turret
            // see if they overlap

            if ( polygon.is_inside( turret->position ) ) return true;
        }

        return false;
    }

    auto Prediction::minion_in_line_predicted(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float projectile_width,
        const float cast_time,
        const float missile_speed
    ) const -> bool{
        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj ) continue;

            if ( obj->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) * 1.3f || obj->is_dead( ) ||
                obj->is_invisible( ) || !obj->is_normal_minion( ) || obj->is_minion_only_autoattackable( ) )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );

            auto rect    = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( minion_radius ) );

            // make a box around the minion
            // see if they overlap


            for ( auto i = 0; i <= 8; i++ ) {
                const auto time_traveled = i == 0 ? 0.f : start_pos.dist_to( end_pos ) / missile_speed / 8.f * i;

                auto pred = predict_default( obj->index, cast_time + time_traveled );
                if ( !pred || obj->position.dist_to( *pred ) <= 1.f ) {
                    if ( polygon.is_inside( obj->position ) ) return true;
                    break;
                }

                if ( polygon.is_inside( *pred ) ) return true;
            }
        }

        return false;
    }

    auto Prediction::get_skillshot_collision_position(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float projectile_width,
        const bool  edge_range,
        const float cast_time,
        const float missile_speed
    ) -> std::optional< Vec3 >{
        if ( !g_config->evade.minion_collision->get< bool >( ) && !g_config->evade.champion_collision->get<
            bool >( ) )
            return std::nullopt;

        const auto minion_collision = get_minion_collision_position(
            start_pos,
            end_pos,
            projectile_width,
            edge_range,
            cast_time,
            missile_speed
        );
        if ( minion_collision.has_value( ) ) return std::make_optional( minion_collision.value( ) );

        const auto champion_collision = get_champion_collision_position(
            start_pos,
            end_pos,
            projectile_width,
            edge_range,
            cast_time,
            missile_speed
        );
        return champion_collision.has_value( ) ? std::make_optional( champion_collision.value( ) ) : std::nullopt;
    }

    auto Prediction::get_minion_collision_position(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float projectile_width,
        const bool  edge_range,
        const float cast_time,
        const float missile_speed
    ) -> std::optional< Vec3 >{
        if ( !g_config->evade.minion_collision->get< bool >( ) ) return std::nullopt;

        Vec3 nearest_collision{ };
        bool found_collision{ };

        const auto hitbox_start = cast_time > 0.f ? start_pos : start_pos.extend( end_pos, -( missile_speed * 0.1f ) );

        for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
            if ( !obj || obj->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) * 1.2f || obj->is_dead( ) ||
                obj->is_invisible( ) || !obj->is_lane_minion( ) )
                continue;

            const auto bounding_radius = edge_range ? obj->get_bounding_radius( ) : 0;

            auto rect    = sdk::math::Rectangle( hitbox_start, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( bounding_radius ) );

            if ( polygon.is_outside( obj->position ) ) continue;

            auto       closest = g_features->evade->get_closest_line_point( start_pos, end_pos, obj->position );
            const auto adjusted_collision = closest.extend( hitbox_start, bounding_radius );

            if ( found_collision && closest.dist_to( start_pos ) > nearest_collision.dist_to( start_pos ) ) continue;

            auto pred = g_features->prediction->predict_default(
                obj->index,
                cast_time + start_pos.dist_to( obj->position ) / missile_speed,
                false
            );
            if ( !pred ) continue;

            closest = g_features->evade->get_closest_line_point( start_pos, end_pos, *pred );

            const auto travel_time = cast_time + start_pos.dist_to( closest ) / missile_speed;
            pred                   = g_features->prediction->predict_default( obj->index, travel_time, false );
            if ( !pred || polygon.is_outside( *pred ) ) continue;

            nearest_collision = adjusted_collision;
            found_collision   = true;
        }

        return found_collision ? std::make_optional( nearest_collision ) : std::nullopt;
    }

    auto Prediction::get_champion_collision_position(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float projectile_width,
        const bool  edge_range,
        const float cast_time,
        const float missile_speed
    ) -> std::optional< Vec3 >{
        if ( !g_config->evade.champion_collision->get< bool >( ) ) return std::nullopt;

        Vec3 nearest_collision{ };
        bool found_collision{ };

        const auto hitbox_start = cast_time > 0.f ? start_pos : start_pos.extend( end_pos, -( missile_speed * 0.1f ) );

        for ( const auto obj : g_entity_list.get_allies( ) ) {
            if ( !obj || obj->network_id == g_local->network_id || obj->position.dist_to( start_pos ) > start_pos.
                dist_to( end_pos ) * 1.2f ||
                g_features->target_selector->is_bad_target( obj->index ) )
                continue;

            const auto bounding_radius = edge_range ? obj->get_bounding_radius( ) : 0;

            auto rect    = sdk::math::Rectangle( hitbox_start, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( bounding_radius ) );

            if ( polygon.is_outside( obj->position ) ) continue;

            auto closest            = g_features->evade->get_closest_line_point( start_pos, end_pos, obj->position );
            auto adjusted_collision = closest;

            if ( found_collision && adjusted_collision.dist_to( start_pos ) > nearest_collision.
                dist_to( start_pos ) )
                continue;

            auto pred = g_features->prediction->predict_default(
                obj->index,
                cast_time + start_pos.dist_to( closest ) / missile_speed
            );
            if ( !pred || polygon.is_outside( *pred ) ) continue;

            nearest_collision = adjusted_collision;
            found_collision   = true;
        }

        return found_collision ? std::make_optional( nearest_collision ) : std::nullopt;
    }



    auto Prediction::champion_in_line(
        const Vec3&    start_pos,
        const Vec3&    end_pos,
        const float    projectile_width,
        const bool     edge_range,
        const float    cast_time,
        const float    missile_speed,
        const unsigned ignored_network_id
    ) const -> bool{
        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) * 1.1f ||
                enemy->network_id == ignored_network_id ||
                g_features->target_selector->is_bad_target( enemy->index ) )
                continue;

            const auto minion_radius = edge_range ? get_champion_radius( rt_hash( enemy->champion_name.text ) ) : 0.f;

            auto rect    = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( minion_radius ) );

            for ( auto i = 0; i <= 8; i++ ) {
                const auto time_traveled = i == 0 ? 0.f : start_pos.dist_to( end_pos ) / missile_speed / 8.f * i;

                auto pred = predict_default( enemy->index, cast_time + time_traveled, false );
                if ( !pred || enemy->position.dist_to( *pred ) <= 1.f ) {
                    if ( polygon.is_inside( enemy->position ) ) return true;
                    break;
                }

                if ( polygon.is_inside( *pred ) ) return true;
            }
        }

        return false;
    }

    auto Prediction::minion_in_circle_predicted(
        const Vec3& center,
        const float radius,
        const float traveltime,
        const bool  edge_range
    ) const -> bool{
        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj || obj->is_dead( ) || obj->is_invisible( ) ||
                !obj->is_normal_minion( ) || obj->is_minion_only_autoattackable( ) )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );

            auto circle  = sdk::math::Circle( center, radius );
            auto polygon = circle.to_polygon( static_cast< int32_t >( edge_range ? minion_radius : 0 ) );

            // make a box around the minion
            // see if they overlap

            auto pred = predict_default( obj->index, traveltime );
            if ( !pred || obj->position.dist_to( *pred ) <= 5.f ) {
                if ( polygon.is_inside( obj->position ) ) return true;

                continue;
            }

            if ( polygon.is_inside( *pred ) ) return true;
        }

        return false;
    }

    auto Prediction::minion_in_circle(
        const Vec3&    center,
        const float    radius,
        const unsigned ignored_network_id
    ) const -> bool{
        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->position.dist_to( center ) > radius ||
                !obj->is_normal_minion( ) || obj->is_minion_only_autoattackable( ) ||
                obj->network_id == ignored_network_id )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );

            auto circle  = sdk::math::Circle( center, radius );
            auto polygon = circle.to_polygon( static_cast< int32_t >( minion_radius ) );

            // make a box around the minion
            // see if they overlap

            if ( polygon.is_inside( obj->position ) ) return true;

            auto pred = predict_default( obj->index, 0.25f );
            if ( !pred ) continue;

            if ( polygon.is_inside( *pred ) ) return true;
        }

        return false;
    }


    auto Prediction::windwall_in_line( const Vec3& start_pos, const Vec3& end_pos ) const -> bool{
        if ( !m_windwall.is_active ) return false;

        auto& begin = g_entity_list.get_by_index( m_windwall.start_index );
        auto& end   = g_entity_list.get_by_index( m_windwall.end_index );
        if ( !begin || !end ) return false;

        begin.update( );
        end.update( );

        if ( begin->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) + 150.f ||
            end->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) + 150.f )
            return false;

        const auto rectangle = sdk::math::Rectangle( begin->position, end->position, 70.f );
        auto       poly      = rectangle.to_polygon( 0 );

        const auto loop_count = static_cast< int32_t >( std::ceil( start_pos.dist_to( end_pos ) / 60.f ) );
        Vec3       test_position{ };
        for ( auto i = 0; i < loop_count; i++ ) {
            const auto extend_amount = std::min( 60.f * static_cast< float >( i ), start_pos.dist_to( end_pos ) );
            test_position            = start_pos.extend( end_pos, extend_amount );

            if ( poly.is_inside( test_position ) ) return true;
        }

        return false;
    }

    auto Prediction::update_windwall( ) -> void{
        if ( m_windwall.is_active ) {
            const auto& wall_start = g_entity_list.get_by_index( m_windwall.start_index );
            const auto& wall_end   = g_entity_list.get_by_index( m_windwall.end_index );
            if ( !wall_start || !wall_end || *g_time > m_windwall.end_time ) m_windwall.is_active = false;
        } else {
            int16_t first{ }, second{ };
            bool    found{ };
            float   spawn_time{ };

            for ( const auto missile : g_entity_list.get_enemy_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 5000.f || !missile->is_windwall( ) ) continue;

                spawn_time = missile->missile_spawn_time( );
                if ( *g_time - spawn_time > 3.9f ) continue;

                if ( first == 0 ) first = missile->index;
                else {
                    second = missile->index;
                    found  = true;
                    break;
                }
            }

            if ( !found ) return;

            m_windwall.is_active   = true;
            m_windwall.start_index = first;
            m_windwall.end_index   = second;
            m_windwall.end_time    = spawn_time + 3.9f;
        }
    }



    auto Prediction::count_minions_in_line(
        const Vec3&    start_pos,
        const Vec3&    end_pos,
        const float    projectile_width,
        const unsigned ignored_network_id
    ) -> int32_t{
        // Function is used in LUA, message @tore if you change args
        int count{ };
        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj ) continue;

            if ( obj->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) ||
                obj->is_dead( ) ||
                obj->is_invisible( ) ||
                !obj->is_lane_minion( ) && !obj->is_jungle_monster( ) ||
                obj->network_id == ignored_network_id )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );

            auto rect    = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto polygon = rect.to_polygon( static_cast< int32_t >( minion_radius ) );

            if ( polygon.is_inside( obj->position ) ) count++;
        }

        return count;
    }

    auto Prediction::minions_in_line(
        const Vec3&    start_pos,
        const Vec3&    end_pos,
        const float    projectile_width,
        const unsigned ignored_network_id,
        const float    cast_delay,
        const float    travel_time
    ) const -> int{
        int count{ };

        for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj || obj->position.dist_to( start_pos ) > start_pos.dist_to( end_pos ) * 1.25f || obj->is_dead( ) ||
                obj->is_invisible( ) || obj->network_id == ignored_network_id || !obj->is_normal_minion( ) ||
                obj->is_minion_only_autoattackable( ) )
                continue;

            const auto minion_radius = obj->get_bounding_radius( );
            auto       rect          = sdk::math::Rectangle( start_pos, end_pos, projectile_width );
            auto       polygon       = rect.to_polygon( static_cast< int32_t >( minion_radius ) );

            for ( auto i = 0; i <= 5; i++ ) {
                const auto predict_amount = ( travel_time - cast_delay ) / 5.f * i;

                auto pred = predict_default( obj->index, cast_delay + predict_amount, false );
                if ( !pred || obj->position.dist_to( *pred ) <= 1.f ) {
                    if ( polygon.is_inside( obj->position ) ) ++count;

                    break;
                }

                if ( polygon.is_inside( *pred ) ) {
                    ++count;
                    break;
                }

                if ( travel_time <= 0.f ) break;
            }
        }

        return count;
    }

    auto Prediction::remove_attack( int16_t index ) -> void{
        const auto to_remove =
            std::ranges::remove_if(
                m_attacks,
                [index]( const AttackInstance& it ) -> bool{ return it.source_index == index; }
            ).begin( );

        if ( to_remove == m_attacks.end( ) ) return;

        m_attacks.erase( to_remove );
    }

    auto Prediction::remove_hero_attack( const int16_t index, const float server_cast_time ) -> void{
        const auto to_remove =
            std::ranges::remove_if(
                m_hero_attacks,
                [&]( const HeroAttack& it ) -> bool{
                    return it.source_index == index && it.server_cast_time == server_cast_time;
                }
            ).begin( );

        if ( to_remove == m_hero_attacks.end( ) ) return;

        m_hero_attacks.erase( to_remove );
    }

    auto Prediction::remove_champion_spell( const int16_t index, const int slot, const float server_cast_time ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_hero_spells,
            [ & ]( const HeroSpell& it ) -> bool{
                return it.source_index == index && it.slot == slot && it.raw_server_cast_time == server_cast_time;
            }
        ).begin( );

        if ( to_remove == m_hero_spells.end( ) ) return;

        m_hero_spells.erase( to_remove );
    }

    auto Prediction::remove_turret_shot( unsigned network_id ) -> void{
        const auto to_remove =
            std::ranges::remove_if(
                m_turret_attacks,
                [network_id]( const TurretShot& it ) -> bool{ return it.source_nid == network_id; }
            ).begin( );

        if ( to_remove == m_turret_attacks.end( ) ) return;

        m_turret_attacks.erase( to_remove );
    }

    auto Prediction::is_turret_attack_active( const unsigned network_id ) const -> bool{
        for ( const auto& attack : m_turret_attacks ) if ( attack.source_nid == network_id ) return true;

        return false;
    }


    auto Prediction::update_existing_attacks( ) -> void{
        //if ( *g_time - m_last_attacks_update_time <= 0.01f ) return;

        //m_extra_duration = 0.08f;

        // update turret attacks
        for ( auto& attack : m_turret_attacks ) {
            if ( attack.end_time <= *g_time ) {
                remove_turret_shot( attack.source_nid );
                continue;
            }

            if ( !attack.missile_found && attack.missile_nid > 0 ) {
                for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile || missile->network_id != attack.missile_nid ) continue;

                    attack.missile_found = true;
                    attack.missile_index = missile->index;
                    //std::cout << "Missile distance to turret: "
                    //          << missile->missile_start_position.dist_to(attack.source->position) << std::endl;
                    break;
                }
            } else if ( attack.missile_found && !attack.landed ) {
                auto& missile = g_entity_list.get_by_index( attack.missile_index );
                if ( !missile ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.land_time = *g_time;
                    attack.landed    = true;
                    // std::cout << "attack has ended " << *g_time << "\n";
                    continue;
                }

                missile.update( );

                if ( !missile ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.land_time = *g_time;
                    attack.landed    = true;
                    // std::cout << "attack has ended " << *g_time << "\n";
                    continue;
                }

                // auto missile = obj->create_updated_copy( );
                auto& target = g_entity_list.get_by_index( attack.target_index );
                if ( !target ) continue;

                target.update( );

                if ( !target ) continue;

                auto travel_time = missile->position.dist_to( target->position ) / attack.missile_speed;
                if ( target->is_hero( ) ) {
                    auto pred = g_features->prediction->predict_default( target->index, travel_time );
                    if ( pred ) travel_time = missile->position.dist_to( *pred ) / attack.missile_speed;
                }


                if ( missile->position.dist_to( target->position ) <= 120.f ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.landed    = true;
                    attack.land_time = *g_time;
                } else attack.land_time = *g_time + travel_time;
            }
        }

        for ( auto& attack : m_attacks ) {
            if ( attack.end_time + 0.15f < *g_time ) {
                remove_attack( attack.source_index );
                continue;
            }

            if ( !attack.missile_found && attack.missile_nid > 0 ) {
                for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile || missile->network_id != attack.missile_nid ) continue;

                    attack.missile_found = true;
                    attack.missile_index = missile->index;
                    break;
                }
            }
            else if ( attack.missile_found && !attack.landed ) {
                auto missile = g_entity_list.get_by_index( attack.missile_index );
                if ( !missile ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.land_time = *g_time;
                    attack.landed    = true;
                    // std::cout << "attack has ended " << *g_time << "\n";
                    continue;
                }

                missile.update( );

                if ( !missile ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.land_time = *g_time;
                    attack.landed    = true;
                    // std::cout << "attack has ended " << *g_time << "\n";
                    continue;
                }

                // auto missile = obj->create_updated_copy( );
                auto target = g_entity_list.get_by_index( attack.target_index );
                if ( !target ) continue;

                auto source = g_entity_list.get_by_index( attack.source_index );
                if ( !source ) continue;

                source.update( );
                target.update( );

                if ( !target || !source ) continue;

                const auto travel_time = missile->position.dist_to( target->position ) / attack.missile_speed;

                attack.missile_last_position = missile->position;

                const auto missile_threshold = attack.type == Object::EMinionType::ranged ? 80.f : 120.f;

                if ( missile->position.dist_to( target->position ) <= missile_threshold ) {
                    m_ignored_missiles.push_back( { attack.missile_nid, *g_time } );
                    attack.landed = true;
                } else attack.land_time = *g_time + travel_time + EXTRA_DURATION;

                continue;
            }

            if ( !attack.landed && attack.land_time < *g_time + 0.075f ) {
                //attack.land_time = *g_time;
                attack.landed = true;
            }
        }

        //m_last_attacks_update_time = *g_time;
    }


    auto Prediction::get_attack( const int16_t index ) -> Prediction::AttackInstance*{
        for ( auto& attack : m_attacks ) if ( attack.source_index == index ) return &attack;

        return nullptr;
    }



    auto Prediction::is_attack_active( const int16_t index ) const -> bool{
        for ( const auto& attack : m_attacks ) if ( attack.source_index == index ) return true;

        return false;
    }

    auto Prediction::get_incoming_champion_damage(
        const int16_t target_index,
        const float   time,
        const float   strict_calculation
    ) -> float{
        update_existing_hero_attacks( );
        update_existing_champion_spells( );

        float total_damage{ };

        for ( const auto attack : m_hero_attacks ) {
            if ( attack.target_index != target_index || strict_calculation && attack.land_time > *g_time +
                time )
                continue;

            total_damage += attack.damage;
        }

        for ( const auto spell : m_hero_spells ) {
            if ( spell.target_index != target_index || strict_calculation && spell.land_time > *g_time + time ) {
                continue
                    ;
            }

            total_damage += spell.damage;
        }

        for ( const auto instance : m_special_attacks ) {
            if ( !instance.is_autoattack || instance.land_time <= *g_time || instance.target_index !=
                target_index )
                continue;

            total_damage += instance.damage;
        }

        return total_damage;
    }


    auto Prediction::get_incoming_attack_count( const int16_t target_index ) const -> int{
        int count{ };

        for ( const auto instance : m_special_attacks ) {
            if ( !instance.is_autoattack || instance.land_time <= *g_time || instance.target_index !=
                target_index )
                continue;

            ++count;
        }

        return count;
    }

    auto Prediction::get_minion_tick_damage(int16_t index) -> float {
        float total_damage{};

        for (auto inst : m_attacks)
        {
            if (inst.target_index != index) continue;

            total_damage += inst.damage;
        }

        for (auto inst : m_turret_attacks)
        {
            if (inst.target_index != index) continue;

            total_damage += inst.damage;
        }

        return total_damage;
    }

    auto Prediction::should_damage_minion(int16_t target_index, float attack_damage, float attack_cast_delay, float attack_delay) -> bool {

        std::vector<ConstantDamage> damage_list{ };

        damage_list.push_back({ attack_damage, attack_cast_delay, attack_delay, 0.f, true });

        for (auto inst : m_attacks)
        {
            if (inst.target_index != target_index) continue;

            damage_list.push_back({ inst.damage, inst.attack_time, inst.total_cast_time, inst.land_time > *g_time ? 0.f : inst.end_time - *g_time, false });
        }

        for (auto inst : m_turret_attacks)
        {
            if (inst.target_index != target_index) continue;

            damage_list.push_back(
                { inst.damage, inst.windup_time + inst.source->position.dist_to(inst.target_position) / 1200.f,
                  inst.end_time - ( inst.server_cast_time - inst.windup_time ), inst.land_time > *g_time ? 0.f : inst.end_time - *g_time, false });
        }

        ConstantDamage root_damage{ };
        float          shortest_delay{ 999.f };

        for (auto inst : damage_list) {

            if (inst.attack_delay < shortest_delay)
            {
                shortest_delay = inst.attack_delay;
                root_damage    = inst;
            }
        }

        auto target = g_entity_list.get_by_index(target_index);
        if (!target || target->is_dead()) return false;

        const int tick_count         = static_cast<int>( std::ceil( target->health / root_damage.damage ) );

        float time_passed{ };

        for (int i = 0; i <= tick_count; i++) {

            time_passed += i == 0 ? root_damage.cast_delay : root_damage.attack_delay;

            float simulated_health = target->health;

            for ( auto inst : damage_list ) {

                const auto damage = inst.compile_damage(time_passed);

                simulated_health -= damage;

                if (simulated_health <= 0.f) {

                    return inst.needs_lasthit;
                }
            }
        }

        return false;
    }


    auto Prediction::simulate_minion_damage(int16_t target_index, float duration) -> float {

        float total_damage{ };

        auto target = g_entity_list.get_by_index(target_index);
        if (!target) return 0.f;

        for (auto inst : m_minions)
        {
            if (!inst.unit || inst.unit->is_dead() 
                || inst.state == MinionState::TargetingUnit && !g_features->orbwalker->is_ignored( inst.last_target_index ) ) continue;

            Vec3 source_position{ inst.unit->position };

            auto pred = g_features->prediction->predict_default(inst.index, 0.15f);
            if (pred) source_position = *pred;

            if (source_position.dist_to(target->position) > inst.acquisition_radius) continue;

            float distance = inst.unit->position.dist_to(target->position);
            float attack_range = inst.unit->attack_range + inst.unit->get_bounding_radius() + target->get_bounding_radius();

            float time_to_tick{ };

            if ( distance > attack_range ) {

                float delta = distance - attack_range;
                time_to_tick = delta / inst.unit->movement_speed;
            }

            float attack_time = inst.get_attack_cast_delay( );
            switch ( inst.type ) {
            case Object::EMinionType::ranged:
                attack_time += inst.unit->position.dist_to(target->position) / RANGED_MISSILE_SPEED;
                break;
            case Object::EMinionType::siege:
                attack_time += inst.unit->position.dist_to(target->position) / SIEGE_MISSILE_SPEED;
                break;
            default:
                break;
            }

            if (time_to_tick + attack_time >= duration) continue;

            total_damage += inst.unit->attack_damage();

            if (time_to_tick + inst.get_attack_delay() + attack_time > duration) continue;

            total_damage += inst.unit->attack_damage();
        }

        for (const auto attack : m_attacks) {
            if (attack.target_index != target_index) continue;

            const auto land_time = attack.land_time;
            if (land_time > *g_time + duration ) continue;

            if ( !attack.landed || land_time + EXTRA_DURATION > *g_time ) total_damage += attack.damage;

            const auto next_land_time = attack.end_time + attack.attack_time;
            if (next_land_time > *g_time + duration ) continue;

            total_damage += attack.damage;
        }

        for (const auto shot : m_turret_attacks)
        {
            if (shot.target_nid != target->network_id || target->is_hero() && !shot.missile_found) continue;

            const auto land_time = shot.land_time;

            if (land_time > duration + *g_time) continue;

            if (land_time > *g_time || !shot.landed) total_damage += shot.damage;

            auto next_shot_time =
                shot.end_time + shot.windup_time + shot.source->position.dist_to(target->position) / 1200.f;
            if (next_shot_time > *g_time + duration) continue;

            total_damage += shot.damage;
        }

        return total_damage;
    }


    auto Prediction::get_attack_count( const int16_t index ) const -> int32_t{
        int count{ };
        for ( const auto& attack : m_attacks ) if ( attack.target_index == index ) count++;

        return count;
    }

    auto Prediction::get_data_index( const unsigned network_id ) -> int{
        for ( size_t i = 0u; i < m_prediction_data.size( ); i++ ) {
            if ( m_prediction_data[ i ].network_id ==
                network_id )
                return i;
        }

        bool found_enemy{ };
        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->network_id != network_id ) continue;

            found_enemy = true;
            break;
        }

        if ( !found_enemy ) return -1;

        PredictionInstance data{ };

        data.network_id = network_id;
        m_prediction_data.push_back( data );

        return m_prediction_data.size( ) - 1;
    }

    auto Prediction::add_special_attack(
        const int16_t    index,
        const float      damage,
        const float      travel_time,
        const bool       cast_condition,
        const ESpellSlot condition_slot,
        const bool       is_autoattack
    ) -> void{
        SpecialAttackInstance data{ };

        data.target_index  = index;
        data.damage        = damage;
        data.land_time     = *g_time + travel_time;
        data.creation_time = *g_time;
        data.is_autoattack = is_autoattack;

        if ( cast_condition ) {
            data.travel_time    = travel_time;
            data.has_condition  = true;
            data.condition_slot = condition_slot;

            data.land_time = 0.f;
        }

        m_special_attacks.push_back( data );
    }

    auto Prediction::is_blink_spell( const hash_t name ) -> bool{
        switch ( name ) {
        case ct_hash( "EzrealE" ):
        case ct_hash( "EkkoEAttack" ):
        case ct_hash( "ViegoR" ):
        case ct_hash( "Riftwalk" ):
            return true;
        default:
            return false;
        }
    }

    auto Prediction::is_dash_spell( const hash_t name ) -> bool{
        switch ( name ) {
        case ct_hash( "AkaliE" ):
        case ct_hash( "GalioE" ):
        case ct_hash( "TristanaW" )://YuumiWCast
        case ct_hash( "UrgotE" ):
        case ct_hash( "YoneQ3" ):
        case ct_hash( "YuumiWCast" ):
            return true;
        default:
            return false;
        }
    }

    auto Prediction::is_channel_spell( const hash_t name ) -> bool{
        switch ( name ) {
        case ct_hash( "XerathLocusOfPower2" ):
        case ct_hash( "KatarinaR" ):
        case ct_hash( "CaitlynR" ):
        case ct_hash( "JhinR" ):
        case ct_hash( "MissFortuneBulletTime" ):
        case ct_hash( "FiddleSticksR" ):
        case ct_hash( "FiddleSticksW" ):
        case ct_hash( "KarthusFallenOne" ):
        case ct_hash( "NunuR" ):
        case ct_hash( "MalzaharR" ):
        case ct_hash( "ShenR" ):
        case ct_hash( "VelkozR" ):
        case ct_hash( "TaliyahR" ):
        case ct_hash( "QuinnR" ):
        case ct_hash( "ReapTheWhirlwind" ):
        case ct_hash( "WarwickRChannel" ):
        case ct_hash( "TahmKenchW" ):
        case ct_hash( "ZacE" ):
        case ct_hash( "SionQ" ):
        case ct_hash( "Meditate" ):
        case ct_hash( "IreliaW" ):
            return true;
        default:
            return false;
        }
    }

    auto Prediction::is_channel_blink( const hash_t name ) -> bool{
        switch ( name ) {
        case ct_hash( "FiddleSticksR" ):
        case ct_hash( "TahmKenchW" ):
            return true;
        default:
            return false;
        }
    }

    auto Prediction::is_hero_tracked( const int16_t index ) const -> bool{
        for ( const auto& inst : m_hero_data ) if ( inst.index == index ) return true;

        return false;
    }

    auto Prediction::get_hero_missile_speed( const int16_t index ) const -> float{
        if ( index == g_local->index ) return g_features->orbwalker->get_aa_missile_speed( );

        const auto source = g_entity_list.get_by_index( index );
        if ( !source ) return 0.f;

        switch ( helper::get_champion_hero( rt_hash( source->champion_name.text ) ) ) {
        case EHeroes::caitlyn:
            return g_features->buff_cache->get_buff( source->index, ct_hash( "caitlynpassivedriver" ) )
                       ? 3000.f
                       : 2500.f;
        case EHeroes::zoe:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "zoepassivesheenbuff" ) ) ) return 0.f;
            break;
        case EHeroes::viktor:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "ViktorPowerTransferReturn" ) ) ) return 0.f;
        case EHeroes::twitch:
            if ( g_features->buff_cache->get_buff( source->index, ct_hash( "TwitchFullAutomatic" ) ) ) return 4000.f;
            break;
        default:
            break;
        }

        for ( const auto inst : m_hero_data ) if ( inst.index == index ) return inst.missile_speed;

        return 0.f;
    }

    /*std::vector<std::string> m_interruptable_list = {
           _("XerathLocusOfPower2"),
           _("KatarinaR"),
           _("CaitlynR"),
           _("JhinR"),
           _("MissFortuneBulletTime"),
           _("FiddleSticksR"),
           _("KarthusFallenOne"),
           _("TeleportChannelBar4500"),
           _("PantheonR"),
           _("NunuR"),
           _("MalzaharR"),
           _("ShenR"),
           _("VelkozR"),
           _("QuinnR"),
           _("TaliyahR"),
           _("ReapTheWhirlwind"),
           _("WarwickRChannel"),
           _("SionQ"),
           _("FiddleSticksW"),
           _("TahmKenchW"),
           _("Meditate"),
           _("ZacE")
        };*/

    auto Prediction::get_dash_speed( const hash_t name, const float movespeed ) -> float{
        switch ( name ) {
        case ct_hash( "AkaliE" ):
            return 1500.f;
        case ct_hash( "GalioE" ):
            return 2300.f;
        case ct_hash( "TristanaW" ):
            return 1100.f;
        case ct_hash( "UrgotE" ):
            return 1200.f + movespeed;
        case ct_hash( "YoneQ3" ):
            return 1500.f;
        default:
            return 0.f;
        }
    }

    auto Prediction::get_fixed_range( const hash_t name ) -> std::optional< float >{
        switch ( name ) {
        case ct_hash( "AkaliE" ):
            return std::make_optional( -400.f );
        case ct_hash( "GalioE" ):
            return std::make_optional( 650.f );
        case ct_hash( "UrgotE" ):
        case ct_hash( "YoneQ3" ):
            return std::make_optional( 450.f );
        default:
            return std::nullopt;
        }
    }

    auto Prediction::get_spell_max_range( const hash_t name ) -> std::optional< float >{
        switch ( name ) {
        case ct_hash( "TristanaW" ):
            return std::make_optional( 900.f );
        case ct_hash( "RiftWalk" ):
        case ct_hash( "ViegoR" ):
            return std::make_optional( 500.f );
        case ct_hash( "EzrealE" ):
            return std::make_optional( 475.f );
        default:
            return std::nullopt;
        }
    }

    auto Prediction::fix_server_cast_time( const hash_t name, const float server_cast_time ) -> float{
        switch ( name ) {
        case ct_hash( "ZyraE" ):
        case ct_hash( "KaynW" ):
            return server_cast_time + 0.15f;
        default:
            break;
        }

        return server_cast_time;
    }

    auto Prediction::get_targetable_spell_attributes( hash_t name ) const -> std::optional< SpellAttribute >{
        SpellAttribute spell{ };

        switch ( name ) {
        case ct_hash( "AnnieQ" ):
        {
            spell.name        = _( "AnnieQ" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1400.f;

            break;
        }
        case ct_hash( "BlindingDart" ):
        {
            // Teemo Q

            spell.name        = _( "BlindingDart" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2500.f;

            break;
        }
        case ct_hash( "BlindMonkRKick" ):
        {
            // Leesin R

            spell.name        = _( "BlindMonkRKick" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        case ct_hash( "BrandE" ):
        {
            spell.name        = _( "BrandE" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        case ct_hash( "BrandR" ):
        {
            spell.name        = _( "BrandR" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1000.f;
            break;
        }
        case ct_hash( "CaitlynR" ):
        {
            spell.name        = _( "CaitlynR" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 3200.f;
            spell.delayed_cast  = true;

            break;
        }
        case ct_hash( "CassiopeiaE" ):
        {
            spell.name        = _( "CassiopeiaE" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2500.f;

            break;
        }
        case ct_hash( "EvelynnE" ):
        {
            spell.name        = _( "EvelynnE" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        case ct_hash( "EvelynnE2" ):
        {
            spell.name        = _( "EvelynnE2" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        case ct_hash( "Frostbite" ):
        {
            spell.name        = _( "AniviaE" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1600.f;

            break;
        }
        case ct_hash( "GangplankQProceed" ):
        {
            spell.name        = _( "GangplankQProceed" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2600.f;

            break;
        }
        case ct_hash( "GangplankQProceedCrit" ):
        {
            spell.name        = _( "GangplankQProceedCrit" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2600.f;

            break;
        }
        case ct_hash( "JhinQ" ):
        {
            spell.name        = _( "JhinQ" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1800.f;


            break;
        }
        case ct_hash( "KatarinaQ" ):
        {
            spell.name        = _( "KatarinaQ" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1600.f;

            break;
        }
        case ct_hash( "KhazixQ" ):
        case ct_hash( "KhazixQLong" ):
        {
            spell.name        = _( "KhazixQ" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::physical;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        case ct_hash( "LeblancQ" ):
        {
            spell.name        = _( "LeblancQ" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2000.f;

            break;
        }
        case ct_hash( "LeblancRQ" ):
        {
            spell.name        = _( "LeblancRQ" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2000.f;

            break;
        }
        case ct_hash( "NullLance" ):
        {
            // Kassadin Q
            spell.name        = _( "NullLance" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1400.f;


            break;
        }
        case ct_hash( "SeismicShard" ):
        {
            // Malphite Q

            spell.name        = _( "SeismicShard" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1200.f;

            break;
        }
        case ct_hash( "SowTheWind" ):
        {
            // Janna W

            spell.name        = _( "SowTheWind" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1600.f;

            break;
        }
        case ct_hash( "TristanaR" ):
        {
            spell.name        = _( "TristanaR" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2000.f;

            break;
        }
        case ct_hash( "TwoShivPoison" ):
        {
            // Shaco Q

            spell.name        = _( "TwoShivPoison" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 1500.f;

            break;
        }
        case ct_hash( "VeigarR" ):
        {
            spell.name        = _( "VeigarR" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2250.f;


            break;
        }
        case ct_hash( "ViktorPowerTransfer" ):
        {
            spell.name        = _( "ViktorPowerTransfer" );
            spell.spell_type  = EAttackType::missile;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 2000.f;


            break;
        }
        case ct_hash( "VladimirQ" ):
        {
            spell.name        = _( "VladimirQ" );
            spell.spell_type  = EAttackType::instant;
            spell.damage_type = ESpellDamageType::magic;
            spell.get_damage  = [ this ](
                const hash_t  spell_name,
                const int16_t source_index,
                const int16_t target_index,
                const float   server_cast_time,
                const float   impact_time
            ) -> float{
                    return helper::get_spell_damage(
                        spell_name,
                        source_index,
                        target_index,
                        server_cast_time,
                        impact_time
                    );
                };

            spell.missile_speed = 0.f;

            break;
        }
        default:
            return std::nullopt;
        }

        return std::make_optional( spell );
    }

    auto Prediction::is_hero_attack_active( const int16_t index, const float server_cast_time ) const -> bool{
        for ( const auto inst : m_hero_attacks ) {
            if ( inst.source_index == index && inst.server_cast_time ==
                server_cast_time )
                return true;
        }

        return false;
    }

    auto Prediction::is_champion_spell_active(
        const int16_t index,
        const int     slot,
        const float   server_cast_time
    ) const -> bool{
        for ( const auto inst : m_hero_spells ) {
            if ( inst.source_index == index && inst.slot == slot && inst.raw_server_cast_time ==
                server_cast_time )
                return true;
        }

        return false;
    }

    auto Prediction::remove_ignored_missile( const unsigned network_id ) -> void{
        const auto to_remove =
            std::ranges::remove_if(
                m_ignored_missiles,
                [&]( const IgnoredMissile& it ) -> bool{ return it.network_id == network_id; }
            ).begin( );

        if ( to_remove == m_ignored_missiles.end( ) ) return;

        m_ignored_missiles.erase( to_remove );
    }


    auto Prediction::update_ignored_missiles( ) -> void{
        for ( const auto inst : m_ignored_missiles ) {
            if ( *g_time - inst.ignore_start_time > 5.f ) {
                remove_ignored_missile( inst.network_id );
                continue;
            }
        }
    }

    auto Prediction::draw_prediction_debug() -> void {


        for (auto enemy : g_entity_list.get_enemies()) {

            if (!enemy || enemy->is_dead() || enemy->is_invisible()) continue;

            const auto data_index          = get_data_index(enemy->network_id);
            auto       data                = data_index >= 0 ? m_prediction_data[data_index] : PredictionInstance{};
            auto       has_prediction_data = data_index >= 0;
            //if (!has_prediction_data) continue;

            auto position = enemy->get_hpbar_position();

            position.x -= 75.f;
            position.y -= 25.f;

            //g_render->filled_circle(position, Color(255, 255, 255, 255), 3.f, 15);

            auto pred = g_features->prediction->predict(enemy->index, 1150.f, 2000.f, 60.f, 0.25f, {}, true);

            std::string reason_text{};
            std::string hitchance_text{};
            Color text_color{ 255, 255, 255 };

            switch (pred.hitchance)
            {
                case Prediction::EHitchance::low:
                hitchance_text = "Low";
                text_color     = Color(225, 225, 225);
                break;
                case EHitchance::medium:
                hitchance_text = "Medium";
                text_color     = Color(255, 255, 75);
                break;
                case EHitchance::high:
                    hitchance_text = "High";
                text_color     = Color(75, 255, 75);
                break;
            case EHitchance::very_high:
                hitchance_text = "Very High";
                text_color     = Color(75, 255, 255);
                break;
            case EHitchance::immobile:
                hitchance_text = "Immobile";
                text_color     = Color(255, 75, 75);
                break;
            case EHitchance::invalid:
                hitchance_text = "INVALID";
                text_color     = Color(75, 75, 255);
                break;
            }
            switch (pred.reason)
            {
            case Prediction::EHitchanceReason::idle:
                reason_text = "idle";
                break;
            case Prediction::EHitchanceReason::recent_path:
                reason_text = "new path";
                break;
            case Prediction::EHitchanceReason::crowd_control:
                reason_text = "crowdcontrol";
                break;
            case Prediction::EHitchanceReason::dash:
                reason_text = "dashing";
                break;
            case Prediction::EHitchanceReason::guaranteed_hit:
                reason_text = "guaranteed";
                break;
            case Prediction::EHitchanceReason::similar_path_angle:
                reason_text = "similiar angle";
                break;
            case Prediction::EHitchanceReason::fresh_path_angle:
                reason_text = "fresh angle";
                break;
            case Prediction::EHitchanceReason::abnormal_path_length:
                reason_text = "abnormal path length";
                break;
            case Prediction::EHitchanceReason::post_spell:
                reason_text = "post spellcast";
                break;
            case Prediction::EHitchanceReason::reaction_time:
                reason_text = "reaction";
                break;
            case Prediction::EHitchanceReason::predicted_stun:
                reason_text = "pred stun";
                break;
            case Prediction::EHitchanceReason::spellcast:
                reason_text = "spellcast";
                break;
            case Prediction::EHitchanceReason::bad_duration:
                reason_text = "bad duration";
                break;
            case Prediction::EHitchanceReason::slowed:
                reason_text = "slowed";
                break;
            default:
                reason_text = "Unknown";
                break;
            }

            auto text_size = g_render->get_text_size(hitchance_text, g_fonts->get_zabel(), 20.f);
            Vec2 hitchance_position = Vec2( position.x - text_size.x, position.y);

            g_render->text_shadow(hitchance_position, text_color, g_fonts->get_zabel(), hitchance_text.data(), 20.f);

        }



    }


    auto Prediction::on_draw( ) -> void{
        //
        //draw_minions();
        //draw_prediction_debug();

        return;

        for (auto attack : m_turret_attacks) {

            g_render->circle_3d(attack.source->position, Color::green(), attack.source->get_bounding_radius(),
                                Renderer::outline, -1, 3.f);

            g_render->line_3d(attack.source->position, attack.target_position,
                              attack.landed ? Color::white() : Color::red(), 2.f);

            if( attack.land_time > *g_time ) std::cout << "Time until land: " << attack.land_time - *g_time << std::endl;
        }
    }

    auto Prediction::draw_minions() -> void {


        for (auto inst : m_minions) {

            if (!inst.unit) continue;

            auto aimgr = inst.unit->get_ai_manager();
            if (!aimgr) continue;

            /*if (GetAsyncKeyState(VK_CONTROL) && inst.type == Object::EMinionType::siege)
            {
                auto obj = g_entity_list.get_by_index(inst.index);
                std::cout << "[ SiegeMinion: " << std::hex << obj.get_address() << " ] AIMGR: " << aimgr.get_address()
                          << std::endl;
            }*/

            switch ( inst.state ) {
            case MinionState::Idle:
                break;
            case MinionState::TargetingUnit: {

                float modifier = std::clamp( ( *g_time - inst.state_change_time ) / 0.25f, 0.f, 1.f );
                    modifier       = utils::ease::ease_out_quart(modifier);

                if (modifier < 1.f)
                        g_render->circle_3d(inst.unit->position, Color(235, 177, 52),
                                            inst.acquisition_radius - inst.acquisition_radius * modifier,
                                            Renderer::outline);

                
                auto target       = g_entity_list.get_by_index(inst.last_target_index);
                if (!target) continue;

                g_render->line_3d(inst.unit->position, inst.unit->position.extend( target->position, target->position.dist_to( inst.unit->position ) * modifier ), Color(255, 50, 50), 2.f);
                break;
            }
            case MinionState::Moving: {

                   float modifier = std::clamp((*g_time - inst.state_change_time) / 0.25f, 0.f, 1.f);
                modifier       = utils::ease::ease_out_quart(modifier);

                
                g_render->circle_3d(inst.unit->position, Color(235, 177, 52), inst.acquisition_radius * modifier, Renderer::outline);

                auto path = aimgr->get_path();
                if (path.size() > 1)
                {

                        for (auto i = aimgr->next_path_node; i < static_cast<int>(path.size()); i++)
                        {

                        if (i == aimgr->next_path_node)
                            g_render->line_3d(aimgr->get_server_position(), path[i], Color::white().alpha(155), 2.f);
                        else
                            g_render->line_3d(path[i - 1], path[i], Color::white().alpha(155), 2.f);
                        }

                        continue;
                }

                break;
            }
            default:
                break;
            }
        }

    }


    auto Prediction::create_new_patterns( ) -> void{
        for ( auto& inst : m_pattern_instances ) {
            for ( auto i = 0; i < static_cast< int >( inst.recent_actions.size( ) - 2 ); i++ ) {
                auto action = inst.recent_actions[ i ];
                if ( action.type == EActionType::none ) continue;

                std::vector< PatternCondition > conditions{ };
                std::vector< EChangeType >      angle_pattern{ };
                std::vector< EActionType >      actions{ };

                auto change = get_angle_change_type( action.path_angle );
                if ( action.type == EActionType::spell ) change = EChangeType::none;
                else if ( action.type == EActionType::new_path && change == EChangeType::fresh ) continue;

                conditions.push_back( { action.type, change } );

                bool skip{ };
                auto max_loop = action.type == EActionType::spell ? 2 : 3;

                for ( auto a = 1; a < max_loop; a++ ) {
                    auto next_action = inst.recent_actions[ i + a ];
                    if ( next_action.type != EActionType::new_path ) {
                        skip = true;
                        break;
                    }

                    change = get_angle_change_type( next_action.path_angle );
                    if ( change == EChangeType::fresh || change == EChangeType::none ) continue;

                    conditions.push_back( { next_action.type, change } );
                }

                if ( skip || conditions.size( ) > 3 || conditions.size( ) < 2 ) continue;

                ActionPattern pattern;

                pattern.creation_action_count = inst.total_actions;
                pattern.conditions            = conditions;
                pattern.creation_time         = *g_time;
                pattern.pattern_size          = static_cast< int >( conditions.size( ) );

                if ( matches_existing_pattern( pattern ) ) continue;

                inst.patterns.push_back( pattern );

                std::string msg{ "[ " };

                for ( auto o = 0; o < pattern.conditions.size( ); o++ ) {
                    if ( pattern.conditions[ o ].action_type == EActionType::spell ) msg += "spell";
                    else {
                        switch ( pattern.conditions[ o ].angle_difference ) {
                        case EChangeType::none:
                            msg += "none";
                            break;
                        case EChangeType::equal:
                            msg += "equal";
                            break;
                        case EChangeType::minor:
                            msg += "minor";
                            break;
                        case EChangeType::average:
                            msg += "avg";
                            break;
                        case EChangeType::major:
                            msg += "major";
                            break;
                        case EChangeType::fresh:
                            msg += "fresh";
                            break;
                        }
                    }

                    if ( o < pattern.conditions.size( ) - 1 ) msg += ", ";
                }

                debug_log( "[ PATTERN ] Created pattern for {} {}", inst.champion_name, msg );
            }
        }
    }

    auto Prediction::detect_patterns( ) -> void{
        for ( auto& inst : m_pattern_instances ) {
            for ( auto& pattern : inst.patterns ) {
                if ( pattern.is_submitted ) continue;

                if ( static_cast< int >( inst.recent_actions.size( ) ) <= pattern.pattern_size ) continue;

                int conditions_met{ };

                for ( auto i = 0; i <= 1; i++ ) {
                    if ( !inst.last_action.valid ) break;

                    const auto action = i == 0
                                            ? inst.recent_actions[ inst.recent_actions.size( ) - 1 ]
                                            : inst.last_action;
                    const auto condition = pattern.conditions[ i ];

                    const auto change = get_angle_change_type( action.path_angle );

                    if ( condition.angle_difference != change || condition.action_type != action.type ) break;

                    ++conditions_met;

                    if ( conditions_met < pattern.pattern_size - 1 ) continue;

                    pattern.predicted_change = pattern.conditions.back( ).angle_difference;
                    pattern.is_submitted     = true;

                    // std::string msg{ };
                    // switch ( pattern.conditions.back( ).angle_difference ) {
                    // case EChangeType::none:
                    //     msg = "none";
                    //     break;
                    // case EChangeType::equal:
                    //     msg = "equal";
                    //     break;
                    // case EChangeType::minor:
                    //     msg = "minor";
                    //     break;
                    // case EChangeType::average:
                    //     msg = "avg";
                    //     break;
                    // case EChangeType::major:
                    //     msg = "major";
                    //     break;
                    // case EChangeType::fresh:
                    //     msg = "freeesh";
                    //     break;
                    // }
                }
            }
        }
    }


    auto Prediction::matches_existing_pattern( const ActionPattern& instance ) const -> bool{
        for ( auto& inst : m_pattern_instances ) {
            for ( auto& pattern : inst.patterns ) {
                if ( pattern.pattern_size != instance.pattern_size ) continue;

                int similiarity{ };
                for ( auto i = 0; i < instance.pattern_size; i++ ) {
                    const auto original = pattern.conditions[ i ];
                    const auto cond     = instance.conditions[ i ];

                    if ( original.action_type == cond.action_type && original.angle_difference == cond.
                        angle_difference )
                        similiarity++;
                }

                if ( similiarity >= instance.pattern_size ) return true;
            }
        }

        return false;
    }

    auto Prediction::get_angle_change_type( const float angle ) -> EChangeType{
        constexpr auto equal_threshold{ 15.f };
        constexpr auto minor_threshold{ 45.f };
        constexpr auto average_threshold{ 90.f };

        EChangeType change;

        if ( angle < 0.f ) change = EChangeType::fresh;
        else if ( angle <= equal_threshold ) change = EChangeType::equal;
        else if ( angle <= minor_threshold ) change = EChangeType::minor;
        else if ( angle <= average_threshold ) change = EChangeType::average;
        else change                                   = EChangeType::major;

        return change;
    }

    auto Prediction::get_champion_radius( const hash_t name ) -> float{
        switch ( name ) {
        case ct_hash( "Amumu" ):
        case ct_hash( "Annie" ):
        case ct_hash( "Fizz" ):
        case ct_hash( "Heimerdinger" ):
        case ct_hash( "Kennen" ):
        case ct_hash( "Poppy" ):
        case ct_hash( "Teemo" ):
        case ct_hash( "Tristana" ):
        case ct_hash( "Veigar" ):
        case ct_hash( "Warwick" ):
        case ct_hash( "Ziggs" ):
            return 55.f;
        case ct_hash( "Ivern" ):
            return 70.f;
        case ct_hash( "Bard" ):
        case ct_hash( "Blitzcrank" ):
        case ct_hash( "Braum" ):
        case ct_hash( "Chogath" ):
        case ct_hash( "Darius" ):
        case ct_hash( "DrMundo" ):
        case ct_hash( "Galio" ):
        case ct_hash( "Gragas" ):
        case ct_hash( "Hecarim" ):
        case ct_hash( "Malphite" ):
        case ct_hash( "Maokai" ):
        case ct_hash( "Mordekaiser" ):
        case ct_hash( "Nasus" ):
        case ct_hash( "Nautilus" ):
        case ct_hash( "Ornn" ):
        case ct_hash( "Renekton" ):
        case ct_hash( "Rumble" ):
        case ct_hash( "Sejuani" ):
        case ct_hash( "Sion" ):
        case ct_hash( "Skarner" ):
        case ct_hash( "TahmKench" ):
        case ct_hash( "Urgot" ):
        case ct_hash( "Volibear" ):
        case ct_hash( "Yorick" ):
        case ct_hash( "Zac" ):
            return 80.f;
        default:
            return 65.f;
        }
    }

    auto Prediction::get_normal_move_speed( const int16_t index ) -> float{
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit ) return 0.f;

        unit.update( );

        auto uic = unit->get_unit_info_component( );
        if ( !uic ) return 0.f;

        auto constructed_movespeed{ uic->get_base_ms( ) };
        if ( constructed_movespeed <= 0.f ) {
            if ( rt_hash( unit->champion_name.text ) ==
                ct_hash( "Rammus" ) )
                constructed_movespeed = 335.f;
        }

        float additional_speed{ };
        auto  speed_multiplier{ 1.f };

        for ( auto i = 1; i < 7; i++ ) {
            auto slot = unit->inventory.get_inventory_slot( i );
            if ( !slot ) continue;
            auto item_base = slot->get_base_item( );
            if ( !item_base ) continue;
            auto item_data = item_base->get_item_data( );
            if ( !item_data ) continue;

            switch ( static_cast< EItemId >( item_data->id ) ) {
            case EItemId::berserkers_greaves:
            case EItemId::boots_of_lucidity:
            case EItemId::sorcerers_shoes:
            case EItemId::plated_steelcaps:
            case EItemId::mercurys_treads:
                additional_speed += 45.f;
                break;
            case EItemId::slightly_magical_boots:
                additional_speed += 35.f;
                break;
            case EItemId::boots_of_swiftness:
                additional_speed += 60.f;
                break;
            case EItemId::boots:
            case EItemId::mobility_boots:
                additional_speed += 25.f;
                break;
            case EItemId::zeal:
            case EItemId::mortal_reminder:
            case EItemId::phantom_dancer:
            case EItemId::runaans_hurricane:
            case EItemId::rapidfire_cannon:
                speed_multiplier += 0.07f;
            case EItemId::lichbane:
                speed_multiplier += 0.08f;
                break;
            default:
                break;
            }
        }

        constructed_movespeed += additional_speed;

        return constructed_movespeed * speed_multiplier;
    }

    auto Prediction::calculate_movement_speed( const int16_t index, const float time ) -> float{
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit ) return 0.f;

        unit.update( );

        const auto default_movespeed = get_normal_move_speed( index );
        const auto current_time      = *g_time + time;
        auto       constructed_speed{ default_movespeed };

        auto bonus_movespeed{ 0.f };

        switch ( rt_hash( unit->champion_name.text ) ) {
        case ct_hash( "Blitzcrank" ):
        {
            const auto buff = g_features->buff_cache->get_buff( index, ct_hash( "Overdrive" ) );
            if ( buff ) {
                if ( current_time < buff->buff_data->start_time + 2.5f ) {
                    auto slot = unit->spell_book.get_spell_slot( ESpellSlot::w );
                    if ( !slot ) break;

                    const auto speed_modifier    = 0.55f + std::min( slot->level * 0.05f, 0.25f );
                    const auto duration_modifier = std::clamp(
                        ( buff->buff_data->start_time + 2.5f - current_time ) / 2.5f,
                        0.f,
                        1.f
                    );

                    constructed_speed = default_movespeed + default_movespeed * ( 0.1f + speed_modifier *
                        duration_modifier );
                } else if ( current_time < buff->buff_data->end_time ) constructed_speed += default_movespeed * 0.1f;
                else if ( current_time < buff->buff_data->end_time + 1.5f ) {
                    constructed_speed -= default_movespeed *
                        0.3f;
                }
            }

            break;
        }
        case ct_hash( "Draven" ):
        {
            const auto buff = g_features->buff_cache->get_buff( index, ct_hash( "DravenFury" ) );
            if ( buff && buff->buff_data->end_time > current_time ) {
                auto slot = unit->spell_book.get_spell_slot( ESpellSlot::w );
                if ( !slot ) break;

                const auto speed_modifier    = 0.45f + std::min( slot->level * 0.05f, 0.25f );
                const auto duration_modifier =
                    std::clamp( ( buff->buff_data->end_time - current_time ) / 1.5f, 0.f, 1.f );

                constructed_speed += default_movespeed * ( speed_modifier * duration_modifier );
            }

            break;
        }
        case ct_hash( "Rammus" ):
        {
            const auto buff = g_features->buff_cache->get_buff( index, ct_hash( "PowerBall" ) );
            if ( buff && buff->buff_data->end_time > current_time ) {
                const auto speed_modifier    = 0.5f + 0.45f + std::min( unit->level * 0.05f, 0.9f );
                const auto duration_modifier =
                    std::clamp(
                        1.f -
                        ( buff->buff_data->end_time - current_time ) /
                        ( buff->buff_data->end_time - buff->buff_data->start_time ),
                        0.f,
                        1.f
                    );

                constructed_speed = default_movespeed + default_movespeed * ( speed_modifier * duration_modifier );
            }

            break;
        }
        default:
            break;
        }

        auto buff = g_features->buff_cache->get_buff(
            index,
            ct_hash( "ASSETS/Perks/Styles/Precision/FleetFootwork/FleetFootworkHaste.lua" )
        );
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.2f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "SummonerHeal" ) );
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.3f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "3046buff" ) ); // phantom dancer passive buff
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.07f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "spectralfury" ) ); // yomuus active buff
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.2f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "ItemMercurial" ) ); // mercurial scimitar active buff
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.5f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "item3078triforcespeed" ) ); // triforce passive buff
        if ( buff && buff->buff_data->end_time > current_time ) bonus_movespeed += 0.2f;

        buff = g_features->buff_cache->get_buff( index, ct_hash( "Sru_CrabSpeedBoost" ) ); // scuttle speed buff
        if ( buff && ( *g_time - buff->buff_data->start_time < 0.5f || buff->buff_data->end_time >
            current_time ) )
            bonus_movespeed += 0.3f;

        constructed_speed += default_movespeed * bonus_movespeed;

        return constructed_speed;
    }

    auto Prediction::predict_movement(
        int16_t index,
        float   duration,
        float   hitbox_size
    ) -> std::optional< Vec3 >{
        auto target = g_entity_list.get_by_index( index );
        if ( !target || target->is_dead( ) || target->is_invisible( ) ) return std::nullopt;

        target.update( );

        auto aimgr = target->get_ai_manager( );
        if ( !aimgr || duration <= 0.f ) return std::make_optional( target->position );

        aimgr.update( );

        const auto path       = aimgr->get_path( );
        auto       goal_node  = aimgr->next_path_node;
        const auto is_moving  = aimgr->is_moving;
        const auto is_dashing = aimgr->is_dashing;

        if ( !is_moving || path.size( ) <= 1 || path.size( ) == goal_node ) {
            return std::make_optional(
                target->position
            );
        }

        const auto movement_speed   = is_dashing ? aimgr->dash_speed : target->movement_speed;
        const auto should_construct = !is_dashing && g_config->prediction.speed_calculation_mode->get< int >( ) == 1;

        constexpr auto terrain_height = 0.f; // g_navgrid->get_height( Vec3( target->position + aimgr->velocity ) );

        auto raw_direction = ( aimgr->path_end - target->position ).normalize( );


        auto calculated_position = aimgr->get_server_position( );
        calculated_position.y    = 0;

        const auto tick_count = std::ceil( duration / 0.05f );
        auto       waypoint_node{ goal_node };
        bool       reached_path_end{ };

        //std::cout << "[ Current speed: " << g_local->movement_speed
        //          << " | Constructed speed: " << calculate_movementspeed( target->index, 0.f ) << std::endl;

        float units_predicted{ };

        for ( auto tick = 0; tick <= tick_count; tick++ ) {
            const auto tick_delay = 0.05f; // tick < tick_count ? 0.05f : 0.05f - ( tick * 0.05f - duration );

            const auto time_addition = tick < tick_count ? static_cast< float >( tick ) * 0.05f : duration;
            const auto tick_speed    = should_construct
                                           ? calculate_movement_speed( target->index, time_addition )
                                           : movement_speed;

            const auto tick_delta = tick_speed * tick_delay;
            auto       tick_units = tick_delta;

            //if ( tick == tick_count )
            //    std::cout << "Tick units: " << tick_units << " | simTIME: " << time_addition << " | pTIME: " << duration
            //              << " | tDelay: " << tick_delay << std::endl;
            //std::cout << "[ TICK #" << tick << " ] Tick delta: " << tick_units << std::endl;

            //movable_units -= tick_delta;

            auto current_position = calculated_position;

            for ( auto i = waypoint_node; i < path.size( ); i++ ) {
                auto path_start = i == waypoint_node ? calculated_position : path[ i - 1 ];
                auto path_end   = path[ i ];

                path_start.y = terrain_height;
                path_end.y   = terrain_height;

                const auto distance = path_start.dist_to( path_end );

                if ( distance > tick_units ) {
                    calculated_position = path_start.extend( path_end, tick_units );
                    break;
                }

                if ( i == path.size( ) - 1 ) {
                    calculated_position = path_end;
                    reached_path_end    = true;
                    break;
                }

                tick_units -= distance;
                calculated_position = path_end;
                ++waypoint_node;
            }

            if ( reached_path_end ) {
                units_predicted += current_position.dist_to( calculated_position );
                break;
            }

            units_predicted += tick_delta;
        }

        if ( reached_path_end ) {
            const auto additional_prediction_amount = duration * movement_speed - units_predicted;

            auto extended_position =
                calculated_position.extend( calculated_position + raw_direction, additional_prediction_amount );

            if ( !helper::is_wall_in_line( calculated_position, extended_position ) ) {
                calculated_position = extended_position;
                waypoint_node       = path.size( );
            }
        }

        if ( hitbox_size > 0.f ) {
            //const float compensated_units = hitbox_size;
            //const auto extension_direction = calculated_position - raw_direction;

            auto deductable_units = hitbox_size;
            auto projected_position{ calculated_position };

            for ( auto i = waypoint_node; i > 0; i-- ) {
                auto path_start = i == waypoint_node ? calculated_position : path[ i ];
                auto path_end   = path[ i - 1 ];

                path_start.y = terrain_height;
                path_end.y   = terrain_height;

                const auto distance = path_start.dist_to( path_end );

                if ( distance > deductable_units ) {
                    projected_position = path_start.extend( path_end, deductable_units );
                    break;
                }

                deductable_units -= distance;
                projected_position = path_end;
                --waypoint_node;

                if ( waypoint_node == 0 ) break;
            }

            //Vec3 adjusted_position = calculated_position.extend( extension_direction, compensated_units );
            projected_position.y = g_navgrid->get_height( projected_position );

            return std::make_optional( projected_position );
        }

        calculated_position.y = g_navgrid->get_height( calculated_position );
        return std::make_optional( calculated_position );
    }

    auto Prediction::get_server_position( const int16_t index ) -> Vec3{
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit || unit->is_dead( ) || unit->is_invisible( ) ) return Vec3( );

        unit.update( );

        auto ai_manager = unit->get_ai_manager( );
        if ( !ai_manager ) return unit->position;

        return ai_manager->get_server_position( );
    }
}
