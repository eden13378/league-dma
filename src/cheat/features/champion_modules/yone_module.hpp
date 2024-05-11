#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class yone_module final : public IModule {
    public:
        virtual ~yone_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "yone_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Yone" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "yone" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            //const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->yone.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->yone.q_harass );
            q_settings->select(
                _( "tornado hitchance" ),
                g_config->yone.q_tornado_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->checkbox( _( "tornado turret check" ), g_config->yone.q_tornado_turret_check );

            spellclear->select(
                _( "lasthit q" ),
                g_config->yone.q_lasthit_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "laneclear q" ),
                g_config->yone.q_laneclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "fastclear q" ),
                g_config->yone.q_fastclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );
            spellclear->select(
                _( "jungleclear q" ),
                g_config->yone.q_jungleclear_mode,
                { _( "Off" ), _( "Q1" ), _( "Q1 + Q3" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->yone.w_enabled );
            q_settings->checkbox( _( "harass w" ), g_config->yone.w_harass );
            w_settings->checkbox( _( "only in aa range" ), g_config->yone.w_only_in_aa_range );
            w_settings->select( _( "logic" ), g_config->yone.w_mode, { _( "Always" ), _( "After AA" ) } );
            w_settings->select(
                _( "hitchance" ),
                g_config->yone.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //e_settings->checkbox( _( "enabled (?)" ), g_config->yone.e_enabled )->set_tooltip( _( "full combo will ignore min snap back. E is manual cast" ) );
            //e_settings->slider_int( _( "min enemy snapback" ), g_config->yone.e_min_snap_kill, 1, 5, 1 );

            r_settings->checkbox( _( "enable" ), g_config->yone.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->yone.r_full_combo_check );
            r_settings->select(
                _( "hitchance" ),
                g_config->yone.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            //r_settings->checkbox( _( "multihit" ), g_config->yone.r_enabled );
            //r_settings->slider_int( _( "min multihit count" ), g_config->yone.r_multihit_count, 2, 5 );

            drawings->checkbox( _( "draw q range" ), g_config->yone.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->yone.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->yone.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->yone.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->yone.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yone.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_tornado_q ? m_q3_range : m_q_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->yone.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yone.dont_draw_on_cooldown->get<
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

            if ( g_config->yone.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->yone.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            m_tornado_q = rt_hash( m_slot_q->get_name( ).data( ) ) == ct_hash( "YoneQ3" );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_q( );
                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->yone.q_harass->get< bool >( ) && spell_q( ) ||
                    g_config->yone.w_harass->get< bool >( ) && spell_w( ) )
                    break;

                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->yone.q_harass->get< bool >( ) && spell_q( ) ||
                    g_config->yone.w_harass->get< bool >( ) && spell_w( ) )
                    break;

                lasthit_q( );
                laneclear_q( );
                fastclear_q( );
                jungleclear_q( );
                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_config->yone.q_harass->get< bool >( ) && spell_q( ) ||
                    g_config->yone.w_harass->get< bool >( ) && spell_w( ) )
                    break;

                lasthit_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->yone.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

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

            const auto hitchance = m_tornado_q ? g_config->yone.q_tornado_hitchance->get< int >( ) : 0;

            auto pred = g_features->prediction->predict(
                target->index,
                m_tornado_q ? m_q3_range : m_q_range,
                m_tornado_q ? 1500.f : 0.f,
                m_tornado_q ? 80.f : 40.f,
                cast_time,
                { },
                true
            );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < hitchance ) return false;

            if ( m_tornado_q && g_config->yone.q_tornado_turret_check->get< bool >( ) &&
                helper::is_position_under_turret( g_local->position.extend( pred.position, 450.f ) ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->yone.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_w_time <= 0.4f || g_config->yone.w_mode->get< int >( ) == 1 && !g_features->orbwalker->
                should_reset_aa( ) || !m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->yone.w_only_in_aa_range->get< bool >( ) && !g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            auto       cast_time = 0.5f;
            const auto bas       = g_local->bonus_attack_speed;

            if ( bas >= 1.05f ) cast_time = 0.19f;
            else if ( bas >= 0.95f ) cast_time = 0.22f;
            else if ( bas >= 0.84f ) cast_time = 0.25f;
            else if ( bas >= 0.735f ) cast_time = 0.28f;
            else if ( bas >= 0.63f ) cast_time = 0.31f;
            else if ( bas >= 0.525f ) cast_time = 0.34f;
            else if ( bas >= 0.42f ) cast_time = 0.38f;
            else if ( bas >= 0.315f ) cast_time = 0.41f;
            else if ( bas >= 0.21f ) cast_time = 0.44f;
            else if ( bas >= 0.105f ) cast_time = 0.47f;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 250.f, cast_time );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->yone.w_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.default_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        //WIP: not sure what death mark is stored as yet
        auto spell_e( ) -> bool override{
            /*auto enemies  = g_entity_list->get_enemies( );
            bool has_buff = false;
            float stacks = {};

            for ( auto enemy : enemies ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;
                has_buff = !!g_features->buff_cache->get_buff( enemy->index, ct_hash( "yoneedeathmark" ) );
                if ( has_buff ) {
                    //stacks = g_features->buff_cache->get_buff( enemy->index, ct_hash( "yoneedeathmark" ) )->name_hash;
                    debug_log( "stacks {}", g_features->buff_cache->get_buff( enemy->index, ct_hash( "yoneedeathmark" ) )->alt_amount );
                    return true;
                }
            }*/
            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->yone.r_enabled->get< bool >( ) ||
                g_config->yone.r_full_combo_check->get< bool >( ) && !g_input->is_key_pressed( utils::EKey::control )
                ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 110.f, 0.75f, { }, true );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->yone.r_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( g_config->yone.q_lasthit_mode->get< int >( ) == 0 || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yone.q_lasthit_mode->get< int >( ) < 2 ) return false;

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
                m_tornado_q ? 80.f : 40.f,
                cast_time,
                0,
                false
            );

            if ( !lasthit_data ) return false;

            if ( m_tornado_q && g_config->yone.q_tornado_turret_check->get< bool >( ) &&
                helper::is_position_under_turret( g_local->position.extend( lasthit_data->cast_position, 450.f ) ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

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
            if ( g_config->yone.q_laneclear_mode->get< int >( ) == 0 ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yone.q_laneclear_mode->get< int >( ) < 2 ) return false;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                false
            );

            if ( !laneclear_data ) return false;

            if ( m_tornado_q && g_config->yone.q_tornado_turret_check->get< bool >( ) &&
                helper::is_position_under_turret( g_local->position.extend( laneclear_data->cast_position, 450.f ) ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

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
            if ( g_config->yone.q_fastclear_mode->get< int >( ) == 0 || !GetAsyncKeyState( VK_CONTROL ) ||
                *g_time - m_last_cast_time <= 0.1f || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yone.q_fastclear_mode->get< int >( ) < 2 ) return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                false
            );

            if ( !laneclear_data ) return false;

            if ( m_tornado_q && g_config->yone.q_tornado_turret_check->get< bool >( ) &&
                helper::is_position_under_turret( g_local->position.extend( laneclear_data->cast_position, 450.f ) ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

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
            if ( g_config->yone.q_jungleclear_mode->get< int >( ) == 0 || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            if ( m_tornado_q && g_config->yone.q_jungleclear_mode->get< int >( ) < 2 ) return false;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_tornado_q ? m_q3_range : m_q_range,
                0.f,
                true
            );

            if ( !laneclear_data ) return false;

            if ( m_tornado_q && g_config->yone.q_tornado_turret_check->get< bool >( ) &&
                helper::is_position_under_turret( g_local->position.extend( laneclear_data->cast_position, 450.f ) ) &&
                !helper::is_position_under_turret( g_local->position ) )
                return false;

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

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 1.05f,
                    target->index,
                    true
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.5f,
                    target->index,
                    false
                );
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

                if ( !m_tornado_q ) return cast_time;

                const auto tt   = cast_time + g_local->position.dist_to( target->position ) / 1500.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return cast_time + g_local->position.dist_to( target->position ) / 1500.f;

                return cast_time + g_local->position.dist_to( pred.value( ) ) / 1500.f;
            }
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1150.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1150.f;
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

        std::vector< float > m_q_damage{ 0.f, 20.f, 40.f, 60.f, 80.f, 100.f };
        std::vector< float > m_e_damage{ 0.f, 60.f, 105.f, 150.f, 195.f, 240.f };
        std::vector< float > m_r_damage{ 0.f, 180.f, 265.f, 350.f };

        float m_q_range{ 500.f };
        float m_q3_range{ 1050.f };

        float m_w_range{ 600.f };
        float m_e_range{ 0.f };
        float m_r_range{ 1000.f };

        bool m_tornado_q{ };
    };
}
