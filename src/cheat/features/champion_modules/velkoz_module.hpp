#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class velkoz_module final : public IModule {
    public:
        virtual ~velkoz_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "velkoz_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Velkoz" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "velkoz" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto debug      = navigation->add_section( _( "-" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->velkoz.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->velkoz.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->velkoz.q_flee );
            q_settings->select(
                _( "spell mode" ),
                g_config->velkoz.q_mode,
                { _( "Both" ), _( "Indirect only" ), _( "Direct only" ) }
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->velkoz.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "direct max range %" ), g_config->velkoz.q_direct_max_range, 10, 100 );
            q_settings->slider_int( _( "indirect max range %" ), g_config->velkoz.q_split_max_range, 10, 100 );
            q_settings->select(
                _( "indirect range check" ),
                g_config->velkoz.q_split_logic,
                { _( "Disabled" ), _( "Half" ), _( "Full" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->velkoz.w_enabled );
            w_settings->checkbox( _( "flee w" ), g_config->velkoz.w_flee );
            w_settings->select(
                _( "hitchance" ),
                g_config->velkoz.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            w_settings->slider_int( _( "max range %" ), g_config->velkoz.w_max_range, 10, 100 );

            e_settings->checkbox( _( "enable" ), g_config->velkoz.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->velkoz.e_flee );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->velkoz.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "antigapclose" ), g_config->velkoz.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->velkoz.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->velkoz.r_enabled );
            r_settings->select(
                _( "targeting" ),
                g_config->velkoz.r_targeting_mode,
                { _( "Nearest to cursor" ), _( "Automatic" ) }
            );

            //debug->checkbox( _( "" ), g_config->velkoz.q_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->velkoz.q_draw_range );
            drawings->checkbox( _( "draw q extended range" ), g_config->velkoz.q_draw_max_range );
            drawings->checkbox( _( "draw w range" ), g_config->velkoz.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->velkoz.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->velkoz.r_draw_range );
            drawings->checkbox( _( "draw r hitbox area" ), g_config->velkoz.r_draw_hitbox );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->velkoz.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->velkoz.q_draw_range->get< bool >( )
                && !g_config->velkoz.q_draw_max_range->get< bool >( )
                && !g_config->velkoz.w_draw_range->get< bool >( )
                && !g_config->velkoz.e_draw_range->get< bool >( )
                && !g_config->velkoz.r_draw_range->get< bool >( )
                && !m_in_ult
                || g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->velkoz.q_draw_range->get< bool >( ) || g_config->velkoz.q_draw_max_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->velkoz.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    if ( g_config->velkoz.q_draw_max_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            1595.f *
                            ( static_cast< float >( g_config->velkoz.q_split_max_range->get< int >( ) ) / 100.f ),
                            Renderer::outline,
                            75,
                            3.f
                        );
                    }

                    if ( g_config->velkoz.q_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 25, 100, 255, 255 ),
                            1100.f * ( static_cast< float >( g_config->velkoz.q_direct_max_range->get< int >( ) ) /
                                100.f ),
                            Renderer::outline,
                            60,
                            2.f
                        );
                    }
                }
            }

            if ( g_config->velkoz.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->velkoz.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->velkoz.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->velkoz.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->velkoz.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->velkoz.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( m_in_ult ) {
                if ( g_config->velkoz.r_draw_hitbox->get< bool >( ) ) {
                    auto direction = g_local->position + g_local->get_direction( );

                    g_render->rectangle_3d(
                        g_local->position,
                        g_local->position.extend( direction, m_r_range ),
                        85.f + 55.f,
                        Color( 255, 255, 255 ),
                        Renderer::outline,
                        3.f
                    );
                }


                auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                if ( g_config->velkoz.r_targeting_mode->get< int >( ) == 0 )
                    g_render->circle_3d(
                        cursor,
                        Color( 255, 255, 255, 255 ),
                        600.f,
                        Renderer::outline,
                        60,
                        3.f
                    );

                auto target = g_entity_list.get_by_index( m_ult_target_index );
                if ( !target ) return;

                target.update( );

                if ( g_config->velkoz.r_targeting_mode->get< int >( ) == 0 ) {
                    if ( target->position.dist_to( cursor ) <= 600.f ) {
                        g_render->line_3d(
                            cursor,
                            target->position,
                            Color( 255, 255, 25 ),
                            5.f
                        );
                    } else return;
                }

                auto pred = g_features->prediction->predict_default( target->index, 0.2f );
                if ( !pred ) return;

                g_render->line_3d(
                    target->position,
                    target->position.extend(
                        *pred,
                        target->position.dist_to( *pred ) - 20.f > 0.f ? target->position.dist_to( *pred ) - 20.f : 0.f
                    ),
                    Color( 25, 102, 255 ),
                    3.f
                );
                g_render->circle_3d(
                    *pred,
                    Color( 25, 102, 255, 70 ),
                    20.f,
                    Renderer::outline | Renderer::filled,
                    16,
                    2.f
                );
            }

            if ( false && *g_time - m_last_calculation < 1.f ) {
                for ( auto point : m_backward_points )
                    g_render->line_3d(
                        g_local->position,
                        point,
                        Color( 255, 255, 255 ),
                        3.f
                    );

                g_render->rectangle_3d(
                    m_nearest_start,
                    m_nearest_end,
                    90.f,
                    Color( 255, 255, 255 ),
                    Renderer::outline,
                    3.f
                );

                g_render->circle_3d(
                    m_backward_pred,
                    Color( 50, 255, 50, 80 ),
                    15.f,
                    Renderer::outline | Renderer::filled,
                    20,
                    1.f
                );

                //for ( auto point : m_last_split_candidates )
                //   g_render->circle_3d( point, color( 255, 255, 255, 50 ), 10.f, c_renderer::outline | c_renderer::filled, -1, 1.f );
            }

            if ( m_split_active ) {
                auto time_since_cast = *g_time - m_last_split_time;
                if ( time_since_cast > 0.85f ) {
                    m_split_active = false;
                    return;
                }

                g_render->line_3d(
                    m_last_target_position,
                    m_predicted_point,
                    m_good_hitchance ? Color( 50, 255, 50 ) : Color( 255, 255, 50 ),
                    3.f
                );
                Vec2 sp{ };

                if ( world_to_screen( m_predicted_point, sp ) ) {
                    g_render->filled_circle(
                        sp,
                        m_good_hitchance ? Color( 50, 255, 50 ) : Color( 255, 255, 50 ),
                        3.f,
                        20
                    );
                }


                auto position = get_q_position( true );

                auto travel_distance = std::max( ( time_since_cast - 0.25f ) * 2100.f, 0.f );
                if ( travel_distance >= 1100 ) travel_distance = 1100.f;

                auto modifier = std::max( time_since_cast / 0.15f, 0.f );
                if ( modifier > 1.f ) modifier = 1.f;

                //g_render->circle_3d(position, color( 50, 255, 50, 60 ), 25.f, c_renderer::outline | c_renderer::filled, 25, 2.f );
                //g_render->circle_3d(m_cast_missile_position, color( 255, 255, 50, 60 ), 20.f, c_renderer::outline | c_renderer::filled, 25, 2.f );

                /* g_render->circle_3d( m_nearest_raw_point,
                                     color( 10, 125, 255, 60 ),
                                     20.f,
                                     c_renderer::outline | c_renderer::filled,
                                     25,
                                     2.f );*/

                if ( !m_did_print && time_since_cast > 0.25f ) {
                    //std::cout << "Cast delta: " << m_cast_missile_position.dist_to( position ) << std::endl;
                    m_did_print = true;
                }

                auto base_position = position;
                auto delta         = ( base_position - m_start_position ).normalize( );
                auto first_split   = base_position.extend( base_position + delta.rotated_raw( 90.f ), 1100.f );
                auto second_split  = base_position.extend( base_position + delta.rotated_raw( -90.f ), 1100.f );

                auto first_start  = position.extend( first_split, travel_distance );
                auto second_start = position.extend( second_split, travel_distance );

                auto first_hitbox = sdk::math::Rectangle( first_start, first_split, 90.f ).to_polygon(
                    50 - 50 * modifier
                );
                auto second_hitbox = sdk::math::Rectangle( second_start, second_split, 90.f ).to_polygon(
                    50 - 50 * modifier
                );

                for ( auto i = 0; i < first_hitbox.points.size( ); i++ ) {
                    auto start = first_hitbox.points[ i ];
                    auto end   = i == first_hitbox.points.size( ) - 1
                                     ? first_hitbox.points[ 0 ]
                                     : first_hitbox.points[ i + 1 ];

                    auto line_length = start.dist_to( end );

                    auto draw_start = start.extend( end, line_length / 2.f );
                    auto box_color  = Color( 255.f, 255.f, 255.f, 255.f * modifier );

                    g_render->line_3d(
                        draw_start.extend( start, line_length / 2.f ),
                        draw_start.extend( end, line_length / 2.f ),
                        box_color,
                        3.f
                    );
                }


                for ( auto i = 0; i < second_hitbox.points.size( ); i++ ) {
                    auto start = second_hitbox.points[ i ];
                    auto end   = i == second_hitbox.points.size( ) - 1
                                     ? second_hitbox.points[ 0 ]
                                     : second_hitbox.points[ i + 1 ];

                    auto line_length = start.dist_to( end );

                    auto draw_start = start.extend( end, line_length / 2.f );
                    auto box_color  = Color( 255.f, 255.f, 255.f, 255.f * modifier );

                    g_render->line_3d(
                        draw_start.extend( start, line_length / 2.f ),
                        draw_start.extend( end, line_length / 2.f ),
                        box_color,
                        3.f
                    );
                }
            }

            if ( m_spell_detected ) {
                //g_render->circle_3d( get_q_position( ), color( 52, 235, 210 ), 20.f, c_renderer::outline, 32, 2.f );


                //for ( auto point : m_near_points )
                //    g_render->circle_3d( point, color::white( ), 20.f, c_renderer::outline, 16, 2.f );


                /* auto target = g_features->target_selector->get_default_target( );
                if(target) {

                    g_render->line_3d( target->position, m_predicted_point, color( 50, 255, 50 ), 2.f );
                }

                g_render->circle_3d( m_cast_missile_position,
                                     color( 255, 255, 255, 50 ),
                                     25.f,
                                     c_renderer::outline | c_renderer::filled,
                                     28,
                                     2.f );

                g_render->circle_3d( m_nearest_raw_point, color( 255, 255, 50, 50 ), 25.f, c_renderer::outline | c_renderer::filled, 28, 2.f );*/
            }

            if ( !m_q_active ) return;

            auto position = get_q_position( );

            auto hitbox = sdk::math::Rectangle( position, m_end_position, 100.f ).to_polygon( );

            for ( auto i = 0; i < hitbox.points.size( ); i++ ) {
                auto start = hitbox.points[ i ];
                auto end   = i == hitbox.points.size( ) - 1 ? hitbox.points[ 0 ] : hitbox.points[ i + 1 ];

                auto line_length = start.dist_to( end );

                auto draw_start = start.extend( end, line_length / 2.f );
                auto box_color  = Color( 255, 255, 255, 255 );

                g_render->line_3d(
                    draw_start.extend( start, line_length / 2.f ),
                    draw_start.extend( end, line_length / 2.f ),
                    box_color,
                    3.f
                );
            }

            //g_render->rectangle_3d(
            //    m_split_start, m_split_end, 90.f, color( 255, 255, 255 ), c_renderer::outline, 3.f );

            if ( !m_calculated_split ) return;

            auto obj = g_entity_list.get_by_index( m_q_target_index );
            if ( !obj ) return;

            obj.update( );


            //g_render->rectangle_3d(
            //    m_nearest_raw_point, m_nearest_raw_end_point, 90.f, color( 10, 120, 255 ), c_renderer::outline, 3.f );

            m_last_target_position = obj->position;

            g_render->line_3d(
                obj->position,
                m_predicted_point,
                m_good_hitchance ? Color( 50, 255, 50 ) : Color( 255, 255, 25 ),
                3.f
            );

            Vec2 sp{ };
            if ( world_to_screen( m_predicted_point, sp ) ) {
                g_render->filled_circle(
                    sp,
                    m_good_hitchance ? Color( 50, 255, 50 ) : Color( 255, 255, 25 ),
                    3.f,
                    20
                );
            }

            return;

            g_render->circle_3d(
                m_predicted_point,
                Color( 255, 255, 25, 70 ),
                20.f,
                Renderer::outline | Renderer::filled,
                -1,
                2.f
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            track_q_missile( );

            m_was_ulting = m_in_ult;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            m_in_ult = sci && sci->slot == 3;

            if ( g_config->velkoz.r_enabled->get< bool >( ) ) {
                if ( m_in_ult != m_was_ulting ||
                    m_in_ult && m_ult_target_index == 0 && g_function_caller->is_update_chargeable_blocked( ) ||
                    m_in_ult && m_ult_target_index > 0 && !g_function_caller->is_update_chargeable_blocked( ) ) {
                    g_function_caller->set_update_chargeable_blocked( m_in_ult && m_ult_target_index > 0 );
                    std::cout << "[ VELKOZ ] Set chargeable block " << ( m_in_ult && m_ult_target_index > 0 ) <<
                        std::endl;
                }

                if ( m_in_ult ) {
                    spell_r( );
                    return;
                }
            }

            if ( !g_features->evade->is_active( ) && !g_features->orbwalker->in_action( ) ) {
                autointerrupt_e( );
                antigapclose_e( );
            }

            if ( m_spell_detected && m_q_active ) spell_q( );

            if ( g_features->evade->is_active( ) || g_features->orbwalker->in_action( ) || g_features->orbwalker->
                in_attack( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->velkoz.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:

                if ( g_config->velkoz.e_flee->get< bool >( ) ) spell_e( );
                if ( g_config->velkoz.w_flee->get< bool >( ) ) spell_w( );
                if ( g_config->velkoz.q_flee->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }

            //std::cout << "state: " << m_slot_q->get_usable_state( ) << "\nname: " << m_slot_q->get_name( ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->velkoz.q_enabled->get< bool >( ) || m_in_ult ) return false;

            if ( rt_hash( m_slot_q->get_name( ).data( ) ) == ct_hash( "VelkozQSplitActivate" ) ) {
                if ( !m_q_active ) return false;


                if ( m_q_target_index > 0 ) {
                    auto main_target = g_entity_list.get_by_index( m_q_target_index );
                    if ( main_target && q_split_logic( main_target.get( ) ) ) return true;
                }

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->index == m_q_target_index ) return false;

                return q_split_logic( target );
            }

            if ( *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time < 0.1f || m_q_active ||
                !m_slot_q->is_ready( true ) )
                return false;

            auto allow_q{ g_config->velkoz.q_mode->get< int >( ) != 1 };

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range * 2.f,
                1300.f,
                45.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->velkoz.q_hitchance->
                get< int >( ) ) )
                return false;

            auto direct_range = m_q_range * ( g_config->velkoz.q_direct_max_range->get< int >( ) / 100.f );

            if ( pred.position.dist_to( g_local->position ) > direct_range ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 50.f ) )
                allow_q = false;

            if ( allow_q ) {
                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->on_cast( );
                    m_last_q_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }

                return false;
            }

            if ( g_config->velkoz.q_mode->get< int >( ) == 2 ) return false;

            const auto move_direction = target->position - g_local->position;
            const auto base_direction = g_local->position + move_direction;

            auto direction = ( base_direction - g_local->position ).normalize( ).rotated_raw( -45.f );

            auto points             = g_render->get_3d_circle_points( g_local->position, 800.f, 10, 90, direction );
            m_last_split_candidates = points;
            m_last_calculation      = *g_time;

            auto bounding_radius = g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) );

            Vec3 best_direction{ };
            auto best_split_length{ 9999.f };
            auto best_travel_time{ 100.f };

            Vec3 split_point{ };
            Vec3 hit_point{ };

            auto simulation_range = 1090.f * ( static_cast< float >( g_config->velkoz.q_split_max_range->get<
                int >( ) ) / 100.f );
            auto ping         = g_features->orbwalker->get_ping( );
            auto compensation = 0.f; // 20.f / target->movement_speed;

            if ( false && g_config->velkoz.q_split_logic->get< int >( ) == 1 ) {
                std::vector< Vec3 > candidates{ };
                std::vector< Vec3 > split_path{ };

                for ( auto i = 0; i < 2; i++ ) {
                    auto backward_dir = ( base_direction - g_local->position ).normalize( ).rotated(
                        i == 0 ? -0.8f : 0.8f
                    );

                    auto missile_start{ g_local->position };
                    auto missile_end = missile_start.extend( missile_start + backward_dir, 1100.f );
                    candidates.push_back( missile_end );

                    auto near_point =
                        g_features->evade->get_closest_line_point( missile_start, missile_end, target->position );
                    auto time_to_point = 0.5f + missile_start.dist_to( near_point ) / 1300.f;


                    auto predict = g_features->prediction->predict(
                        target->index,
                        1100.f,
                        2100.f,
                        0.f,
                        time_to_point,
                        near_point
                    );
                    if ( !predict.valid ) continue;

                    near_point = g_features->evade->get_closest_line_point(
                        missile_start,
                        missile_end,
                        predict.position
                    );
                    time_to_point = 0.5f + missile_start.dist_to( predict.position ) / 1300.f;

                    predict = g_features->prediction->predict(
                        target->index,
                        1100.f,
                        2100.f,
                        45.f,
                        time_to_point,
                        near_point,
                        true
                    );
                    if ( !predict.valid ) continue;

                    near_point = g_features->evade->get_closest_line_point(
                        missile_start,
                        missile_end,
                        predict.position
                    );
                    time_to_point        = 0.5f + missile_start.dist_to( predict.position ) / 1300.f;
                    auto collision_point = g_features->evade->get_closest_line_point(
                        near_point,
                        near_point.extend( predict.position, 1100.f ),
                        predict.position
                    );

                    if ( g_features->prediction->minion_in_line( missile_start, near_point, 75.f ) ||
                        g_features->prediction->minion_in_line( near_point, collision_point, 90.f ) )
                        continue;

                    auto travel_time = time_to_point + near_point.dist_to( collision_point ) / 2100.f;

                    if ( travel_time > best_travel_time ) continue;

                    best_direction   = missile_end;
                    best_travel_time = travel_time;

                    m_nearest_start = near_point;
                    m_nearest_end   = near_point.extend( collision_point, 1100.f );
                    m_backward_pred = pred.position;
                }

                m_backward_points = candidates;
            } else {
                for ( const auto point : points ) {
                    auto missile_start = g_local->position;
                    auto missile_end   = g_local->position.extend( point, 1050.f );
                    auto loop_amount{ 10 };

                    if ( g_features->prediction->minion_in_line( missile_start, missile_end, 60.f ) ) continue;

                    if ( g_config->velkoz.q_split_logic->get< int >( ) > 0 ) {
                        auto target_move_time = g_config->velkoz.q_split_logic->get< int >( ) == 2 ? 0.9f : 0.50f;
                        auto possible_target_position = g_local->position.extend(
                            target->position,
                            g_local->position.dist_to( target->position ) + target_move_time * target->movement_speed
                        );

                        auto rect = sdk::math::Rectangle( missile_start, missile_end, simulation_range );
                        auto poly = rect.to_polygon( );

                        if ( poly.is_outside( possible_target_position ) ) continue;
                    }

                    for ( auto i = 1; i <= loop_amount; i++ ) {
                        auto time_traveled = 0.25f + 1050.f / loop_amount * i / 1300.f;
                        auto position      = missile_start.extend( missile_end, 1050.f / loop_amount * i );
                        if ( time_traveled <= 0.55f ) continue;

                        auto base_position = position;
                        auto delta         = ( base_position - g_local->position ).normalize( );
                        auto first_split   = base_position.extend( base_position + delta.rotated_raw( 90.f ), 1100.f );
                        auto second_split  = base_position.extend( base_position + delta.rotated_raw( -90.f ), 1100.f );

                        bool found_hit{ };
                        auto hit_delay{ 9999.f };
                        auto split_length{ 9999.f };
                        Vec3 collision_coordinates{ };

                        auto meta_loop_amount{ 10 };
                        for ( auto o = 1; o <= meta_loop_amount; o++ ) {
                            auto travel_time =
                                time_traveled + 0.25f + simulation_range / meta_loop_amount * o / 2100.f;
                            auto hitbox_one =
                                sdk::math::Rectangle(
                                    position,
                                    position.extend( first_split, simulation_range ),
                                    45.f + static_cast< int >( bounding_radius * 0.25f )
                                )
                                .to_polygon( );
                            auto hitbox_two =
                                sdk::math::Rectangle(
                                    position,
                                    position.extend( second_split, simulation_range ),
                                    45.f + static_cast< int >( bounding_radius * 0.25f )
                                )
                                .to_polygon( );

                            auto predicted = g_features->prediction->predict_default(
                                target->index,
                                travel_time + ping - compensation
                            );
                            if ( !predicted ) continue;

                            if ( hitbox_one.is_outside( *predicted ) && hitbox_two.is_outside( *predicted ) ) continue;

                            auto is_first = hitbox_one.is_inside( *predicted );

                            if ( g_features->prediction->minion_in_line(
                                position,
                                is_first ? first_split : second_split,
                                70.f
                            ) )
                                break;

                            found_hit             = true;
                            collision_coordinates = g_features->evade->get_closest_line_point(
                                position,
                                is_first ? first_split : second_split,
                                *predicted
                            );
                            hit_delay    = simulation_range / meta_loop_amount * o / 2100.f;
                            split_length = position.dist_to( collision_coordinates ) < 200.f
                                               ? 200.f
                                               : position.dist_to( collision_coordinates );
                            break;
                        }

                        if ( !found_hit ) continue;

                        if ( best_direction.length( ) <= 0.f || best_split_length > split_length ) {
                            best_direction = missile_end;
                            hit_point      = collision_coordinates;
                            split_point    = position;
                            //total_travel_time = time_traveled + hit_delay;
                            //split_travel_time = time_traveled;
                            best_split_length = split_length;
                            break;
                        }
                    }
                }
            }

            if ( best_direction.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::q, best_direction ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                m_last_cast_direction = best_direction;
                m_last_cast_position  = g_local->position;
                m_q_target_index      = target->index;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->velkoz.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || m_in_ult || !m_slot_w->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_w( target ) ) return true;
            }


            return false;
        }

        auto combo_w( Object* target ) -> bool{
            auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_w_range * ( g_config->velkoz.w_max_range->get< int >( ) / 100.f ),
                    1700.f,
                    70.f,
                    0.25f,
                    { },
                    true,
                    Prediction::include_ping,
                    Prediction::ESpellType::linear
                );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->velkoz.w_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->velkoz.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.1f || m_in_ult || !m_slot_e->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_e( target ) ) return true;
            }

            return false;
        }

        auto combo_e( Object* target ) -> bool{
            if ( !target ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                225.f,
                1.f,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->velkoz.e_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !m_in_ult || !g_config->velkoz.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.025f )
                return false;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto       target{
                g_config->velkoz.r_targeting_mode->get< int >( ) == 1
                    ? g_features->target_selector->get_default_target( )
                    : nullptr
            };
            auto lowest_cursor_distance{ FLT_MAX };

            if ( g_config->velkoz.r_targeting_mode->get< int >( ) == 0 ) {
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || g_local->position.dist_to( enemy->position ) > m_r_range ||
                        cursor.dist_to( enemy->position ) > 600.f ||
                        g_features->target_selector->is_bad_target( enemy->index ) )
                        continue;

                    const auto cursor_distance = cursor.dist_to( enemy->position );

                    if ( cursor_distance > lowest_cursor_distance ) continue;

                    target                 = enemy;
                    lowest_cursor_distance = cursor_distance;
                }
            }

            if ( !target ) {
                m_ult_target_index = 0;

                return false;
            }

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 0.f, 0.2f );
            if ( !pred.valid ) return false;

            if ( release_chargeable( ESpellSlot::r, pred.position, false ) ) {
                m_last_cast_time   = *g_time;
                m_ult_target_index = target->index;
                return true;
            }

            return false;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->velkoz.e_autointerrupt->get< bool >( ) || m_in_ult ||
                *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_e_range, 0.f, 225.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance < 2 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ VELKOZ ] Autointerrupt E on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->velkoz.e_antigapclose->get< bool >( ) || m_in_ult ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 0.f, 225.f, 1.f );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_e_range, 0.f, 225.f, 1.f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ VELKOZ ] Antigapclose E on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto q_split_logic( Object* target ) -> bool{
            if ( !target || target->index == 0 ) return false;

            auto       position        = get_q_position( );
            const auto future_position = get_q_position( true, true );

            bool found_collision{ };

            auto bounding_radius = g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) );

            auto missile_width = 45.f;
            auto reduction     = 0.f; // 55.f;
            auto nearest_split =
                g_features->evade->get_closest_line_point( m_start_position, m_end_position, target->position );
            // if ( nearest_split.dist_to( m_start_position ) < position.dist_to( m_start_position ) )
            //     nearest_split = position;

            auto base_position = nearest_split;
            auto delta         = ( base_position - m_start_position ).normalize( );
            auto split         = base_position.extend( base_position + delta.rotated_raw( 90.f ), 1100.f );
            auto alt_split     = base_position.extend( base_position + delta.rotated_raw( -90.f ), 1100.f );

            if ( target->position.dist_to( nearest_split ) > 1200.f ) return false;

            auto split_end =
                split.dist_to( target->position ) < alt_split.dist_to( target->position ) ? split : alt_split;

            auto primitive_collision =
                g_features->evade->get_closest_line_point( nearest_split, split_end, target->position );

            auto time_to_point = 0.25f + future_position.dist_to( nearest_split ) / 1300.f +
                nearest_split.dist_to( primitive_collision ) / 2100.f;

            auto pred = g_features->prediction->predict(
                target->index,
                1300.f,
                0.f,
                0.f,
                time_to_point,
                nearest_split,
                false,
                Prediction::include_ping
            );
            if ( !pred.valid ) return false;

            auto is_good_hitchance{ static_cast< int >( pred.hitchance ) > 2 };
            auto is_high_hitchance{ static_cast< int >( pred.hitchance ) > 1 };

            nearest_split = g_features->evade->get_closest_line_point(
                m_start_position,
                m_end_position,
                pred.position
            );

            base_position = nearest_split;
            delta         = ( base_position - m_start_position ).normalize( );
            split         = base_position.extend( base_position + delta.rotated_raw( 90.f ), 1100.f );
            alt_split     = base_position.extend( base_position + delta.rotated_raw( -90.f ), 1100.f );
            split_end     = split.dist_to( pred.position ) <
                            alt_split.dist_to( pred.position )
                                ? split
                                : alt_split;

            auto collision_point = g_features->evade->get_closest_line_point(
                nearest_split,
                split_end,
                pred.position
            );

            time_to_point = 0.25f + ( is_good_hitchance ? future_position.dist_to( nearest_split ) / 1300.f : 0.f ) +
                ( nearest_split.dist_to( collision_point ) - reduction ) / 2100.f;

            pred = g_features->prediction->predict(
                target->index,
                1300.f,
                0.f,
                0.f,
                time_to_point,
                nearest_split,
                true
            );

            if ( !pred.valid || g_features->prediction->minion_in_line( nearest_split, collision_point, 45.f ) ) {
                return
                    false;
            }

            auto compensation = 45.f + bounding_radius / 2.f;

            auto base  = future_position.extend( m_start_position, compensation );
            auto delt  = ( base - m_start_position ).normalize( );
            auto path1 = base.extend( base + delt.rotated_raw( 90.f ), 1100.f );
            auto path2 = base.extend( base + delt.rotated_raw( -90.f ), 1100.f );

            auto hitbox_end = path1.dist_to( target->position ) < path2.dist_to( target->position ) ? path1 : path2;

            if ( target->index == m_q_target_index || m_q_target_index == 0 ) {
                m_split_start = base;
                m_split_end   = hitbox_end;

                m_good_hitchance        = is_high_hitchance;
                m_predicted_point       = pred.position;
                m_nearest_raw_point     = nearest_split;
                m_nearest_raw_end_point = split_end;

                if ( m_q_target_index == 0 ) m_q_target_index = target->index;

                m_calculated_split = true;
            }

            auto rect = sdk::math::Rectangle(
                base,
                hitbox_end,
                is_good_hitchance ? 45.f + bounding_radius * 0.25f : 45.f + bounding_radius
            );
            auto poly = rect.to_polygon( );

            if ( poly.is_inside( pred.position ) ) {
                found_collision = true;
                // distance_to_pred = position.dist_to( nearest_split );

                m_predicted_point   = pred.position;
                m_nearest_raw_point = nearest_split;
                m_found_point       = true;
                m_q_target_index    = target->index;
            } else if ( !m_found_point ) m_previous_distance = nearest_split.dist_to( position );

            if ( !found_collision || *g_time - m_last_q_time <= 0.2f || !m_slot_q->is_ready( ) ) return false;


            if ( cast_spell( ESpellSlot::q ) ) {
                // std::cout << "[ VELKOZ ] Q Splitted | pred distance: " << distance_to_pred
                //           << " | previous dist: " << m_previous_distance << std::endl;
                // std::cout << "cast ping: " << static_cast< int >( g_features->orbwalker->get_ping( ) * 1000.f )
                //           << std::endl;

                m_last_q_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_last_split_time = *g_time;

                m_cast_missile_position = future_position;
                m_found_point           = false;
                m_q_target_index        = target->index;

                return true;
            }

            return false;
        }

        auto track_q_missile( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 0 || sci->is_autoattack ) return;

                m_q_active         = true;
                m_server_cast_time = sci->server_cast_time;
                m_start_position   = sci->start_position;
                m_end_position     = sci->start_position.extend( sci->end_position, 1155.f );
                m_spell_detected   = false;
                m_did_print        = false;

                m_calculated_split = false;

                if ( m_q_target_index == 0 ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( !target ) return;

                    m_q_target_index = target->index;
                }

                return;
            }

            if ( *g_time - m_server_cast_time > 1.f ) {
                m_q_active            = false;
                m_last_split_position = get_q_position( );
                m_last_tracking_time  = *g_time;
                m_q_target_index      = 0;

                return;
            }

            if ( rt_hash( m_slot_q->get_name( ).data( ) ) == ct_hash( "VelkozQSplitActivate" ) ) {
                if ( !m_spell_detected ) m_spell_detected = true;

                m_last_split_position = get_q_position( );

                return;
            }

            if ( m_spell_detected ) {
                m_last_tracking_time = *g_time;
                m_q_active           = false;
                m_split_active       = true;
                m_last_split_time    = *g_time;
                m_q_target_index     = 0;
            }
        }

        auto get_q_position( const bool next_tick = false, const bool include_ping = false ) -> Vec3{
            if ( !m_q_active ) return m_simulated_split;

            const auto time_traveled     = std::max( *g_time - m_server_cast_time, 0.f );
            auto       distance_traveled = time_traveled * 1300.f + (
                next_tick && *g_time > m_server_cast_time ? 55.f : 0.f );

            if ( include_ping && *g_time > m_server_cast_time ) {
                distance_traveled += g_features->orbwalker->get_ping( )
                    * 1300.f;
            }

            if ( distance_traveled > 1155.f ) distance_traveled = 1155.f;

            if ( next_tick ) return m_start_position.extend( m_end_position, distance_traveled );

            m_simulated_split = m_start_position.extend( m_end_position, distance_traveled );

            return m_simulated_split;
        }

        auto stop_tracking_q( ) -> void{ m_q_active = false; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1700.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1700.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_cast_time{ };
        float m_last_calculation{ };

        // split
        float m_last_split_time{ };
        bool  m_split_active{ };

        std::vector< Vec3 > m_last_split_candidates{ };

        // new q logic
        std::vector< Vec3 > m_backward_points{ };
        std::vector< Vec3 > m_split_path{ };

        Vec3 m_nearest_start{ };
        Vec3 m_nearest_end{ };
        Vec3 m_backward_pred{ };

        Vec3 m_last_target_position{ };

        int16_t m_q_target_index{ };

        // r
        bool m_refreshing{ };

        // q tracking
        bool  m_q_active{ };
        float m_server_cast_time{ };

        Vec3 m_start_position{ };
        Vec3 m_end_position{ };

        Vec3 m_simulated_split{ };
        Vec3 m_simulated_hit{ };

        Vec3 m_predicted_point{ };
        int  m_good_hitchance{ };

        Vec3 m_nearest_raw_point{ };
        Vec3 m_nearest_raw_end_point{ };

        Vec3 m_split_end{ };
        Vec3 m_split_start{ };

        bool m_calculated_split{ };

        float m_previous_distance{ };
        bool  m_found_point{ };

        Vec3 m_cast_missile_position{ };

        bool m_did_print{ };

        std::vector< Vec3 > m_near_points{ };

        float m_last_tracking_time{ };
        Vec3  m_last_split_position{ };

        std::vector< Vec3 > m_split_one_points{ };
        std::vector< Vec3 > m_split_two_points{ };
        Vec3                m_split_prediction{ };

        Vec3 m_last_cast_direction{ };
        Vec3 m_last_cast_position{ };

        bool m_spell_detected{ };

        // ult
        bool m_in_ult{ };
        bool m_was_ulting{ };

        int16_t m_ult_target_index{ };

        std::array< float, 6 > m_q_damage = { 0.f, 75.f, 100.f, 125.f, 150.f, 175.f };

        float m_q_range{ 1100.f };
        float m_w_range{ 1100.f };
        float m_e_range{ 800.f };
        float m_r_range{ 1600.f };
    };
}
