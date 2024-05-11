#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"
#include "../../utils/directory_manager.hpp"
#include "../../utils/path.hpp"

namespace features::champion_modules {
    class draven_module final : public IModule {
    public:
        virtual ~draven_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "draven_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Draven" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "draven" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto axe_catch  = navigation->add_section( _( "axe catch" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->draven.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->draven.q_harass );
            q_settings->checkbox( _( "keep q buff in combo" ), g_config->draven.q_keep_buff );

            w_settings->checkbox( _( "enable" ), g_config->draven.w_enabled );
            w_settings->multi_select(
                _( "use conditions" ),
                {
                    g_config->draven.w_on_slow,
                    g_config->draven.w_attackspeed,
                    g_config->draven.w_chase,
                    g_config->draven.w_flee
                },
                { _( "On slow" ), _( "Increase attackspeed" ), _( "On chase" ), _( "On flee" ) }
            );
            e_settings->checkbox( _( "antigapcloser" ), g_config->draven.e_antigapcloser );
            e_settings->checkbox( _( "anti melee" ), g_config->draven.e_anti_melee );

            r_settings->checkbox( _( "enable killsteal" ), g_config->draven.r_killsteal );
            r_settings->select(
                _( "hitchance" ),
                g_config->draven.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->checkbox( _( "recall ult (?)" ), g_config->draven.r_recall_ult )->set_tooltip(
                _( "Will R killable targets who are recalling" )
            );
            r_settings->checkbox( _( "prefer delay (?)" ), g_config->draven.r_recall_ult_delay )->set_tooltip(
                _( "Prefer to wait until last possible time to ult, recommended" )
            );

            axe_catch->checkbox( _( "enable catch" ), g_config->draven.q_catch_axes );
            axe_catch->checkbox( _( "prefer dps over catching (?)" ), g_config->draven.q_prefer_dps )->set_tooltip(
                _( "Will allow orbwalker to always attack during axe catch, might result in dropping axes" )
            );
            //axe_catch->checkbox(_("slower path speed (?)"), g_config->draven.q_slow_order)->set_tooltip(_("If you are having disconnects when catching axes, enable this option"));
            axe_catch->select( _( "pathing mode" ), g_config->draven.catch_mode, { _( "Basic" ), _( "Advanced" ) } );
            axe_catch->slider_int( _( "catch axe range" ), g_config->draven.catch_axe_range, 200, 1000, 5 );


            drawings->checkbox( _( "draw axe catch circle" ), g_config->draven.draw_catch_circle );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            g_local.update( );

            if ( g_config->draven.draw_catch_circle->get< bool >( ) ) {
                auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                g_render->circle_3d(
                    cursor,
                    Color( 35, 135, 250 ),
                    g_config->draven.catch_axe_range->get< int >( ),
                    2,
                    80,
                    2.f
                );
            }

            //g_render->text_3d(g_local->position, color(255, 255, 255), g_fonts->get_bold(), std::to_string(m_axe_count).c_str(), 16.f, true);

            for ( auto& index : m_axes ) {
                auto& missile = g_entity_list.get_by_index( index );
                if ( !missile || *g_time - missile->missile_spawn_time( ) > 1.25f ) continue;

                missile.update( );

                auto time_until_catch = *g_time - missile->missile_spawn_time( );
                auto anim_modifier    = utils::ease::ease_out_quart( std::clamp( time_until_catch / 1.55f, 0.f, 1.f ) );

                /* std::string text = std::to_string(1.25f - time_until_catch);
                 text.resize(4);
                 text += "s";*/

                g_render->circle_3d(
                    missile->missile_end_position,
                    time_until_catch > 1.2f ? Color( 25, 255, 25, 40 ) : Color( 10, 10, 10, 40 ),
                    100.f,
                    Renderer::outline | Renderer::filled,
                    32,
                    2.f
                );


                auto mod_radius = 100.f - 100.f * anim_modifier;
                g_render->circle_3d(
                    missile->missile_end_position,
                    Color( 255, 255, 255 ),
                    100.f,
                    Renderer::outline,
                    32,
                    3.f,
                    360.f * anim_modifier
                );
                //g_render->circle_3d(nearest_point, color(35, 236, 250), 15.f, 2, 40, 2.f);

                //g_render->text_3d(missile->missile_end_position, color(255, 255, 255), g_fonts->get_bold(), text.c_str(), 16, true);
            }

            if ( m_recall_ult_active ) {
                auto& target = g_entity_list.get_by_index( m_ult_target_index );
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

                auto champ_square =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    champ_square.has_value(  ) ? *champ_square : ""
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_axes( );
            update_axe_count( );
            update_attack( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            axe_catcher( );
            antigapclose_e( );
            recall_ult( );
            ult_cast_tracking( );


            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::flee && g_features->orbwalker->
                get_mode( ) != Orbwalker::EOrbwalkerMode::recalling )
                spell_r( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                combo_q( );

                if ( g_features->orbwalker->in_attack( ) ) return;

                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                harass_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                flee_w( );
                break;
            default:
                break;
            }

            // DravenSpinningAttack | also AA missile name
            // DravenFury
            // dravenfurybuff
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto combo_q( ) -> bool{
            if ( !g_config->draven.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            bool allow_q{ };
            if ( g_config->draven.q_keep_buff->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
                if ( buff && buff->buff_data->end_time - *g_time <= 0.1f ) allow_q = true;
            }

            if ( !allow_q && g_features->orbwalker->in_attack( ) && m_axe_count < 2 ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci ) return false;

                const auto& target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target || !target->is_hero( ) ) return false;

                allow_q = true;
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->draven.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                    true
                )
                || g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenFury" ) ) )
                return false;

            bool allow_w{ };
            if ( g_config->draven.w_attackspeed->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "dravenfurybuff" ) );
                if ( !buff || buff->buff_data->end_time - *g_time <= 0.1f ) {
                    const auto target = g_features->target_selector->get_default_target( );
                    if ( target && g_features->orbwalker->
                                               is_attackable(
                                                   target->index,
                                                   g_local->attack_range + 25.f
                                               ) )
                        allow_w = true;
                }
            }

            if ( !allow_w && g_config->draven.w_on_slow->get< bool >( ) ) {
                allow_w = g_features->buff_cache->has_buff_type(
                    g_local->index,
                    { EBuffType::slow, EBuffType::attack_speed_slow },
                    0.2f
                );
            }

            if ( !allow_w && g_config->draven.w_chase->get< bool >( ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) > 900.f ) return false;

                const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.2f );
                const auto pred       = g_features->prediction->predict_default( target->index, 0.2f );
                if ( !pred || !local_pred ) return false;

                allow_w = g_local->position.dist_to( target->position ) < g_local->position.dist_to( *pred ) && target->
                    dist_to_local( ) > target->position.dist_to( *local_pred );
            }

            if ( !allow_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->draven.e_anti_melee->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 275.f ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 130.f, 0.25f );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->draven.r_killsteal->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                3500.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); }
            );
            if ( !target || target->dist_to_local( ) <= 600.f ) return false;

            const auto will_kill = get_spell_damage( ESpellSlot::r, target ) > target->health + target->
                total_health_regen;
            if ( !will_kill ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                5000.f,
                2000.f,
                160.f,
                0.5f
            );

            if ( !predicted.valid || predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->draven.
                r_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto harass_q( ) -> void{
            if ( !g_config->draven.q_harass->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !g_features->orbwalker
                ->in_attack( ) || !m_slot_q->is_ready( true ) )
                return;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
            if ( buff ) return;

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( !sci ) return;

            const auto& target = g_entity_list.get_by_index( sci->get_target_index( ) );
            if ( !target || !target->is_hero( ) ) return;

            if ( cast_spell( ESpellSlot::q ) ) m_last_q_time = *g_time;
        }

        auto flee_w( ) -> void{
            if ( !g_config->draven.w_enabled->get< bool >( ) || !g_config->draven.w_flee->get< bool >( ) || *g_time -
                m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenFury" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 0.2f ) return;

            if ( cast_spell( ESpellSlot::w ) ) m_last_w_time = *g_time;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->draven.e_antigapcloser->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1400.f, 130.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1400.f,
                130.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto axe_catcher( ) -> void{
            if ( !g_config->draven.q_catch_axes->get< bool >( ) ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::recalling ) {
                g_features->orbwalker->allow_movement( true );
                g_features->orbwalker->allow_attacks( true );
                return;
            }

            Vec3 catch_position{ };
            auto axe_land_time{ 999.f };
            bool found_axe{ };

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            for ( const auto& index : m_axes ) {
                const auto& axe = g_entity_list.get_by_index( index );
                if ( !axe ) {
                    remove_axe( index );
                    continue;
                }

                auto axe_position = axe->missile_end_position;
                if ( axe_position.dist_to( cursor ) > g_config->draven.catch_axe_range->get< int >( ) ||
                    false && axe_position.dist_to( g_local->position ) > 350.f )
                    continue;

                const auto time_to_catch = 1.25f - ( *g_time - axe->missile_spawn_time( ) );
                const auto time_to_axe   = ( g_local->position.dist_to( axe_position ) - 150.f ) / g_local->
                    movement_speed;

                if ( time_to_axe > time_to_catch ||
                    found_axe && time_to_catch > axe_land_time )
                    continue;

                catch_position = axe_position;
                axe_land_time  = time_to_catch;
                found_axe      = true;
            }

            if ( !found_axe ) {
                if ( g_features->orbwalker->is_movement_disabled( ) ) {
                    g_features->orbwalker->allow_movement( true );
                    g_features->orbwalker->allow_fast_move( );
                }

                g_features->orbwalker->allow_attacks( true );
                return;
            }

            const auto distance       = g_local->position.dist_to( catch_position );
            const auto free_move_time = distance <= 110.f ? ( 130.f - distance ) / g_local->movement_speed : 0.f;

            if ( axe_land_time <= 0.05f ) return;

            // local inside axe radius
            if ( distance <= 110.f /* || free_move_time > axe_land_time*/ ) {
                if ( g_features->orbwalker->is_movement_disabled( ) && ( free_move_time >= axe_land_time || distance <=
                    50.f ) ) {
                    g_features->orbwalker->allow_movement( true );
                    g_features->orbwalker->allow_fast_move( );
                }

                if ( g_config->draven.catch_mode->get< int >( ) == 1 && m_will_impact && ( !m_moved_for_impact || *
                    g_time - m_last_move_time >= 0.03f ) ) {
                    g_features->orbwalker->allow_attacks( false );
                    g_features->orbwalker->allow_movement( false );

                    const auto move_position = g_local->position.extend( cursor, 150.f );
                    if ( g_features->orbwalker->send_move_input( move_position, true ) ) {
                        m_last_move_time   = *g_time;
                        m_moved_for_impact = true;
                    }

                    return;
                }

                if ( distance <= 95.f ) g_features->orbwalker->allow_attacks( true );

                if ( ( !m_missile_found || m_time_to_impact > 0.125f ) &&
                    g_features->orbwalker->is_movement_disabled( ) && g_features->orbwalker->should_reset_aa( ) &&
                    distance <= 75.f ) {
                    const auto pred = g_features->prediction->predict_default( g_local->index, 0.1f );
                    if ( !pred || g_local->position.dist_to( *pred ) > 1.f ) return;

                    const auto move_position = g_local->position.dist_to( catch_position ) <= 65.f
                                                   ? g_local->position.extend( catch_position, 100.f )
                                                   : catch_position;

                    if ( ( !g_config->draven.q_slow_order->get< bool >( ) || *g_time - m_last_move_time > 0.05f ) &&
                        g_features->orbwalker->send_move_input( move_position, true ) ) {
                        m_last_move_time = *g_time;
                        g_features->orbwalker->allow_movement( true );
                    }
                }

                /* if (!m_will_impact && g_features->orbwalker->get_mode() == c_orbwalker::e_orbwalker_mode::combo)
                 {
                     if (axe_land_time < g_features->orbwalker->get_attack_cast_delay() ||
                         axe_land_time > g_features->orbwalker->get_attack_cast_delay() + 0.075f) g_features->orbwalker->allow_attacks(true);
                     else
                     {
                         std::cout << "delaying attack stack by: " << axe_land_time - g_features->orbwalker->get_attack_cast_delay() << "\n";
                         g_features->orbwalker->allow_attacks(false);
                     }
                 }*/

                return;
            }

            g_features->orbwalker->allow_movement( false );

            const auto nearest_point = catch_position.extend( g_local->position, 140.f );
            const auto time_to_axe   = nearest_point.dist_to( g_local->position ) / g_local->movement_speed;
            const auto extra_time    = time_to_axe < axe_land_time ? time_to_axe - axe_land_time : 0.f;

            g_features->orbwalker->allow_attacks(
                g_config->draven.q_prefer_dps->get< bool >( ) || extra_time > g_features->orbwalker->
                get_attack_cast_delay( )
            );

            // send movement order to axe position
            if ( *g_time - m_last_move_time > 0.05f && !g_features->orbwalker->in_attack( ) ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                const auto path     = aimgr->get_path( );
                const auto path_end = path[ path.size( ) - 1 ];

                if ( path_end.dist_to( catch_position ) < 10.f ) return;

                if ( g_features->orbwalker->send_move_input( catch_position, true ) ) {
                    m_last_move_time = *g_time;
                    //std::cout << "catcher issueorder: " << distance << "\n";
                }
            }
        }

        auto update_axe_count( ) -> void{
            int        axe_count{ };
            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
            if ( buff ) axe_count = buff->stacks( );
            axe_count += m_axes.size( );
            m_axe_count = axe_count;
        }

        auto update_axes( ) -> void{
            for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 3000.f || is_axe_logged( missile->index )
                    || *g_time - missile->missile_spawn_time( ) >= 1.3f )
                    continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto name = data->get_name( );
                if ( name.find( "DravenSpinningReturn" ) == std::string::npos ||
                    name.find( "DravenSpinningReturnCatch" ) != std::string::npos )
                    continue;

                m_axes.push_back( missile->index );
            }

            for ( const auto& index : m_axes ) {
                const auto& missile = g_entity_list.get_by_index( index );
                if ( !missile || *g_time - missile->missile_spawn_time( ) > 1.3f ) {
                    remove_axe( index );
                    continue;
                }
            }
        }

        auto update_attack( ) -> void{
            if ( !m_attack_active ) {
                if ( !g_features->orbwalker->in_attack( ) ) return;

                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "DravenSpinningAttack" ) );
                if ( !buff ) return;

                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci ) return;

                m_attack_active    = true;
                m_attack_time      = sci->start_time;
                m_server_cast_time = sci->server_cast_time;
                m_missile_nid      = sci->missile_nid;
                m_target_index     = sci->get_target_index( );
                return;
            }

            auto& missile = g_entity_list.get_by_network_id( m_missile_nid );
            if ( !missile ) {
                if ( m_missile_found || *g_time > m_server_cast_time && *g_time - m_server_cast_time > g_features->
                    orbwalker->get_attack_cast_delay( ) * 2.f )
                    reset_attack( );

                return;
            }

            m_missile_found = true;
            auto& target    = g_entity_list.get_by_index( m_target_index );
            if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                reset_attack( );
                return;
            }

            missile.update( );
            target.update( );

            const auto time_to_collision = missile->position.dist_to( target->position ) / 1700.f;
            m_time_to_impact             = time_to_collision;

            if ( !m_will_impact && time_to_collision <= 0.05f + g_features->orbwalker->get_ping( ) )
                m_will_impact =
                    true;
        }

        auto reset_attack( ) -> void{
            m_attack_active    = false;
            m_missile_found    = false;
            m_will_impact      = false;
            m_moved_for_impact = false;
            m_attack_time      = 0.f;
            m_server_cast_time = 0.f;
            m_time_to_impact   = 0.f;
            m_target_index     = 0;
            m_missile_nid      = 0;
        }

        auto recall_ult( ) -> void{
            if ( !g_config->draven.r_recall_ult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->
                is_ready( true ) )
                return;

            if ( m_recall_ult_active ) {
                base_ult_tracking( );
                return;
            }

            const Object* target{ };
            bool          found_target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || !enemy->is_recalling( ) ) continue;

                const auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                const auto recall_time_left = recall->finish_time - *g_time;
                const auto travel_time      = 0.5f + g_local->position.dist_to( enemy->position ) / 2000.f;
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

                const auto damage = get_spell_damage( ESpellSlot::r, enemy ) * 2.f;
                if ( damage < enemy->health + health_regenerated ) continue;

                target       = enemy;
                found_target = true;
                break;
            }

            if ( !found_target || !target ) return;

            m_ult_target_index   = target->index;
            m_baseult_start_time = *g_time;
            m_recall_ult_active  = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_recall_ult_active || !m_slot_r->is_ready( true ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_ult_target_index );
            if ( !target || !target->is_recalling( ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 0.5f + target->dist_to_local( ) / 2000.f;
            const auto damage           = get_spell_damage( ESpellSlot::r, target.get( ) ) * 2.f;

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
                if ( time_missing * target->movement_speed >= 160.f ) {
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
                if ( g_config->draven.r_recall_ult_delay->get< bool >( ) ) {
                    int        max_wait_time{ };
                    const auto seconds_until_recall = static_cast< int >( std::floor(
                        recall_time_left - travel_time
                    ) );

                    for ( auto i = 1; i <= seconds_until_recall; i++ ) {
                        const auto regen_amount = min_possible_health_regen + target->total_health_regen * i;
                        if ( target->health + regen_amount >= damage ) break;

                        max_wait_time = i;
                    }

                    m_predicted_cast_time = recall->finish_time - travel_time - ( static_cast< float >( max_wait_time )
                        + 2.f ) - 0.5f;
                    should_cast = max_wait_time <= 2;
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
                m_last_r_time = *g_time;
                m_waiting_recast = true;
                m_recast_time = *g_time + 0.5f + ( g_local->position.dist_to( target->position ) - 850.f ) / 2000.f;
                g_features->orbwalker->on_cast( );
            }
        }

        auto ult_cast_tracking( ) -> void{
            if ( !m_waiting_recast || !m_slot_r->is_ready( ) || *g_time - m_last_r_time <= 0.4f ) {
                if ( m_waiting_recast && *g_time > m_recast_time
                    && rt_hash( m_slot_r->get_name().c_str() ) != ct_hash( "DravenRDoublecast" ) )
                    m_waiting_recast = false;
                return;
            }

            if ( rt_hash( m_slot_r->get_name().c_str() ) != ct_hash( "DravenRDoublecast" ) ) {
                m_waiting_recast = false;
                return;
            }

            if ( *g_time < m_recast_time ) {
                debug_log( "recast in: {}", m_recast_time - *g_time );
                return;
            }

            if ( cast_spell( ESpellSlot::r ) ) {
                debug_log( "Recast R  {}", *g_time );
                m_last_r_time    = *g_time;
                m_waiting_recast = false;
                return;
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->bonus_attack_damage( ) * m_r_ad_modifier[
                        get_slot_r( )->level ],
                    target->index,
                    true
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::r:
            {
                const auto tt   = 0.5f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.5f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto is_axe_logged( const int16_t index ) const -> bool{
            for ( auto& handle : m_axes ) if ( handle == index ) return true;

            return false;
        }

        auto remove_axe( const int16_t index ) -> void{
            const auto to_remove = std::ranges::remove_if(
                m_axes,
                [&]( const int16_t& axe_index ) -> bool{ return axe_index == index; }
            );

            if ( to_remove.empty( ) ) return;

            m_axes.erase( to_remove.begin( ), to_remove.end( ) );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        int                    m_axe_count{ };
        std::vector< int16_t > m_axes{ };

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 175.f, 275.f, 375.f };

        std::vector< float > m_r_ad_modifier{ 0.f, 1.1f, 1.3f, 1.5f };

        // auto catching
        Vec3  m_last_axe_position{ };
        float m_last_move_time{ };

        // attack tracking
        bool     m_attack_active{ };
        bool     m_missile_found{ };
        int16_t  m_target_index{ };
        unsigned m_missile_nid{ };
        float    m_attack_time{ };
        bool     m_will_impact{ };
        bool     m_moved_for_impact{ };
        float    m_server_cast_time{ };
        float    m_time_to_impact{ };

        // silent baseult
        bool    m_recall_ult_active{ };
        int16_t m_ult_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        bool  m_waiting_recast{ };
        float m_recast_time{ };


        float m_q_range{ 0.f };
        float m_w_range{ 0.f };
        float m_e_range{ 1100.f };
        float m_r_range{ 575.f };
    };
}
