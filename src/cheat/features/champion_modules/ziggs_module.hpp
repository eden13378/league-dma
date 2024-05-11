#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class ZiggsModule final : public IModule {
    public:
        enum class EDashOperation {
            none,
            selecting,
            processing,
            aligning_position,
            executing_dash,
            dashing,
            post_dash
        };

        virtual ~ZiggsModule( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "ziggs_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Ziggs" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "ziggs" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto combat     = navigation->add_section( _( "combat logic" ) );

            q_settings->checkbox( _( "enable" ), g_config->ziggs.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->ziggs.q_harass );
            q_settings->multi_select(
                _( "q logic mode " ),
                { g_config->ziggs.q_direct, g_config->ziggs.q_bounce, g_config->ziggs.q_double_bounce },
                { _( "Direct" ), _( "Bounce" ), _( "Double bounces" ) }
            );

            q_settings->checkbox( _( "try direct q minion impact" ), g_config->ziggs.q_minion_impact );
            q_settings->select(
                _( "hitchance" ),
                g_config->ziggs.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //w_settings->checkbox( _( "enable" ), g_config->ziggs.w_enabled );
            w_settings->checkbox( _( "pull enemies" ), g_config->ziggs.w_pull_enemies )->set_tooltip(
                _( "While fleeing, hold CTRL to make ziggs use W to jump towards cursor position" )
            );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->ziggs.w_autointerrupt )
                      ->set_tooltip( _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" ) );

            e_settings->checkbox( _( "enable" ), g_config->ziggs.e_enabled );
            e_settings->checkbox( _( "antigapclose" ), g_config->ziggs.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->ziggs.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //r_settings->checkbox( _( "enable" ), g_config->ziggs.r_enabled );
            r_settings->checkbox( _( "semi manual r (keybind)" ), g_config->autocombo.semi_manual_ult_enabled );
            r_settings->select(
                _( "semi manual hitchance" ),
                g_config->ziggs.r_semi_manual_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "standalone r multihit" ), g_config->ziggs.r_multihit );
            r_settings->slider_int( _( "minimum multihit" ), g_config->ziggs.r_min_multihit_count, 2, 5, 1 );

            combat->checkbox( _( "enable combat logic" ), g_config->ziggs.combat_mode_enabled );
            combat->checkbox( _( "force toggle (hotkey)" ), g_config->ziggs.combat_mode_toggle );
            combat->multi_select(
                _( "combat logic options " ),
                {
                    g_config->ziggs.combat_direct_q_only,
                    g_config->ziggs.combat_ignore_q_hitchance,
                    g_config->ziggs.combat_w_allowed,
                    g_config->ziggs.combat_r_disabled
                },
                { _( "Direct q only" ), _( "Ignore q hitchance" ), _( "Allow EW combo" ), _( "Disable r multihit" ), }
            );
            combat->slider_int( _( "auto toggle range" ), g_config->ziggs.combat_max_threshold, 300, 1000, 25 );

            drawings->multi_select(
                _( "draw q mode" ),
                {
                    g_config->ziggs.q_draw_range,
                    g_config->ziggs.q_draw_bounce_range,
                    g_config->ziggs.q_draw_double_bounce_range
                },
                { _( "Direct" ), _( "Bounce" ), _( "Double bounce" ) }
            );
            drawings->checkbox( _( "draw w range" ), g_config->ziggs.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->ziggs.e_draw_range );
            drawings->checkbox( _( "draw r range minimap" ), g_config->ziggs.r_draw_range_minimap );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ziggs.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw r kill indicator" ), g_config->ziggs.r_show_indicator );
            drawings->checkbox( _( "draw combat activation range" ), g_config->ziggs.combat_draw_threshold );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->ziggs.q_draw_range->get< bool >( ) || g_config->ziggs.q_draw_bounce_range->get< bool >( ) ||
                g_config->ziggs.q_draw_double_bounce_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ziggs.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( g_config->ziggs.q_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 90, 255, 255 ),
                            m_q_range,
                            Renderer::outline,
                            70,
                            3.f
                        );
                    }
                    if ( g_config->ziggs.q_draw_bounce_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 0, 165, 255, 255 ),
                            m_q2_range,
                            Renderer::outline,
                            70,
                            3.f
                        );
                    }
                    if ( g_config->ziggs.q_draw_double_bounce_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 190, 255, 255 ),
                            m_q3_range,
                            Renderer::outline,
                            70,
                            3.f
                        );
                    }
                }
            }

            if ( g_config->ziggs.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ziggs.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->ziggs.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->ziggs.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->ziggs.r_draw_range_minimap->get< bool >( ) ) {
                auto r_slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( r_slot && r_slot->level > 0 && r_slot->is_ready( false ) ) {
                    g_render->circle_minimap(
                        g_local->position,
                        Color( 255, 255, 255, 255 ),
                        m_r_range,
                        60,
                        2.f
                    );
                }
            }

            switch ( m_current_operation ) {
            case EDashOperation::selecting:
            {
                if ( !m_dash_selected ) {
                    auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                    g_render->line_3d( m_selection_start, cursor, Color( 255, 255, 255 ), 3.f );
                } else g_render->line_3d( m_selection_start, m_selection_end, Color( 255, 255, 255 ), 5.f );
                break;
            }
            case EDashOperation::processing:
            {
                g_render->line_3d( m_selection_start, m_selection_end, Color( 255, 255, 25 ), 5.f );

                break;
            }
            case EDashOperation::aligning_position:
            case EDashOperation::executing_dash:
            {
                g_render->circle_3d( m_dash_start_point, Color( 255, 255, 255 ), 25.f, Renderer::outline, 20, 3.f );

                g_render->line_3d(
                    m_dash_start_point,
                    m_dash_end_point,
                    g_features->orbwalker->get_pulsing_color( ),
                    5.f
                );
                break;
            }
            case EDashOperation::dashing:
            {
                auto nearest = g_features->evade->get_closest_line_point(
                    m_dash_start_point,
                    m_dash_end_point,
                    g_local->position
                );

                if ( nearest.dist_to( m_dash_start_point ) > m_dash_start_point.dist_to( m_dash_end_point ) ) break;

                g_render->line_3d(
                    m_dash_end_point,
                    g_features->evade->get_closest_line_point(
                        m_dash_start_point,
                        m_dash_end_point,
                        g_local->position
                    ),
                    Color( 255, 255, 255 ),
                    6.f
                );
                break;
            }
            default:
                break;
            }

            int draw_count{ };

            Vec2 largest_size{ };

            for ( auto inst : m_executable_indexes ) {
                auto enemy = g_entity_list.get_by_index( inst );
                if ( !enemy ) continue;

                Vec2 indicator_base{
                    static_cast< float >( g_render_manager->get_width( ) ) * 0.7f,
                    static_cast< float >( g_render_manager->get_height( ) ) * 0.65f + 45.f * draw_count
                };

                Vec2 texture_size{ 40.f, 40.f };
                Vec2 texture_position{ indicator_base.x, indicator_base.y - texture_size.y * 0.5f };

                auto box_color = g_features->orbwalker->get_pulsing_color( );

                std::string champ_name = enemy->champion_name.text;

                auto       text      = champ_name + " executable";
                const auto text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), 32 );

                auto champ_texture_path =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    champ_texture_path.has_value(  ) ? *champ_texture_path : ""
                );
                Vec2 box_point{ texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 };
                Vec2 box_size{ texture_size.x + text_size.x * 1.2f, texture_size.y };

                if ( box_size.x > largest_size.x ) largest_size = box_size;
                else if ( box_size.x < largest_size.x ) box_size.x = largest_size.x;

                g_render->filled_box( box_point, box_size, Color( 25, 25, 25, 100 ) );
                g_render->box( box_point, box_size, box_color );

                if ( texture ) {
                    g_render->image(
                        { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                        texture_size,
                        texture
                    );


                    g_render->box(
                        { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                        texture_size,
                        box_color,
                        -1
                    );
                }

                Vec2 text_position{
                    indicator_base.x + texture_size.x,
                    indicator_base.y - texture_size.y / 2.f - text_size.y / 2.f
                };

                g_render->text_shadow( text_position, box_color, g_fonts->get_zabel( ), text.data( ), 32 );

                ++draw_count;
            }

            if ( *g_time - m_last_calculation < 2.f && m_bounce_type > 1 ) {
                g_render->circle_3d( m_throw_point, Color( 255, 50, 50 ), 150.f, Renderer::outline, 32, 3.f );
                g_render->circle_3d( m_bounce_point, Color( 255, 255, 50 ), 150.f, Renderer::outline, 32, 3.f );
                if ( m_bounce_type == 3 ) {
                    g_render->circle_3d(
                        m_explosion_point,
                        Color( 50, 255, 50 ),
                        240.f,
                        Renderer::outline,
                        48,
                        3.f
                    );
                }

                g_render->circle_3d(
                    m_predicted_point,
                    Color( 50, 255, 50, 80 ),
                    15.f,
                    Renderer::outline | Renderer::filled,
                    20,
                    1.f
                );
            }

            if ( g_config->ziggs.combat_draw_threshold->get< bool >( ) && m_threat_nearby ) {
                g_render->circle_3d(
                    g_local->position,
                    m_in_combat
                        ? ( m_threat_level >= 1.f
                                ? g_features->orbwalker->get_pulsing_color( )
                                : g_features->orbwalker->get_pulsing_color( ).alpha( 255 * m_threat_level ) )
                        : Color( 255.f, 255.f, 255.f, 255.f * m_threat_level ),
                    g_config->ziggs.combat_max_threshold->get< int >( ),
                    Renderer::outline,
                    72,
                    2.f
                );
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                std::string text{ "COMBAT MODE: " };
                std::string toggle_text{ "[ MOUSE 5 ] FORCE COMBAT " };
                const auto  text_size = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

                std::string text_state{
                    m_in_combat
                        ? "ACTIVE"
                        : g_config->ziggs.combat_mode_enabled->get< bool >( )
                              ? "INACTIVE"
                              : "DISABLED"
                };
                auto state_color = m_in_combat ? g_features->orbwalker->get_pulsing_color( ) : Color( 50, 255, 50 );

                if ( !g_config->ziggs.combat_mode_enabled->get< bool >( ) ) state_color = Color( 255, 50, 50 );

                auto toggle_color =
                    g_config->ziggs.combat_mode_toggle->get< bool >( ) ? Color( 50, 255, 50 ) : Color( 190, 190, 190 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_block( ), text.c_str( ), 8 );

                g_render->text_shadow(
                    { sp.x + text_size.x, sp.y },
                    state_color,
                    g_fonts->get_block( ),
                    text_state.c_str( ),
                    8
                );

                sp.y += 8.f;

                g_render->text_shadow( sp, toggle_color, g_fonts->get_block( ), toggle_text.c_str( ), 8 );
            }

            if ( g_config->autocombo.semi_manual_ult_enabled->get< bool >( ) ) {
                Vec2 sp{ };
                auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                if ( world_to_screen( cursor, sp ) ) {
                    sp.y -= 16.f;

                    g_render->circle_3d(
                        cursor,
                        g_features->orbwalker->get_pulsing_color( ),
                        600.f,
                        Renderer::outline,
                        50,
                        3.f
                    );

                    std::string text      = "Semi manual R";
                    const auto  text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), 20.f );

                    g_render->text_shadow(
                        { sp.x - text_size.x / 2.f, sp.y - text_size.y / 2.f },
                        g_features->orbwalker->get_pulsing_color( ),
                        g_fonts->get_zabel( ),
                        text.data( ),
                        20.f
                    );
                }
            }

            if ( *g_time - m_combo_start_time < 1.5f ) {
                g_render->circle_3d( m_w_pred, Color( 50, 255, 50 ), 20.f, Renderer::outline, 16, 2.f );

                for ( auto point : m_push_points )
                    g_render->circle_3d(
                        point,
                        Color( 255, 255, 255 ),
                        25.f,
                        Renderer::outline,
                        16,
                        1.f
                    );

                g_render->circle_3d( m_e_position, Color( 255, 255, 50 ), 300.f, Renderer::outline, 64, 2.f );
                g_render->circle_3d( m_w_position, Color( 255, 50, 50 ), 30.f, Renderer::outline, 64, 2.f );

                //m_w_position
            }

            if ( m_r_active ) {
                auto end_time     = m_r_server_cast_time + m_r_end_time;
                auto total_time   = m_r_end_time;
                auto modifier     = 1.f - std::min( ( end_time - *g_time ) / total_time, 1.f );
                auto end_modifier = std::min( ( *g_time - end_time ) / 0.2f, 1.f );
                if ( end_modifier < 0.f ) end_modifier = 0.f;

                Vec3 position = { m_r_end_position.x, g_navgrid->get_height( m_r_end_position ), m_r_end_position.z };

                g_render->circle_3d(
                    position,
                    Color( 255.f, 255.f, 255.f, 255.f - 255.f * end_modifier ),
                    525.f + 40.f * end_modifier,
                    Renderer::outline,
                    64,
                    5.f,
                    360.f * modifier
                );
            }

            if ( m_q_active ) {
                auto end_time     = m_server_cast_time + m_start_position.dist_to( m_end_position ) / 1700.f;
                auto total_time   = 0.25f + m_start_position.dist_to( m_end_position ) / 1700.f;
                auto modifier     = 1.f - std::min( ( end_time - *g_time ) / total_time, 1.f );
                auto end_modifier = std::min( ( *g_time - end_time ) / 0.2f, 1.f );
                if ( end_modifier < 0.f ) end_modifier = 0.f;

                g_render->circle_3d(
                    m_end_position,
                    Color( 255.f, 255.f, 255.f, 255.f - 255.f * end_modifier ),
                    150.f + 20.f * end_modifier,
                    Renderer::outline,
                    32,
                    3.f,
                    360.f * modifier
                );
            }

            return;
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            dash_helper( );
            w_detonator( );
            cast_tracker( );

            track_killable_enemies( );

            if ( g_config->ziggs.combat_mode_enabled->get< bool >( ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( target && target->is_alive( ) ) {
                    if ( !g_config->ziggs.combat_mode_toggle->get< bool >( ) ) {
                        const auto distance = target->dist_to_local( );
                        if ( !m_in_combat ) {
                            m_threat_level = distance < g_config->ziggs.combat_max_threshold->get< int >( )
                                                 ? 1.f
                                                 : 1.f -
                                                 std::min(
                                                     ( distance - g_config->ziggs.combat_max_threshold->get< int >( ) )
                                                     /
                                                     200.f,
                                                     1.f
                                                 );

                            m_in_combat     = distance < g_config->ziggs.combat_max_threshold->get< int >( );
                            m_threat_nearby = m_threat_level > 0.f;
                        } else {
                            m_in_combat     = distance < g_config->ziggs.combat_max_threshold->get< int >( ) + 75.f;
                            m_threat_nearby = m_in_combat;
                        }
                    } else {
                        m_in_combat = g_config->ziggs.combat_mode_toggle->get< bool >( );

                        m_threat_level =
                            target->dist_to_local( ) < g_config->ziggs.combat_max_threshold->get< int >( )
                                ? 1.f
                                : 1.f -
                                std::min(
                                    ( target->dist_to_local( ) -
                                        g_config->ziggs.combat_max_threshold->get< int >( ) ) /
                                    200.f,
                                    1.f
                                );

                        m_threat_nearby = m_threat_level > 0.f;
                    }
                } else {
                    m_in_combat     = g_config->ziggs.combat_mode_toggle->get< bool >( );
                    m_threat_nearby = false;
                }
            } else {
                m_in_combat     = false;
                m_threat_nearby = false;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            ew_combo( );
            antigapclose_e( );
            autointerrupt_w( );
            semi_manual_r( );

            if ( m_should_w ) spell_w( );

            if ( *g_time - m_combo_start_time > 1.25f && ( m_should_e || m_should_w ) ) {
                m_should_e = false;
                m_should_w = false;
            }


            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                multihit_r( );

                pull_w( );
            //spell_w( );
                spell_q( );
            //spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->ziggs.q_harass->get< bool >( ) ) spell_q( );

                break;
            case Orbwalker::EOrbwalkerMode::flee:

                flee_w( );
                spell_e( );
                break;
            default:
                break;
            }

            if ( g_features->orbwalker->is_movement_disabled( ) && !m_dash_selected && m_current_operation ==
                EDashOperation::none )
                g_features->orbwalker->allow_movement( true );
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->ziggs.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( direct_q( target ) || direct_impact_q( target ) || bounce_q( target ) ||
                    double_bounce_q( target ) )
                    return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->ziggs.combat_w_allowed->get< bool >( ) || g_features->orbwalker->get_mode( ) !=
                Orbwalker::EOrbwalkerMode::combo
                || *g_time - m_last_w_time <= 0.5f || *g_time - m_last_cast_time <= 0.1f || !m_in_combat || !m_slot_w->
                is_ready( true ) ||
                !m_slot_e->is_ready( true ) && *g_time - m_last_e_time > 0.5f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 1750.f, 0.f, 0.25f );
            if ( !pred.valid ) return false;


            const auto points = g_render->get_3d_circle_points( pred.position, 125.f, 20 );
            m_push_points     = points;
            m_w_pred          = pred.position;

            auto cast_position = pred.position;
            auto local_distance{ 99999.f };

            const auto e_ready = m_slot_e->is_ready( true ) && *g_time - m_last_e_time > 1.f;

            for ( auto point : points ) {
                auto extend = point.extend( pred.position, 500.f );

                if ( e_ready ) {
                    if ( extend.dist_to( g_local->position ) > m_e_range * 0.95f || g_navgrid->is_wall( extend ) ||
                        extend.dist_to( g_local->position ) < g_local->position.dist_to( pred.position ) ||
                        extend.dist_to( g_local->position ) > local_distance )
                        continue;
                } else { if ( extend.dist_to( m_e_position ) > local_distance ) continue; }

                cast_position  = point;
                local_distance = e_ready ? g_local->position.dist_to( extend ) : extend.dist_to( m_e_position );
            }

            if ( e_ready ) {
                m_should_e         = true;
                m_e_position       = cast_position.extend( pred.position, 500.f );
                m_combo_start_time = *g_time;

                return false;
            }


            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_should_detonate = true;

                m_should_w   = false;
                m_w_position = cast_position;

                return true;
            }

            return false;
        }

        auto ew_combo( ) -> void{
            if ( !m_should_e || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) ) return;

            if ( cast_spell( ESpellSlot::e, m_e_position.extend( g_local->position, 162.5f ) ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                m_should_e       = false;
                m_should_w       = true;
            }
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->ziggs.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 1800.f, 20.f, 0.75f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->ziggs.e_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto pull_w( ) -> bool{
            if ( !g_config->ziggs.w_pull_enemies->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo || !GetAsyncKeyState( 0x06 ) ||
                *g_time - m_last_w_time <= 0.5f || *g_time - m_last_cast_time <= 0.1f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 1750.f, 0.f, 0.25f );
            if ( !pred.valid ) return false;

            const auto cast_pos = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) + 200.f
            );

            if ( cast_spell( ESpellSlot::w, cast_pos ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time  = *g_time;
                m_last_w_time     = *g_time;
                m_should_detonate = true;
                return true;
            }
            return false;
        }

        auto semi_manual_r( ) -> bool{
            if ( !g_config->autocombo.semi_manual_ult_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return false;

            const Object* selected_target{ };
            const auto    cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            auto          lowest_distance{ FLT_MAX };
            Vec2          sp{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_local->position.dist_to( enemy->position ) > m_r_range ||
                    cursor.dist_to( enemy->position ) > 600.f ||
                    !sdk::math::world_to_screen( enemy->position, sp ) ||
                    g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                const auto cursor_distance = enemy->position.dist_to( cursor );

                if ( cursor_distance > lowest_distance ) continue;

                selected_target = enemy;
                lowest_distance = cursor_distance;
            }

            if ( !selected_target ) return false;

            bool is_far{ };

            auto pred = g_features->prediction->predict( selected_target->index, m_r_range, 0.f, 220.f, 1.625f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->ziggs.r_semi_manual_hitchance->get<
                int >( ) )
                return false;

            if ( pred.position.dist_to( g_local->position ) > 2700.f ) {
                is_far = true;
                pred   = g_features->prediction->predict( selected_target->index, m_r_range, 2250.f, 220.f, 0.375f );
                if ( !pred.valid ||
                    static_cast< int >( pred.hitchance ) < g_config->ziggs.r_semi_manual_hitchance->get< int >( ) )
                    return false;
            }

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                std::cout << "[ semi manual r ] cast at " << selected_target->champion_name.text
                    << " | is far: " << is_far << std::endl;

                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto multihit_r( ) -> bool{
            if ( !g_config->ziggs.r_multihit->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_r_time <= 0.5f || g_config->ziggs.combat_r_disabled->get< bool >( ) && m_in_combat ||
                !m_slot_r->is_ready( true ) )
                return false;

            const auto center_multihit = get_multihit_position( 2700.f, 0.f, 230.f, 1.625f, false );

            if ( center_multihit.hit_count > g_config->ziggs.r_min_multihit_count->get< int >( ) ) {
                if ( cast_spell( ESpellSlot::r, center_multihit.position ) ) {
                    debug_log(
                        "[ Ziggs ] R Multihit count {} | DIST: {} | T: {}",
                        center_multihit.hit_count,
                        g_local->position.dist_to( center_multihit.position ),
                        *g_time
                    );

                    m_last_r_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            }

            return false;
        }

        auto track_killable_enemies( ) -> void{
            if ( !g_config->ziggs.r_show_indicator->get< bool >( ) || *g_time - m_last_r_time <= 1.f || !m_slot_r->
                is_ready( true ) ) {
                if ( !m_executable_indexes.empty( ) ) m_executable_indexes.clear( );

                return;
            }

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || is_tracked( enemy->index ) ||
                    enemy->dist_to_local( ) > m_r_range )
                    continue;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy );
                const auto health = helper::get_real_health( enemy->index, EDamageType::magic_damage );
                if ( damage < health ) continue;

                m_executable_indexes.push_back( enemy->index );
            }

            for ( const auto inst : m_executable_indexes ) {
                auto enemy = g_entity_list.get_by_index( inst );
                if ( !enemy ) continue;

                enemy.update( );

                if ( enemy->is_dead( ) || enemy->is_invisible( ) ) {
                    remove_executable( inst );
                    continue;
                }


                const auto damage = get_spell_damage( ESpellSlot::r, enemy.get( ) );
                const auto health = helper::get_real_health( enemy->index, EDamageType::magic_damage );
                if ( damage < health ) remove_executable( inst );
            }
        }

        auto direct_q( Object* target ) -> bool{
            if ( !target || !g_config->ziggs.q_direct->get< bool >( ) || m_should_w || m_should_e ||
                m_should_detonate || *g_time - m_last_q_time <= 0.5f ||
                !m_slot_q->is_ready( true ) )
                return false;


            const auto hitchance = g_config->ziggs.combat_ignore_q_hitchance->get< bool >( ) && m_in_combat
                                       ? 0.f
                                       : g_config->ziggs.q_hitchance->get< int >( );

            auto pred = g_features->prediction->predict( target->index, m_q_range + 25.f, 1700.f, 100.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < hitchance ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_last_q_position = pred.position;
                g_features->orbwalker->on_cast( );

                m_bounce_type = 1;

                m_q_land_time = *g_time + 0.25f + g_local->position.dist_to( pred.position ) / 1700.f;
                return true;
            }

            return false;
        }

        auto bounce_q( Object* target ) -> bool{
            if ( !target || !g_config->ziggs.q_bounce->get< bool >( )
                || *g_time - m_last_q_time <= 0.5f ||
                g_config->ziggs.combat_direct_q_only->get< bool >( ) && m_in_combat || !m_slot_q->is_ready( true ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_q2_range + 25.f, 0.f, 0.f, 1.25f );
            if ( !pred.valid ) return false;

            const auto throw_point = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) * 0.725f
            );
            const auto travel_time = 0.275f + g_local->position.dist_to( throw_point ) / 1700.f;

            pred = g_features->prediction->predict( target->index, m_q2_range + 25.f, 0.f, 100.f, travel_time + 0.5f );
            if ( !pred.valid || g_local->position.dist_to( pred.position ) <= 900.f ) return false;

            m_throw_point = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) * 0.725f
            );
            m_bounce_point = g_local->position.extend(
                m_throw_point,
                g_local->position.dist_to( m_throw_point ) * 1.38f
            );
            m_predicted_point  = pred.position;
            m_last_calculation = *g_time;
            m_bounce_type      = 2;

            if ( pred.position.dist_to( m_bounce_point ) > 125.f ||
                static_cast< int >( pred.hitchance ) < g_config->ziggs.q_hitchance->get< int >( ) ||
                g_features->prediction->minion_in_circle( m_throw_point, 150.f ) || g_navgrid->
                is_wall( m_throw_point ) )
                return false;


            if ( cast_spell( ESpellSlot::q, m_throw_point ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto direct_impact_q( Object* target ) -> bool{
            if ( !target || !g_config->ziggs.q_minion_impact->get< bool >( ) ||
                g_config->ziggs.combat_direct_q_only->get< bool >( ) && m_in_combat || *g_time - m_last_q_time <= 0.5f
                ||
                !m_slot_q->is_ready( true ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1700.f, 0.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->ziggs.q_hitchance->get<
                int >( ) )
                return false;

            const auto travel_time = 0.25f + g_local->position.dist_to( pred.position ) / 1700.f;

            const auto m_cast_position{ pred.position };

            if ( !g_features->prediction->minion_in_circle_predicted( pred.position, 150.f, travel_time ) ) {
                return
                    false;
            }

            pred = g_features->prediction->predict( target->index, m_q_range, 1700.f, 200.f, 0.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->ziggs.q_hitchance->get<
                int >( ) )
                return false;

            if ( pred.position.dist_to( m_cast_position ) > 240.f ) return false;

            m_throw_point = pred.position;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time     = *g_time;
                m_last_cast_time  = *g_time;
                m_last_q_position = pred.position;

                g_features->orbwalker->on_cast( );

                m_bounce_type = 1;

                m_q_land_time = *g_time + 0.25f + g_local->position.dist_to( pred.position ) / 1700.f;
                return true;
            }

            return false;
        }

        auto bounce_impact_q( Object* target ) -> bool{
            if ( !target || *g_time - m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q2_range + 50.f, 0.f, 0.f, 1.25f );
            if ( !pred.valid ) return false;

            const auto throw_point = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) * 0.725f
            );
            const auto travel_time = 0.25f + g_local->position.dist_to( throw_point ) / 1700.f;

            pred = g_features->prediction->predict( target->index, m_q2_range, 0.f, 0.f, travel_time + 0.5f );
            if ( !pred.valid ) return false;

            m_throw_point = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) * 0.725f
            );
            m_bounce_point = g_local->position.extend(
                m_throw_point,
                g_local->position.dist_to( m_throw_point ) * 1.38f
            );
            m_predicted_point  = pred.position;
            m_last_calculation = *g_time;
            m_bounce_type      = 2;

            if ( g_features->prediction->minion_in_circle_predicted( m_throw_point, 150.f, travel_time ) ||
                !g_features->prediction->minion_in_circle_predicted( m_bounce_point, 150.f, travel_time + 0.5f ) )
                return false;

            pred = g_features->prediction->predict( target->index, m_q2_range + 75.f, 0.f, 200.f, travel_time + 0.5f );
            if ( !pred.valid ) return false;

            if ( pred.position.dist_to( m_bounce_point ) > 200.f || ( int )pred.hitchance < 2 ) return false;

            if ( cast_spell( ESpellSlot::q, m_throw_point ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto double_bounce_q( Object* target ) -> bool{
            if ( !target || !g_config->ziggs.q_double_bounce->get< bool >( ) ||
                g_config->ziggs.combat_direct_q_only->get< bool >( ) && m_in_combat ||
                *g_time - m_last_q_time <= 0.5f ||
                !m_slot_q->is_ready( true ) )
                return false;

            auto pred = g_features->prediction->predict( target->index, m_q3_range + 50.f, 0.f, 0.f, 1.75f );
            if ( !pred.valid ) return false;

            const auto throw_point =
                g_local->position.extend( pred.position, g_local->position.dist_to( pred.position ) * 0.61f );

            const auto travel_time = 0.25f + g_local->position.dist_to( throw_point ) / 1700.f;

            pred = g_features->prediction->predict( target->index, m_q3_range, 0.f, 200.f, travel_time + 1.f );
            if ( !pred.valid || g_local->position.dist_to( pred.position ) <= m_q2_range ) return false;

            m_throw_point = g_local->position.extend(
                pred.position,
                g_local->position.dist_to( pred.position ) * 0.61f
            );
            m_bounce_point = g_local->position.extend(
                m_throw_point,
                g_local->position.dist_to( m_throw_point ) * 1.38f
            );
            m_explosion_point = g_local->position.extend(
                m_bounce_point,
                g_local->position.dist_to( m_throw_point ) * 1.65f
            );
            m_bounce_type      = 3;
            m_predicted_point  = pred.position;
            m_last_calculation = *g_time;

            if ( pred.position.dist_to( m_explosion_point ) > 200.f ||
                static_cast< int >( pred.hitchance ) < g_config->ziggs.q_hitchance->get< int >( ) ||
                g_navgrid->is_wall( m_throw_point ) || g_navgrid->is_wall( m_bounce_point ) ||
                g_features->prediction->minion_in_circle( m_throw_point, 150.f ) ||
                g_features->prediction->minion_in_circle( m_bounce_point, 150.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, m_throw_point ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->ziggs.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1550.f, 200.f, 0.75f );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1550.f, 200.f, 0.75f );
            if ( !pred.valid || ( int )pred.hitchance < 2 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->ziggs.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 1750.f, 300.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_cast_time  = *g_time;
                m_last_w_time     = *g_time;
                m_should_detonate = true;
                g_features->orbwalker->on_cast( );
            }
        }

        auto flee_w( ) -> bool{
            if ( *g_time - m_last_w_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 350.f ) return false;

            if ( cast_spell(
                ESpellSlot::w,
                g_local->position.extend( target->position, g_local->position.dist_to( target->position ) / 2.f )
            ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time     = *g_time;
                m_should_detonate = true;
                return true;
            }

            return false;
        }

        auto w_detonator( ) -> void{
            if ( !m_should_detonate || *g_time - m_last_cast_time <= 0.05f ) return;

            if ( rt_hash( m_slot_w->get_name( ).c_str( ) ) == ct_hash( "ZiggsWToggle" ) ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_cast_time  = *g_time;
                    m_last_w_time     = *g_time;
                    m_should_detonate = false;
                }
            }
        }

        auto dash_helper( ) -> void{
            if ( !m_dash_selected ) {
                if ( !m_slot_w->is_ready( ) ) {
                    if ( m_current_operation != EDashOperation::none ) reset_dasher( );

                    return;
                }

                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                if ( !m_selection_active ) {
                    if ( GetAsyncKeyState( 0x01 ) && GetAsyncKeyState( VK_SHIFT ) ) {
                        m_selection_start   = cursor;
                        m_selection_active  = true;
                        m_current_operation = EDashOperation::selecting;
                    }

                    return;
                }

                if ( !GetAsyncKeyState( 0x01 ) ) {
                    m_selection_end    = cursor;
                    m_dash_selected    = true;
                    m_selection_active = false;
                    m_selection_time   = *g_time;
                }

                return;
            }

            if ( !m_dash_processed ) {
                g_features->orbwalker->allow_movement( false );
                m_current_operation = EDashOperation::processing;
                const auto start    = m_selection_start;
                const auto end      = m_selection_end;

                bool wall_found{ };
                Vec3 prewall_position{ };
                Vec3 wall_position{ };

                const auto selected_length = start.dist_to( end );

                // find wall beginning
                for ( auto i = 0; i <= 30; i++ ) {
                    auto temp = start.extend( end, selected_length / 30.f * i );
                    if ( !g_navgrid->is_wall( temp ) ) {
                        prewall_position = temp;
                        continue;
                    }

                    wall_found    = true;
                    wall_position = temp;
                    break;
                }

                if ( !wall_found ) {
                    std::cout << "[ DASHER ] Processing failed, no wall start found\n";
                    reset_dasher( );
                    return;
                }

                Vec3  wall_end_position{ };
                int   found_end{ };
                float wall_height{ };
                // find the end of the wall
                for ( auto i = 1; i <= 40; i++ ) {
                    auto temp = prewall_position.extend( wall_position, 20.f * i );
                    if ( g_navgrid->is_wall( temp ) ) {
                        if ( g_navgrid->get_height( temp ) > wall_height ) wall_height = g_navgrid->get_height( temp );

                        found_end         = 0;
                        wall_end_position = Vec3( );
                        continue;
                    }

                    if ( wall_end_position.length( ) <= 0.f ) wall_end_position = temp;

                    found_end++;

                    if ( found_end > 3 ) break;
                }

                if ( wall_end_position.length( ) <= 0.f ) {
                    std::cout << "[ DASHER ] Processing failed, no wall end\n";
                    reset_dasher( );
                    return;
                }

                const auto wall_length = prewall_position.dist_to( wall_end_position );

                m_selection_start = prewall_position;
                m_selection_end   = wall_end_position;

                m_dash_start_point       = prewall_position.extend( wall_end_position, -25.f );
                m_dash_end_point         = wall_end_position;
                m_dash_processed         = true;
                m_processing_finish_time = *g_time;

                m_dash_start_point.y = g_navgrid->get_height( m_dash_start_point );

                m_current_operation = EDashOperation::aligning_position;
                std::cout << "[ DASHER ] Processing finished, wall length is       " << wall_length << " | height: " <<
                    wall_height << "\n";

                if ( wall_length > m_dash_max_range || wall_height > 150.f ) {
                    std::cout << "[ DASHER ] Processing failed, wall too thick to cross\n";
                    reset_dasher( );
                    return;
                }
            }

            if ( m_current_operation == EDashOperation::aligning_position ) {
                const auto path_point = m_dash_start_point;

                if ( !m_started_pathing ) {
                    if ( g_local->position.dist_to( path_point ) > 75.f && g_features->orbwalker->send_move_input(
                        path_point,
                        true
                    ) ) {
                        std::cout << "[ DASHER ] Started pathing, blocked orbwalker\n";
                        m_started_pathing = true;
                    }

                    return;
                }

                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                const auto path = aimgr->get_path( );
                if ( path.empty( ) ) return;

                const auto path_end = path[ path.size( ) - 1 ];

                if ( !m_confirmed_path ) {
                    m_confirmed_path = path_end.dist_to( m_dash_start_point ) <= 50.f;

                    if ( *g_time - m_processing_finish_time > 2.f ) {
                        std::cout << "[ DASHER ] Aligning failed, couldnt confirm path fast enough\n";
                        reset_dasher( );
                    }

                    return;
                }

                if ( path_end.dist_to( m_dash_start_point ) > 50.f ) {
                    std::cout << "[ DASHER ] Aligning failed, user canceled procedure\n";
                    reset_dasher( );
                    g_features->orbwalker->allow_movement( true );
                    return;
                }

                if ( g_local->position.dist_to( m_dash_start_point ) <= 35.f ) {
                    //if ( g_features->orbwalker->send_move_input( g_local->position, true ) ) {

                    m_stopped_moving      = true;
                    m_next_operation_time = *g_time + 0.033f;
                    m_current_operation   = EDashOperation::executing_dash;
                    m_movement_stop_point = g_local->position;
                    //}
                }
            }

            if ( m_current_operation == EDashOperation::executing_dash ) {
                if ( m_next_operation_time > *g_time ) return;

                if ( g_local->position.dist_to( m_dash_start_point ) > 45.f ) {
                    std::cout << "[ DASHER ] Execution failed, user canceled procedure\n";
                    reset_dasher( );
                    g_features->orbwalker->allow_movement( true );
                    return;
                }

                if ( !m_is_dash_organized ) {
                    const auto cast_position{
                        m_dash_end_point.extend(
                            g_local->position,
                            g_local->position.dist_to( m_dash_end_point ) + 40.f
                        )
                    };

                    if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                        std::cout << "[ DASHER ] Casting W for dash setup\n";
                        m_is_dash_organized = true;
                        //m_next_operation_time = *g_time + 0.f;
                    }

                    return;
                }

                if ( *g_time < m_next_operation_time ) return;

                if ( rt_hash( m_slot_w->get_name( ).c_str( ) ) == ct_hash( "ZiggsWToggle" ) ) {
                    if ( cast_spell( ESpellSlot::w ) ) {
                        std::cout << "[ DASHER ] Recasting W to start dash\n";
                        m_last_w_time       = *g_time;
                        m_last_cast_time    = *g_time;
                        m_current_operation = EDashOperation::dashing;
                        m_last_dash_time    = *g_time;
                        //reset_dasher( );
                    }

                    return;
                }
            }

            if ( m_current_operation == EDashOperation::dashing && *g_time - m_last_dash_time > 1.f ) {
                reset_dasher( );
                std::cout << "[ DASHER ]: Reseted dash due to procedure complete.\n";
            }
        }

        auto reset_dasher( ) -> void{
            m_dash_selected     = false;
            m_dash_processed    = false;
            m_current_operation = EDashOperation::none;

            m_selection_start     = Vec3( );
            m_selection_end       = Vec3( );
            m_path_end_point      = Vec3( );
            m_dash_start_point    = Vec3( );
            m_dash_end_point      = Vec3( );
            m_movement_stop_point = Vec3( );

            m_selection_time         = 0.f;
            m_selection_active       = 0.f;
            m_confirmed_path         = false;
            m_stopped_moving         = false;
            m_is_dash_organized      = false;
            m_started_pathing        = false;
            m_last_dash_time         = 0.f;
            m_next_operation_time    = 0.f;
            m_processing_finish_time = 0.f;

            g_features->orbwalker->allow_movement( true );
        }

        auto cast_tracker( ) -> void{
            if ( !m_r_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( sci && sci->slot == 3 && sci->server_cast_time > *g_time ) {
                    m_r_active           = true;
                    m_r_server_cast_time = sci->server_cast_time;
                    m_r_end_position     = sci->start_position.dist_to( sci->end_position ) > m_r_range
                                               ? sci->start_position.extend( sci->end_position, m_r_range )
                                               : sci->end_position;
                    m_r_start_position = sci->start_position;

                    m_r_end_time = m_r_start_position.dist_to( m_r_end_position ) > 2700.f
                                       ? m_r_start_position.dist_to( m_r_end_position ) / 2250.f
                                       : 1.625f - 0.375f;
                    return;
                }
            } else if ( *g_time > m_r_server_cast_time + m_r_end_time + 0.25f ) m_r_active = false;

            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 0 || sci->server_cast_time < *g_time ) return;

                m_q_active         = true;
                m_server_cast_time = sci->server_cast_time;
                m_end_position     = sci->start_position.dist_to( sci->end_position ) > 850.f
                                         ? sci->start_position.extend( sci->end_position, 850.f )
                                         : sci->end_position;
                m_start_position = sci->start_position;
                return;
            }

            if ( *g_time > m_server_cast_time + 0.75f ) m_q_active = false;
        }

        auto remove_executable( const int16_t index ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_executable_indexes,
                [ & ]( const int16_t& executable_index ) -> bool{ return executable_index == index; }
            );

            if ( to_remove.empty( ) ) return;

            m_executable_indexes.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_tracked( const int16_t index ) const -> bool{
            for ( const auto inst : m_executable_indexes ) if ( inst == index ) return true;

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 1.1f,
                    target->index,
                    false
                );
            }

            return 0.f;
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
        float m_last_cast_time{ };

        // ult killable indicator
        std::vector< int16_t > m_executable_indexes{ };


        std::vector< Vec3 > m_push_points{ };
        Vec3                m_w_pred{ };

        // bounce simulation
        Vec3  m_throw_point{ };
        Vec3  m_bounce_point{ };
        Vec3  m_explosion_point{ };
        Vec3  m_predicted_point{ };
        float m_last_calculation{ };
        bool  m_double_bounce{ };
        int   m_bounce_type{ };

        // cast track
        bool  m_q_active{ };
        float m_server_cast_time{ };
        Vec3  m_end_position{ };
        Vec3  m_start_position{ };

        bool  m_r_active{ };
        float m_r_server_cast_time{ };
        float m_r_end_time{ };
        Vec3  m_r_end_position{ };
        Vec3  m_r_start_position{ };

        // combat mode
        bool  m_in_combat{ };
        bool  m_threat_nearby{ };
        float m_threat_level{ };

        // e w combo
        bool m_should_e{ };
        bool m_should_w{ };
        Vec3 m_e_position{ };

        float m_combo_start_time{ };

        Vec3 m_w_position{ };


        // dash helper
        bool  m_dash_selected{ };
        bool  m_dash_processed{ };
        float m_processing_finish_time{ };

        float m_selection_time{ };
        bool  m_selection_active{ };
        Vec3  m_selection_start{ };
        Vec3  m_selection_end{ };

        // organization
        EDashOperation m_current_operation{ };
        float          m_next_operation_time{ };
        float          m_last_dash_time{ };

        Vec3  m_path_end_point{ };
        float m_last_path_time{ };
        bool  m_confirmed_path{ };
        bool  m_started_pathing{ };
        bool  m_stopped_moving{ };
        Vec3  m_movement_stop_point{ };

        // execution
        bool m_is_dash_organized{ };

        // dynamic data
        Vec3 m_dash_start_point{ };
        Vec3 m_dash_end_point{ };

        float m_dash_max_range{ 675.f };

        // w detonation
        bool m_should_detonate{ };

        bool m_dash_in_process{ };
        bool m_stopped_movement{ };

        float m_repositioning_dash{ };
        float m_walk_expire_time{ };
        bool  m_use_wall_cast{ };
        Vec3  m_prewall_cast_position{ };
        Vec3  m_prewall{ };
        Vec3  m_wall_end{ };

        Vec3  m_last_q_position{ };
        float m_q_land_time{ };

        Vec3  m_simulated_bounce{ };
        Vec3  m_simulated_goal{ };
        float m_last_simulation_time{ };

        std::vector< float > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage = { 0.f, 300.f, 450.f, 600.f };

        float m_q_range{ 850.f };
        float m_q2_range{ 1175.f };
        float m_q3_range{ 1400.f };

        float m_w_range{ 1000.f };
        float m_e_range{ 900.f };
        float m_r_range{ 5000.f };
    };
}
