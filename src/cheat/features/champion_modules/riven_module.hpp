#pragma once
#include "module.hpp"
#include "../../sdk/game/ai_manager.hpp"

namespace features::champion_modules {
    class riven_module final : public IModule {
    public:
        virtual ~riven_module( ) = default;


        auto get_name( ) -> hash_t override{ return ct_hash( "riven_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Riven" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "riven" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->riven.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->riven.q_harass );

            w_settings->checkbox( _( "enable" ), g_config->riven.w_enabled );
            w_settings->checkbox( _( "E-W combo" ), g_config->riven.e_combo_w );
            w_settings->select(
                _( "hitchance" ),
                g_config->riven.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
        }

        auto on_draw( ) -> void override{
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( m_combo_active ) {
                    ew_combo( );
                    return;
                }

                spell_e( );
                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->riven.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                return;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->riven.q_enabled->get< bool >( ) || !m_slot_q->is_ready( ) || *g_time - m_last_q_time <
                0.25f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 175.f ) )
                return false;

            if ( m_slot_q->charges != 0 && !g_features->orbwalker->should_reset_aa( ) && g_features->orbwalker->
                is_attackable( target->index, g_local->attack_range + 50.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.2f );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->riven.w_enabled->get< bool >( ) || !m_slot_w->is_ready( ) || *g_time - m_last_w_time < 0.4f
                || *g_time - m_last_q_time < 0.4f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( m_slot_q->charges != 0 && g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->riven.w_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->riven.e_combo_w->get< bool >( ) || m_combo_active || !m_slot_w->is_ready( ) || !m_slot_e->
                is_ready( )
                || *g_time - m_last_w_time < 0.4f || *g_time - m_last_e_time < 0.4f )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, 450.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || pred.position.dist_to( g_local->position ) < 250.f ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->riven.w_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time  = *g_time;
                m_combo_active = true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto ew_combo( ) -> void{
            if ( !m_combo_active ) return;

            if ( !m_slot_w->is_ready( ) || *g_time - m_last_e_time >= 0.4f ) m_combo_active = false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr || !aimgr->is_dashing ) return;

            const auto path     = aimgr->get_path( );
            const auto path_end = path[ path.size( ) - 1 ];
            if ( target->position.dist_to( path_end ) > 250.f ) return;

            const auto pred = g_features->prediction->predict_default(
                g_local->index,
                g_features->orbwalker->get_ping( )
            );
            if ( !pred || pred.value( ).dist_to( target->position ) > 250.f ) return;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time  = *g_time;
                m_combo_active = false;
            }
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };

        float m_q_range{ 225.f };
        float m_w_range{ 250.f };
        float m_e_range{ 250.f };

        // ew combo
        bool m_combo_active{ };
    };
}
