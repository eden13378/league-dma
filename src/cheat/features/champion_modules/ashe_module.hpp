#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class ashe_module final : public IModule {
    public:
        virtual ~ashe_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "ashe_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Ashe" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "ashe" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->ashe.q_enabled );
            q_settings->checkbox( _( "use in laneclear" ), g_config->ashe.q_laneclear );

            w_settings->checkbox( _( "enable" ), g_config->ashe.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->ashe.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable(?)" ), g_config->ashe.r_enabled )->set_tooltip(
                _( "Semi-manual, must be in full combo" )
            );
            r_settings->select(
                _( "hitchance" ),
                g_config->ashe.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw w range" ), g_config->ashe.w_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ashe.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->ashe.w_draw_range->get< bool >( ) || g_local->is_dead( ) ) return;

            g_local.update( );

            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->ashe.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ashe.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                if ( g_config->ashe.q_enabled->get< bool >( ) ) spell_q( );
                spell_w( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->ashe.q_laneclear->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) || !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "asheqcastready" ) );
            if ( !buff || buff->stacks( ) != 4 ) return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->ashe.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                2000.f,
                60.f,
                0.25f
            );

            if ( !predicted.valid ||
                predicted.position.length( ) == 0.f ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->ashe.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->ashe.r_enabled->get< bool >( ) || !GetAsyncKeyState( VK_CONTROL )
                || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict( target->index, m_r_range, 1500.f, 130.f, 0.25f );
            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->ashe.r_hitchance->get<
                    int >( ) )
            )
                return false;

            if ( cast_spell( ESpellSlot::r, predicted.position ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::array< float, 4 > m_r_damage = { 0.f, 200.f, 400.f, 600.f };

        float m_q_range{ 0.f };
        float m_w_range{ 1100.f };
        float m_r_range{ 25000.f };
    };
}
