#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class KayleModule final : public IModule {
    public:
        virtual ~KayleModule( ) = default;


        auto get_name( ) -> hash_t override{ return ct_hash( "kayle_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Kayle" ); }

        auto initialize( ) -> void override{ m_priority_list = { w_spell, e_spell, q_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kayle" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->kayle.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->kayle.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->kayle.w_enabled );
            w_settings->select( _( "logic" ), g_config->kayle.w_logic, { _( "Wasteless" ), _( "Below %" ) } );
            w_settings->slider_int( _( "minimum mana %" ), g_config->kayle.w_min_mana, 0, 75, 1 );
            w_settings->slider_int( _( "minimum hp %" ), g_config->kayle.w_min_hp_percent, 5, 75, 1 );
            w_settings->checkbox( _( "only in combo" ), g_config->kayle.w_only_combo );

            e_settings->checkbox( _( "enable" ), g_config->kayle.e_enabled );
            e_settings->checkbox( _( "use in laneclear" ), g_config->kayle.e_lane_clear );

            r_settings->checkbox( _( "enable" ), g_config->kayle.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->kayle.r_only_full_combo );
            r_settings->slider_int( _( "hp% threshold to cast" ), g_config->kayle.r_health_threshold, 0, 25 );

            drawings->checkbox( _( "draw q range" ), g_config->kayle.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->kayle.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kayle.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->kayle.q_draw_range->get< bool >( ) &&
                !g_config->kayle.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->kayle.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kayle.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        40,
                        2.f
                    );
                }
            }


            if ( g_config->kayle.e_draw_range->get< bool >( ) && g_local->level < 6 ) {
                // check below 6 cuz otherwise it's just our AA range
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kayle.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_e_range,
                        Renderer::outline,
                        40,
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

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                IModule::run( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->kayle.e_lane_clear->get< bool >( ) ) spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->kayle.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                { },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto cast_time = g_features->orbwalker->get_attack_cast_delay( );

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                // range
                m_q_speed,
                // speed
                m_q_radius,
                // width
                cast_time + 0.264f // cast time + delay before sword shoots
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->kayle.q_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, m_q_radius )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( cast_time );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->kayle.w_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ) return false;

            if ( g_config->kayle.w_only_combo->get< bool >( ) &&
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo
            )
                return false;

            const auto mana = g_local->mana * 100 / g_local->max_mana;

            if ( mana < g_config->kayle.w_min_mana->get< int >( ) ) return false;
            switch ( g_config->kayle.w_logic->get< int >( ) ) {
            default:
                break;
            case 0:
            {
                // wasteless
                const auto missing_hp = g_local->max_health - g_local->health;

                if ( missing_hp < get_w_heal( ) ) return false;
                break;
            }

            case 1:
            {
                // below%
                const auto hp_percent = g_local->health / g_local->max_health * 100.f;

                if ( hp_percent > g_config->kayle.w_min_hp_percent->get< int >( ) ) return false;

                break;
            }
            }

            if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( m_w_cast_time );
                return true;
            }
            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->kayle.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable(
                    target->index,
                    m_e_range
                ) )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->kayle.r_enabled->get< bool >( ) || g_config->kayle.r_only_full_combo->get< bool >( ) && !
                GetAsyncKeyState( VK_CONTROL )
                || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto predicted_health = g_features->prediction->predict_health( g_local.get( ), 0.3f );
            const auto hp_threshold     = g_local->max_health * ( g_config->kayle.r_health_threshold->get< int >( ) *
                0.01f );
            if ( predicted_health > hp_threshold ) return false;

            if ( cast_spell( ESpellSlot::r, g_local->network_id ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                if ( get_slot_q( )->level ) return 0.f;

                return helper::calculate_damage(
                    ( 20.f + ( 40.f * static_cast< float >( get_slot_q( )->level ) ) ) + ( g_local->bonus_attack * .6f )
                    + ( g_local->ability_power( ) * .5f ),
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        static auto get_w_heal( ) -> float{
            auto w_spell = g_local->spell_book.get_spell_slot( ESpellSlot::w );

            if ( !w_spell ) return 0;

            const auto level = w_spell->level;

            if ( level == 0 ) return 0;

            return 30 + level + g_local->ability_power( ) * .3f;
        }

        float m_last_q_time{ };
        float m_last_cast_time{ };
        float m_q_range{ 900.f };
        float m_q_radius{ 75.f };
        float m_q_speed{ 1600.f };

        float m_last_w_time{ };
        float m_w_cast_time{ 0.25f };

        float m_last_e_time{ };
        float m_e_range{ 550.f };

        float m_last_r_time{ };
    };
}
