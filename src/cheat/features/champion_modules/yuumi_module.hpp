#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class yuumi_module final : public IModule {
    public:
        virtual ~yuumi_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "yuumi_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Yuumi" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        struct projectile_rotation_instance_t {
            Vec3  end_position{ };
            float rotation_duration{ };

            std::vector< Vec3 > trajectory{ };
        };

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "yuumi" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->yuumi.q_enabled );
            q_settings->checkbox( _( "auto q1" ), g_config->yuumi.q_auto_first_cast );
            q_settings->checkbox( _( "autoharass q" ), g_config->yuumi.q_auto_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->yuumi.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->yuumi.e_enabled );
            e_settings->multi_select(
                _( "e block damage from " ),
                {
                    g_config->yuumi.e_block_damage_from_autoattacks,
                    g_config->yuumi.e_block_damage_from_spells,
                    g_config->yuumi.e_block_damage_from_turret_shots,
                    g_config->yuumi.e_block_damage_from_ignite,
                    g_config->yuumi.e_block_damage_from_poison,
                    g_config->yuumi.e_block_damage_from_item_burn
                },
                {
                    _( "Autoattacks" ),
                    _( "Skillshots" ),
                    _( "Turret shots" ),
                    _( "Ignite" ),
                    _( "Poison" ),
                    _( "Liandrys burn" )
                }
            );

            e_settings->slider_int(
                _( "min skillshot danger to shield" ),
                g_config->yuumi.e_skillshot_minimum_danger,
                1,
                5,
                1
            );
            e_settings->checkbox( _( "buff ally autoattack" ), g_config->yuumi.e_increase_damage );

            drawings->checkbox( _( "draw q range" ), g_config->yuumi.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->yuumi.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->yuumi.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->yuumi.dont_draw_on_cooldown );
            drawings->slider_int( _( "q slot" ), g_config->yuumi.q_spellslot, 1, 63, 1 );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            auto q_slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( g_config->yuumi.q_draw_range->get< bool >( ) && q_slot && q_slot->level > 0 &&
                ( q_slot->is_ready( true ) || !g_config->yuumi.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            auto w_slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            if ( g_config->yuumi.w_draw_range->get< bool >( ) && w_slot && w_slot->level > 0 &&
                ( w_slot->is_ready( true ) || !g_config->yuumi.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 132, 245, 66, 255 ),
                    m_w_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            auto r_slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
            if ( g_config->yuumi.r_draw_range->get< bool >( ) && r_slot && r_slot->level > 0 &&
                ( r_slot->is_ready( true ) || !g_config->yuumi.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 173, 47, 68, 255 ),
                    m_r_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }


            if ( m_q_active ) {
                const auto position  = m_missile_position;
                const auto direction = m_missile_position.extend( m_missile_position + m_missile_delta, 100.f );

                g_render->circle_3d(
                    m_missile_position,
                    Color( 255, 255, 25, 50 ),
                    35.f,
                    Renderer::outline | Renderer::filled,
                    50,
                    2.f
                );

                const auto sim_points =
                    get_projectile_position_after_turn(
                        g_pw_hud->get_hud_manager( )->cursor_position_unclipped
                    ).trajectory;

                if ( sim_points.size( ) > 1 ) {
                    for ( auto i = 0; i < sim_points.size( ) - 1; i++ ) {
                        auto start = sim_points[ i ];
                        auto end   = sim_points[ i + 1 ];

                        g_render->line_3d( start, end, Color( 255, 200, 10 ), 3.f );
                    }
                }

                /* for ( auto point : sim_points ) {

                    g_render->circle_3d(
                        point, Color( 255, 50, 50 ), 10.f, Renderer::outline | Renderer::filled, 16, 1.f );
                }*/

                g_render->line_3d(
                    m_missile_position,
                    *g_time - m_last_cast_time <= 0.25f ? m_q_last_direction : direction,
                    Color( 50, 255, 50 ),
                    3.f
                );

                for ( auto point : m_redirect_points ) {
                    g_render->circle_3d(
                        point,
                        point == m_q_last_direction ? Color( 50, 255, 50, 50 ) : Color( 255, 255, 255, 50 ),
                        20.f,
                        Renderer::outline | Renderer::filled,
                        26,
                        2.f
                    );
                }

                const auto modifier = std::clamp(
                    ( m_acceleration_time - *g_time ) / ( m_acceleration_time - m_q_start_time ),
                    0.f,
                    1.f
                );
                const auto angle = 243.f * modifier;
                //if ( angle > 150.f ) return;

                const auto sector = sdk::math::Sector(
                    position,
                    direction,
                    angle * 2.f,
                    425.f
                );

                g_render->polygon_3d(
                    sector.to_polygon_new( ),
                    Color( 255.f, 255.f, 255.f, 255.f - 255.f * modifier ),
                    Renderer::outline,
                    2.f
                );

                /* auto details = m_slot_q->get_details( );
                 if ( !details ) return;
 
                 g_render->circle_3d( details->last_start_position,
                                      Color( 255, 255, 255, 50 ),
                                      25.f,
                                      Renderer::outline | Renderer::filled,
                                      32,
                                      2.f );
 
                 g_render->circle_3d( details->last_end_position,
                                      Color( 10, 255, 255, 50 ),
                                      25.f,
                                      Renderer::outline | Renderer::filled,
                                      32,
                                      2.f );
 
                  g_render->circle_3d( details->last_unknown_position,
                                        Color( 255, 0, 0, 50 ),
                                        25.f,
                                        Renderer::outline | Renderer::filled,
                                        32,
                                        2.f );*/
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            track_q_projectile( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            m_is_attached = rt_hash( m_slot_w->get_name( ).data( ) ) == ct_hash( "YuumiWEndWrapper" );

            if ( g_config->yuumi.q_enabled->get< bool >( ) && g_config->yuumi.q_auto_harass->get< bool >( ) ) {
                /* if ( m_q_active != m_was_q_active ||
                     m_q_active && m_q_target_index == 0 && g_function_caller->is_update_chargeable_blocked( ) |
                     m_q_active && m_q_target_index > 0 && !g_function_caller->is_update_chargeable_blocked( ) ) {|
                    g_function_caller->set_update_chargeable_blocked( m_q_active );
                    std::cout << "[ Yuumez ] Set chargeable block " << m_q_active << std::endl;
                }*/

                //m_was_q_active = m_q_active;

                if ( m_q_active ) {
                    direct_q_missile(
                        m_q_target_index > 0
                            ? g_entity_list.get_by_index( m_q_target_index ).get( )
                            : g_features->target_selector->get_default_target( )
                    );
                } else if ( g_config->yuumi.q_auto_first_cast->get< bool >( ) ) {
                    automatic_q(
                        g_features->target_selector->get_default_target( )
                    );
                }
            }

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                automatic_q( g_features->target_selector->get_default_target( ) );
                break;
            default:
                break;
            }

            // YuumiPMat - heal aa ready buff
            // yuumiwcooldowntracker - when w too early buff
            // YuumiWAlly - buff on w target
            // yuumiphealmark - heal mark buff
            // YuumiQCast
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto combo_q( Object* target ) -> bool{
            if ( !g_config->yuumi.q_enabled->get< bool >( ) || !target || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid ||
                pred.hitchance <
                static_cast< Prediction::EHitchance >( g_config->mundo.q_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    60.f,
                    0.25f,
                    2000.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto automatic_q( Object* target ) -> bool{
            if ( !g_config->yuumi.q_enabled->get< bool >( ) || !m_is_attached
                || !target || target->dist_to_local( ) > 1100.f || m_q_active || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_q_time <= 0.5f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target_position = get_q_target_position( target );
            if ( !target_position || g_features->prediction->minion_in_circle( *target_position, 100.f ) ||
                g_features->prediction->minion_in_line( g_local->position, target_position.value( ), 60.f ) &&
                g_local->position.dist_to( target_position.value( ) ) > 450.f )
                return false;

            if ( cast_spell( ESpellSlot::q, target_position.value( ) ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_q_target_index = target->index;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Yuumi: Q ] Auto Q1, targeting " << target->get_name( ) << " | T: " << *g_time
                    << std::endl;

                return true;
            }

            return false;
        }

        auto direct_q_missile( Object* target ) -> void{
            if ( !target || !m_q_active || !m_projectile_initialized || !m_is_attached ||
                m_acceleration_time <= *g_time )
                return;


            const auto projectile_target = get_q_target_position( target );
            if ( !projectile_target ) return;

            Vec2 sp{ };
            if ( !world_to_screen( *projectile_target, sp ) ) return;

            g_input->set_cursor_position_run( sp );
            m_last_cast_time   = *g_time;
            m_q_last_direction = *projectile_target;
            std::cout << "[ Yuumi: Q ] Update trajectory | " << *g_time << std::endl;
        }

        auto get_q_target_position( Object* target ) -> std::optional< Vec3 >{
            if ( !target ) return std::nullopt;

            auto projectile_data = get_projectile_position_after_turn( target->position );
            auto travel_time     = projectile_data.rotation_duration;

            const auto q_directing_time = m_q_active ? m_acceleration_time - *g_time : 1.35f;

            auto missile_travel_time = m_missile_position.dist_to( target->position ) / 850.f;
            if ( missile_travel_time > q_directing_time ) {
                const auto directing_end_position =
                    m_missile_position.extend( target->position, q_directing_time * 850.f );
                missile_travel_time = m_missile_position.dist_to( directing_end_position ) / 850.f +
                    directing_end_position.dist_to( target->position ) / 1650.f;
            }

            if ( !m_projectile_initialized ) travel_time = 0.f;
            else travel_time += missile_travel_time;

            const auto missile = m_missile_position;
            const auto pred    = g_features->prediction->predict(
                target->index,
                10000.f,
                0.f,
                60.f,
                travel_time,
                { },
                true
            );
            if ( !pred.valid ||
                !m_projectile_initialized &&
                ( g_features->prediction->minion_in_circle( target->position, 125.f ) ||
                    g_features->prediction->minion_in_circle( pred.position, 125.f ) ) )
                return std::nullopt;

            projectile_data = get_projectile_position_after_turn( pred.position );

            auto direction_position{ pred.position };

            if ( g_features->prediction->minion_in_line( projectile_data.end_position, pred.position, 60.f )
                //||g_features->prediction->champion_in_line( projectile_data.end_position, pred.position, 60.f, true, 0.f, 0.f, target->network_id )
            ) {
                const auto missile_position = projectile_data.end_position;

                const auto modifier = std::clamp(
                    q_directing_time / ( m_acceleration_time - m_q_start_time ),
                    0.f,
                    1.f
                );
                const auto distance = 825.f + 1150.f * modifier;

                const auto points = g_render->get_3d_circle_points( target->position, distance, 22 );
                m_redirect_points = points;

                Vec3 best_point{ };
                auto best_travel_time{ 999.f };

                for ( auto point : points ) {
                    auto closest = g_features->evade->get_closest_line_point( missile, point, pred.position );

                    if ( g_features->prediction->minion_in_line( closest, pred.position, 60.f ) ||
                        g_features->prediction->minion_in_line( missile_position, closest, 60.f ) )
                        // g_features->prediction->champion_in_line( closest, pred.position, 60.f, true, 0.f, 0.f,target->network_id ) ||
                        // g_features->prediction->champion_in_line( missile_position, closest, 60.f, true, 0.f, 0.f, target->network_id ) )
                        continue;

                    const auto time_to_point = get_projectile_q_time_to_point( closest );

                    const auto future_modifier = std::clamp(
                        q_directing_time - ( time_to_point + g_features->orbwalker->get_ping( ) * 1.75f ) / (
                            m_acceleration_time - m_q_start_time ),
                        0.f,
                        1.f
                    );

                    const auto max_possible_rotation = 243.f * future_modifier;

                    auto       v1             = pred.position - missile;
                    auto       v2             = closest - missile;
                    const auto dot            = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto required_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    if ( required_angle > max_possible_rotation ||
                        best_point.length( ) > 0.f && ( best_point.dist_to( direction_position ) + 50.f < closest.
                            dist_to( direction_position )
                            || best_point.dist_to( direction_position ) > closest.dist_to( direction_position ) - 25.f
                            && best_travel_time < time_to_point ) )
                        continue;

                    best_point       = closest;
                    best_travel_time = time_to_point;
                }

                if ( best_point.length( ) > 0.f ) direction_position = best_point;
                else if ( !m_projectile_initialized ) return std::nullopt;
            }

            return direction_position.length( ) > 0.f ? std::make_optional( direction_position ) : std::nullopt;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->mundo.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f ||
                !g_features->orbwalker->should_reset_aa( ) || !m_slot_e->is_ready( ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DrMundoE" ) ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto track_q_projectile( ) -> void{
            if ( !m_q_active ) {
                if ( m_q_target_index > 0 ) m_q_target_index = 0;

                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "YuumiQCast" ) );
                if ( !buff ) return;

                std::cout << "Found Q buff, ending in " << buff->buff_data->end_time - *g_time << " seconds\n";
                m_q_active               = true;
                m_missile_found          = false;
                m_acceleration_time      = buff->buff_data->start_time + 1.35f;
                m_q_start_time           = buff->buff_data->start_time;
                m_projectile_initialized = false;
                return;
            }

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "YuumiQCast" ) );
            if ( !buff ) {
                m_q_active = false;
                return;
            }

            if ( !m_missile_found ) {
                for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile || missile->dist_to_local( ) > 400.f ) continue;

                    auto info = missile->missile_spell_info( );
                    if ( !info ) continue;

                    auto data = info->get_spell_data( );
                    if ( !data ) continue;

                    auto name = data->get_name( );
                    if ( rt_hash( name.data( ) ) != ct_hash( "YuumiQCast" ) ) continue;


                    m_missile_found = true;
                    m_missile_index = missile->index;

                    m_projectile_start     = missile->missile_start_position;
                    m_projectile_direction = ( missile->missile_end_position - missile->missile_start_position ).
                        normalize( );

                    std::cout << "[ Q TRACK ] Found missile, spawn time: " << missile->missile_spawn_time( )
                        << " | buff start: " << buff->buff_data->start_time << std::endl;
                    break;
                }

                if ( !m_missile_found ) return;
            }

            auto missile = g_entity_list.get_by_index( m_missile_index );
            if ( !missile ) {
                m_q_active = false;
                return;
            }

            missile.update( );


            if ( !m_projectile_initialized || missile->position != m_missile_position ) {
                m_missile_delta = m_projectile_initialized
                                      ? missile->position - m_missile_position
                                      : m_slot_q->get_details( )->last_unknown_position - missile->position;
                m_missile_position       = missile->position;
                m_projectile_initialized = true;
            }

            //std::cout << "Q Buff time left: " << buff->buff_data->end_time - *g_time << std::endl;
        }

        auto get_projectile_q_time_to_point( const Vec3& target_position ) const -> float{
            const auto inst = get_projectile_position_after_turn( target_position );

            return inst.rotation_duration + inst.end_position.dist_to( target_position ) / 850.f;
        }

        auto get_projectile_position_after_turn( const Vec3& target_position ) const -> projectile_rotation_instance_t{
            if ( !m_projectile_initialized ) return { g_local->position, 0.f, { } };

            const auto position  = m_missile_position;
            const auto direction = m_missile_position.extend( m_missile_position + m_missile_delta, 100.f );

            const auto v1         = direction - position;
            const auto v2         = target_position - position;
            const auto dot        = v1.normalize( ).dot_product( v2.normalize( ) );
            const auto turn_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

            const auto turn_duration = turn_angle / 180.f;

            //std::cout << "Turn angle: " << turn_angle << " | duration: " << turn_duration << std::endl;

            auto                calculated_position{ position };
            auto                calculated_direction{ ( direction - position ).normalize( ) };
            std::vector< Vec3 > simulated_steps{ };

            const auto simulated_angle    = turn_angle / 10.f;
            const auto simulated_time     = turn_duration / 10.f;
            const auto simulated_distance = simulated_time * 425.f;

            const auto temp{ calculated_direction }, temp1{ calculated_direction };

            const auto direction_delta = calculated_position.extend(
                calculated_position + temp.rotated_raw( turn_angle ),
                simulated_distance
            );
            const auto negative_direction = calculated_position.extend(
                calculated_position + temp1.rotated_raw( -turn_angle ),
                simulated_distance
            );

            const auto negative_angle{
                direction_delta.dist_to( target_position ) >
                negative_direction.dist_to( target_position )
            };

            Vec3 end_position{ };
            for ( auto i = 1; i <= 10; i++ ) {
                auto simulated_delta =
                    calculated_position.extend( calculated_position + calculated_direction, simulated_distance );
                //Vec3 rotation = ( calculated_position - simulated_delta ).rotated_raw( simulated_angle );

                auto simulated_product = simulated_delta;

                simulated_steps.push_back( simulated_product );

                calculated_direction = calculated_direction.rotated_raw(
                    negative_angle ? -simulated_angle : simulated_angle
                );
                calculated_position = simulated_product;

                if ( i != 10 ) continue;

                simulated_delta =
                    calculated_position.extend( calculated_position + calculated_direction, simulated_distance );

                simulated_steps.push_back( simulated_delta );
                end_position = simulated_delta;

                break;
            }

            return { end_position, turn_duration, simulated_steps };
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage( m_q_damage[ get_slot_q( )->level ], target->index, false );
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::w:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        Vec3 m_q_last_direction{ };

        // q things
        bool    m_q_active{ };
        bool    m_missile_found{ };
        int16_t m_missile_index{ };

        Vec3  m_missile_delta{ };
        Vec3  m_missile_position{ };
        float m_acceleration_time{ };
        float m_q_start_time{ };
        bool  m_projectile_initialized{ };

        std::vector< Vec3 > m_redirect_points{ };

        Vec3 m_projectile_start{ };
        Vec3 m_projectile_direction{ };

        bool    m_was_q_active{ };
        int16_t m_q_target_index{ };

        // attached
        bool m_is_attached{ };

        std::vector< float > m_q_damage = { 0.f, 80.f, 130.f, 180.f, 230.f, 280.f };


        float m_q_range{ 1150.f };
        float m_w_range{ 700.f };
        float m_e_range{ 0.f };
        float m_r_range{ 1100.f };
    };
} // namespace features::champion_modules
