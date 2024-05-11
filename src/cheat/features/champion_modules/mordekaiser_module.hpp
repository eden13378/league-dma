#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class mordekaiser_module final : public IModule {
    public:
        virtual ~mordekaiser_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "mordekaiser_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Mordekaiser" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "mordekaiser" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->mordekaiser.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->mordekaiser.q_harass );
            q_settings->checkbox(
                _( "ignore hitchance in aa range" ),
                g_config->mordekaiser.q_aa_reset_ignore_hitchance
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->mordekaiser.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->mordekaiser.w_enabled );
            w_settings->slider_int(
                _( "HP% must be lower than" ),
                g_config->mordekaiser.w_health_threshold,
                10,
                95,
                1
            );
            w_settings->slider_int( _( "min stored damage%" ), g_config->mordekaiser.w_min_mana, 0, 95, 1 );

            e_settings->checkbox( _( "enable" ), g_config->mordekaiser.e_enabled );
            e_settings->checkbox( _( "auto-interrupt (?)" ), g_config->mordekaiser.e_autointerrupt )
                      ->set_tooltip( _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" ) );
            e_settings->select(
                _( "hitchance" ),
                g_config->mordekaiser.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            e_settings->slider_int( _( "minimum range %" ), g_config->mordekaiser.e_min_range, 5, 70, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->mordekaiser.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->mordekaiser.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->mordekaiser.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->mordekaiser.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->mordekaiser.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( ) || !g_config->mordekaiser.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->mordekaiser.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( ) || !g_config->mordekaiser.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 213, 0, 255 ),
                        m_e_range + 200.f,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->mordekaiser.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->mordekaiser.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 200, 50, 70, 255 ),
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

            if ( g_features->orbwalker->in_action( ) ) return;

            autointerrupt_e( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->mordekaiser.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->mordekaiser.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto hitchance = g_config->mordekaiser.q_aa_reset_ignore_hitchance->get< bool >( ) &&
                                   g_features->orbwalker->is_attackable( target->index ) && g_features->orbwalker->
                                   should_reset_aa( )
                                       ? 0
                                       : g_config->mordekaiser.q_hitchance->get< int >( );

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 80.f, 0.5f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( hitchance ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->mordekaiser.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_w_time <= 0.4f || m_slot_w->get_usable_state( ) == 1 ||
                g_config->mordekaiser.w_health_threshold->get< int >( ) / 100.f * g_local->max_health < g_local->health
                ||
                g_config->mordekaiser.w_min_mana->get< int >( ) / 100.f * g_local->max_mana > g_local->mana ||
                !m_slot_w->is_ready( ) )
                return false;

            const auto incoming_damage = helper::get_incoming_damage( g_local->index, 0.5f );
            if ( incoming_damage <= 50.f && g_local->health > 100.f ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->mordekaiser.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || m_e_range * ( g_config->mordekaiser.e_min_range->get< int >( ) / 100.f ) > target->
                dist_to_local( ) )
                return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 100.f, 0.75f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->mordekaiser.
                e_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->mordekaiser.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 100.f, 0.5f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1200.f;
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

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 150.f, 225.f, 300.f };

        float m_q_range{ 625.f };
        float m_w_range{ 900.f };
        float m_e_range{ 700.f };
        float m_r_range{ 650.f };
    };
}
