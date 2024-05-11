#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/ai_manager.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class yasuo_module final : public IModule {
    public:
        virtual ~yasuo_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "yasuo_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Yasuo" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "yasuo" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->yasuo.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->yasuo.q_harass );
            q_settings->checkbox( _( "exploit q(?)" ), g_config->yasuo.q_exploit )->set_tooltip(
                _( "When doing E->Q combo, Q will have no cooldown" )
            );
            q_settings->select(
                _( "tornado hitchance" ),
                g_config->yasuo.q_tornado_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->yasuo.e_enabled );
            e_settings->checkbox( _( "farm e" ), g_config->yasuo.e_lasthit );
            e_settings->checkbox( _( "flee e" ), g_config->yasuo.e_flee );
            e_settings->checkbox( _( "allow e underturret" ), g_config->yasuo.e_underturret );

            r_settings->checkbox( _( "enable" ), g_config->yasuo.r_enabled );
            r_settings->slider_int( "r when target hp% < x", g_config->yasuo.r_min_health_percent, 5, 100 );
            r_settings->slider_int( "r when knocked enemy >= x", g_config->yasuo.r_min_targets, 1, 5 );

            spellclear->select(
                _( "lasthit q" ),
                g_config->yasuo.q_lasthit_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "laneclear q" ),
                g_config->yasuo.q_laneclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "fastclear q" ),
                g_config->yasuo.q_fastclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "jungleclear q" ),
                g_config->yasuo.q_jungleclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->yasuo.q_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->yasuo.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->yasuo.dont_draw_on_cooldown );
            //drawings->checkbox( _( "draw q exploit area" ), g_config->yasuo.draw_exploit_area );// draw_exploit_area
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->yasuo.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yasuo.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        m_tornado_q
                            ? g_features->orbwalker->animate_color(
                                Color( 31, 88, 255, 255 ),
                                EAnimationType::pulse,
                                5,
                                255
                            )
                            : Color( 31, 88, 255, 255 ),
                        m_tornado_q ? m_q3_range : m_q_range,
                        Renderer::outline,
                        80,
                        3.f
                    );
                }
            }

            if ( g_config->yasuo.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yasuo.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( m_dash_active && g_local->position.dist_to( m_dash_start ) < m_dash_start.dist_to( m_dash_end ) ) {
                const auto dash_position = m_dash_end.extend( m_dash_start, g_local->position.dist_to( m_dash_end ) );

                g_render->line_3d( dash_position, m_dash_end, Color( 255, 255, 0 ), 7.f );
            }

            Vec2 sp{ };
            if ( m_exploit_available && world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                // sp.y += 16.f;

                const std::string text1{ "EXPLOIT: " };
                std::string       text_mode{ "READY" };
                auto              mode_color = Color( 50, 255, 50 );

                if ( m_exploit_available && !m_slot_q->is_ready( ) ) {
                    text_mode = std::to_string( m_slot_q->cooldown_expire - *g_time );
                    text_mode.resize( 3 );

                    mode_color = Color( 255, 255, 20 );
                }

                const auto size = g_render->get_text_size( text1, g_fonts->get_zabel_16px( ), 16 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_16px( ), text1.c_str( ), 16 );
                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    mode_color,
                    g_fonts->get_zabel_16px( ),
                    text_mode.c_str( ),
                    16
                );

                /*sp.y += 12.f;

                text1     = { "[ MB3 ] FLASH COMBO: " };
                text_mode = { m_flash_moving_dagger ? "ON" : "OFF" };

                size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );
                g_render->text_shadow( { sp.x + size.x, sp.y },
                                       m_flash_moving_dagger ? Color( 50, 255, 50 ) : Color( 255, 50, 50 ),
                                       g_fonts->get_zabel_12px( ),
                                       text_mode.c_str( ),
                                       12 );*/
            }

            return;

            if ( !g_config->yasuo.draw_exploit_area->get< bool >( ) ) return;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            g_render->circle_3d(
                g_local->position.extend( target->position, 475.f ),
                Color( 255, 50, 50 ),
                215,
                2,
                75,
                2.f
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            dash_tracking( );

            m_tornado_q         = rt_hash( m_slot_q->get_name().c_str() ) == ct_hash( "YasuoQ3Wrapper" );
            m_exploit_available = rt_hash( m_slot_q->get_name( ).data( ) ) != ct_hash( "YasuoQ1Wrapper" ) && g_config->
                yasuo.q_exploit->get< bool >( );

            if ( g_features->orbwalker->in_action( ) ) return;

            spell_r( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return;

            exploit_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                exploit_e( );
                combo_e( );
            //spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->yasuo.q_harass->get< bool >( ) ) spell_q( );

                lasthit_q( );
                lasthit_e( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( GetAsyncKeyState( VK_CONTROL ) ) stack_minion_q( );

                lasthit_e( );
                lasthit_q( );
                laneclear_q( );
                fastclear_q( );
                jungleclear_q( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                lasthit_q( );
                lasthit_e( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->yasuo.e_flee->get< bool >( ) ) flee_e( );
                exploit_q( );
                break;
            default:
                break;
            }


            return;

            std::cout << "[ Slot: Q ] Name: " << m_slot_q->get_name( ) << " | State: " << std::dec
                << m_slot_q->get_usable_state( ) << " | 0x" << std::hex << m_slot_q.get_address( ) << std::endl;

            if ( m_printed ) return;

            std::cout << "[ Slot: E ] Active: " << ( int )m_slot_e->get_active_state( ) << " | 0x" << std::hex <<
                m_slot_e.get_address( )
                << std::endl;

            m_printed = true;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->yasuo.q_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.2f || *g_time -
                m_last_exploit_time <= 0.2f
                || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr || aimgr->is_dashing ) return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) < m_e_range && m_exploit_available && m_slot_e->
                is_ready( ) )
                return false;

            auto       cast_time = 0.4f;
            const auto bas       = g_local->bonus_attack_speed;

            if ( bas >= 1.11f ) cast_time = 0.133f;
            else if ( bas >= 1.05f ) cast_time = 0.148f;
            else if ( bas >= 0.9f ) cast_time = 0.184f;
            else if ( bas >= 0.75f ) cast_time = 0.22f;
            else if ( bas >= 0.6f ) cast_time = 0.256f;
            else if ( bas >= 0.45f ) cast_time = 0.292f;
            else if ( bas >= 0.3f ) cast_time = 0.328f;
            else if ( bas >= 0.15f ) cast_time = 0.364f;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_tornado_q ? m_q3_range : m_q_range,
                m_tornado_q ? 1200.f : 0.f,
                m_tornado_q ? 90.f : 20.f,
                cast_time,
                { },
                true
            );

            const auto hitchance = m_tornado_q
                                       ? static_cast< Prediction::EHitchance >( g_config->yasuo.q_tornado_hitchance->get
                                           <
                                               int >( ) )
                                       : static_cast< Prediction::EHitchance >( 0 );

            if ( !pred.valid ||
                pred.hitchance < hitchance &&
                !will_kill(
                    target,
                    [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
                )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                // g_features->orbwalker->set_cast_time( cast_time );
                m_last_q_time = *g_time;

                //std::cout << "default q " << std::endl;
                return true;
            }

            return false;
        }

        auto exploit_q( ) -> void{
            if ( !m_dash_active || !m_exploit_available || *g_time - m_last_exploit_time <= 0.5f ||
                !m_slot_q->is_ready( ) )
                return;


            if ( cast_spell( ESpellSlot::q, Vec3( 5000000.f, 50000000.f, 5000000.f ) ) ) {
                m_last_exploit_time = *g_time;

                std::cout << "[ Yasuo: Q ] Exploit cast | " << *g_time << std::endl;
            }
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->yasuo.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_e_range || g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "YasuoE" )
            ) )
                return false;

            auto dash_end_position = g_local->position.extend( target->position, 475.f );
            dash_end_position.y    = g_navgrid->get_height( dash_end_position );

            const auto dash_time = 450.f / ( 750.f + g_local->movement_speed * 0.6f );

            const auto possible_exploit = m_tornado_q || g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "YasuoQ1" )
            );

            const auto pred = g_features->prediction->predict_default( target->index, dash_time );
            if ( !pred || m_slot_q->is_ready( ) && dash_end_position.dist_to( *pred ) > 220.f
                || !m_slot_q->is_ready( ) && possible_exploit && ( target->dist_to_local( ) <= 400.f || target->
                    dist_to_local( ) >= g_local->position.dist_to( *pred ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;
                return true;
            }


            return false;

            /*if (!g_config->yasuo.e_enabled->get< bool >() || *g_time - m_last_e_time <= 0.5f || !m_slot_e->is_ready() || !m_slot_q->is_ready()) return false;

            if ( !m_tornado_q && !g_features->buff_cache->get_buff( g_local->index, ct_hash( "YasuoQ1" ) ) ) return false;

            auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 475.f || g_features->buff_cache->get_buff( target->index, ct_hash( "YasuoE" ) ) ) return false;

            vec3 dash_end_position = g_local->position.extend( target->position, 475.f );
            dash_end_position.y = g_navgrid->get_height( dash_end_position );

            float dash_time = 475.f / ( 750.f + g_local->movement_speed * 0.6f );

            auto pred = g_features->prediction->predict_default( target->index, dash_time );
            if ( !pred || dash_end_position.dist_to( *pred ) > 215.f ) return false;

            if ( cast_spell( e_spell_slot::e, target->network_id ) ) {
                // std::cout << "height: " << dash_end_position.y << std::endl;
                // std::cout << "original: " << original_pos.y << std::endl;

                m_last_e_time = *g_time;
                return true;
            }

            return false;*/
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->yasuo.r_enabled->get< bool >( ) || g_features->orbwalker->get_mode( ) !=
                Orbwalker::EOrbwalkerMode::combo
                || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready( ) )
                return false;

            int target_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) >= 1400.f || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                const auto has_buff = g_features->buff_cache->has_buff_type(
                    enemy->index,
                    { EBuffType::knockup, EBuffType::knockback },
                    0.1f
                );
                if ( !has_buff ) continue;

                ++target_count;
            }

            const auto allow_r{ target_count >= g_config->yasuo.r_min_targets->get< int >( ) };

            if ( !allow_r ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || target->dist_to_local( ) >= 1400.f ||
                    target->health / target->max_health > static_cast< float >( g_config->yasuo.r_min_health_percent->
                        get< int >( ) ) / 100.f )
                    return false;

                const auto has_buff = g_features->buff_cache->has_buff_type(
                    target->index,
                    { EBuffType::knockup, EBuffType::knockback },
                    0.1f
                );
                if ( !has_buff ) return false;
            }

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( g_config->yasuo.q_lasthit_mode->get< int >( ) == 0 || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yasuo.q_lasthit_mode->get< int >( ) < 2 ) return false;

            auto       cast_time = 0.4f;
            const auto bas       = g_local->bonus_attack_speed;

            if ( bas >= 1.11f ) cast_time = 0.133f;
            else if ( bas >= 1.05f ) cast_time = 0.148f;
            else if ( bas >= 0.9f ) cast_time = 0.184f;
            else if ( bas >= 0.75f ) cast_time = 0.22f;
            else if ( bas >= 0.6f ) cast_time = 0.256f;
            else if ( bas >= 0.45f ) cast_time = 0.292f;
            else if ( bas >= 0.3f ) cast_time = 0.328f;
            else if ( bas >= 0.15f ) cast_time = 0.364f;

            const auto            lasthit_data = get_line_lasthit_target_advanced(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                m_tornado_q ? 90.f : 40.f,
                cast_time,
                0,
                false
            );

            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time );
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( g_config->yasuo.q_laneclear_mode->get< int >( ) == 0 || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yasuo.q_laneclear_mode->get< int >( ) < 2 ) return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                false
            );

            if ( !laneclear_data ) return false;


            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->on_cast( );
                g_features->prediction->add_special_attack(
                    laneclear_data->target_index,
                    laneclear_data->damage,
                    laneclear_data->travel_time,
                    true,
                    ESpellSlot::q,
                    false
                );
                return true;
            }

            return false;
        }

        auto fastclear_q( ) -> bool{
            if ( g_config->yasuo.q_fastclear_mode->get< int >( ) == 0 || !GetAsyncKeyState( VK_CONTROL ) ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yasuo.q_fastclear_mode->get< int >( ) < 2 ) return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                false
            );

            if ( !laneclear_data ) return false;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->on_cast( );
                g_features->prediction->add_special_attack(
                    laneclear_data->target_index,
                    laneclear_data->damage,
                    laneclear_data->travel_time,
                    true,
                    ESpellSlot::q,
                    false
                );
                return true;
            }

            return false;
        }

        auto jungleclear_q( ) -> bool{
            if ( g_config->yasuo.q_jungleclear_mode->get< int >( ) == 0 || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yasuo.q_jungleclear_mode->get< int >( ) < 2 ) return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                true
            );

            if ( !laneclear_data ) return false;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                g_features->orbwalker->on_cast( );
                g_features->prediction->add_special_attack(
                    laneclear_data->target_index,
                    laneclear_data->damage,
                    laneclear_data->travel_time,
                    true,
                    ESpellSlot::q,
                    false
                );
                return true;
            }

            return false;
        }

        auto lasthit_e( ) -> bool{
            if ( !g_config->yasuo.e_lasthit->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto          lasthit_data = get_targetable_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); }
            );
            if ( !lasthit_data ) return false;

            const auto& obj = g_entity_list.get_by_index( lasthit_data->index );
            if ( !obj ) return false;

            const auto dash_end_position{ g_local->position.extend( obj->position, m_e_range ) };

            // underturret e check
            if ( !g_config->yasuo.e_underturret->get< bool >( ) && is_position_in_turret_range( dash_end_position )
                && !is_position_in_turret_range( g_local->position ) )
                return false;

            if ( cast_spell( ESpellSlot::e, lasthit_data->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time + 0.5f );

                return true;
            }

            return false;
        }

        auto combo_e( ) -> void{
            if ( !g_config->yasuo.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) {
                flee_e( );
                return;
            }

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

            const auto    cursor_dash_end = g_local->position.extend( cursor, 475.f );
            auto          lowest_distance{ std::numeric_limits< float >::max( ) };
            const Object* target_minion{ };

            const auto dash_time = 475.f / ( 750.f + g_local->movement_speed * 0.6f );
            const auto pred      = g_features->prediction->predict_default( target->index, dash_time );
            if ( !pred ) return;

            const auto target_position{ *pred };
            const auto dash_threshold{ 125.f };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_e_range
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                auto       dash_end_position           = g_local->position.extend( minion->position, 475.f );
                const auto dash_end_to_target_distance = dash_end_position.dist_to( target->position );

                if ( dash_end_to_target_distance + dash_threshold > g_local->position.dist_to( target_position ) ||
                    dash_end_position.dist_to( cursor_dash_end ) > 300.f ||
                    g_features->buff_cache->get_buff( minion->index, ct_hash( "YasuoE" ) ) )
                    continue;

                // underturret e check
                if ( !g_config->yasuo.e_underturret->get< bool >( ) && is_position_in_turret_range( dash_end_position )
                    && !is_position_in_turret_range( g_local->position ) )
                    continue;

                if ( dash_end_to_target_distance < lowest_distance ) {
                    target_minion   = minion;
                    lowest_distance = dash_end_to_target_distance;
                }
            }

            if ( !target_minion ) return;

            if ( cast_spell( ESpellSlot::e, target_minion->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
            }

            return;
        }

        auto exploit_e( ) -> bool{
            if ( !g_config->yasuo.e_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( exploit_process( target ) ) return true;
            }

            return false;
        }

        auto exploit_process( Object* target ) -> bool{
            if ( !target || !g_config->yasuo.e_enabled->get< bool >( ) || !m_exploit_available
                || *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) )
                return false;

            const bool dash_duration = 475.f / ( 750.f + g_local->movement_speed * 0.6f );
            const auto allow_dash = m_slot_q->is_ready( ) || m_slot_q->cooldown_expire - *g_time < dash_duration / 2.f;
            if ( !allow_dash ) return false;

            const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            unsigned   target_nid{ };
            bool       found_target{ };

            if ( target->dist_to_local( ) > 475.f || g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "YasuoE" )
            ) ) {
                target_nid = 0;
                auto lowest_dash_angle{ 180.f };

                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) || minion->is_invisible( ) ||
                        minion->dist_to_local( ) >= m_e_range ||
                        !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                        continue;

                    auto       v1            = minion->position - g_local->position;
                    auto       v2            = cursor - g_local->position;
                    const auto dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;
                    if ( current_angle > 45.f || current_angle > lowest_dash_angle || g_features->buff_cache->get_buff(
                        minion->index,
                        ct_hash( "YasuoE" )
                    ) )
                        continue;

                    const auto dash_end_position = g_local->position.extend( minion->position, 475.f );

                    // underturret e check
                    if ( !g_config->yasuo.e_underturret->get< bool >( ) &&
                        helper::is_position_under_turret( dash_end_position ) &&
                        !helper::is_position_under_turret( g_local->position ) )
                        continue;

                    target_nid        = minion->network_id;
                    lowest_dash_angle = current_angle;
                    found_target      = true;
                }
            } else {
                found_target = target;
                target_nid   = target->network_id;
            }

            if ( !found_target ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                return true;
            }

            return false;
        }

        auto flee_e( ) -> void{
            if ( !g_config->yasuo.e_flee->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( ) )
                return;

            const Object* target{ };
            auto          lowest_distance_to_cursor{ std::numeric_limits< float >::max( ) };
            const auto    cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const auto    direct_dash_end{ g_local->position.extend( cursor, 475.f ) };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || minion->dist_to_local( ) > m_e_range
                    || !minion->is_lane_minion( ) && !minion->is_jungle_monster( ) )
                    continue;

                auto dash_end_position{ g_local->position.extend( minion->position, 475.f ) };
                if ( direct_dash_end.dist_to( dash_end_position ) > 200.f || g_features->buff_cache->get_buff(
                    minion->index,
                    ct_hash( "YasuoE" )
                ) )
                    continue;

                // underturret e check
                if ( !g_config->yasuo.e_underturret->get< bool >( ) && is_position_in_turret_range( dash_end_position )
                    && !is_position_in_turret_range( g_local->position ) )
                    continue;

                if ( dash_end_position.dist_to( cursor ) < lowest_distance_to_cursor ) {
                    target                    = minion;
                    lowest_distance_to_cursor = dash_end_position.dist_to( cursor );
                }
            }

            if ( !target ) return;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) m_last_e_time = *g_time;
        }

        auto stack_minion_q( ) -> void{
            if ( *g_time - m_last_q_time <= 0.3f || !m_slot_q->is_ready( ) ) return;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr || aimgr->is_dashing ) return;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                m_tornado_q ? 90.f : 40.f,
                true,
                false,
                true
            );

            if ( !laneclear_data ) return;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time = *g_time;

                g_features->orbwalker->set_cast_time( 0.1f );
                //g_features->prediction->add_special_attack(laneclear_data->target_index, laneclear_data->damage, laneclear_data->travel_time);
            }

            return;
        }

        auto dash_tracking( ) -> void{
            if ( !m_dash_active ) {
                auto aimgr = g_local->get_ai_manager( );
                if ( !aimgr ) return;

                aimgr.update( );
                if ( !aimgr->is_moving || !aimgr->is_dashing ) return;

                m_dash_active    = true;
                m_last_dash_time = *g_time;

                m_dash_start = aimgr->path_start;
                m_dash_end   = aimgr->path_end;
                return;
            }

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            aimgr.update( );

            m_dash_active = aimgr->is_dashing && aimgr->is_moving;

            if ( m_dash_active ) g_features->orbwalker->disable_autoattack_until( *g_time + 0.25f );
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->attack_damage( ) * 1.05f,
                    target->index,
                    true
                ) + helper::get_onhit_damage( target->index );
            case ESpellSlot::e:
            {
                auto base_damage = m_e_damage[ get_slot_e( )->level ];

                const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "YasuoDashScalar" ) );
                if ( buff && buff->buff_data->end_time - *g_time > 0.1f ) {
                    base_damage *= buff->stacks( ) == 1 ? 1.25f : 1.5f;
                }

                return helper::calculate_damage(
                    base_damage + g_local->bonus_attack_damage( ) * 0.2f + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            }
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return g_local->position.dist_to( target->position ) / ( 750.f + g_local->movement_speed * 0.6f );
            case ESpellSlot::q:
            {
                auto       cast_time = 0.4f;
                const auto bas       = g_local->bonus_attack_speed;

                if ( bas >= 1.11f ) cast_time = 0.133f;
                else if ( bas >= 1.05f ) cast_time = 0.148f;
                else if ( bas >= 0.9f ) cast_time = 0.184f;
                else if ( bas >= 0.75f ) cast_time = 0.22f;
                else if ( bas >= 0.6f ) cast_time = 0.256f;
                else if ( bas >= 0.45f ) cast_time = 0.292f;
                else if ( bas >= 0.3f ) cast_time = 0.328f;
                else if ( bas >= 0.15f ) cast_time = 0.364f;

                if ( m_tornado_q ) {
                    const auto tt   = cast_time + g_local->position.dist_to( target->position ) / 1200.f;
                    const auto pred = g_features->prediction->predict_default( target->index, tt );
                    if ( !pred ) return 0.f;

                    return cast_time + g_local->position.dist_to( pred.value( ) ) / 1200.f;
                }

                return cast_time;
            }
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

        float m_last_exploit_time{ };

        // dash tracking
        bool  m_dash_active{ };
        float m_last_dash_time{ };

        Vec3 m_dash_start{ };
        Vec3 m_dash_end{ };

        Vec3 m_after_dash_position{ };

        // exploit
        bool m_exploit_available{ };

        std::vector< float > m_q_damage{ 0.f, 20.f, 45.f, 70.f, 95.f, 120.f };
        std::vector< float > m_e_damage{ 0.f, 60.f, 70.f, 80.f, 90.f, 100.f };
        std::vector< float > m_r_damage{ 0.f, 180.f, 265.f, 350.f };

        float m_q_range{ 480.f };
        float m_q3_range{ 1150.f };
        bool  m_tornado_q{ };

        // E stuff
        bool m_underturret_e{ };

        bool m_pressed{ };
        bool m_printed{ };

        float m_w_range{ 0.f };
        float m_e_range{ 475.f };
        float m_r_range{ 1400.f };
    };
}
