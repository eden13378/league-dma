#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class janna_module final : public IModule {
    public:
        virtual ~janna_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "janna_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Janna" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "janna" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->janna.q_enabled );
            q_settings->checkbox( _( "autoharass q2" ), g_config->janna.q_recast );
            q_settings->select(
                _( "hitchance" ),
                g_config->janna.q2_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->checkbox( _( "antigapclose q" ), g_config->janna.q_antigapclose );
            q_settings->checkbox( _( "autointerrupt q" ), g_config->janna.q_autointerrupt );
            q_settings->checkbox( _( "crowdcontrol q" ), g_config->janna.q_on_crowdcontrol );
            q_settings->checkbox( _( "flee q" ), g_config->janna.q_flee );

            q_settings->checkbox( _( "antimelee q" ), g_config->janna.q_antimelee );
            q_settings->slider_int( _( "antimelee max range %" ), g_config->janna.q_antimelee_max_range, 25, 75, 1 );

            w_settings->checkbox( _( "enable" ), g_config->janna.w_enabled );
            w_settings->checkbox( _( "harass w" ), g_config->janna.w_harass );
            w_settings->checkbox( _( "flee w" ), g_config->janna.w_flee );

            e_settings->checkbox( _( "enable" ), g_config->janna.e_enabled );
            e_settings->multi_select(
                _( "e block damage from " ),
                {
                    g_config->janna.e_block_damage_from_autoattacks,
                    g_config->janna.e_block_damage_from_spells,
                    g_config->janna.e_block_damage_from_turret_shots,
                    g_config->janna.e_block_damage_from_ignite,
                    g_config->janna.e_block_damage_from_poison,
                    g_config->janna.e_block_damage_from_item_burn
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
                g_config->janna.e_skillshot_minimum_danger,
                1,
                5,
                1
            );
            e_settings->checkbox( _( "buff ally on autoattack" ), g_config->janna.e_increase_damage );

            const auto ally_priority = navigation->add_section( _( "priority ally" ) );
            ally_priority->checkbox( _( "enable" ), g_config->janna.ally_priority_enabled )
                         ->set_tooltip( _( "Press MB5 to select a priority ally to buff with E/R" ) );
            ally_priority->multi_select(
                _( "e ally if " ),
                {
                    g_config->janna.ally_buff_on_chase,
                    g_config->janna.ally_buff_on_flee,
                    g_config->janna.ally_buff_in_combat
                },
                { _( "Chasing" ), _( "Fleeing" ), _( "In combat" ) }
            );

            ally_priority->checkbox( _( "only buff selected ally (?)" ), g_config->janna.only_buff_priority_ally )
                         ->set_tooltip( _( "When priority ally is nearby, will not cast E on other allies" ) );


            drawings->multi_select(
                _( "draw q " ),
                {
                    g_config->janna.q_draw_range,
                    g_config->janna.q_draw_max_range,
                },
                {
                    _( "Range" ),
                    _( "Max range" )
                }
            );

            drawings->checkbox( _( "draw w range" ), g_config->janna.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->janna.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->janna.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->janna.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw q hitbox" ), g_config->janna.q_draw_hitbox );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            auto q_slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( ( g_config->janna.q_draw_range->get< bool >( ) || g_config->janna.q_draw_max_range->get< bool >( ) ) &&
                q_slot && q_slot->level > 0 &&
                ( q_slot->is_ready( true ) || !g_config->yuumi.dont_draw_on_cooldown->get< bool >( ) ) ) {
                if ( g_config->janna.q_draw_range->get< bool >( ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }

                if ( g_config->janna.q_draw_max_range->get< bool >( ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_max_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( g_config->janna.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->janna.dont_draw_on_cooldown->get< bool >( ) ) ) {
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

            if ( g_config->janna.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->janna.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 235, 204, 52, 255 ),
                        m_e_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }

            if ( g_config->janna.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->janna.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }


            if ( g_config->janna.q_draw_hitbox->get< bool >( ) && m_tornado_alive ) {
                auto ready_color = Color( 0, 255, 155, 25 );

                auto start = m_tornado_start;
                auto end   = start.extend( m_tornado_end, m_tornado_max_range );

                auto direction = ( end - start ).normalize( );

                auto modified = ( end - start ).rotated( 1.58f );

                auto modified_start = start.extend( start + modified, 120.f );
                auto modified_end   = modified_start.extend( modified_start + direction, m_tornado_max_range );

                auto second_mod_start = start.extend( modified_start, -120.f );
                auto second_end       = second_mod_start.extend( second_mod_start + direction, m_tornado_max_range );


                if ( m_q_active ) g_render->rectangle_3d( start, end, 175.f, Color( 255, 255, 255, 225 ), 2, 3.f );

                g_render->rectangle_3d(
                    start,
                    get_smooth_tornado_end_position( ),
                    175.f,
                    ready_color,
                    Renderer::filled | Renderer::outline,
                    3.f
                );


                /* g_render->rectangle_3d( modified_start,
                                              modified_end,
                                         60.f, Color( 255, 25, 25, 15 ),
                                         Renderer::filled | Renderer::outline,
                                         3.f );

                 g_render->rectangle_3d( second_mod_start,
                                        second_end,
                                        60.f,
                                        Color( 255, 25, 25, 15 ),
                                        Renderer::filled | Renderer::outline,
                                        3.f );*/
            }

            if ( m_ally_selected ) {
                auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally ) {
                    ally.update( );

                    auto draw_color = Color( 255, 255, 255 );

                    if ( *g_time - m_ally_selection_time <= 1.7f && *g_time - m_ally_selection_time >= 0.75f ) {
                        auto mod = std::clamp( ( *g_time - m_ally_selection_time - 0.75f ) / 1.f, 0.f, 1.f );
                        mod      = utils::ease::ease_out_quint( mod );

                        auto circle_color = draw_color;
                        circle_color.alpha( 255 - 255 * mod );

                        g_render->circle_3d(
                            ally->position,
                            circle_color,
                            80.f + 150.f * mod,
                            Renderer::outline,
                            50,
                            2.f,
                            360.f,
                            ( g_local->position - ally->position ).normalize( )
                        );
                    }

                    if ( *g_time - m_ally_selection_time > 0.75f ) {
                        auto start_extend = g_local->position.extend( ally->position, 80.f );
                        auto extended     = g_local->position.extend( ally->position, ally->dist_to_local( ) - 80.f );


                        auto max_thickness = 6.f;
                        auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 3500.f );

                        /* g_render->circle_3d( ally->position,
                                             draw_color,
                                             80.f,
                                             c_renderer::outline,
                                             32,
                                             3.f,
                                             360.f,
                                             ( g_local->position - ally->position ) );*/

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color,
                                thickness
                            );
                        }

                        Vec2 screen_position{ };
                        if ( world_to_screen(
                            g_local->position.extend( ally->position, ally->dist_to_local( ) / 2.f ),
                            screen_position
                        ) ) {
                            std::string text{ };

                            if ( ally->dist_to_local( ) > m_e_range ) {
                                auto distance = ally->dist_to_local( ) - m_e_range;

                                text = std::to_string( static_cast< int >( distance ) ) + "m";
                            } else {
                                auto distance = m_e_range - ally->dist_to_local( );

                                text = std::to_string( static_cast< int >( distance ) ) + "m";
                            }

                            auto size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                            Vec2 text_position{ screen_position.x - size.x / 2.f, screen_position.y - size.y / 2.f };

                            Vec2 background_position = {
                                screen_position.x - size.x / 2.f,
                                screen_position.y - size.y / 2.f
                            };
                            auto background_size = size;

                            g_render->filled_box( background_position, background_size, Color( 5, 5, 5, 200 ), 0 );

                            g_render->text_shadow(
                                text_position,
                                ally->dist_to_local( ) <= m_e_range ? Color( 60, 255, 20 ) : Color( 255, 255, 25 ),
                                g_fonts->get_zabel_16px( ),
                                text.data( ),
                                16
                            );
                        }
                    } else {
                        auto modifier       = std::clamp( ( *g_time - m_ally_selection_time ) / 1.f, 0.f, 1.f );
                        auto eased_modifier = utils::ease::ease_out_quart( modifier );


                        auto extended = g_local->position.extend(
                            ally->position,
                            ( ally->dist_to_local( ) - 80.f ) * eased_modifier
                        );
                        auto start_extend = g_local->position.extend( ally->position, 80.f );

                        auto max_thickness = 6.f;
                        auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 2500.f );

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color.alpha( 255 * eased_modifier ),
                                thickness
                            );
                        }

                        g_render->circle_3d(
                            ally->position,
                            draw_color,
                            80.f,
                            Renderer::outline,
                            32,
                            3.f,
                            360.f * eased_modifier,
                            ( g_local->position - ally->position )
                        );
                    }
                }
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                if ( false && m_ally_selected ) {
                    Vec2 texture_size{ 32.f, 32.f };

                    auto ally = g_entity_list.get_by_index( m_ally_index );
                    if ( ally ) {
                        std::string name    = ally->champion_name.text;
                        auto texture_path =                             path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    name,
                                    name + "_square.png"
                                }
                            );
                        auto        texture = g_render->load_texture_from_file(
                            texture_path.has_value(  ) ? *texture_path : ""
                        );
                        if ( texture ) {
                            g_render->image(
                                { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f },
                                texture_size,
                                texture
                            );
                        }

                        std::string text = "ALLY SELECTED";
                        auto        size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                        Vec2 text_position{ sp.x + texture_size.x / 2.f, sp.y - size.y / 2.f };

                        g_render->text_shadow(
                            text_position,
                            Color( 60, 255, 20 ),
                            g_fonts->get_zabel_16px( ),
                            text.data( ),
                            16
                        );
                    }
                }

                // sp.y += 16.f;

                std::string text1{ "[ U ] AUTO-Q1: " };
                std::string text_mode{ m_q1_enable ? "ON" : "OFF" };

                auto size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_q1_enable ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );

                // auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "2065passivemovespeed" ) );
                /*if ( buff ) {

                    std::string texte{ "DURATION: " };
                    texte += std::to_string( buff->buff_data->end_time - *g_time );


                    size = g_render->get_text_size( texte, g_fonts->get_zabel_12px( ), 12 );
                    g_render->text_shadow( { sp.x, sp.y + size.y },
                                           Color( 255, 200, 255 ),
                                           g_fonts->get_zabel_12px( ),
                                           texte.c_str( ),
                                           12 );
                }*/
            }

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


        auto run( ) -> void override{
            initialize_spell_slots( );

            update_cursor( );
            track_q_projectile( );
            select_priority_ally( );

            spell_e( );

            fast_q( );

            semi_manual_e( );

            autointerrupt_q( );
            antigapclose_q( );
            antimelee_q( );
            crowdcontrol_q( );

            if ( !m_key_down && GetAsyncKeyState( 0x55 ) ) {
                m_q1_enable = !m_q1_enable;
                m_key_down  = true;
            } else if ( m_key_down && !GetAsyncKeyState( 0x55 ) ) m_key_down = false;

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;


            logic_q2( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                logic_q1( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->janna.w_harass->get< bool >( ) && spell_w( ) ) break;

                logic_q1( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:

                if ( g_config->janna.w_flee->get< bool >( ) && spell_w( ) ) break;

                flee_q( );
                break;
            default:
                break;
            }

            //std::cout << "name: " << m_slot_q->get_name( ) << " | state " << std::hex
            //          << ( int )m_slot_q->get_usable_state( ) << " | address " << std::hex << m_slot_q.get_address(  ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto logic_q1( ) -> bool{
            if ( !m_q1_enable || m_q_active || *g_time - m_last_q_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;


            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) <= 800.f ) return false;


            auto pred = g_features->prediction->predict(
                target->index,
                m_q_max_range * 0.7f,
                0.f,
                120.f,
                1.25f,
                { },
                true
            );
            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;

                std::cout << "[ Janna: Q1 ] Autoharass | HC: " << ( int )pred.hitchance << std::endl;

                return true;
            }


            return false;
        }

        auto logic_q2( ) -> bool{
            if ( !g_config->janna.q_recast->get< bool >( ) || !m_q_active || *g_time - m_last_q_recast_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( ) )
                return false;


            const auto tornado_end = get_tornado_end_position( );

            auto full_hitbox = sdk::math::Rectangle( m_tornado_start, tornado_end, 175.f );
            auto pred_hitbox = sdk::math::Rectangle( m_tornado_start, tornado_end, 175.f );

            auto start = m_tornado_start;
            auto end   = start.extend( m_tornado_end, m_tornado_max_range );

            auto direction = ( end - start ).normalize( );

            auto modified = ( end - start ).rotated( 1.5707f );

            auto modified_start = start.extend( start + modified, 100.f );
            auto modified_end   = modified_start.extend( modified_start + direction, m_tornado_max_range );

            auto second_mod_start = start.extend( modified_start, -100.f );
            auto second_end       = second_mod_start.extend( second_mod_start + direction, m_tornado_max_range );

            auto charge_time = *g_time - m_tornado_start_time;

            auto edge_hitbox_one = sdk::math::Rectangle( modified_start, modified_end, 80.f ).to_polygon( );
            auto edge_hitbox_two = sdk::math::Rectangle( second_mod_start, second_end, 80.f ).to_polygon( );

            auto total_hitbox_poly = full_hitbox.to_polygon( );
            auto area_hitbox       = pred_hitbox.to_polygon( 1.f );

            auto multiplier = std::clamp( ( *g_time - m_tornado_start_time ) / 3.f, 0.f, 1.f );

            auto speed = 880.f + 520.f * multiplier;
            //std::cout << " speed: " << speed << std::endl;

            int hitcount{ };
            int hitchance_count{ };

            auto require_cast = charge_time > 2.9f;

            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || area_hitbox.is_outside(
                    enemy->position
                ) )
                    continue;


                auto projected = g_features->evade->get_closest_line_point(
                    m_tornado_start,
                    m_tornado_end,
                    enemy->position
                );
                auto travel_time = m_tornado_start.dist_to( projected ) / speed;

                auto pred = g_features->prediction->predict(
                    enemy->index,
                    m_q_max_range,
                    0.f,
                    120.f,
                    travel_time,
                    m_tornado_start,
                    true
                );
                if ( !pred.valid || total_hitbox_poly.is_outside( pred.position ) && ( !require_cast ||
                    total_hitbox_poly.is_outside( pred.default_position ) ) )
                    continue;


                if ( edge_hitbox_one.is_inside( enemy->position ) &&
                    ( edge_hitbox_two.is_inside( pred.position ) ||
                        edge_hitbox_two.is_inside( pred.default_position ) ) ||
                    edge_hitbox_two.is_inside( enemy->position ) &&
                    ( edge_hitbox_one.is_inside( pred.position ) ||
                        edge_hitbox_one.is_inside( pred.default_position ) ) )
                    ++hitchance_count;

                ++hitcount;
                if ( static_cast< int >( pred.hitchance ) > g_config->janna.q2_hitchance->get< int >( ) ) {
                    ++
                        hitchance_count;
                }
            }

            if ( hitchance_count == 0 && hitcount < g_config->janna.q2_hitchance->get< int >( ) && ( !require_cast ||
                hitcount == 0 ) )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_cast_time     = *g_time;
                m_last_q_time        = *g_time;
                m_last_q_recast_time = *g_time;

                std::cout << "[ Janna: Q2 ] Combo Q2 hitcount: " << hitcount
                    << " | hitchance count: " << hitchance_count << std::endl;

                return true;
            }


            return false;
        }

        auto fast_q( ) -> bool{
            if ( !m_doing_fast_q ) return false;


            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_cast_time = *g_time;
                m_doing_fast_q   = false;

                std::cout << "Fast Q finished | " << *g_time << std::endl;

                return true;
            }


            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->janna.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.5f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;


            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_w_range, false ) ) return false;


            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna ] Cast W at " << target->champion_name.text << std::endl;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->janna.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            if ( e_block_autoattacks( ) || e_block_skillshots( ) || e_block_turret_shots( ) ||
                e_block_ignite_and_poison( ) || e_increase_ally_damage( ) )
                return true;

            return false;
        }

        auto e_block_autoattacks( ) -> bool{
            if ( !g_config->janna.e_enabled->get< bool >( ) ||
                !g_config->janna.e_block_damage_from_autoattacks->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                    3000.f )
                    continue;

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time < *
                    g_time )
                    continue;

                const auto victim = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !victim || !victim->is_hero( ) || victim->position.dist_to( local_position ) > m_e_range ||
                    victim->team != g_local->team || victim->is_invisible( ) || victim->get_selectable_flag( ) != 1 ||
                    victim->network_id == g_local->network_id ||
                    m_ally_selected && g_config->janna.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != m_ally_nid )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority( victim->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target_nid = victim->network_id;
                cast_allowed = true;
                reason = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from " ) + enemy->get_name( ) +
                    "s autoattack";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_e( ) -> bool{
            if ( *g_time - m_last_e_time <= 0.4f || !g_input->is_key_pressed( utils::EKey::T ) ||
                *g_time - m_last_cast_time < 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            bool        allow_cast{ };
            unsigned    target_nid{ };
            std::string ally_name{ };

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            if ( m_ally_selected && m_ally_distance <= m_nearby_threshold ) {
                const auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally && ally->is_alive( ) && !ally->is_invisible( ) && ally->get_selectable_flag( ) == 1 &&
                    ally->position.dist_to(local_position) <= m_e_range ) {
                    allow_cast = true;
                    target_nid = ally->network_id;
                    ally_name  = ally->champion_name.text;
                }
            }


            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] Semi manual cast, target ally " << ally_name << " | " << *g_time <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto e_block_skillshots( ) -> bool{
            if ( !g_config->janna.e_enabled->get< bool >( ) ||
                !g_config->janna.e_block_damage_from_spells->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->janna.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                auto colliding_spell = get_colliding_skillshot( ally->index );
                if ( !colliding_spell ||
                    colliding_spell->danger < g_config->janna.e_skillshot_minimum_danger->get< int >( ) ||
                    colliding_spell->time_until_collision > 0.016f + g_features->orbwalker->get_ping( ) * 2.f )
                    continue;

                target_nid      = ally->network_id;
                target_priority = priority;
                cast_allowed    = true;

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from " ) + colliding_spell->name +
                    " spell";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_turret_shots( ) -> bool{
            if ( !g_config->janna.e_enabled->get< bool >( ) ||
                !g_config->janna.e_block_damage_from_turret_shots->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto missile : g_entity_list.get_enemy_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 2000.f ) continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                const auto name_hash = rt_hash( data->get_name( ).data( ) );
                if ( name_hash != ct_hash( "TurretBasicAttack" ) ) continue;

                const auto target_index = missile->get_missile_target_index( );
                if ( target_index == 0 ) continue;

                const auto victim = g_entity_list.get_by_index( target_index );
                if ( !victim || !victim->is_hero( ) || victim->position.dist_to(local_position) > m_e_range ||
                    m_ally_selected && g_config->janna.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != g_local->network_id && victim->
                    network_id != m_ally_nid )
                    break;

                auto missile_object = g_entity_list.get_by_index( missile->index );
                if ( !missile_object ) break;

                missile_object.update( );

                const auto time_until_impact = missile_object->position.dist_to( victim->position ) / 1200.f;
                if ( time_until_impact > 0.075f + g_features->orbwalker->get_ping( ) * 2.f ) break;

                target_nid   = victim->network_id;
                cast_allowed = true;
                reason       = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from turret shot" );
                break;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_ignite_and_poison( ) -> bool{
            if ( !g_config->janna.e_enabled->get< bool >( ) ||
                !g_config->janna.e_block_damage_from_ignite->get< bool >( ) &&
                !g_config->janna.e_block_damage_from_poison->get< bool >( ) &&
                !g_config->janna.e_block_damage_from_item_burn->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->janna.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                auto dot_data = get_damage_overtime_data(
                    ally->index,
                    g_config->janna.e_block_damage_from_poison->get< bool >( ),
                    g_config->janna.e_block_damage_from_ignite->get< bool >( ),
                    g_config->janna.e_block_damage_from_item_burn->get< bool >( ),
                    false
                );

                if ( !dot_data ) continue;

                std::string names{ };
                float       longest_duration{ };

                for ( auto inst : dot_data.value( ) ) {
                    const auto duration = inst.end_time - *g_time;
                    if ( duration <= 1.f ) continue;

                    if ( duration > longest_duration ) longest_duration = duration;
                    names += inst.name + ", ";
                }

                target_nid      = ally->network_id;
                target_priority = priority;
                cast_allowed    = true;

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from " ) + names;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_increase_ally_damage( ) -> bool{
            if ( !g_config->janna.e_enabled->get< bool >( ) || !g_config->janna.e_increase_damage->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);
            // auto pred =
            //     g_features->prediction->predict_default( g_local->index, g_features->orbwalker->get_ping( ) / 2.f );
            // if ( pred.has_value( ) ) local_position = pred.value( );

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->network_id == g_local->network_id || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->janna.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                    sci->server_cast_time < *g_time )
                    continue;

                const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target || !target->is_hero( ) ) continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target_nid      = ally->network_id;
                cast_allowed    = true;

                reason = _( "Buff " ) + ally->get_name( ) + " autoattack damage";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Janna: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto track_q_projectile( ) -> void{
            if ( !m_q_active ) {
                if ( m_tornado_alive && *g_time - m_last_tornado_cast_time > 1.25f ) m_tornado_alive = false;

                if ( m_slot_q->get_active_state( ) == 7 ) return;

                m_last_tornado_time      = m_slot_q->cooldown_expire;
                m_last_tornado_cast_time = 0.f;

                m_q_active           = true;
                m_tornado_start      = m_last_cast_start_position;
                m_tornado_end        = m_last_cast_start_position.extend( m_last_cast_position, m_tornado_max_range );
                m_tornado_start_time = *g_time;
                m_tornado_alive      = true;
                return;
            }


            if ( m_slot_q->get_active_state( ) == 7 ) {
                m_q_active               = false;
                m_last_tornado_cast_time = *g_time;

                std::cout << "Tornado ended charing after " << *g_time - m_tornado_start_time << std::endl;
            }

            return;


            auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "HowlingGale" ) );
            if ( !buff || !m_slot_q->is_ready( ) ) {
                m_q_active = *g_time - m_tornado_start_time < 0.5f && m_slot_q->is_ready( );

                if ( !m_q_active ) {
                    m_last_tornado_cast_time = *g_time;

                    std::cout << "Charge end after " << *g_time - m_tornado_start_time << " | address " << std::hex
                        << m_slot_q.get_address( ) << std::endl;
                }

                return;
            }
        }

        auto update_cursor( ) -> void{
            auto details = m_slot_q->get_details( );
            if ( !details || !m_q_active && ( details->last_unknown_position.dist_to( m_last_cast_position ) <= 5.f ||
                details->last_unknown_position.length( ) <= 100.f ) )
                return;


            /* if ( m_tornado_alive ) {


                if ( details->last_end_position.dist_to( m_last_cast_position ) <= 5.f ||
                     details->last_end_position.length( ) <= 100.f )
                    return;

                    m_last_cast_position = details->last_end_position;
                m_last_cast_start_position = details->last_unknown_position;
                m_last_cast_update         = *g_time;

                m_tornado_start = m_last_cast_start_position;
                m_tornado_end   = m_last_cast_position;
                std::cout << "[ Janna ] Fixed tornado sim path NEW\n";
                return;

            }*/

            m_last_cast_position       = details->last_unknown_position;
            m_last_cast_start_position = details->last_start_position;
            m_last_cast_update         = *g_time;

            if ( m_q_active || !m_q_active && m_tornado_alive ) return;

            if ( *g_time - m_last_tornado_time > 0.2f ) {
                m_tornado_start = m_last_cast_start_position;
                m_tornado_end   = m_tornado_start.extend( m_last_cast_position, m_tornado_max_range );

                //std::cout << "start length: " << m_tornado_start.length( )
                //          << " | end length: " << m_tornado_end.length( ) << std::endl;
                //std::cout << "[ Janna ] Fixed tornado sim path old\n";
            }
        }

        auto get_tornado_end_position( ) const -> Vec3{
            auto base_range = 1100.f;

            if ( !m_q_active ) {
                if ( !m_tornado_alive ) return Vec3( );

                const auto time_charged   = m_last_tornado_cast_time - m_tornado_start_time;
                const auto range_increase = std::min( std::floor( time_charged / 0.1f ) * 22.f, 660.f );

                if ( range_increase < 500.f ) base_range -= 125.f;

                return m_tornado_end.dist_to( m_tornado_start ) < base_range + range_increase
                           ? m_tornado_end
                           : m_tornado_start.extend( m_tornado_end, base_range + range_increase );
            }

            const auto time_charged   = *g_time - m_tornado_start_time;
            const auto range_increase = std::min( std::floor( time_charged / 0.1f ) * 22.f, 660.f );

            if ( range_increase < 500.f ) base_range -= 125.f;

            return m_tornado_end.dist_to( m_tornado_start ) < base_range + range_increase
                       ? m_tornado_end
                       : m_tornado_start.extend( m_tornado_end, base_range + range_increase );
        }

        auto get_smooth_tornado_end_position( ) const -> Vec3{
            if ( !m_q_active ) {
                if ( !m_tornado_alive ) return Vec3( );

                const auto time_charged   = m_last_tornado_cast_time - m_tornado_start_time;
                auto       range_increase = std::min( std::ceil( time_charged / 0.01f ) * 2.f, 660.f );

                range_increase += time_charged > 0.1f ? std::clamp( time_charged / 3.f, 0.f, 1.f ) * 50.f : 0.f;

                return m_tornado_end.dist_to( m_tornado_start ) < 1100.f + range_increase
                           ? m_tornado_end
                           : m_tornado_start.extend( m_tornado_end, 1100.f + range_increase );
            }

            const auto time_charged   = *g_time - m_tornado_start_time;
            auto       range_increase = std::min( std::ceil( time_charged / 0.01f ) * 2.f, 660.f );

            range_increase += time_charged > 0.1f ? std::clamp( time_charged / 3.f, 0.f, 1.f ) * 50.f : 0.f;

            return m_tornado_end.dist_to( m_tornado_start ) < 1100.f + range_increase
                       ? m_tornado_end
                       : m_tornado_start.extend( m_tornado_end, 1100.f + range_increase );
        }

        auto select_priority_ally( ) -> bool{
            if ( !g_config->janna.ally_priority_enabled->get< bool >( ) ) {
                if ( m_ally_selected ) unselect_ally( );

                return false;
            }

            if ( !m_ally_selected ) {
                if ( !GetAsyncKeyState( 0x06 ) ) return false;

                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                int16_t  ally_index{ };
                unsigned ally_nid{ };
                auto     ally_distance_to_cursor{ 200.f };

                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->dist_to_local( ) > 2250.f || ally->network_id == g_local->network_id ||
                        ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 )
                        continue;

                    const auto distance = ally->position.dist_to( cursor );
                    if ( distance > ally_distance_to_cursor ) continue;

                    ally_index              = ally->index;
                    ally_nid                = ally->network_id;
                    ally_distance_to_cursor = distance;
                }

                if ( ally_index == 0 || ally_nid == 0 ) return false;

                m_ally_selected = true;
                m_ally_index    = ally_index;
                m_ally_nid      = ally_nid;

                m_holding_key = true;

                m_ally_selection_time = *g_time;

                return false;
            }

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) {
                unselect_ally( );
                return false;
            }

            ally.update( );
            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) > 3500.f ) {
                unselect_ally( );
                return false;
            }

            m_ally_distance = ally->dist_to_local( );

            if ( m_unselecting ) {
                unselect_ally( );
                return false;
            }

            if ( ( !m_ally_glowing || !m_ally_second_glowing ) && g_function_caller->is_glow_queueable( ) ) {
                Color layer_color{ };

                if ( !m_ally_glowing ) {
                    layer_color = Color( 255, 200, 0 );


                    g_function_caller->enable_glow(
                        m_ally_nid,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        0,
                        5,
                        10
                    );
                    m_ally_glowing = true;
                } else if ( !m_ally_second_glowing ) {
                    layer_color = Color( 255, 255, 0 );


                    g_function_caller->enable_glow(
                        m_ally_nid,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        1,
                        3,
                        0
                    );

                    m_ally_second_glowing = true;
                }
            }

            if ( !GetAsyncKeyState( 0x06 ) ) m_holding_key = false;

            if ( m_holding_key ) return false;

            if ( GetAsyncKeyState( 0x06 ) ) {
                unselect_ally( );
                return false;
            }

            return false;
        }

        auto unselect_ally( ) -> void{
            if ( m_ally_glowing || m_ally_second_glowing ) {
                if ( g_function_caller->is_glow_queueable( ) ) {
                    if ( m_ally_glowing ) {
                        g_function_caller->enable_glow( m_ally_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 0, 3, 3, true );
                        m_ally_glowing = false;
                        m_unselecting  = true;
                    } else if ( m_ally_second_glowing ) {
                        g_function_caller->enable_glow( m_ally_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 1, 3, 3, true );
                        m_ally_second_glowing = false;
                        m_unselecting         = true;
                    }
                }

                if ( m_ally_second_glowing || m_ally_glowing ) return;
            }

            m_ally_selected       = false;
            m_ally_last_glow_time = 0.f;
            m_ally_glowing        = false;
            m_ally_second_glowing = false;
            m_unselecting         = false;
        }

        auto flee_q( ) -> void{
            if ( !g_config->janna.q_flee->get< bool >( ) || m_tornado_alive || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return;


            const auto max_range = 850.f;
            const auto pred      =
                g_features->prediction->predict( target->index, max_range, 880.f, 120.f, 0.f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 2 ) return;

            const auto distance      = target->dist_to_local( );
            const auto pred_distance = g_local->position.dist_to( pred.position );

            if ( distance <= pred_distance ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_doing_fast_q = true;

                std::cout << "[ Janna: Fast Q ] Flee targeted against " << target->champion_name.text << std::endl;
            }
        }

        auto crowdcontrol_q( ) -> void{
            if ( !g_config->janna.q_on_crowdcontrol->get< bool >( ) || m_tornado_alive || *g_time - m_last_q_time <=
                0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range * 1.1f ) return;


            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range,
                    880.f,
                    120.f,
                    0.f,
                    { },
                    true,
                    Prediction::include_ping | Prediction::extend_crowdcontrol
                );
            if ( !pred.valid || ( int )pred.hitchance < 4 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_doing_fast_q = true;

                std::cout << "[ Janna: Fast Q ] Crowdcontrol extension, target is " << target->champion_name.text <<
                    std::endl;
            }
        }

        auto antimelee_q( ) -> void{
            if ( !g_config->janna.q_antimelee->get< bool >( ) || m_tornado_alive ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_q_range ) return;


            const auto max_range = m_q_range * ( g_config->janna.q_antimelee_max_range->get< int >( ) / 100.f );
            const auto pred = g_features->prediction->predict( target->index, max_range, 880.f, 120.f, 0.f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 1 ) return;

            const auto distance      = target->dist_to_local( );
            const auto pred_distance = g_local->position.dist_to( pred.position );

            if ( distance <= pred_distance ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_doing_fast_q = true;

                std::cout << "[ Janna: Fast Q ] Anti-melee against " << target->champion_name.text << std::endl;
            }
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->janna.q_antigapclose->get< bool >( ) || m_tornado_alive ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 880.f, 120.f, 0.f, true );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 880.f, 120.f, 0.f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_doing_fast_q = true;

                std::cout << "[ Janna: Fast Q ] Anti-gapclose against " << target->champion_name.text << std::endl;
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->janna.q_autointerrupt->get< bool >( ) || m_tornado_alive ||
                *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 880.f, 120.f, 0.f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                m_doing_fast_q = true;

                std::cout << "[ Janna: Fast Q ] Auto-interrupted " << target->champion_name.text << std::endl;
            }
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
        float m_last_q_recast_time{ };

        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        // auto q
        bool m_q1_enable{ };
        bool m_key_down{ };

        // fast q
        bool m_doing_fast_q{ };

        // priority ally logic
        bool     m_ally_selected{ };
        int16_t  m_ally_index{ };
        unsigned m_ally_nid{ };

        bool m_holding_key{ };
        bool m_unselecting{ };

        bool  m_ally_glowing{ };
        bool  m_ally_second_glowing{ };
        float m_ally_last_glow_time{ };

        float m_ally_selection_time{ };
        float m_ally_distance{ };

        float m_nearby_threshold{ 750.f };

        // q things
        bool  m_q_active{ };
        bool  m_q_charging{ };
        Vec3  m_tornado_start{ };
        Vec3  m_tornado_end{ };
        float m_tornado_start_time{ };

        bool  m_tornado_alive{ };
        float m_last_tornado_time{ };
        float m_last_tornado_cast_time{ };

        // TRACKING CAST
        Vec3  m_last_cast_position{ };
        Vec3  m_last_cast_start_position{ };
        float m_last_cast_update{ };

        float m_tornado_max_range{ 1720.f };

        std::vector< float > m_q_damage = { 0.f, 80.f, 130.f, 180.f, 230.f, 280.f };

        float m_q_range{ 1100.f };
        float m_q_max_range{ 1760.f };

        float m_w_range{ 700.f };
        float m_e_range{ 800.f };
        float m_r_range{ 1100.f };
    };
} // namespace features::champion_modules
