#pragma once
#include "module.hpp"
#include "../entity_list.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"
#include "../tracker.hpp"
#include "../../menu/menu.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"
#include "../../utils/directory_manager.hpp"
#include "../../utils/path.hpp"

namespace features::champion_modules {
    class ezreal_module final : public IModule {
    public:
        virtual ~ezreal_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "ezreal_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Ezreal" ); }

        auto initialize( ) -> void override{ m_priority_list = { w_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "ezreal" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto combat     = navigation->add_section( _( "combat mode" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            //const auto combat_settings     = navigation->add_section( _( "combat mode" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->ezreal.q_enabled );
            q_settings->checkbox( _( "killsteal q" ), g_config->ezreal.q_killsteal );
            q_settings->checkbox( _( "auto harass q" ), g_config->ezreal.q_auto_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->ezreal.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max range %" ), g_config->ezreal.q_max_range, 50, 100, 1 );

            w_settings->checkbox( _( "enable" ), g_config->ezreal.w_enabled );
            w_settings->checkbox( _( "prefer w before q" ), g_config->ezreal.w_preferred );
            w_settings->select(
                _( "hitchance" ),
                g_config->ezreal.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            w_settings->slider_int( _( "max range %" ), g_config->ezreal.w_max_range, 50, 100, 1 );


            r_settings->multi_select(
                _( "cast logic " ),
                {
                    g_config->ezreal.r_on_immobile,
                    g_config->ezreal.r_killsteal,
                    g_config->ezreal.r_multihit,
                    g_config->ezreal.r_recall_ult
                },
                { _( "On immobile" ), _( "Killsteal" ), _( "Multihit" ), _( "Recall ult" ) }
            );

            r_settings->select(
                _( "killsteal hitchance" ),
                g_config->ezreal.r_killsteal_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "minimum multihit count" ), g_config->ezreal.r_multihit_count, 2, 5, 1 );


            //r_settings->checkbox(_("prefer delay (?)"), g_config->ezreal.r_recall_ult_delay)->set_tooltip(_("Prefer to wait until last possible time to ult, recommended"));

            spellclear->checkbox( _( "lasthit q" ), g_config->ezreal.lasthit_use_q );
            spellclear->select(
                _( "lasthit logic" ),
                g_config->ezreal.q_lasthit_mode,
                { _( "Only necessary" ), _( "Stack tear" ), _( "Always" ) }
            );
            spellclear->checkbox( _( "fastclear q (hold ctrl)" ), g_config->ezreal.laneclear_use_q );
            spellclear->checkbox( _( "jungleclear q" ), g_config->ezreal.jungleclear_q );

            // combat mode
            combat->checkbox( _( "enable combat logic" ), g_config->ezreal.combat_mode_enabled );
            combat->checkbox( _( "force toggle (hotkey)" ), g_config->ezreal.combat_mode_toggle );
            combat->multi_select(
                _( "combat logic options " ),
                {
                    g_config->ezreal.combat_ignore_w_prefer,
                    g_config->ezreal.combat_ignore_q_hitchance,
                    g_config->ezreal.combat_ignore_w_hitchance,
                    g_config->ezreal.combat_disable_r_logic
                },
                {
                    _( "Ignore prefer w" ),
                    _( "Ignore q hitchance" ),
                    _( "Ignore w hitchance" ),
                    _( "Disable r logic" )
                }
            );

            combat->slider_int( _( "auto toggle range" ), g_config->ezreal.combat_max_threshold, 300, 1000, 25 );

            drawings->checkbox( _( "draw q range" ), g_config->ezreal.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->ezreal.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->ezreal.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ezreal.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw wall e blink position" ), g_config->ezreal.e_draw_blink_position );
            drawings->checkbox( _( "draw combat activation range" ), g_config->ezreal.combat_draw_threshold );
            drawings->select(
                _( "draw combat mode text" ),
                g_config->ezreal.combat_draw_text_mode,
                { _( "Off" ), _( "In combat" ), _( "Always" ) }
            );

            const auto debug = navigation->add_section( _( "debug" ) );
            debug->checkbox( _( "draw prediction" ), g_config->ezreal.show_debug_prediction );
            //show_debug_prediction
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            bool is_q_drawn{ };

            if ( g_config->ezreal.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ezreal.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        64,
                        3.f
                    );

                    is_q_drawn = true;
                }
            }

            if ( g_config->ezreal.w_draw_range->get< bool >( ) && !is_q_drawn ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->ezreal.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            if ( g_config->ezreal.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->ezreal.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 255, 25, 255 ),
                        m_e_range,
                        Renderer::outline,
                        64,
                        2.f
                    );
                }
            }

            if ( g_config->ezreal.e_draw_blink_position->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && slot->is_ready( true ) ) {
                    auto cursor        = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                    auto cast_position = g_local->position.extend( cursor, m_e_range );
                    if ( g_navgrid->is_wall( cast_position ) ) {
                        auto blink_position = get_e_position( cast_position );

                        if ( blink_position.dist_to( g_local->position ) > 550.f ) {
                            g_render->line_3d( g_local->position, blink_position, Color( 255, 255, 25 ), 2.f );

                            Vec2 sp{ };
                            if ( world_to_screen( blink_position, sp ) ) {
                                static auto texture_path = 
                                    path::join(
                                        {
                                            directory_manager::get_resources_path( ),
                                            "champions",
                                            "Ezreal",
                                            "spells",
                                            "Ezreal_e.png"
                                        }
                                    );
                                auto texture = g_render->load_texture_from_file(
                                    texture_path.has_value() ? *texture_path : ""
                                );

                                if ( texture ) {
                                    Vec2 texture_size{ 22.f, 22.f };

                                    g_render->image(
                                        { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f },
                                        texture_size,
                                        texture
                                    );

                                    g_render->box(
                                        { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f },
                                        texture_size,
                                        Color( 25, 25, 25, 255 ),
                                        -1,
                                        1.f
                                    );
                                } else {
                                    g_render->filled_circle( sp, Color( 255, 255, 25 ), 3.f, 16 );

                                    std::string text{ "BLINK END" };
                                    const auto  text_size = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

                                    g_render->text_shadow(
                                        Vec2{ sp.x - text_size.x / 2.f, sp.y - text_size.y * 2.f },
                                        Color( 255, 255, 255 ),
                                        g_fonts->get_block( ),
                                        text.data( ),
                                        8.f
                                    );
                                }
                            }
                        }
                    }
                }
            }

            if ( m_recall_ult_active ) {
                auto& target = g_entity_list.get_by_index( m_target_index );
                if ( !target ) return;

                const auto  duration   = m_predicted_cast_time - m_baseult_start_time;
                const auto  time_left  = m_predicted_cast_time - *g_time;
                std::string champ_name = target->champion_name.text;

                if ( time_left > 0.f ) {
                    auto angle = 360.f - 360.f * ( time_left / duration );
                    g_render->circle_3d( g_local->position, Color( 50, 255, 50 ), 150.f, 2, 50, 3.f, angle );
                }

                Vec2 indicator_base{ 960.f, 600.f };
                Vec2 texture_size{ 40.f, 40.f };

                Vec2 indicator_data{ indicator_base.x, indicator_base.y + 28.f };
                Vec2 texture_position{ indicator_base.x, indicator_base.y - texture_size.y * 0.5f };

                auto box_color = Color( 255, 255, 50 );

                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                    || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                    std::string warning_text = _( "Wont cast due to orbwalker mode " );

                    if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                        warning_text += _(
                            "COMBO"
                        );
                    } else warning_text += _( "FLEE" );

                    Vec2 indicator_warning{ indicator_base.x, indicator_base.y + 48.f };

                    const auto text_size_warning = g_render->get_text_size( warning_text, g_fonts->get_bold( ), 20 );
                    g_render->text_shadow(
                        Vec2( indicator_warning.x - text_size_warning.x / 2.f, indicator_warning.y ),
                        Color( 255, 50, 50 ),
                        g_fonts->get_bold( ),
                        warning_text.data( ),
                        20
                    );
                    box_color = Color( 200, 200, 200 );
                } else if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                    m_recall_ult_active = false;
                    return;
                }

                std::string text      = "RECALL ULT";
                const auto  text_size = g_render->get_text_size( text, g_fonts->get_bold( ), 32 );

                auto data_text = champ_name + " in ";
                auto time_text = std::to_string( time_left );
                time_text.resize( 4 );

                data_text += time_text + "s";
                const auto text_size_data = g_render->get_text_size( data_text, g_fonts->get_bold( ), 16 );

                Vec2 box_start = { indicator_base.x - text_size.x / 2.f - 3.f, indicator_base.y + 3.f };

                g_render->filled_box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, Color( 20, 20, 20, 90 ) );
                g_render->box( box_start, { text_size.x + 6.f, text_size.y + 12.f }, box_color );

                auto texture_path2 =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    texture_path2.has_value() ? *texture_path2 : ""
                );
                if ( texture ) {
                    g_render->image(
                        { texture_position.x - texture_size.x / 2, texture_position.y - texture_size.y / 2 },
                        texture_size,
                        texture
                    );
                }

                g_render->text_shadow(
                    Vec2( indicator_base.x - text_size.x / 2.f, indicator_base.y ),
                    box_color,
                    g_fonts->get_bold( ),
                    text.data( ),
                    32
                );
                g_render->text_shadow(
                    Vec2( indicator_data.x - text_size_data.x / 2.f, indicator_data.y ),
                    Color( 255, 255, 255 ),
                    g_fonts->get_bold( ),
                    data_text.data( ),
                    16
                );
            }

            if ( g_config->ezreal.combat_draw_threshold->get< bool >( ) && m_threat_nearby ) {
                g_render->circle_3d(
                    g_local->position,
                    m_in_combat
                        ? ( m_threat_level >= 1.f
                                ? g_features->orbwalker->get_pulsing_color( )
                                : g_features->orbwalker->get_pulsing_color( ).alpha(
                                    static_cast< int32_t >(
                                        255.f * m_threat_level
                                    )
                                ) )
                        : Color( 255.f, 255.f, 255.f, 255.f * m_threat_level ),
                    g_config->ezreal.combat_max_threshold->get< int >( ),
                    Renderer::outline,
                    72,
                    2.f
                );
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) &&
                ( g_config->ezreal.combat_draw_text_mode->get< int >( ) == 1 && m_in_combat || g_config->ezreal.
                    combat_draw_text_mode->get< int >( ) == 2 ) ) {
                sp.x += 30.f;
                sp.y -= 8.f;

                std::string text{ "COMBAT MODE: " };
                //std::string toggle_text{ "[ MOUSE 5 ] FORCE COMBAT " };
                const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

                std::string text_state{
                    m_in_combat
                        ? "ACTIVE"
                        : g_config->ezreal.combat_mode_enabled->get< bool >( )
                              ? "INACTIVE"
                              : "DISABLED"
                };
                auto state_color = m_in_combat ? g_features->orbwalker->get_pulsing_color( ) : Color( 50, 255, 50 );

                if ( !g_config->ezreal.combat_mode_enabled->get< bool >( ) ) state_color = Color( 255, 50, 50 );

                //color toggle_color =
                //    g_config->ezreal.combat_mode_toggle->get< bool >( ) ? color( 50, 255, 50 ) : color( 190, 190, 190 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_block( ), text.c_str( ), 8 );

                g_render->text_shadow(
                    { sp.x + text_size.x, sp.y },
                    state_color,
                    g_fonts->get_block( ),
                    text_state.c_str( ),
                    8
                );

                //sp.y += 8.f;
                //g_render->text_shadow( sp, toggle_color, g_fonts->get_block( ), toggle_text.c_str( ), 8 );
            }

            if ( !g_config->ezreal.show_debug_prediction->get< bool >( ) || *g_time - m_cast_time > 3.f ) return;

            g_render->circle_3d(
                m_cast_pos,
                Color( 255, 255, 0, 75 ),
                25.f,
                Renderer::E3dCircleFlags::filled | Renderer::E3dCircleFlags::outline,
                20,
                1.f
            );

            Vec2  sp_next;
            auto& path = m_cast_path;

            if ( path.size( ) == 1u ) return;

            for ( auto i = m_next_path_node; i < static_cast< int >( path.size( ) ); i++ ) {
                if ( i == m_next_path_node ) {
                    if ( !world_to_screen( m_target_current_position, sp ) || !sdk::math::world_to_screen(
                        path[ i ],
                        sp_next
                    ) )
                        break;
                } else {
                    if ( !world_to_screen( path[ i - 1 ], sp ) || !
                        sdk::math::world_to_screen( path[ i ], sp_next ) )
                        break;
                }

                g_render->line(
                    { sp.x + 1, sp.y + 1 },
                    { sp_next.x + 1, sp_next.y + 1 },
                    Color( 10, 255, 10, 255 ),
                    1.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            m_did_cast = false;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::flee ) killsteal_r( );

            if ( g_config->ezreal.combat_mode_enabled->get< bool >( ) ) {
                //m_in_combat = g_config->ezreal.combat_mode_toggle->get< bool >( );

                const auto target = g_features->target_selector->get_default_target( );
                if ( target && target->is_alive( ) ) {
                    if ( !g_config->ezreal.combat_mode_toggle->get< bool >( ) ) {
                        const auto distance = target->dist_to_local( );
                        if ( !m_in_combat ) {
                            m_threat_level = distance < g_config->ezreal.combat_max_threshold->get< int >( )
                                                 ? 1.f
                                                 : 1.f -
                                                 std::min(
                                                     ( distance - g_config->ezreal.combat_max_threshold->get< int >( ) )
                                                     /
                                                     200.f,
                                                     1.f
                                                 );

                            m_in_combat     = distance < g_config->ezreal.combat_max_threshold->get< int >( );
                            m_threat_nearby = m_threat_level > 0.f;
                        } else {
                            m_in_combat     = distance < g_config->ezreal.combat_max_threshold->get< int >( ) + 75.f;
                            m_threat_nearby = m_in_combat;
                        }
                    } else {
                        m_in_combat = g_config->ezreal.combat_mode_toggle->get< bool >( );

                        m_threat_level =
                            target->dist_to_local( ) < g_config->ezreal.combat_max_threshold->get< int >( )
                                ? 1.f
                                : 1.f -
                                std::min(
                                    ( target->dist_to_local( ) -
                                        g_config->ezreal.combat_max_threshold->get< int >( ) ) /
                                    200.f,
                                    1.f
                                );

                        m_threat_nearby = m_threat_level > 0.f;
                    }
                } else {
                    m_in_combat     = g_config->ezreal.combat_mode_toggle->get< bool >( );
                    m_threat_nearby = false;
                }
            } else {
                m_in_combat     = false;
                m_threat_nearby = false;
            }

            recall_ult( );
            killsteal_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:

                if ( g_config->ezreal.q_auto_harass->get< bool >( ) && !is_position_in_turret_range( ) &&
                    spell_q( ) )
                    break;

                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:

                if ( g_config->ezreal.q_auto_harass->get< bool >( ) && !is_position_in_turret_range( ) &&
                    spell_q( ) )
                    break;

                if ( lasthit_q( ) || laneclear_q( ) || jungleclear_q( ) ) break;
                break;
            case Orbwalker::EOrbwalkerMode::harass:

                if ( g_config->ezreal.q_auto_harass->get< bool >( ) && spell_q( ) ) break;

                lasthit_q( );
                break;
            default:
                break;
            }

            update_tear_state( );
        }

    private:
        auto combo_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto q_damage = get_spell_damage( ESpellSlot::q, target );

            if ( !m_in_combat || !g_config->ezreal.combat_ignore_w_prefer->get< bool >( ) ) {
                // prefer w over q
                if ( g_config->ezreal.w_preferred->get< bool >( ) && g_features->orbwalker->get_mode( ) ==
                    Orbwalker::EOrbwalkerMode::combo &&
                    m_slot_w->is_ready( true ) &&
                    !g_features->prediction->turret_in_line( g_local->position, target->position, 70.f ) &&
                    helper::get_real_health( target->index, EDamageType::physical_damage ) > q_damage )
                    return false;
            }

            const auto hitchance = m_in_combat && g_config->ezreal.combat_ignore_q_hitchance->get< bool >( )
                                       ? 0
                                       : g_config->ezreal.q_hitchance->get< int >( );

            const auto predicted =
                g_features->prediction->predict(
                    target->index,
                    1200.f * ( g_config->ezreal.q_max_range->get< int >( ) / 100.f ),
                    2000.f,
                    60.f,
                    0.25f,
                    { },
                    true,
                    Prediction::include_ping,
                    Prediction::ESpellType::linear
                );

            if ( !predicted.valid || static_cast< int >( predicted.hitchance ) < hitchance ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    predicted.position,
                    60.f,
                    0.25f,
                    2000.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;
                g_features->orbwalker->on_cast( );

                // save path
                auto aimgr = target->get_ai_manager( );
                if ( aimgr ) {
                    const auto path = aimgr->get_path( );

                    if ( !aimgr->is_moving || path.size( ) == 1 || path.size( ) == aimgr->next_path_node ) return true;

                    m_target_current_position = target->position;
                    m_next_path_node          = aimgr->next_path_node;
                    m_cast_path               = path;
                    m_cast_time               = *g_time;
                    m_cast_pos                = predicted.position;
                }

                return true;
            }

            return false;
        }

        auto combo_w( Object* target ) -> bool{
            if ( !target ) return false;

            const auto will_kill = get_spell_damage( ESpellSlot::q, target ) >
                helper::get_real_health( target->index, EDamageType::physical_damage );

            const auto should_w{
                g_features->orbwalker->is_attackable(
                    target->index,
                    g_local->attack_range + ( m_in_combat ? 125.f : 50.f )
                ) ||
                g_config->ezreal.q_enabled->get< bool >( ) &&
                ( m_slot_q->is_ready( true ) || m_slot_q->cooldown_expire - *g_time <= 1.f ) &&
                !g_features->prediction->minion_in_line( g_local->position, target->position, 60 )
            };

            if ( !should_w || m_slot_q->is_ready( true ) && will_kill ) return false;

            const auto hitchance = m_in_combat && g_config->ezreal.combat_ignore_w_hitchance->get< bool >( )
                                       ? 0
                                       : g_config->ezreal.w_hitchance->get< int >( );

            const auto predicted =
                g_features->prediction->predict(
                    target->index,
                    1200.f * ( g_config->ezreal.w_max_range->get< int >( ) / 100.f ),
                    1700.f,
                    70.f,
                    0.25f,
                    { },
                    true
                );

            if ( !predicted.valid || static_cast< int >( predicted.hitchance ) < hitchance
                || g_features->prediction->turret_in_line( g_local->position, predicted.position, 70.f ) )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_q( ) -> bool override{
            if ( !g_config->ezreal.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) ) return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->ezreal.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_w->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_w( target ) ) return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{ return false; }

        auto spell_r( ) -> bool override{
            if ( g_config->ezreal.combat_disable_r_logic->get< bool >( ) && m_in_combat ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready( true ) )
                return false;


            if ( multihit_r( ) ) return true;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( immobile_r( target ) ) return true;
            }

            return false;
        }

        auto killsteal_r( ) -> bool{
            if ( g_config->ezreal.combat_disable_r_logic->get< bool >( ) && m_in_combat ||
                !g_config->ezreal.r_killsteal->get< bool >( ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                3000.f,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); },
                2
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, 5000.f, 2000.f, 160.f, 1.f, { }, true );

            if ( !predicted.valid ||
                predicted.hitchance <
                static_cast< Prediction::EHitchance >( g_config->ezreal.r_killsteal_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_did_cast = true;
                return true;
            }

            return false;
        }

        auto immobile_r( Object* target ) -> bool{
            if ( !target || !g_config->ezreal.r_on_immobile->get< bool >( ) ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                5000.f,
                2000.f,
                150.f,
                1.f,
                { },
                true
            );
            if ( !predicted.valid || ( int )predicted.hitchance < 4 ) return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_did_cast = true;
                return true;
            }

            return false;
        }

        auto multihit_r( ) -> bool{
            if ( !g_config->ezreal.r_multihit->get< bool >( ) ) return false;

            const auto multihit = get_multihit_position( 3000.f, 2000.f, 160.f, 1.f, true );

            if ( multihit.hit_count < g_config->ezreal.r_multihit_count->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                debug_log(
                    "[ Ezreal ] R Multihit count {} | DIST: {} | T: {}",
                    multihit.hit_count,
                    g_local->position.dist_to( multihit.position ),
                    *g_time
                );

                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->ezreal.lasthit_use_q->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            auto conserve_mana{ true };
            switch ( g_config->ezreal.q_lasthit_mode->get< int >( ) ) {
            case 0:
                break;
            case 1:
                conserve_mana = !m_tear_stackable;
                break;
            default:
                conserve_mana = false;
                break;
            }

            const auto            lasthit_data = get_line_lasthit_target_advanced(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                60.f,
                0.25f,
                1,
                conserve_mana
            );

            if ( !lasthit_data ) return false;

            const auto target = g_entity_list.get_by_index( lasthit_data.value( ).index );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;

                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time * 2.f );
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !g_config->ezreal.laneclear_use_q->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::control )
                ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                60.f,
                true
            );

            if ( !laneclear_data ) return false;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;

                g_features->orbwalker->on_cast( );
                g_features->prediction->add_special_attack(
                    laneclear_data->target_index,
                    laneclear_data->damage,
                    laneclear_data->travel_time,
                    true,
                    ESpellSlot::q
                );
                return true;
            }

            return false;
        }

        auto jungleclear_q( ) -> bool{
            if ( !g_config->ezreal.jungleclear_q->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                60.f,
                true,
                true
            );

            if ( !laneclear_data ) return false;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_did_cast       = true;

                g_features->orbwalker->on_cast( );
                g_features->prediction->add_special_attack(
                    laneclear_data->target_index,
                    laneclear_data->damage,
                    laneclear_data->travel_time,
                    true,
                    ESpellSlot::q
                );
                return true;
            }

            return false;
        }

        auto recall_ult( ) -> void{
            if ( !g_config->ezreal.r_recall_ult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) ) {
                m_recall_ult_active = false;
                return;
            }


            if ( m_recall_ult_active ) {
                base_ult_tracking( );
                return;
            }

            const Object* target{ };
            bool          found_target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) ) continue;

                const auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                const auto recall_time_left = recall->finish_time - *g_time;
                const auto travel_time      = 1.f + g_local->position.dist_to( enemy->position ) / 2000.f;
                if ( travel_time >= recall_time_left ) continue;

                float health_regenerated{ };

                if ( enemy->is_invisible( ) ) {
                    const auto last_seen_data = g_features->tracker->get_last_seen_data( enemy->index );
                    if ( !last_seen_data ) continue;

                    const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                    const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                    if ( time_missing * enemy->movement_speed >= 160.f ) continue;

                    health_regenerated = ( *g_time - last_seen_data->last_seen_time ) * enemy->total_health_regen;
                    health_regenerated += std::ceil( travel_time ) * enemy->total_health_regen;
                } else health_regenerated = std::ceil( travel_time ) * enemy->total_health_regen;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy );
                if ( damage < enemy->health + health_regenerated ) continue;

                target       = enemy;
                found_target = true;
                break;
            }

            if ( !found_target || !target ) return;

            m_target_index       = target->index;
            m_baseult_start_time = *g_time;
            m_recall_ult_active  = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_recall_ult_active || !m_slot_r->is_ready( true ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || target->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 1.f + target->dist_to_local( ) / 2000.f;
            const auto damage           = get_spell_damage( ESpellSlot::r, target.get( ) );

            const auto time_until_hit = recall_time_left - travel_time;
            auto       health_regenerated{ std::ceil( time_until_hit ) * target->total_health_regen };
            auto       min_possible_health_regen = std::ceil( travel_time ) * target->total_health_regen;

            if ( target->is_invisible( ) ) {
                const auto last_seen_data = g_features->tracker->get_last_seen_data( target->index );
                if ( !last_seen_data ) {
                    m_recall_ult_active = false;
                    return;
                }

                const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                if ( time_missing * target->movement_speed > 160.f ) {
                    m_recall_ult_active = false;
                    return;
                }

                health_regenerated += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
                min_possible_health_regen += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
            }

            bool should_cast{ };

            m_predicted_cast_time = recall->finish_time - travel_time - 0.5f;

            if ( damage > target->health + health_regenerated ) should_cast = time_until_hit <= 0.5f;
            else if ( damage > target->health + min_possible_health_regen ) {
                if ( g_config->ezreal.r_recall_ult_delay->get< bool >( ) ) {
                    int        max_wait_time{ };
                    const auto seconds_until_recall = static_cast< int >( std::floor( recall_time_left - 1.f ) );

                    for ( auto i = 1; i <= seconds_until_recall; i++ ) {
                        const auto regen_amount = min_possible_health_regen + target->total_health_regen * i;
                        if ( target->health + regen_amount >= damage ) break;

                        max_wait_time = i;
                    }

                    m_predicted_cast_time = recall->finish_time - travel_time - ( static_cast< float >( max_wait_time )
                        + 1.f ) - 0.5f;
                    should_cast = max_wait_time <= 1;
                } else should_cast = true;
            } else {
                m_recall_ult_active = false;
                return;
            }

            if ( !should_cast ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                m_recall_ult_active = false;
                return;
            }

            if ( cast_spell( ESpellSlot::r, target->position ) ) {
                m_recall_ult_active = false;
                m_last_r_time       = *g_time;
                m_last_cast_time    = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->ezreal.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                1
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, 1200.f, 2000.f, 60.f, 0.25f, { }, true );

            if ( !predicted.valid || g_features->prediction->minion_in_line(
                g_local->position,
                predicted.position,
                60.f
            ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                m_did_cast = true;

                std::cout << "[ KS Q ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto get_e_position( const Vec3& cast_position ) const -> Vec3{
            if ( !g_navgrid->is_wall( cast_position ) ) return cast_position;

            for ( auto i = 1; i <= 20; i++ ) {
                auto points = g_render->get_3d_circle_points( cast_position, 25.f * i, 16 );

                for ( auto point : points ) {
                    if ( point.dist_to( g_local->position ) > m_e_range * 2.f ) continue;

                    if ( !g_navgrid->is_wall( point ) ) return point;
                }
            }

            return cast_position;
        }

        auto update_tear_state( ) -> void{
            if ( g_config->ezreal.q_lasthit_mode->get< int >( ) != 1 || m_is_tear_stacked ) {
                if ( m_is_tear_stacked ) m_tear_stackable = false;

                return;
            }

            for ( auto i = 1; i < 8; i
                  ++ ) // cuz Inventory Slot starts at 1, so we iterate all, 7 is trinket so no check
            {
                const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
                auto       spell_slot        = g_local->spell_book.get_spell_slot( spell_slot_object );

                if ( !spell_slot ) continue;

                auto slot = g_local->inventory.get_inventory_slot( i );
                if ( !slot ) continue;
                auto item_base = slot->get_base_item( );
                if ( !item_base ) continue;
                auto item_data = item_base->get_item_data( );
                if ( !item_data ) continue;


                switch ( static_cast< EItemId >( item_data->id ) ) {
                case EItemId::tear_of_goddess:
                case EItemId::manamune:
                    m_is_tear_stacked = item_base->stacks_left >= 360;
                    m_tear_stackable = spell_slot->is_ready( );
                    return;
                default:
                    break;
                }
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 1.3f,
                    target->index,
                    true
                ) + helper::get_onhit_damage( target->index );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->bonus_attack_damage( ) + g_local->ability_power( ) *
                    0.9f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                const auto compensation = 60.f / target->movement_speed;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f - compensation;
            }
            case ESpellSlot::r:
            {
                const auto tt   = 1.f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 1.f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            default:
                break;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        bool m_did_cast{ };

        // lasthit debug
        float m_last_lasthit_time{ };
        Vec3  m_lasthit_position{ };

        float m_q_range{ 1200.f };
        float m_w_range{ 1200.f };
        float m_e_range{ 475.f };

        Vec3                m_target_current_position{ };
        std::vector< Vec3 > m_cast_path{ };
        int                 m_next_path_node{ };
        Vec3                m_cast_pos{ };
        float               m_cast_time{ };

        // recall ult
        bool    m_recall_ult_active{ };
        int16_t m_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        // combat mode
        bool  m_in_combat{ };
        bool  m_threat_nearby{ };
        float m_threat_level{ };

        // tear
        bool m_tear_stackable{ };
        bool m_is_tear_stacked{ };

        std::array< float, 6 > m_q_damage
        {
            0.f,
            20.f,
            45.f,
            70.f,
            95.f,
            120.f
        };

        std::vector< float > m_r_damage
        {
            0.f,
            350.f,
            500.f,
            650.f
        };
    };
}
