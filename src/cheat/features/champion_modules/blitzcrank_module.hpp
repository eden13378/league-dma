#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class blitzcrank_module final : public IModule {
    public:
        virtual ~blitzcrank_module( ) = default;


        auto get_name( ) -> hash_t override{ return ct_hash( "blitzcrank_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Blitzcrank" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "blitzcrank" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );


            q_settings->checkbox( _( "enable" ), g_config->blitzcrank.q_enabled );
            q_settings->checkbox( _( "auto interrupt (?)" ), g_config->blitzcrank.q_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->blitzcrank.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max range %" ), g_config->blitzcrank.q_max_range, 10, 100, 1 );
            q_settings->checkbox( _( "^ ignore in full combo" ), g_config->blitzcrank.q_ignore_range_full_combo );

            e_settings->checkbox( _( "enable" ), g_config->blitzcrank.e_enabled );
            e_settings->checkbox( _( "wait for aa reset (?)" ), g_config->blitzcrank.e_reset_aa )->set_tooltip(
                _( "This will be ignored if Q is not ready" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->blitzcrank.draw_q );
            drawings->checkbox( _( "draw r range" ), g_config->blitzcrank.draw_r );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->blitzcrank.draw_q->get< bool >( ) &&
                !g_config->blitzcrank.draw_r->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            auto q_slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( g_config->blitzcrank.draw_q->get< bool >( ) && q_slot && q_slot->level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            auto r_slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
            if ( g_config->blitzcrank.draw_r->get< bool >( ) && r_slot && r_slot->level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color::red( ),
                    m_r_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            autointerrupt_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_e( );
                spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->blitzcrank.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f
                || *g_time - m_last_cast_time <= 0.05f || g_features->orbwalker->in_attack( ) ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                1800.f,
                70.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->blitzcrank.q_hitchance->get<
                    int >( ) )
                || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    predicted.position,
                    80.f,
                    0.25f,
                    1800.f
                ) )
                return false;


            if ( !g_config->blitzcrank.q_ignore_range_full_combo->get< bool >( ) || !GetAsyncKeyState( VK_CONTROL ) ) {
                const auto max_range = m_q_range * ( static_cast< float >( g_config->blitzcrank.q_max_range->get<
                        int >( ) )
                    / 100.f );
                if ( target->dist_to_local( ) > max_range && static_cast< int >( predicted.hitchance ) < 3 ) {
                    return
                        false;
                }
            }

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->blitzcrank.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || g_features->orbwalker->in_attack( ) && g_config->blitzcrank.
                e_reset_aa->get< bool >( ) ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( g_config->blitzcrank.e_reset_aa->get< bool >( ) && m_slot_q->is_ready( true ) && !g_features->orbwalker
                ->should_reset_aa( ) )
                return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->blitzcrank.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1800.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance <= 3 || g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                80.f,
                0.25f,
                1800.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) m_last_q_time = *g_time;
        }

        float m_q_range{ 1075.f };
        float m_r_range{ 600.0f };
        float m_last_q_time{ };
        float m_last_e_time{ };
        float m_last_cast_time{ };
    };
}
