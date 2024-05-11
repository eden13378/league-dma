#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class milio_module final : public IModule {
    public:
        virtual ~milio_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "milio_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Milio" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation    = g_window->push( _( "milio" ), menu_order::champion_module );
            const auto q_settings    = navigation->add_section( _( "q settings" ) );
            const auto drawings      = navigation->add_section( _( "drawings" ) );
            const auto w_settings    = navigation->add_section( _( "w settings" ) );
            const auto e_settings    = navigation->add_section( _( "e settings" ) );
            const auto r_settings    = navigation->add_section( _( "r settings" ) );
            const auto ally_priority = navigation->add_section( _( "priority ally" ) );
            const auto hotkey        = navigation->add_section( _( "semi manual" ) );

            // Q settings
            q_settings->checkbox( _( "enable" ), g_config->milio.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->milio.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->milio.q_flee );
            q_settings->checkbox( _( "killsteal q" ), g_config->milio.q_killsteal );
            q_settings->checkbox( _( "antigapclose q" ), g_config->milio.q_antigapclose );
            q_settings->checkbox( _( "autointerrupt q" ), g_config->milio.q_autointerrupt );
            q_settings->checkbox( _( "bounce ignore moving minion" ), g_config->milio.q_bounce_ignore_moving );
            q_settings->select(
                _( "hitchance" ),
                g_config->milio.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max range %" ), g_config->milio.q_max_range, 75, 100, 1 );
            q_settings->multi_select(
                _( "cast logic " ),
                { g_config->milio.q_direct, g_config->milio.q_bounce },
                { _( "Direct" ), _( "Minion bounce" ) }
            );

            // W settings
            w_settings->checkbox( _( "enable" ), g_config->milio.w_enabled );
            w_settings->checkbox( _( "only in full combo" ), g_config->milio.w_full_combo );

            // E settings
            e_settings->checkbox( _( "enable" ), g_config->milio.e_enabled );
            e_settings->multi_select(
                _( "e block damage from " ),
                {
                    g_config->milio.e_block_damage_from_autoattacks,
                    g_config->milio.e_block_damage_from_spells,
                    g_config->milio.e_block_damage_from_turret_shots,
                    g_config->milio.e_block_damage_from_ignite,
                    g_config->milio.e_block_damage_from_poison,
                    g_config->milio.e_block_damage_from_item_burn
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

            e_settings->multi_select(
                _( "delay e " ),
                {
                    g_config->milio.e_delay_passive,
                    g_config->milio.e_delay_movespeed,
                    g_config->milio.e_delay_shield
                },
                {
                    _( "Passive left" ),
                    _( "MS left" ),
                    _( "Shield left" )
                }
            );

            e_settings->slider_int(
                _( "min skillshot danger to shield" ),
                g_config->milio.e_skillshot_minimum_danger,
                1,
                5,
                1
            );
            e_settings->checkbox( _( "buff ally autoattack" ), g_config->milio.e_increase_damage );
            e_settings->checkbox( _( "allow e self" ), g_config->milio.e_self );


            // R settings
            r_settings->checkbox( _( "enable" ), g_config->milio.r_enabled );
            r_settings->slider_int( _( "min cleanse multihit" ), g_config->milio.r_multihit_count, 1, 5, 1 );
            r_settings->slider_int( _( "cast delay (?)" ), g_config->milio.cleanse_delay, 0, 200, 5 )
                      ->set_tooltip( _( "Delay cast by X milliseconds" ) );

            drawings->multi_select(
                _( "cleanse list" ),
                {
                    g_config->milio.cleanse_stun,
                    g_config->milio.cleanse_root,
                    g_config->milio.cleanse_charm,
                    g_config->milio.cleanse_fear,
                    g_config->milio.cleanse_suppression,
                    g_config->milio.cleanse_taunt,
                    g_config->milio.cleanse_blind,
                    g_config->milio.cleanse_sleep,
                    g_config->milio.cleanse_polymorph,
                    g_config->milio.cleanse_berserk,
                    g_config->milio.cleanse_disarm,
                    g_config->milio.cleanse_drowsy,
                    g_config->milio.cleanse_silence,
                    g_config->milio.cleanse_nasus_w,
                    g_config->milio.cleanse_yasuo_r,
                    g_config->milio.cleanse_ignite,
                    g_config->milio.cleanse_exhaust
                },
                {
                    _( "Stun" ),
                    _( "Root" ),
                    _( "Charm" ),
                    _( "Fear" ),
                    _( "Suppression" ),
                    _( "Taunt" ),
                    _( "Blind" ),
                    _( "Asleep" ),
                    _( "Polymorph" ),
                    _( "Berserk" ),
                    _( "Disarm" ),
                    _( "Drowsy" ),
                    _( "Silence" ),
                    _( "Nasus W" ),
                    _( "Yasuo R" ),
                    _( "Ignite" ),
                    _( "Exhaust" )
                }
            );

            r_settings->slider_int( _( "min heal multihit" ), g_config->milio.r_heal_multihit_count, 1, 5, 1 );
            r_settings->slider_int( _( "heal hp% threshold" ), g_config->milio.r_heal_threshold, 10, 70, 1 );

            // Ally priority settings
            ally_priority->checkbox( _( "enable" ), g_config->milio.ally_priority_enabled )->set_tooltip(
                _( "Press MB5 to select a priority ally to buff with E/W" )
            );
            ally_priority->multi_select(
                _( "e ally if " ),
                {
                    g_config->milio.ally_buff_on_chase,
                    g_config->milio.ally_buff_on_flee,
                    g_config->milio.ally_buff_in_combat
                },
                { _( "Chasing" ), _( "Fleeing" ), _( "In combat" ) }
            );

            ally_priority->checkbox( _( "only buff selected ally (?)" ), g_config->milio.only_buff_priority_ally )
                         ->set_tooltip( _( "When priority ally is nearby, will not cast E/W on other allies" ) );
            ally_priority->checkbox(
                _( "ally override ult requirement" ),
                g_config->milio.ally_override_cleanse_count
            );

            hotkey->checkbox( _( "enable [ hotkey: T ]" ), g_config->milio.semi_manual_we );
            hotkey->multi_select(
                _( "semi manual toggle " ),
                { g_config->milio.semi_manual_e, g_config->milio.semi_manual_w },
                { _( "Cast E" ), _( "Cast W" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->milio.q_draw_range );
            drawings->checkbox( _( "draw q bounce range" ), g_config->milio.q_draw_max_range );
            drawings->checkbox( _( "draw w range" ), g_config->milio.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->milio.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->milio.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->milio.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->milio.q_draw_range->get< bool >( ) || g_config->milio.q_draw_max_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->milio.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    if ( g_config->milio.q_draw_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            m_q_range * ( g_config->milio.q_max_range->get< int >( ) / 100.f ),
                            Renderer::outline,
                            64,
                            3.f
                        );
                    }

                    if ( g_config->milio.q_draw_max_range->get< bool >( ) ) {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            1000.f + 500.f * ( g_config->milio.q_max_range->get< int >( ) / 100.f ),
                            Renderer::outline,
                            64,
                            3.f
                        );
                    }
                }
            }

            if ( g_config->milio.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) && !m_w_active ||
                        !g_config->milio.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 0, 255, 155, 255 ),
                        m_w_range,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }

            if ( g_config->milio.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) && slot->charges > 0 || !g_config->milio.dont_draw_on_cooldown->get<
                        bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 225, 0, 255 ),
                        m_e_range,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            if ( g_config->milio.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->milio.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range + 65.f,
                        Renderer::outline,
                        64,
                        3.f
                    );
                }
            }

            if ( m_q_active && m_q_missile_found && *g_time - m_q_missile_spawn_time <= 0.875f ) {
                g_render->circle_3d(
                    m_q_bounce_end_position,
                    Color( 255, 255, 25, 50 ),
                    250.f,
                    Renderer::outline | Renderer::filled,
                    32,
                    3.f
                );
            }

            if ( m_w_active && m_buddy_found ) {
                auto buddy = g_entity_list.get_by_index( m_buddy_index );
                if ( buddy ) {
                    buddy.update( );

                    g_render->circle_3d(
                        buddy->position,
                        Color( 255, 255, 255, 20 ),
                        400.f,
                        Renderer::outline | Renderer::filled,
                        60,
                        4.f
                    );

                    Vec2 sp{ };
                    if ( m_w_end_time > *g_time && world_to_screen( buddy->position, sp ) ) {
                        auto duration      = m_w_end_time - *g_time;
                        auto duration_text = std::to_string( duration );
                        duration_text.resize( 3 );

                        auto font_size = 28;

                        auto size       = g_render->get_text_size( duration_text, g_fonts->get_nexa( ), font_size );
                        auto dummy_size = g_render->get_text_size( "2.75", g_fonts->get_nexa( ), font_size );
                        auto bar_height = 8.f;


                        Vec2 text_position{ sp.x - size.x / 2.f, sp.y - size.y / 2.f };

                        g_render->filled_box(
                            { sp.x - dummy_size.x / 2.f, sp.y - dummy_size.y / 2.f },
                            { dummy_size.x, dummy_size.y + bar_height },
                            Color( 0, 0, 0, 180 ),
                            -1
                        );

                        auto multiplier = std::clamp( duration / ( m_w_end_time - m_w_start_time ), 0.f, 1.f );

                        auto bar_color = Color(
                            255.f - 255.f * std::clamp( ( multiplier - 0.5f ) / 0.5f, 0.f, 1.f ),
                            255.f * std::clamp( multiplier / 0.5f, 0.f, 1.f ),
                            0.f
                        );

                        g_render->filled_box(
                            { sp.x - dummy_size.x / 2.f, sp.y + dummy_size.y / 2.f },
                            { dummy_size.x, bar_height },
                            Color( 0, 0, 0, 255 ),
                            -1
                        );

                        g_render->filled_box(
                            { sp.x - dummy_size.x / 2.f, sp.y + dummy_size.y / 2.f },
                            { dummy_size.x * multiplier, bar_height },
                            bar_color,
                            -1
                        );

                        g_render->box(
                            { sp.x - dummy_size.x / 2.f, sp.y - dummy_size.y / 2.f },
                            { dummy_size.x, dummy_size.y + bar_height },
                            bar_color,
                            -1,
                            2.f
                        );

                        g_render->text_shadow(
                            text_position,
                            Color( 255, 255, 255 ),
                            g_fonts->get_nexa( ),
                            duration_text.data( ),
                            font_size
                        );
                    }
                }
            }

            if ( m_ally_selected && m_ally_distance <= 3000.f ) {
                auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally ) {
                    ally.update( );

                    auto draw_color = Color( 255, 255, 255 );

                    if ( *g_time - m_ally_selection_time <= 1.7f && *g_time - m_ally_selection_time >= 0.75f ) {
                        auto mod = std::clamp( ( *g_time - m_ally_selection_time - 0.75f ) / 1.f, 0.f, 1.f );
                        mod      = utils::ease::ease_out_quint( mod );

                        auto circle_color = draw_color;
                        circle_color.alpha( static_cast< int32_t >( 255.f - 255.f * mod ) );

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
                        auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 3000.f );

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color,
                                thickness
                            );
                        }

                        if ( !m_w_active && ally->dist_to_local( ) < m_w_range + 300.f && m_slot_w->level > 0 &&
                            m_slot_w->is_ready( ) ) {
                            auto ally_bounding = ally->get_bounding_radius( );
                            g_render->circle_3d(
                                ally->position,
                                Color( 255, 255, 255, 200 ),
                                ally->attack_range + ally_bounding,
                                Renderer::outline,
                                60,
                                2.f
                            );

                            auto increase = 0.075f + 0.025f * m_slot_w->level;

                            auto increased_attack_range = ally->attack_range * ( 1.f + increase );

                            g_render->circle_3d(
                                ally->position,
                                Color( 0, 255, 155 ),
                                increased_attack_range + ally_bounding,
                                Renderer::outline,
                                60,
                                3.f
                            );
                        } else if ( m_w_active ) {
                            g_render->circle_3d(
                                ally->position,
                                Color( 0, 255, 155, 255 ),
                                ally->attack_range + ally->get_bounding_radius( ),
                                Renderer::outline,
                                50,
                                2.f
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

                                auto size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                                Vec2 text_position{
                                    screen_position.x - size.x / 2.f,
                                    screen_position.y - size.y / 2.f
                                };

                                Vec2 background_position = {
                                    screen_position.x - size.x / 2.f,
                                    screen_position.y - size.y / 2.f
                                };
                                auto background_size = size;

                                g_render->filled_box( background_position, background_size, Color( 5, 5, 5, 150 ), 0 );

                                g_render->text_shadow(
                                    text_position,
                                    ally->dist_to_local( ) <= m_e_range ? Color( 60, 255, 20 ) : Color( 255, 255, 25 ),
                                    g_fonts->get_zabel_16px( ),
                                    text.data( ),
                                    16
                                );
                            } /* else {

                                float distance = m_e_range - ally->dist_to_local( );
                                text = std::to_string( static_cast< int >( distance ) ) + "m";
                            }*/
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
                                draw_color.alpha( static_cast< int32_t >( 255.f * eased_modifier ) ),
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
                        auto champ_suqare_texture_path =                             path::join(
                                { directory_manager::get_resources_path( ), "champions", name, name + "_square.png" }
                            );
                        auto        texture = g_render->load_texture_from_file(
                            champ_suqare_texture_path.has_value() ? *champ_suqare_texture_path : ""
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

                std::string text1{ "[ U ] PERMASHIELD: " };
                std::string text_mode{ m_permashield_toggle ? "ON" : "OFF" };

                auto size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_permashield_toggle ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );

                auto height = size.y;

                text1     = { "[ J ] REFRESH SHIELD: " };
                text_mode = { m_refresh_ally_shield ? "ON" : "OFF" };

                size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow(
                    { sp.x, sp.y + height },
                    Color( 255, 255, 255 ),
                    g_fonts->get_zabel_12px( ),
                    text1.c_str( ),
                    12
                );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y + height },
                    m_refresh_ally_shield ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            select_priority_ally( );

            track_q( );
            track_w( );

            if ( !m_key_down && GetAsyncKeyState( 0x55 ) ) {
                m_permashield_toggle = !m_permashield_toggle;
                m_key_down           = true;
            } else if ( m_key_down && !GetAsyncKeyState( 0x55 ) ) m_key_down = false;

            if ( !m_refresh_key_down && GetAsyncKeyState( 0x4A ) ) {
                m_refresh_ally_shield = !m_refresh_ally_shield;
                m_refresh_key_down    = true;
            } else if ( m_refresh_key_down && !GetAsyncKeyState( 0x4A ) ) m_refresh_key_down = false;

            if ( g_features->orbwalker->in_action( ) ) return;


            spell_r( );
            permashield_ally( );
            spell_e( );
            buff_allies_w( );
            semi_manual_we( );

            if ( g_features->evade->is_active( ) ) return;

            killsteal_q( );
            antigapclose_q( );
            autointerrupt_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->milio.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->milio.q_flee->get< bool >( ) ) spell_q( );
                flee_e( );
                break;
            default:
                break;
            }

            //MilioWRetarget
            //std::cout << "E Name: " << m_slot_e->get_name( ) << " | charge: " << m_slot_e->charges << " | addy " << std::hex << m_slot_e.get_address(  ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->milio.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( direct_q( target ) || bounce_q( target ) ) return true;
            }

            return false;
        }

        auto direct_q( Object* target ) -> bool{
            if ( !g_config->milio.q_direct->get< bool >( ) || !target || target->dist_to_local( ) > m_q_range ) {
                return
                    false;
            }

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                m_q_speed,
                m_q_width,
                m_q_delay,
                { },
                m_q_extend
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->milio.q_hitchance->
                    get< int >( ) )
                || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    m_q_width,
                    m_q_delay,
                    m_q_speed
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: Q ] Direct cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto bounce_q( Object* target ) -> bool{
            if ( !g_config->milio.q_bounce->get< bool >( ) || !target ) return false;

            Vec3 cast_position{ };
            bool allow_cast{ };

            const auto is_killable = helper::get_real_health( target->index, EDamageType::magic_damage, 1.f, true ) <
                get_spell_damage( ESpellSlot::q, target );

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->dist_to_local( ) >= m_q_range ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) ||
                    g_features->prediction->minion_in_line(
                        g_local->position,
                        minion->position,
                        m_q_width,
                        minion->network_id
                    )
                )
                    continue;

                auto travel_time = m_q_delay + g_local->position.dist_to( minion->position ) / m_q_speed;

                auto minion_pred = g_features->prediction->predict_default( minion->index, travel_time, false );
                if ( !minion_pred || g_config->milio.q_bounce_ignore_moving->get< bool >( ) && minion->position.dist_to(
                    minion_pred.value( )
                ) > 5.f )
                    continue;

                auto minion_position = minion_pred.has_value( ) ? *minion_pred : minion->position;

                travel_time = m_q_delay + g_local->position.dist_to( minion_position ) / m_q_speed;
                auto pred   =
                    g_features->prediction->predict(
                        target->index,
                        500.f,
                        0.f,
                        0.f,
                        travel_time,
                        minion_position
                    );
                if ( !pred.valid || !is_killable && static_cast< int >( pred.hitchance ) < g_config->milio.q_hitchance->
                    get< int >( ) )
                    continue;

                auto poly =
                    sdk::math::Sector(
                        minion_position,
                        g_local->position.extend(
                            minion_position,
                            g_local->position.dist_to( minion_position ) + 500.f
                        ),
                        65.f,
                        500.f * ( g_config->milio.q_max_range->get< int >( ) / 100.f )
                    )
                    .to_polygon_new( );

                if ( poly.is_outside( pred.position ) ) continue;

                cast_position = minion_position;
                allow_cast    = true;
                break;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: Q ] Minion bounce cast at " << target->champion_name.text << " | Executable: " <<
                    is_killable << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->milio.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                m_q_speed,
                m_q_width,
                m_q_delay,
                { },
                m_q_extend
            );
            if ( !pred.valid ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    m_q_width,
                    m_q_delay,
                    m_q_speed
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Milio: Q ] Killsteal, target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->milio.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target(
                m_q_range,
                m_q_speed,
                m_q_width,
                m_q_delay,
                m_q_extend
            );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                m_q_speed,
                m_q_width,
                m_q_delay,
                { },
                m_q_extend
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    m_q_width,
                    m_q_delay,
                    m_q_speed
                ) )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ Milio: Q ] Antigapclose on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->milio.q_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                m_q_speed,
                m_q_width,
                m_q_delay,
                { },
                m_q_extend
            );
            if ( !pred.valid || ( int )pred.hitchance < 2 ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    m_q_width,
                    m_q_delay,
                    m_q_speed
                ) )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ Milio: Q ] Autointerrupt on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->milio.w_enabled->get< bool >( ) || m_w_active || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            if ( buff_allies_w( ) ) return true;

            return false;
        }

        auto buff_allies_w( ) -> bool{
            if ( !g_config->milio.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            if ( buff_priority_ally( ) ||
                m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) )
                return true;

            const Object* target{ };
            auto          target_priority{ -1 };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->network_id == g_local->network_id || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 || ally->dist_to_local( ) > m_w_range )
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                    sci->server_cast_time < *g_time + g_features->orbwalker->get_ping( ) / 2.f )
                    continue;

                const auto attack_target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !attack_target || !attack_target->is_hero( ) ) continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target          = ally;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: W ] Buff ally attackspeed, target: " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto permashield_ally( ) -> bool{
            if ( !m_permashield_toggle || !m_ally_selected || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || m_slot_e->charges < 1 || !m_slot_e->is_ready( true ) )
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ) return false;

            ally.update( );
            if ( ally->dist_to_local( ) > m_e_range ) return false;

            const auto ally_pred  = g_features->prediction->predict_default( ally->index, 0.25f );
            const auto local_pred = g_features->prediction->predict_default( g_local->index, 0.25f );
            if ( !ally_pred || !local_pred ) return false;

            const auto distance = ally_pred.value( ).dist_to( local_pred.value( ) );
            const auto should_cast{ distance > m_e_range };

            if ( !should_cast ) return false;

            if ( cast_spell( ESpellSlot::e, ally->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] Permashielding priority ally | distance: " << ally->dist_to_local( )
                    << std::endl;
                return true;
            }

            return false;
        }

        auto buff_priority_ally( ) -> bool{
            if ( !m_ally_selected || *g_time - m_last_w_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                g_config->milio.w_full_combo->get< bool >( )
                && ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo || !g_input->
                    is_key_pressed( utils::EKey::control ) ) )
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                ally->dist_to_local( ) > m_w_range )
                return false;

            int cast_reason{ };

            bool allow_cast{ };

            if ( !allow_cast && g_config->milio.ally_buff_in_combat->get< bool >( ) ) {
                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( sci && ( sci->is_autoattack || sci->is_special_attack ) && sci->server_cast_time > *g_time ) {
                    const auto target_index = sci->get_target_index( );
                    const auto ally_target  = g_entity_list.get_by_index( target_index );

                    if ( ally_target && ally_target->is_hero( ) &&
                        ally_target->position.dist_to( ally->position ) <= 900.f )
                        allow_cast = true;
                }
            }

            if ( allow_cast ) cast_reason = 1;

            if ( !allow_cast ) {
                const auto pred = g_features->prediction->predict_default( ally->index, 0.5f, false );
                if ( !pred ) return false;

                const auto ally_direction{ *pred };
                const auto chase_threshold{ 50.f };

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                        enemy->position.dist_to( ally->position ) > 1000.f )
                        continue;

                    auto enemy_pred = g_features->prediction->predict_default( enemy->index, 0.5f, false );
                    if ( !enemy_pred ) return false;

                    auto enemy_direction = *enemy_pred;

                    auto       v1                   = ally_direction - ally->position;
                    auto       v2                   = enemy->position - ally->position;
                    auto       dot                  = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto ally_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    v1                               = enemy_direction - enemy->position;
                    v2                               = ally->position - enemy->position;
                    dot                              = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto enemy_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    const auto is_chasing_ally{
                        ally_direction.dist_to( enemy->position ) >
                        ally->position.dist_to( enemy->position ) + chase_threshold &&
                        enemy_direction.dist_to( ally->position ) + chase_threshold <
                        enemy->position.dist_to( ally->position ) &&
                        enemy_direction_angle <= 25.f
                    };

                    const auto is_ally_chasing{
                        ally_direction.dist_to( enemy->position ) + chase_threshold <
                        ally->position.dist_to( enemy->position ) &&
                        enemy_direction.dist_to( ally->position ) >
                        enemy->position.dist_to( ally->position ) + chase_threshold &&
                        ally_direction_angle <= 25.f
                    };

                    if ( is_ally_chasing && g_config->milio.ally_buff_on_chase->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 2;
                        break;
                    }

                    if ( is_chasing_ally && g_config->milio.ally_buff_on_flee->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 3;
                        break;
                    }
                }
            }

            if ( !allow_cast ) return false;

            std::string reason_text{ };

            switch ( cast_reason ) {
            case 1:
                reason_text = "In combat";
                break;
            case 2:
                reason_text = "Chasing enemy";
                break;
            case 3:
                reason_text = "Fleeing enemy";
                break;
            default:
                std::cout << "unknown ally shield reason: " << cast_reason << std::endl;
                return false;
            }

            if ( cast_spell( ESpellSlot::w, ally->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: W ] Buffed priority ally due to " << reason_text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->milio.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || m_slot_e->charges < 1 || !m_slot_e->is_ready( true ) )
                return false;

            if ( e_refresh_ally_shield( ) || e_buff_ally_spells( ) || e_block_autoattacks( ) || e_block_skillshots( ) ||
                e_block_turret_shots( ) ||
                e_block_ignite_and_poison( ) || e_increase_ally_damage( ) )
                return true;

            return false;
        }

        auto e_refresh_ally_shield( ) -> bool{
            if ( !m_refresh_ally_shield || !m_ally_selected ) return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                ally->dist_to_local( ) > m_e_range )
                return false;

            const auto buff = g_features->buff_cache->get_buff( ally->index, ct_hash( "MilioE" ) );
            if ( !buff || buff->buff_data->end_time - *g_time > 0.175f + g_features->orbwalker->get_ping( ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::e, ally->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] Refreshing priority ally shield duration | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_autoattacks( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) ||
                !g_config->milio.e_block_damage_from_autoattacks->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_local->position;
            const auto pred           =
                g_features->prediction->predict_default( g_local->index, g_features->orbwalker->get_ping( ) / 2.f );
            if ( pred.has_value( ) ) local_position = pred.value( );

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
                    victim->network_id == g_local->network_id && !g_config->milio.e_self->get< bool >( ) ||
                    m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != m_ally_nid )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority( victim->champion_name.text );
                if ( priority < target_priority || should_delay_shield( victim->index ) ) continue;

                target_priority = priority;
                target_nid = victim->network_id;
                cast_allowed = true;
                reason = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from " ) + enemy->get_name( ) +
                    "s autoattack";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_skillshots( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) ||
                !g_config->milio.e_block_damage_from_spells->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_local->position;
            const auto pred           =
                g_features->prediction->predict_default( g_local->index, g_features->orbwalker->get_ping( ) / 2.f );
            if ( pred.has_value( ) ) local_position = pred.value( );

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                auto colliding_spell = get_colliding_skillshot( ally->index );
                if ( !colliding_spell ||
                    colliding_spell->danger < g_config->milio.e_skillshot_minimum_danger->get< int >( ) ||
                    colliding_spell->danger <= 2 && should_delay_shield( ally->index ) ||
                    colliding_spell->time_until_collision > g_features->orbwalker->get_ping( ) * 2.f )
                    continue;

                target_nid      = ally->network_id;
                target_priority = priority;
                cast_allowed    = true;

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from " ) + colliding_spell->name +
                    " skillshot";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_turret_shots( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) ||
                !g_config->milio.e_block_damage_from_turret_shots->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_local->position;
            const auto pred           =
                g_features->prediction->predict_default( g_local->index, g_features->orbwalker->get_ping( ) / 2.f );
            if ( pred.has_value( ) ) local_position = pred.value( );

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
                if ( !victim || !victim->is_hero( ) || victim->dist_to_local( ) > m_e_range ||
                    m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != m_ally_nid )
                    break;

                auto missile_object = g_entity_list.get_by_index( missile->index );
                if ( !missile_object ) break;

                missile_object.update( );

                const auto time_until_impact = missile_object->position.dist_to( victim->position ) / 1200.f;
                if ( time_until_impact > 0.5f + g_features->orbwalker->get_ping( ) * 2.f ) break;

                target_nid   = victim->network_id;
                cast_allowed = true;
                reason       = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from turret shot" );
                break;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_ignite_and_poison( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) ||
                !g_config->milio.e_block_damage_from_ignite->get< bool >( ) &&
                !g_config->milio.e_block_damage_from_poison->get< bool >( ) &&
                !g_config->milio.e_block_damage_from_item_burn->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_local->position;
            const auto pred           = g_features->prediction->predict_default(
                g_local->index,
                g_features->orbwalker->get_ping( ) / 2.f,
                false
            );
            if ( pred.has_value( ) ) local_position = pred.value( );

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority || should_delay_shield( ally->index ) ) continue;

                auto dot_data = get_damage_overtime_data(
                    ally->index,
                    g_config->milio.e_block_damage_from_poison->get< bool >( ),
                    g_config->milio.e_block_damage_from_ignite->get< bool >( ),
                    g_config->milio.e_block_damage_from_item_burn->get< bool >( ),
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

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from DOT " ) + names;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_increase_ally_damage( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) || !g_config->milio.e_increase_damage->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            const auto local_position = g_local->position;

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->network_id == g_local->network_id && ( !g_config->milio.e_self->get< bool >( ) || g_local->
                        level > 4 ) ||
                    ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->milio.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                    sci->server_cast_time < *g_time + g_features->orbwalker->get_ping( ) / 2.f )
                    continue;

                const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target || !target->is_hero( ) ) continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority || should_delay_shield( ally->index ) ) continue;

                target_priority = priority;
                target_nid      = ally->network_id;
                cast_allowed    = true;

                reason = _( "Buff " ) + ally->get_name( ) + " autoattack damage";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_buff_ally_spells( ) -> bool{
            if ( !g_config->milio.e_enabled->get< bool >( ) || !g_config->milio.e_increase_damage->get< bool >( ) ||
                !m_ally_selected )
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                ally->dist_to_local( ) > m_e_range )
                return false;

            auto sci = ally->spell_book.get_spell_cast_info( );
            if ( !sci || sci->server_cast_time > *g_time || sci->slot == 0 ) return false;

            bool        allow_cast{ };
            std::string reason{ };

            switch ( rt_hash( ally->champion_name.text ) ) {
            case ct_hash( "Caitlyn" ):

                if ( sci->slot == 3 ) {
                    allow_cast = true;
                    reason     = "Caitlyn Ultimate\n";
                }
                break;
            default:
                return false;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, ally->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] Buffing spell damage for " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_we( ) -> bool{
            if ( !g_config->milio.semi_manual_we->get< bool >( ) ) return false;

            if ( semi_manual_e( ) || semi_manual_w( ) ) return true;

            return false;
        }

        auto semi_manual_e( ) -> bool{
            if ( !g_config->milio.semi_manual_e->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::E ) ||
                *g_time - m_last_e_time <= 0.4f || *g_time - m_last_cast_time < 0.05f || m_slot_e->charges < 1 || !
                m_slot_e->is_ready( true ) )
                return false;

            bool        allow_cast{ };
            unsigned    target_nid{ };
            std::string ally_name{ };

            if ( m_ally_selected && m_ally_distance <= m_nearby_threshold ) {
                const auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally && ally->is_alive( ) && !ally->is_invisible( ) && ally->get_selectable_flag( ) == 1 &&
                    ally->dist_to_local( ) <= m_e_range ) {
                    allow_cast = true;
                    target_nid = ally->network_id;
                    ally_name  = ally->champion_name.text;
                }
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] Semi manual cast, target ally " << ally_name << " | " << *g_time <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_w( ) -> bool{
            if ( !g_config->milio.semi_manual_w->get< bool >( ) || m_w_active ||
                !g_input->is_key_pressed( utils::EKey::T ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f && *g_time - m_last_e_time > 0.1f || !m_slot_w->is_ready( true ) ||
                g_config->milio.semi_manual_e->get< bool >( ) && m_ally_distance <= m_e_range && m_slot_e->charges > 0
                &&
                m_slot_e->is_ready( true ) && *g_time - m_last_e_time > 1.f )
                return false;

            bool        allow_cast{ };
            unsigned    target_nid{ };
            std::string ally_name{ };

            if ( !m_ally_selected ) return false;

            const auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( ally && ally->is_alive( ) && !ally->is_invisible( ) && ally->get_selectable_flag( ) == 1 &&
                ally->dist_to_local( ) <= m_w_range ) {
                allow_cast = true;
                target_nid = ally->network_id;
                ally_name  = ally->champion_name.text;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::w, target_nid ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: W ] Semi manual cast, target ally " << ally_name << " | " << *g_time <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->milio.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            if ( cleanse_r( ) ) return true;

            return false;
        }

        auto cleanse_r( ) -> bool{
            if ( *g_time - m_last_cast_time <= 0.025f || !m_slot_r->is_ready( true ) || !g_features->buff_cache->
                can_cast( g_local->index ) )
                return false;

            int cleanse_count{ };
            int heal_count{ };
            int total_count{ };

            unsigned last_network_id{ };

            bool force_cast{ };

            int enemy_count{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > 1200.f || enemy->is_dead( ) || enemy->
                    is_invisible( ) )
                    continue;

                ++enemy_count;
            }

            const auto allow_heal = enemy_count > 2
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && g_input->is_key_pressed(
                    utils::EKey::control
                ) && enemy_count > 0;

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->network_id != g_local->network_id &&
                    ally->dist_to_local( ) > m_r_range + g_features->evade->get_bounding_radius( ) || ally->is_dead( )
                    || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 )
                    continue;

                if ( allow_heal ) {
                    const auto health_percent = ally->health / ally->max_health;
                    if ( health_percent <= g_config->milio.r_heal_threshold->get< int >( ) / 100.f ) {
                        ++heal_count;
                        ++total_count;
                        last_network_id = ally->network_id;
                    }
                }

                if ( ally->network_id == g_local->network_id ) continue;

                bool should_cast{ };
                bool prevent_cast{ };

                for ( const auto buff : g_features->buff_cache->get_all_buffs( ally->index ) ) {
                    if ( !buff ) continue;

                    const auto type = static_cast< EBuffType >( buff->buff_data->type );
                    bool       is_buff_cleansable{ };
                    switch ( type ) {
                    case EBuffType::snare:
                        if ( !g_config->milio.cleanse_root->get< bool >( ) ) continue;

                        is_buff_cleansable = true;
                        break;
                    case EBuffType::charm:
                        if ( !g_config->milio.cleanse_charm->get< bool >( ) ) continue;

                        is_buff_cleansable = true;
                        break;
                    case EBuffType::taunt:
                        if ( !g_config->milio.cleanse_taunt->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::stun:
                        if ( !g_config->milio.cleanse_stun->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::fear:
                        if ( !g_config->milio.cleanse_fear->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::polymorph:
                        if ( !g_config->milio.cleanse_polymorph->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::asleep:
                        if ( !g_config->milio.cleanse_sleep->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::drowsy:
                        if ( !g_config->milio.cleanse_drowsy->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::berserk:
                        if ( !g_config->milio.cleanse_berserk->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::disarm:
                        if ( !g_config->milio.cleanse_disarm->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::blind:
                        if ( !g_config->milio.cleanse_blind->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::suppression:
                        if ( !g_config->milio.cleanse_suppression->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::silence:
                        if ( !g_config->milio.cleanse_silence->get< bool >( ) ) continue;
                        is_buff_cleansable = true;
                        break;
                    case EBuffType::knockup:
                    case EBuffType::knockback:
                        prevent_cast = true;
                        break;
                    default:

                        switch ( rt_hash( buff->name.c_str( ) ) ) {
                        case ct_hash( "NasusW" ):
                            if ( !g_config->milio.cleanse_nasus_w->get< bool >( ) ) continue;
                            is_buff_cleansable = true;
                            break;
                        case ct_hash( "SummonerExhaust" ):
                            if ( !g_config->milio.cleanse_exhaust->get< bool >( ) ) continue;
                            is_buff_cleansable = true;
                            break;
                        case ct_hash( "SummonerDot" ):
                            if ( !g_config->milio.cleanse_ignite->get< bool >( ) ) continue;
                            is_buff_cleansable = true;
                            break;
                        case ct_hash( "yasuorknockup" ):
                            if ( !g_config->milio.cleanse_yasuo_r->get< bool >( ) ) continue;
                            is_buff_cleansable = true;
                            break;
                        default:
                            break;
                        }

                        break;
                    }

                    if ( prevent_cast ) break;

                    if ( !should_cast && is_buff_cleansable ) {
                        const auto duration_left = buff->buff_data->end_time - *g_time;
                        const auto duration      = *g_time - buff->buff_data->start_time;

                        if ( duration < g_config->milio.cleanse_delay->get< int >( ) / 1000.f ||
                            duration_left <= 0.2f ) {
                            //std::cout << "duration left: " << duration_left << " skipping.. | " << *g_time << std::endl;

                            continue;
                        }

                        std::cout << "> Allow cast cleanse R | duration: " << duration
                            << " | time left: " << duration_left << " | TYPE: " << ( int )buff->buff_data->type
                            << " T: " << *g_time << std::endl;
                        should_cast = true;
                    }
                }

                if ( !should_cast || prevent_cast ) continue;

                if ( m_ally_selected && ally->network_id == m_ally_nid &&
                    g_config->milio.ally_override_cleanse_count->get< bool >( ) && enemy_count > 0 )
                    force_cast = true;

                ++cleanse_count;

                if ( ally->network_id != last_network_id ) {
                    last_network_id = ally->network_id;
                    ++total_count;
                }

                if ( force_cast ) break;
            }

            const auto primary_reason   = cleanse_count >= g_config->milio.r_multihit_count->get< int >( );
            const auto secondary_reason = heal_count >= g_config->milio.r_heal_multihit_count->get< int >( ) &&
                cleanse_count
                > 0;

            if ( !primary_reason && !secondary_reason && !force_cast ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_r_time    = *g_time;

                std::cout << "[ Milio: R ] Cast on " << total_count << " allies, Cleansed: " << cleanse_count
                    << " | Healed: " << heal_count << " | results " << primary_reason << ":" << secondary_reason
                    << " force: " << force_cast
                    << " | T: " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto flee_e( ) -> bool{
            if ( !g_config->milio.e_flee->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.5f || m_slot_e->charges < 1 || !m_slot_e->is_ready( true ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "MilioE" ) );
            if ( buff && buff->buff_data->end_time - *g_time > 0.1f + g_features->orbwalker->get_ping( ) ) return false;

            auto closest_enemy{ 9999.f };
            bool was_moving_towards{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                    1500.f )
                    continue;

                if ( !was_moving_towards ) {
                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f, false );
                    if ( pred && pred.value( ).dist_to( g_local->position ) < enemy->
                        dist_to_local( ) )
                        was_moving_towards = true;
                }

                if ( enemy->dist_to_local( ) < closest_enemy ) closest_enemy = enemy->dist_to_local( );
            }

            if ( closest_enemy > 950.f || !was_moving_towards ) return false;

            if ( cast_spell( ESpellSlot::e, g_local->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Milio: E ] Flee increase speed\n";

                return true;
            }

            return false;
        }

        auto select_priority_ally( ) -> bool{
            if ( !g_config->milio.ally_priority_enabled->get< bool >( ) ) {
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
            if ( ally->is_dead( ) || ally->is_invisible( ) ) {
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

        auto track_q( ) -> void{
            if ( !m_q_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 0 || sci->is_autoattack || sci->is_special_attack ||
                    sci->server_cast_time < *g_time )
                    return;

                m_q_active           = true;
                m_q_server_cast_time = sci->server_cast_time;
                m_q_end_time         = sci->server_cast_time + 1000.f / 1200.f;
                m_q_missile_found    = false;

                return;
            }

            if ( !m_q_missile_found && m_q_end_time < *g_time ) {
                m_q_active = false;
                return;
            }

            if ( !m_q_missile_found ) {
                for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                    if ( !missile ) continue;

                    auto info = missile->missile_spell_info( );
                    if ( !info ) continue;

                    auto data = info->get_spell_data( );
                    if ( !data ) continue;

                    auto name = data->get_name( );
                    if ( rt_hash( name.data( ) ) != ct_hash( "MilioQHit" ) &&
                        rt_hash( name.data( ) ) != ct_hash( "MilioQHitMinion" ) )
                        continue;

                    m_q_missile_found       = true;
                    m_q_missile_index       = missile->index;
                    m_q_bounce_end_position = missile->missile_end_position;
                    m_q_missile_spawn_time  = missile->missile_spawn_time( );
                    break;
                }

                return;
            }

            auto missile = g_entity_list.get_by_index( m_q_missile_index );
            if ( !missile || *g_time - m_q_missile_spawn_time > 1.f ) {
                // std::cout << "[ Milio: Tracking ] Q Bounce missile despawned, duration: "
                //           << *g_time - m_q_missile_spawn_time << std::endl;

                m_q_active        = false;
                m_q_missile_found = false;
                return;
            }

            missile.update( );
            m_q_bounce_end_position = missile->missile_end_position;
        }

        auto track_w( ) -> void{
            if ( !m_w_active ) {
                if ( rt_hash( m_slot_w->get_name( ).data( ) ) != ct_hash( "MilioWRetarget" ) ) return;

                m_w_active    = true;
                m_buddy_found = false;

                return;
            }

            if ( rt_hash( m_slot_w->get_name( ).data( ) ) != ct_hash( "MilioWRetarget" ) ) {
                m_w_active    = false;
                m_buddy_found = false;
                return;
            }

            if ( !m_buddy_found ) {
                for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
                    if ( !minion || minion->dist_to_local( ) > 1500.f ||
                        rt_hash( minion->name.text ) != ct_hash( "Buddy" ) || minion->is_dead( ) ||
                        minion->get_owner_index( ) != g_local->index )
                        continue;

                    m_buddy_index = minion->index;
                    m_buddy_found = true;
                    break;
                }

                return;
            }

            if ( m_w_end_time > *g_time ) return;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "MilioW" ) );
            if ( !buff ) return;

            m_w_end_time   = buff->buff_data->end_time;
            m_w_start_time = buff->buff_data->start_time;
        }

        static auto should_delay_shield( const int16_t index ) -> bool{
            const auto unit = g_entity_list.get_by_index( index );
            if ( !unit ) return true;

            const auto incoming_damage = helper::get_incoming_damage( unit->index, 0.5f );
            const auto health          = unit->health - incoming_damage;
            const auto health_percent  = health / unit->max_health;

            if ( health_percent <= 0.4f && incoming_damage || health_percent <= 0.2f ) return false;

            if ( g_config->milio.e_delay_movespeed->get< bool >( ) || g_config->milio.e_delay_shield->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( index, ct_hash( "MilioE" ) );
                if ( buff && buff->buff_data->end_time - *g_time > 0.5f &&
                    ( !g_config->milio.e_delay_shield->get< bool >( ) || unit->shield > 0.f ) )
                    return true;
            }

            if ( g_config->milio.e_delay_passive->get< bool >( ) ) {
                const auto buff = g_features->buff_cache->get_buff( index, ct_hash( "miliopbuff" ) );
                if ( buff && buff->buff_data->end_time - *g_time > 0.25f ) return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ m_slot_e->level ] + g_local->ability_power( ) * 0.4f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return tt;

                return 0.25f + g_local->position.dist_to( *pred ) / 1200.f;
            }
            case ESpellSlot::e:
                return g_features->orbwalker->get_ping( );
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

        // q track
        bool  m_q_active{ };
        float m_q_server_cast_time{ };
        float m_q_end_time{ };

        int16_t m_q_missile_index{ };
        bool    m_q_missile_found{ };
        float   m_q_missile_spawn_time{ };
        Vec3    m_q_bounce_end_position{ };

        // w track
        bool    m_w_active{ };
        int16_t m_buddy_index{ };
        bool    m_buddy_found{ };
        float   m_w_end_time{ };
        float   m_w_start_time{ };

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

        float m_nearby_threshold{ 800.f };

        // permashielding
        bool m_permashield_toggle{ };
        bool m_key_down{ };

        bool m_refresh_ally_shield{ };
        bool m_refresh_key_down{ };

        Vec3 m_q_position{ };

        std::vector< float > m_q_damage = { 0.f, 90.f, 135.f, 180.f, 225.f, 270.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 120.f, 160.f, 200.f, 240.f };

        // Spelldata: Q
        float m_q_range{ 1000.f };
        float m_q_speed  = 1200.f;
        float m_q_width  = 60.f;
        float m_q_delay  = 0.25f;
        bool  m_q_extend = false;

        float m_q_bounce_range{ 1500.f };
        float m_w_range{ 1050.f };
        float m_e_range{ 650.f };
        float m_r_range{ 700.f };
    };
}
