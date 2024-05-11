#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class ZoeModule final : public IModule {
    public:
        virtual ~ZoeModule( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "zoe_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Zoe" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "zoe" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto magnet     = navigation->add_section( _( "magnet" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto misc       = navigation->add_section( _( "misc" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->zoe.q_enabled );
            q_settings->checkbox( _( "only redirect (?)" ), g_config->zoe.q_only_redirect )->set_tooltip(
                _( "Will only cast Q2" )
            );

            q_settings->checkbox( _( "delay cast (?)" ), g_config->zoe.q_delay_cast )->set_tooltip(
                _( "Delay Q2 to let Q1 reach maximum extension" )
            );

            q_settings->checkbox( _( "override hitchance (?)" ), g_config->zoe.q_override_hitchance )->set_tooltip(
                _( "override hitchance if Q2 will end soon" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->zoe.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            magnet->checkbox( _( "enable q magnet" ), g_config->zoe.q_magnet );
            magnet->slider_int( _( "max magnet distance" ), g_config->zoe.q_magnet_distance, 0, 500, 10 );

            misc->checkbox( _( "hide e animation (hotkey)" ), g_config->zoe.e_hide_animation );

            w_settings->checkbox( _( "enable" ), g_config->zoe.w_enabled );
            //w_settings->select(_("hitchance"), g_config->chogath.w_hitchance, { _("Low"), _("Medium"), _("High"), _("Very high"), _("Immobile") });

            e_settings->checkbox( _( "enable" ), g_config->zoe.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->zoe.e_flee );
            e_settings->checkbox( _( "flee dont extend with walls" ), g_config->zoe.e_flee_only_direct );
            e_settings->checkbox( _( "antigapcloser" ), g_config->zoe.e_antigapcloser );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->zoe.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "extend range with walls" ), g_config->zoe.e_through_walls );
            e_settings->select(
                _( "hitchance" ),
                g_config->zoe.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //r_delay_cast
            r_settings->checkbox( _( "enable (?)" ), g_config->zoe.r_enabled )->set_tooltip(
                _( "will be used to extend Q2 range" )
            );
            r_settings->select(
                _( "allow underturret" ),
                g_config->zoe.r_underturret,
                { _( "Never" ), _( "In full combo" ), _( "Always" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->zoe.q_draw_range );
            drawings->checkbox( _( "draw q extend range" ), g_config->zoe.q_draw_extend_range );
            drawings->checkbox( _( "draw e range" ), g_config->zoe.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->zoe.dont_draw_on_cooldown );

            misc->checkbox( _( "cast e before q in full combo" ), g_config->zoe.e_priority_combo );
            misc->checkbox( _( "aa only if passive ready" ), g_config->zoe.aa_passive_check );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->zoe.q_draw_range->get< bool >( ) || g_config->zoe.q_draw_extend_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zoe.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( g_config->zoe.q_draw_extend_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 3, 227, 255, 255 ),
                            m_q_extended_range,
                            Renderer::outline,
                            70,
                            2.f
                        );
                    }

                    if ( g_config->zoe.q_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            m_q_range,
                            Renderer::outline,
                            50,
                            2.f
                        );
                    }
                }
            }

            if ( g_config->zoe.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zoe.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( m_sleep_found ) {
                g_render->circle_3d(
                    m_sleep_position,
                    Color( 255, 255, 255, 70 ),
                    75.f,
                    Renderer::outline | Renderer::filled,
                    32,
                    5.f
                );
            }

            /*if (should_draw_e)
            {
                g_render->circle_3d(e_wall_start, color(50, 255, 50), 60.f, 2, 50, 2.f);
                g_render->circle_3d(e_wall_end, color(255, 50, 50), 60.f, 2, 50, 2.f);
                g_render->circle_3d(e_spell_end_position, color(25, 100, 255), 60.f, 2, 50, 2.f);
            }*/

            /* if ( m_in_portal ) {

               std::string text = std::to_string( *g_time - ( m_ult_start_time - 0.4f ) );
               text.resize( 4 );

               g_render->text_3d(
                   m_portal_position, color( 255, 255, 255 ), g_fonts->get_zabel( ), text.data( ), 20, true );
               //g_render->circle_3d( m_portal_position, color( 25, 255, 25 ), 70.f, 2, 50, 2.f );
           }*/

            //if (m_q1_detected) g_render->circle_3d(m_missile_source_position, color(25, 255, 25), 30.f, 2, 50, 2.f);

            //auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            //auto ult_pos = get_ult_position( g_local->position.extend( cursor, 575.f ) );
            //g_render->circle_3d( cursor, color( 255, 255, 255 ), 50.f, c_renderer::outline, 16, 2.f );
            //g_render->circle_3d( ult_pos, color( 25, 255, 25 ), 75.f, c_renderer::outline, 16, 2.f );
            //g_render->line_3d( ult_pos, cursor, color( 255, 255, 25, 100 ), 3.f );

            //for ( auto point : m_portal_points )
            //    g_render->circle_3d( point, color( 255, 255, 255 ), 25.f, c_renderer::outline, 16, 3.f );

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                if ( m_magnet_active ) {
                    g_render->text_shadow(
                        { sp.x - 20.f, sp.y - 24.f },
                        Color( 167, 66, 245 ),
                        g_fonts->get_bold( ),
                        "Magnet",
                        16
                    );
                }


                sp.x += 30.f;
                sp.y -= 20.f;

                auto full_combo = GetAsyncKeyState( VK_CONTROL ) && g_features->orbwalker->get_mode( ) ==
                    Orbwalker::EOrbwalkerMode::combo;

                std::string text1{ "Q LOGIC: " };
                std::string text2{ "[ F1 ] HIDE E ANIMATION" };


                std::string text_mode{ full_combo ? "COMBO" : "DAMAGE" };

                auto size = g_render->get_text_size( text1, g_fonts->get_block( ), 8 );

                g_render->text_shadow(
                    sp,
                    Color( 255, 255, 255 ),
                    g_fonts->get_block( ),
                    text1.c_str( ),
                    8
                );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    full_combo ? Color( 50, 255, 50 ) : Color( 255, 255, 50 ),
                    g_fonts->get_block( ),
                    text_mode.c_str( ),
                    8
                );

                sp.y += 8.f;

                g_render->text_shadow(
                    sp,
                    g_config->zoe.e_hide_animation->get< bool >( ) ? Color( 25, 255, 25 ) : Color( 255, 50, 50 ),
                    g_fonts->get_block( ),
                    text2.c_str( ),
                    8
                );
            }

            if ( m_draw_magnet_area ) {
                g_render->circle_3d(
                    m_magnet_goal,
                    Color( 255, 255, 255 ),
                    m_magnet_radius,
                    Renderer::outline,
                    30,
                    2.f,
                    33.f,
                    ( g_local->position - m_magnet_goal ).normalize( ).rotated( -0.33f )
                );


                if ( *g_time - m_last_q_recast_time > 1.f && m_q_flying || !m_q1_detected && !
                    m_q_flying )
                    m_draw_magnet_area = false;
            }

            if ( m_q_flying ) {
                update_missile_trajectory( );

                //g_render->circle_3d( simulated_end, color( 255, 25, 25 ), 50.f, c_renderer::outline, 16, 3.f );
                //g_render->circle_3d( m_target_pred, color( 50, 255, 50, 50 ), 20.f, c_renderer::outline | c_renderer::filled, 32, 2.f );

                //m_raw_missile_end

                auto simulated_end    = m_predicted_end;
                auto missile_position = get_q2_position( );

                auto time_left{ missile_position.dist_to( m_predicted_end ) / 2500.f };
                auto extend_distance = time_left * g_local->movement_speed;
                if ( m_in_portal ) extend_distance = 0.f;

                if ( m_cast_delayed_r || *g_time - m_last_r_time <= 0.25f && !m_in_portal ) extend_distance += 575.f;

                missile_position.y = simulated_end.y;

                auto rectangle = sdk::math::Rectangle(
                    missile_position,
                    missile_position.extend(
                        simulated_end,
                        simulated_end.dist_to( missile_position ) + extend_distance
                    ),
                    70.f
                );

                auto current_rectangle = sdk::math::Rectangle(
                    missile_position,
                    missile_position.extend( simulated_end, simulated_end.dist_to( missile_position ) ),
                    70.f
                );

                auto time_since_start{ *g_time - m_last_q_time };
                auto modifier = std::max( time_since_start / 0.15f, 0.f );
                if ( modifier > 1.f ) modifier = 1.f;

                auto poly         = rectangle.to_polygon( 35 - 35 * modifier );
                auto current_poly = current_rectangle.to_polygon( 35 - 35 * modifier );

                for ( size_t i = 0u; i < poly.points.size( ); i++ ) {
                    const auto start_point = poly.points[ i ];
                    const auto end_point   = i == poly.points.size( ) - 1 ? poly.points[ 0 ] : poly.points[ i + 1 ];

                    Vec2 sp_start;
                    Vec2 sp_end;

                    if ( !sdk::math::world_to_screen( start_point, sp_start ) || !sdk::math::world_to_screen(
                        end_point,
                        sp_end
                    ) )
                        continue;

                    g_render->line(
                        sp_start,
                        sp_end,
                        Color( 255.f, 255.f, 255.f, 255.f * modifier ),
                        3.f
                    );
                }

                //g_render->circle_3d(
                //    m_target_pred, color( 255, 50, 50, 50 ), 20.f, c_renderer::outline | c_renderer::filled, 32, 2.f );

                return; // if ( !m_current_trajectory_valid ) return;

                for ( size_t i = 0u; i < current_poly.points.size( ); i++ ) {
                    const auto start_point = current_poly.points[ i ];
                    const auto end_point   =
                        i == current_poly.points.size( ) - 1 ? current_poly.points[ 0 ] : current_poly.points[ i + 1 ];

                    Vec2 sp_start;
                    Vec2 sp_end;

                    if ( !sdk::math::world_to_screen( start_point, sp_start ) ||
                        !sdk::math::world_to_screen( end_point, sp_end ) )
                        continue;

                    g_render->line(
                        sp_start,
                        sp_end,
                        Color( 25.f, 255.f, 25.f, 255.f * modifier ),
                        3.f
                    );
                }
            } else if ( m_q1_detected ) {
                //g_render->line_3d( get_q1_position( ), m_cast_end_position, color( 255, 255, 255 ), 3.f );

                auto start_time = m_q_cast_end - 0.25f;
                auto modifier   = std::max(
                    ( m_q_reach_end_time - *g_time ) / ( m_q_reach_end_time - start_time ),
                    0.f
                );
                if ( modifier > 1.f ) modifier = 1.f;

                int opacity = 255 - 255 * modifier;

                g_render->circle_3d(
                    m_cast_end_position,
                    m_q_reach_end_time <= *g_time
                        ? Color( 25, 255, 25, 255 )
                        : Color( opacity, opacity, opacity, opacity ),
                    500.f * modifier + 70.f,
                    Renderer::outline,
                    32,
                    3.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            track_data( );
            missile_path_tracking( );
            update_sleep_object( );

            m_full_combo_active = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                GetAsyncKeyState( VK_CONTROL );

            if ( !m_e_detected && *g_time - m_last_e_time > 0.25f && m_hide_animation ) m_hide_animation = false;

            if ( !g_features->evade->is_active( ) ) {
                antigapclose_e( );
                autointerrupt_e( );
                magnet_q( );
            }

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                combo_qr( );
                harass_q( ); // dynamic hitchance, check even during flight

                spell_w( );

                spell_e( );
                if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) || g_features->orbwalker->
                    in_attack( ) )
                    return;

            //spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                harass_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                harass_q( );
                spell_w( );

                if ( g_features->evade->is_active( ) ) return;

                if ( g_config->zoe.e_flee->get< bool >( ) ) spell_e( );
                break;
            default:
                break;
            }

            return;

            auto name = m_slot_w->get_name( );
            if ( rt_hash( name.c_str( ) ) == rt_hash( m_last_w_name.data( ) ) ) return;

            std::cout << "SPELL W Changed: " << name << std::endl;
            m_last_w_name = name;

            // zoepassivesheenbuff || aa buff
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto combo_qr( ) -> bool{
            if ( !g_config->zoe.q_enabled->get< bool >( ) || m_q_flying || *g_time - m_last_q_time <= 0.25f ||
                !m_slot_q->is_ready( ) || !g_config->zoe.r_enabled->get< bool >( ) ||
                !m_slot_r->is_ready( true ) ) {
                m_should_extend_range = false;
                return false;
            }

            const auto has_ult_access = g_config->zoe.r_enabled->get< bool >( ) && m_slot_r->is_ready( true );
            if ( !has_ult_access ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( rt_hash( m_slot_q->get_name( ).c_str( ) ) == ct_hash( "ZoeQ" ) ) {
                if ( g_config->zoe.q_only_redirect->get< bool >( ) || m_in_portal ||
                    g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ||
                    g_features->orbwalker->in_attack( ) || !m_slot_q->is_ready( true ) )
                    return false;

                if ( g_config->zoe.e_priority_combo->get< bool >( ) &&
                    m_full_combo_active && m_slot_e->is_ready( true ) )
                    return false;

                const auto pred       = g_features->prediction->predict_default( target->index, 0.25f );
                const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.25f );
                if ( !pred || !local_pred ) return false;

                if ( local_pred.value( ).dist_to( *pred ) > 1400.f ||
                    g_local->position.dist_to( target->position ) > 1400.f )
                    return false;

                const auto possible_source_position = g_local->position.extend( target->position, -800.f );
                if ( g_features->prediction->minion_in_line( possible_source_position, target->position, 80.f ) ) {
                    return
                        false;
                }

                if ( cast_spell( ESpellSlot::q, possible_source_position ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            }

            if ( !m_q1_detected ) return false;

            auto       hitchance = g_config->zoe.q_hitchance->get< int >( );
            const auto buff      = g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZoeQ" ) );
            if ( !buff || buff->buff_data->end_time - *g_time <= 0.25f ) return false;


            if ( g_config->zoe.q_override_hitchance->get< bool >( )
                && ( buff->buff_data->end_time - *g_time <= 0.3f || m_in_portal && m_ult_return_time - *g_time <=
                    0.25f ) )
                hitchance = 0;

            const auto missile_start_position = get_q1_position( );

            auto pred =
                g_features->prediction->predict(
                    target->index,
                    25000.f,
                    2500.f,
                    70.f,
                    0.15f,
                    missile_start_position,
                    false,
                    Prediction::include_ping,
                    Prediction::ESpellType::linear
                );

            if ( !pred.valid ||
                g_features->prediction->minion_in_line( missile_start_position, pred.position, 80.f ) )
                return false;

            const auto damage        = get_q_damage( target->index, missile_start_position.dist_to( pred.position ) );
            const auto can_killsteal = damage > helper::get_real_health( target->index, EDamageType::magic_damage );


            if ( static_cast< int >( pred.hitchance ) < hitchance && !can_killsteal ) return false;

            if ( g_config->zoe.q_delay_cast->get< bool >( ) && !can_killsteal && !m_full_combo_active &&
                *g_time + g_features->orbwalker->get_ping( ) < m_q_reach_end_time )
                return false;

            const auto local_pred   = g_features->prediction->predict_default( g_local->index, 0.25f );
            const auto dist_to_pred = local_pred.has_value( )
                                          ? pred.position.dist_to( *local_pred )
                                          : pred.position.dist_to( g_local->position );

            if ( dist_to_pred <= m_q_range ) return false;

            const auto allow_cast{ dist_to_pred < m_q_extended_range };

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;

                m_cast_delayed_r    = true;
                m_highest_hitchance = can_killsteal ? 5 : static_cast< int >( pred.hitchance );

                m_missile_start      = get_q1_position( );
                m_missile_end        = pred.position;
                m_predicted_end      = m_missile_end;
                m_raw_missile_start  = m_missile_start;
                m_raw_missile_end    = m_missile_end;
                m_local_end_distance = 0.f;

                m_q_flying           = true;
                m_q_target_index     = target->index;
                m_q_valid_trajectory = true;

                return true;
            }

            return false;
        }

        auto harass_q( ) -> bool{
            if ( !g_config->zoe.q_enabled->get< bool >( ) || m_q_flying || *g_time - m_last_q_time <= 0.25f ||
                !m_slot_q->is_ready( ) )
                return false;

            auto       has_ult_access{ g_config->zoe.r_enabled->get< bool >( ) && m_slot_r->is_ready( true ) };
            const auto expecting_to_ult{ false }; //{ GetAsyncKeyState( VK_CONTROL ) && has_ult_access &&
            //g_features->orbwalker->get_mode(  ) == Orbwalker::e_orbwalker_mode::combo };

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( rt_hash( m_slot_q->get_name( ).c_str( ) ) == ct_hash( "ZoeQ" ) ) {
                if ( g_config->zoe.q_only_redirect->get< bool >( ) || m_in_portal
                    || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                    g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                    g_features->evade->is_active( ) || !m_slot_q->is_ready( true ) )
                    return false;

                if ( g_config->zoe.e_priority_combo->get< bool >( ) && m_full_combo_active &&
                    m_slot_e->is_ready( true ) )
                    return false;

                const auto is_fastcombo{ false };

                const auto local_prediction_time  = 0.25f;
                auto       target_prediction_time = 0.25f;
                if ( expecting_to_ult ) target_prediction_time = 0.5f;

                const auto pred = g_features->prediction->predict_default( target->index, target_prediction_time );
                const auto local_pred = g_features->prediction->predict_default(
                    g_local->index,
                    local_prediction_time
                );
                if ( !pred || !local_pred ) return false;

                if ( !expecting_to_ult && local_pred.value( ).dist_to( *pred ) > m_q_range ||
                    expecting_to_ult && g_local->position.dist_to( *pred ) > 1200.f )
                    return false;

                auto direction_candidates = g_render->get_3d_circle_points(
                    g_local->position,
                    800.f,
                    24,
                    200,
                    ( target->position - g_local->position ).normalize( ).rotated( -2.f )
                );
                if ( !expecting_to_ult ) {
                    direction_candidates.push_back(
                        g_local->position.extend( target->position, -800.f )
                    );
                }

                Vec3 best_direction{ };
                auto highest_distance{ is_fastcombo ? FLT_MAX : 0.f };

                std::vector< Vec3 > valid_points{ };

                for ( auto point : direction_candidates ) {
                    auto       v1    = *pred - g_local->position;
                    auto       v2    = point - g_local->position;
                    const auto dot   = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto angle = acos( dot ) * 180.f / 3.14159265358979323846f;
                    if ( angle <= 10.f ) continue;

                    valid_points.push_back( point );

                    if ( point.dist_to( *pred ) > highest_distance ) {
                        if ( g_features->prediction->minion_in_line( g_local->position, point, 100.f ) ||
                            g_features->prediction->minion_in_line( point, target->position, 150.f ) )
                            continue;

                        best_direction   = point;
                        highest_distance = point.dist_to( *pred );
                    }
                }

                m_portal_points = valid_points;

                if ( best_direction.length( ) <= 0.f ) return false;

                if ( cast_spell( ESpellSlot::q, best_direction ) ) {
                    m_last_q_time = *g_time;
                    return true;
                }
            }

            if ( !m_q1_detected ) return false;

            const bool is_fast{ };

            if ( g_config->zoe.q_delay_cast->get< bool >( ) && !is_fast &&
                !m_hide_animation && *g_time + g_features->orbwalker->get_ping( ) < m_q_reach_end_time )
                return false;

            auto       hitchance = g_config->zoe.q_hitchance->get< int >( );
            const auto buff      = g_features->buff_cache->get_buff( g_local->index, ct_hash( "ZoeQ" ) );
            if ( !buff || buff->buff_data->end_time - *g_time <= 0.25f || *g_time - buff->buff_data->start_time <=
                0.3f )
                return false;

            if ( m_hide_animation ||
                g_config->zoe.q_override_hitchance->get< bool >( ) &&
                ( buff->buff_data->end_time - *g_time <= 0.3f ||
                    m_in_portal && m_ult_return_time - *g_time <= 0.25f ) )
                hitchance = 0;


            if ( !m_hide_animation && !m_full_combo_active
                && g_features->buff_cache->get_buff( g_local->index, ct_hash( "zoepassivesheenbuff" ) ) && g_features->
                orbwalker->is_attackable( target->index ) && buff->buff_data->end_time - *g_time >= 0.5f )
                return false;

            const auto missile_start_position = get_q1_position( );

            auto pred = g_features->prediction->predict(
                target->index,
                25000.f,
                2500.f,
                70.f,
                0.15f,
                missile_start_position,
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid && hitchance != 0 ||
                g_features->prediction->minion_in_line(
                    missile_start_position,
                    missile_start_position.extend(
                        pred.position,
                        missile_start_position.dist_to( pred.position ) + 50.f
                    ),
                    75.f
                ) )
                return false;

            const auto damage        = get_q_damage( target->index, missile_start_position.dist_to( pred.position ) );
            const auto can_killsteal = damage > helper::get_real_health( target->index, EDamageType::magic_damage );

            const auto local_pred   = g_features->prediction->predict_default( g_local->index, 0.4f );
            auto       dist_to_pred = local_pred.has_value( )
                                          ? pred.position.dist_to( *local_pred )
                                          : pred.position.dist_to( g_local->position );

            if ( m_in_portal && m_ult_start_time > *g_time ) dist_to_pred = m_portal_position.dist_to( pred.position );

            if ( dist_to_pred > m_q_range && !m_in_portal && !m_hide_animation ) {
                const auto max_movable_distance = std::min(
                    ( 0.15f + get_q1_position( ).dist_to( pred.position ) / 2500.f ) * g_local->movement_speed,
                    static_cast< float >( g_config->zoe.q_magnet_distance->get< int >( ) )
                );

                auto movable_distance{ max_movable_distance };

                for ( auto i = 1; i <= 8; i++ ) {
                    auto temp = g_local->position.extend( pred.position, max_movable_distance / 8.f * i );
                    if ( g_navgrid->is_wall( temp ) ) {
                        movable_distance = max_movable_distance / 8.f * ( i - 1 );
                        break;
                    }

                    movable_distance = max_movable_distance / 8.f * i;
                }

                if ( g_local->position.dist_to( pred.position ) > m_q_range + movable_distance ) return false;
            }

            if ( static_cast< int >( pred.hitchance ) < hitchance && !can_killsteal && !m_hide_animation ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time        = *g_time;
                m_last_q_recast_time = *g_time;

                if ( m_hide_animation ) {
                    std::cout << "[ ZOE Q ] Hiding E animation, E cast time: "
                        << *g_time - ( m_e_server_cast_time - 0.3f ) << std::endl;
                }

                if ( g_local->position.dist_to( pred.position ) >= 800.f ) {
                    m_magnet_goal      = pred.position;
                    m_magnet_radius    = 770.f;
                    m_draw_magnet_area = true;
                    m_should_magnet    = true;
                }

                m_highest_hitchance  = can_killsteal ? 5 : static_cast< int >( pred.hitchance );
                m_missile_start      = get_q1_position( );
                m_missile_end        = pred.position;
                m_raw_missile_start  = m_missile_start;
                m_predicted_end      = m_missile_end;
                m_raw_missile_end    = m_missile_end;
                m_local_end_distance = 0.f;


                m_q_flying           = true;
                m_q_target_index     = target->index;
                m_q_valid_trajectory = true;

                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->zoe.w_enabled->get< bool >( ) || !m_slot_w->is_ready( ) || *g_time - m_last_w_time <=
                0.5f )
                return false;

            const auto name = rt_hash( m_slot_w->get_name( ).data( ) );

            switch ( name ) {
            case ct_hash( "SummonerDot" ):
            {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 600.f ) return false;

                if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                    std::cout << "[ ZOE ] W Autocast ignite on " << target->champion_name.text << std::endl;
                    m_last_w_time = *g_time;
                    return true;
                }
            }
            case ct_hash( "SummonerExhaust" ):
            {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 600.f ) return false;

                if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                    std::cout << "[ ZOE ] W Autocast exhaust on " << target->champion_name.text << std::endl;
                    m_last_w_time = *g_time;
                    return true;
                }
            }

            default:
                break;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->zoe.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                g_features->orbwalker->in_action( ) || g_features->evade->is_active( )
                || !m_full_combo_active && g_features->orbwalker->in_attack( )
                || m_disable_spells || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto delay = m_in_portal && *g_time < m_ult_allowed_cast_time
                                   ? 0.3f + m_ult_allowed_cast_time - *g_time
                                   : 0.3f;
            const auto hitchance = m_in_portal && m_full_combo_active && m_ult_return_time - *g_time <= 0.3f
                                       ? 0
                                       : g_config->zoe.e_hitchance->get< int >( );

            auto pred = g_features->prediction->predict( target->index, 4000.f, 1850.f, 50.f, delay, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < hitchance
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 50.f ) )
                return false;

            if ( pred.position.dist_to( g_local->position ) > 795.f ) {
                if ( !g_config->zoe.e_through_walls->get< bool >( ) || g_config->zoe.e_flee_only_direct->get< bool >( )
                    && g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee )
                    return false;
                bool  wall_found{ };
                float distance_to_wall{ };

                // see if wall is in e trajectory
                for ( auto i = 1; i <= 31; i++ ) {
                    auto temp = g_local->position.extend( pred.position, 25.f * i );
                    if ( !g_navgrid->is_wall( temp ) ) continue;

                    wall_found       = true;
                    distance_to_wall = 25.f * i;
                    break;
                }

                //debug_log( "wall check: {}", wall_found );
                if ( !wall_found ) return false;

                const auto wall_start_position = g_local->position.extend( pred.position, distance_to_wall );
                Vec3       wall_end_position{ };

                // find the end of the wall
                for ( auto i = 1; i < 100; i++ ) {
                    auto temp = wall_start_position.extend( pred.position, 50.f * i );
                    if ( g_navgrid->is_wall( temp ) ) continue;

                    wall_end_position = temp;
                    break;
                }

                /*for (int i = 1; i <= 10; i++)
                {
                    float extend_distance = 25.f * i;
                    vec3 temp = g_local->position.extend(wall_end_position, g_local->position.dist_to(wall_end_position) + extend_distance);
                    if (!g_navgrid->is_wall(temp)) continue;


                    spell_max_range = g_local->position.dist_to(wall_end_position) + extend_distance - 25.f;
                    break;
                }*/

                const auto distance_left   = m_e_range - distance_to_wall;
                const auto spell_max_range = g_local->position.dist_to( wall_end_position ) + distance_left;

                if ( spell_max_range < g_local->position.dist_to( pred.position ) ) return false;

                e_wall_start         = wall_start_position;
                e_wall_end           = wall_end_position;
                e_spell_end_position = g_local->position.extend( pred.position, spell_max_range );
                should_draw_e        = true;
            }

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->zoe.r_enabled->get< bool >( ) || !m_full_combo_active || !m_q1_detected ||
                m_in_portal || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 25000.f, 0.f, 0.f, 0.5f );
            if ( !pred.valid ) return false;

            const auto cast_position   = g_local->position.extend( pred.position, 575.f );
            const auto portal_position = get_ult_position( cast_position );

            if ( pred.position.dist_to( portal_position ) > 675.f ) return false;

            if ( g_features->prediction->minion_in_line( portal_position, pred.position, 100.f )
                || is_position_in_turret_range( portal_position ) )
                return false;


            if ( cast_spell( ESpellSlot::r, cast_position ) ) {
                //std::cout << "[ ZOE R ] Full combo cast" << std::endl;
                m_last_r_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto magnet_q( ) -> void{
            if ( !m_q_flying && !m_q1_detected || m_in_portal || !m_allow_magnet || !m_q_valid_trajectory ||
                m_current_trajectory_valid ||
                !g_config->zoe.q_magnet->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                m_magnet_active = false;
                m_allow_magnet  = false;

                return;
            }

            Vec3       goal_point{ };
            auto       lowest_distance{ 9999.f };
            bool       found_point{ };
            const auto local_under_turret = is_position_in_turret_range( g_local->position );

            const auto magnet_position{ m_magnet_goal };
            const auto magnet_radius{ m_magnet_radius };

            m_magnet_points.clear( );
            const auto candidates =
                g_render->get_3d_circle_points(
                    magnet_position,
                    magnet_radius,
                    10,
                    33.f,
                    ( g_local->position - magnet_position ).normalize( ).rotated( -0.33f )
                );

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            for ( auto& point : candidates ) {
                if ( point.dist_to( g_local->position ) > g_config->zoe.q_magnet_distance->get< int >( ) + (
                        m_magnet_active ? 30.f : 0.f ) ||
                    point.dist_to( g_local->position ) > lowest_distance && g_local->position.dist_to( magnet_position )
                    > magnet_radius ||
                    point.dist_to( cursor ) > lowest_distance && g_local->position.dist_to( magnet_position ) <=
                    magnet_radius ) {
                    m_magnet_points.push_back( point );
                    continue;
                }

                auto walk_point{ point };

                if ( point.dist_to( g_local->position ) <= 80.f ) {
                    const auto point_dist   = point.dist_to( g_local->position );
                    const auto extend_value = 180.f - point_dist;

                    walk_point = magnet_position.extend( point, magnet_radius - extend_value );
                }

                if ( g_navgrid->is_wall( walk_point ) ||
                    !local_under_turret && is_position_in_turret_range( walk_point ) ) {
                    m_magnet_points.push_back( walk_point );
                    continue;
                }

                goal_point      = walk_point;
                lowest_distance = g_local->position.dist_to( magnet_position ) < magnet_radius
                                      ? point.dist_to( cursor )
                                      : point.dist_to( g_local->position );
                found_point = true;

                m_magnet_points.push_back( walk_point );
            }

            if ( !found_point ) {
                // enable orb movement when not magneting
                //std::cout << "[magnet] found no valid point\n";
                if ( g_features->orbwalker->is_movement_disabled( ) ) g_features->orbwalker->allow_fast_move( );

                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                m_allow_magnet  = false;
                return;
            }

            //auto local_pred = g_features->prediction->predict_default( g_local->index, 0.15f );
            if ( g_local->position.dist_to( magnet_position ) < m_magnet_radius ) {
                //std::cout << "early magnet end\n";
                g_features->orbwalker->allow_fast_move( );
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );

                m_magnet_active = false;
                m_allow_magnet  = false;
                return;
            }

            if ( !g_features->orbwalker->is_movement_disabled( ) ) {
                g_features->orbwalker->allow_movement( false );
                g_features->orbwalker->allow_attacks( false );
            }

            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                const auto path     = aimgr->get_path( );
                const auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( goal_point ) < 5.f ) return;

                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_magnet_active  = true;
                }
            }
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->zoe.e_antigapcloser->get< bool >( ) || g_features->orbwalker->in_action( ) || *g_time -
                m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1850.f, 50.f, 0.30f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1850.f,
                50.f,
                0.3f,
                { },
                true
            );

            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                60.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->zoe.e_autointerrupt->get< bool >( ) || g_features->orbwalker->in_action( ) || *g_time -
                m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1850.f,
                50.f,
                0.3f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                80.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto track_data( ) -> void{
            if ( !m_e_detected ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 2 && sci->server_cast_time - 0.2f > *g_time ) {
                    m_e_detected         = true;
                    m_e_server_cast_time = sci->server_cast_time;
                    m_hide_animation     = g_config->zoe.e_hide_animation->get< bool >( );
                }
            } else {
                if ( *g_time > m_e_server_cast_time - 0.15f ) {
                    m_e_detected     = false;
                    m_hide_animation = false;
                }
            }

            if ( !m_in_portal ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 3 ) {
                    m_in_portal = true;
                    m_ult_return_time = sci->server_cast_time + 1.f;
                    m_ult_start_time = sci->server_cast_time + 0.15f;
                    m_ult_allowed_cast_time = sci->server_cast_time + 0.25f;
                    m_portal_position = get_ult_position( g_local->position.extend( sci->end_position, m_r_range ) );
                    m_is_ult_position = *g_time >= m_ult_start_time;
                    m_ult_cast_position = sci->start_position;
                }
            } else {
                m_is_ult_position = *g_time >= m_ult_start_time;

                if ( *g_time >= m_ult_return_time ) m_in_portal = false;
            }

            if ( !m_q1_detected ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 47 ) return;
                m_q1_detected             = true;
                m_missile_source_position = sci->start_position;
                m_cast_end_position       = sci->end_position;

                m_q_cast_end       = sci->server_cast_time;
                m_q_reach_end_time = sci->server_cast_time + 800.f / 1200.f;

                return;
            }

            if ( *g_time > m_q_cast_end && !g_features->buff_cache->
                                                        get_buff(
                                                            g_local->index,
                                                            ct_hash( "ZoeQ" )
                                                        ) )
                m_q1_detected = false;
        }

        auto missile_path_tracking( ) -> void{
            if ( !m_cast_delayed_r && !m_should_magnet && !m_q_flying ) return;

            auto time_since_cast   = *g_time - m_last_q_time;
            auto distance_traveled = ( time_since_cast - 0.15f ) * 2500.f;
            if ( distance_traveled >= m_missile_start.dist_to( m_predicted_end ) ) {
                m_cast_delayed_r           = false;
                m_q_flying                 = false;
                m_q_valid_trajectory       = false;
                m_current_trajectory_valid = false;
                m_missile_found            = false;
                m_disable_spells           = false;
                return;
            }

            auto& target = g_entity_list.get_by_index( m_q_target_index );

            if ( !target || target->is_dead( ) ) {
                m_cast_delayed_r           = false;
                m_q_flying                 = false;
                m_q_valid_trajectory       = false;
                m_current_trajectory_valid = false;
                m_missile_found            = false;
                m_disable_spells           = false;
                return;
            }

            update_missile_trajectory( );

            auto missile_position = get_q2_position( );
            auto end_position = m_predicted_end;
            auto bounding_radius = g_features->prediction->get_champion_radius( rt_hash( target->champion_name.text ) );


            if ( !m_missile_found ) {
                for ( auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile || missile->position.dist_to( g_local->position ) > 3000.f ) continue;

                    auto info = missile->missile_spell_info( );
                    if ( !info ) continue;

                    auto data = info->get_spell_data( );
                    if ( !data ) continue;

                    auto name = data->get_name( );
                    if ( rt_hash( name.data( ) ) != ct_hash( "ZoeQMis2" ) ) continue;

                    m_missile_found = true;
                    m_missile_index = missile->index;
                    break;
                }
            } else {
                auto missile = &g_entity_list.get_by_index( m_missile_index );


                if ( !missile ) {
                    m_cast_delayed_r           = false;
                    m_q_flying                 = false;
                    m_q_valid_trajectory       = false;
                    m_current_trajectory_valid = false;
                    m_missile_found            = false;
                    return;
                }
            }

            auto time_left{ missile_position.dist_to( end_position ) / 2500.f };
            auto extend_distance = time_left * g_local->movement_speed + 70.f;
            if ( m_in_portal || *g_time - m_last_r_time <= 0.25f ) extend_distance = 70.f;
            if ( m_cast_delayed_r || *g_time - m_last_r_time <= 0.2f && !m_in_portal ) extend_distance += 575.f;

            auto rectangle = sdk::math::Rectangle(
                missile_position,
                missile_position.extend( end_position, missile_position.dist_to( end_position ) + extend_distance ),
                70.f + bounding_radius
            );

            auto current_rectangle = sdk::math::Rectangle(
                missile_position,
                missile_position.extend( end_position, missile_position.dist_to( end_position ) + 70.f ),
                70.f + bounding_radius
            );


            auto poly         = rectangle.to_polygon( );
            auto current_poly = current_rectangle.to_polygon( );

            auto pred =
                g_features->prediction->predict( target->index, 25000.f, 2500.f, 70.f, 0.f, missile_position, true );

            m_target_pred = pred.position;
            if ( m_highest_hitchance < g_config->zoe.q_hitchance->get< int >( ) &&
                static_cast< int >( pred.hitchance ) > m_highest_hitchance ) {
                std::cout << "NOTICE | Detected higher hitchance during Q2 flight, hitchance was "
                    << static_cast< int >( pred.hitchance ) << " | old hc: " << m_highest_hitchance << std::endl;

                m_highest_hitchance = static_cast< int >( pred.hitchance );
            }


            m_q_valid_trajectory = poly.is_inside( pred.position ) && pred.valid &&
                !g_features->prediction->minion_in_line( missile_position, pred.position, 75.f );

            m_current_trajectory_valid = current_poly.is_inside( pred.position );

            if ( m_should_magnet ) {
                m_magnet_goal = pred.position;

                auto local_pred           = g_features->prediction->predict_default( g_local->index, 0.016f );
                auto closest_magnet_point = m_magnet_goal.extend( g_local->position, 800.f );

                if ( !m_allow_magnet &&
                    closest_magnet_point.dist_to( *local_pred ) / g_local->movement_speed > time_left - 0.5f ) {
                    //std::cout << "[ ZOE ] Tracker allowed magneting, time left: " << travel_time_left << std::endl;

                    m_allow_magnet  = true;
                    m_should_magnet = false;
                }
            }

            if ( m_cast_delayed_r && m_q_valid_trajectory && m_slot_r->is_ready( ) && time_left <= 0.8f &&
                m_highest_hitchance >= g_config->zoe.q_hitchance->get< int >( ) )
                m_disable_spells = true;

            if ( time_left >= 0.5f + g_features->orbwalker->get_ping( ) * 2.f ) return;

            if ( !m_cast_delayed_r || !m_q_valid_trajectory ||
                m_highest_hitchance < g_config->zoe.q_hitchance->get< int >( ) || !m_slot_r->is_ready( true ) )
                return;

            if ( time_left <= 0.25f ) {
                debug_log( "time left too little: {}", time_left );
                m_q_flying           = false;
                m_cast_delayed_r     = false;
                m_q_valid_trajectory = false;
                m_disable_spells     = false;
                return;
            }

            const auto move_direction = pred.position - g_local->position;
            const auto base_direction = g_local->position + move_direction;

            auto direction = ( base_direction - g_local->position ).normalize( ).rotated( -1.58f );
            auto points    = g_render->get_3d_circle_points( g_local->position, 575.f, 16, 158, direction );

            Vec3 best_position{ };
            auto target_distance{ FLT_MAX };
            bool is_underturret{ };
            auto units_closer{ 0.f };

            auto minimum_range = std::max( g_local->position.dist_to( pred.position ) - m_q_range, 0.f );

            std::vector< Vec3 > candidates{ };
            for ( const auto point : points ) {
                auto temp = get_ult_position( point );
                candidates.push_back( temp );

                auto distance                = temp.dist_to( pred.position );
                auto units_closer_than_local = std::max(
                    g_local->position.dist_to( pred.position ) - distance,
                    -100.f
                );
                auto in_turret_range = is_position_in_turret_range( temp );

                if ( g_config->zoe.r_underturret->get< int >( ) == 0 && in_turret_range ||
                    g_config->zoe.r_underturret->get< int >( ) == 1 && in_turret_range && !m_full_combo_active )
                    continue;

                if ( best_position.length( ) == 0.f || !in_turret_range && is_underturret && units_closer_than_local >
                    minimum_range ||
                    distance < target_distance && ( !in_turret_range || is_underturret && in_turret_range ) ) {
                    best_position   = temp;
                    target_distance = distance;
                    is_underturret  = in_turret_range;
                    units_closer    = units_closer_than_local;
                }
            }

            m_portal_points = candidates;

            if ( target_distance >= m_q_range || best_position.length( ) == 0.f ) {
                //std::cout << "[ R EXTEND ] Error, target distance: " << target_distance << std::endl;
                //std::cout << "minimum range: " << minimum_range << " | closer: " << units_closer << std::endl;

                return;
            }

            if ( cast_spell( ESpellSlot::r, best_position ) ) {
                m_cast_delayed_r = false;
                m_last_r_time    = *g_time;
                m_disable_spells = false;
                debug_log(
                    "r extend at last second: {}  | is_underturret: {} | needed distance: {}",
                    time_left,
                    is_underturret,
                    g_local->position.dist_to( pred.position ) - m_q_range
                );

                g_features->orbwalker->on_cast( );
            }
        }

        auto get_q1_position( ) const -> Vec3{
            if ( !m_q1_detected ) return { };

            if ( m_q_cast_end > *g_time ) return m_missile_source_position;

            const auto time_traveled   = *g_time - m_q_cast_end;
            const auto extend_distance = time_traveled * 1200.f;
            if ( extend_distance > 800.f ) return m_cast_end_position;

            return m_missile_source_position.extend( m_cast_end_position, extend_distance );
        }

        auto get_q2_position( ) const -> Vec3{
            if ( !m_q_flying ) return { };

            const auto time_since_cast = *g_time - m_last_q_time;
            if ( time_since_cast <= 0.15f ) return m_missile_start;

            const auto distance_traveled = ( time_since_cast - 0.15f ) * 2500.f;

            return m_missile_start.extend( m_missile_end, distance_traveled );
        }

        static auto get_ult_position( const Vec3& cast_position ) -> Vec3{
            if ( !g_navgrid->is_wall( cast_position ) ) return cast_position;

            for ( auto i = 1; i <= 20; i++ ) {
                auto points = g_render->get_3d_circle_points( cast_position, 25.f * i, 16 );

                for ( auto point : points ) if ( !g_navgrid->is_wall( point ) ) return point;
            }

            return cast_position;
        }

        auto update_q2_trajectory( ) -> Vec3{
            const auto missile_position    = get_q2_position( );
            const auto m_missile_direction = missile_position + ( m_missile_end - m_missile_start ).normalize( );

            bool in_range{ };

            auto       local_position{ g_local->position };
            const auto pred = g_features->prediction->predict_default( g_local->index, 0.f );
            if ( pred.has_value( ) ) local_position = pred.value( );

            for ( auto i = 1; i <= 50; i++ ) {
                const auto travel_time        = 0.016f * i;
                auto       simulated_position = missile_position.extend( m_missile_direction, travel_time * 2500.f );

                if ( !in_range ) {
                    if ( simulated_position.dist_to( local_position ) < 800.f ) in_range = true;

                    continue;
                }

                if ( simulated_position.dist_to( local_position ) < 800.f ) continue;

                const auto furthest_distance         = m_raw_missile_start.dist_to( m_raw_missile_end );
                const auto simulated_distance        = m_raw_missile_start.dist_to( simulated_position );
                const auto last_simulated_difference = simulated_position.dist_to( m_predicted_end );
                const auto extended_raw_end          = m_missile_start.extend( m_raw_missile_end, 3000.f );

                if ( furthest_distance < simulated_distance
                    || simulated_distance < furthest_distance && simulated_distance > furthest_distance - 100.f
                    || m_raw_missile_end.dist_to( m_missile_end ) <= 1.f ) {
                    if ( furthest_distance < simulated_distance ||
                        m_raw_missile_end.dist_to( m_missile_end ) <= 1.f ) {
                        m_local_end_distance = local_position.dist_to( simulated_position );
                        m_raw_missile_end    = simulated_position;

                        update_raw_end_position( );
                    }

                    if ( simulated_position.dist_to( m_raw_missile_end ) < 100.f &&
                        ( simulated_position.dist_to( extended_raw_end ) >
                            m_predicted_end.dist_to( extended_raw_end )
                            || last_simulated_difference > 40.f ) )
                        m_predicted_end = simulated_position;

                    return m_predicted_end;
                }
            }

            return m_predicted_end;
        }

        auto update_missile_trajectory( ) -> void{
            update_raw_end_position( );
            if ( m_raw_missile_end.dist_to( m_missile_end ) <= 1.f ) update_q2_trajectory( );
            update_predicted_end_position( );
        }

        auto update_raw_end_position( ) -> void{
            auto       local_position = g_local->position;
            const auto pred           = g_features->prediction->predict_default( g_local->index, 0.f );
            if ( pred.has_value( ) ) local_position = *pred;

            if ( m_in_portal && g_local->position.dist_to( m_ult_cast_position ) <= 50.f )
                local_position =
                    get_ult_position( m_portal_position );

            const auto end_position     = m_raw_missile_end;
            const auto current_distance = end_position.dist_to( local_position );

            for ( auto i = 1; i < 20; i++ ) {
                auto temporary = m_raw_missile_start.extend(
                    end_position,
                    m_raw_missile_start.dist_to( end_position ) + 5.f * i
                );
                const auto distance = temporary.dist_to( local_position );

                if ( distance > current_distance ) {
                    if ( distance > 850.f ) break;

                    m_raw_missile_end = temporary;
                }
            }
        }

        auto update_predicted_end_position( ) -> void{
            if ( m_in_portal ) m_predicted_end = m_raw_missile_end;
            else m_predicted_end               = m_raw_missile_end.extend( m_raw_missile_start, 150.f );
        }

        auto update_sleep_object( ) -> void{
            if ( !m_sleep_found ) {
                if ( m_slot_e->level == 0 || m_slot_e->is_ready( ) ) return;

                for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                    if ( !particle || particle->dist_to_local( ) > 3000.f ) continue;

                    const auto name = particle->get_alternative_name( );
                    if ( name.find( "Zoe" ) == std::string::npos || name.find( "Tar_Sleep" ) ==
                        std::string::npos )
                        continue;


                    m_sleep_index      = particle->index;
                    m_sleep_found      = true;
                    auto draw_position = particle->position;
                    draw_position.y    = g_navgrid->get_height( draw_position );

                    m_sleep_position = draw_position;
                    break;
                }

                return;
            }


            const auto particle = g_entity_list.get_by_index( m_sleep_index );
            if ( !particle ) m_sleep_found = false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            default:
                break;
            }

            return 0.f;
        }

        auto get_q_damage( const int16_t index, const float travel_distance ) -> float{
            auto base_damage = m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.6f;

            auto bonus_damage{ 7.f };
            switch ( g_local->level ) {
            case 1:
                bonus_damage = 7.f;
                break;
            case 2:
                bonus_damage = 8.f;
                break;
            case 3:
                bonus_damage = 10.f;
                break;
            case 4:
                bonus_damage = 12.f;
                break;
            case 5:
                bonus_damage = 14.f;
                break;
            case 6:
                bonus_damage = 16.f;
                break;
            case 7:
                bonus_damage = 18.f;
                break;
            case 8:
                bonus_damage = 20.f;
                break;
            case 9:
                bonus_damage = 22.f;
                break;
            case 10:
                bonus_damage = 24.f;
                break;
            case 11:
                bonus_damage = 26.f;
                break;
            case 12:
                bonus_damage = 29.f;
                break;
            case 13:
                bonus_damage = 32.f;
                break;
            case 14:
                bonus_damage = 35.f;
                break;
            case 15:
                bonus_damage = 38.f;
                break;
            case 16:
                bonus_damage = 42.f;
                break;
            case 17:
                bonus_damage = 46.f;
                break;
            default:
                bonus_damage = 50.f;
                break;
            }

            base_damage += bonus_damage;

            const auto modifier = std::min( ( travel_distance - 500.f ) / 2050.f, 1.f ) * 1.5f;

            const auto extra_damage = base_damage * modifier;


            return helper::calculate_damage( base_damage + extra_damage, index, false );
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            case ESpellSlot::r:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_q_recast_time{ };

        std::vector< float > m_q_damage = { 0.f, 50.f, 80.f, 110.f, 140.f, 170.f };

        // q tracking
        bool  m_q1_detected{ };
        float m_q_cast_end{ };
        float m_q_reach_end_time{ };
        Vec3  m_missile_source_position{ };
        Vec3  m_cast_end_position{ };

        bool    m_missile_found{ };
        int16_t m_missile_index{ };

        std::vector< Vec3 > m_portal_points{ };

        int m_highest_hitchance{ };


        bool m_full_combo_active{ };
        // ult tracking
        bool  m_in_portal{ };
        bool  m_is_ult_position{ };
        Vec3  m_portal_position{ };
        float m_ult_return_time{ };
        float m_ult_start_time{ };
        float m_ult_allowed_cast_time{ };
        Vec3  m_ult_cast_position{ };

        // clarify E sleep visual
        bool    m_sleep_found{ };
        int16_t m_sleep_index{ };
        Vec3    m_sleep_position{ };

        // q2 tracking for delayed r
        bool m_q_valid_trajectory{ };
        bool m_current_trajectory_valid{ };

        bool    m_q_flying{ };
        int32_t m_q_target_index{ };
        Vec3    m_missile_start{ };
        Vec3    m_missile_end{ };
        Vec3    m_predicted_end{ };

        bool m_disable_spells{ };

        Vec3  m_raw_missile_start{ };
        Vec3  m_raw_missile_end{ };
        float m_local_end_distance{ };

        Vec3 m_target_pred{ };

        bool m_cast_delayed_r{ };

        float       m_total_distance_traveled{ };
        std::string m_last_w_name{ };

        // e animation hide
        bool  m_e_detected{ };
        float m_e_server_cast_time{ };
        bool  m_hide_animation{ };

        // q magneting
        bool                m_magnet_active{ };
        float               m_last_move_time{ };
        std::vector< Vec3 > m_magnet_points{ };
        bool                m_should_magnet{ };
        bool                m_allow_magnet{ };

        // q magnet visual
        Vec3  m_magnet_goal{ };
        float m_magnet_radius{ };
        bool  m_draw_magnet_area{ };

        // e wall visual

        bool should_draw_e{ };
        Vec3 e_wall_start{ };
        Vec3 e_wall_end{ };
        Vec3 e_spell_end_position{ };

        bool m_should_extend_range{ };
        Vec3 m_extend_direction{ };

        float m_q_range{ 870.f };
        float m_q_extended_range{ 1450.f };
        float m_w_range{ 0.f };
        float m_e_range{ 800.f };
        float m_r_range{ 575.f };
    };
}
