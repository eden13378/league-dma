#pragma once
#include "module.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"
#include "../../menu/menu.hpp"

namespace features::champion_modules {
    class tahm_module final : public IModule {
    public:
        virtual ~tahm_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "tahm_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "TahmKench" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "Tahm" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->tahm_kench.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->tahm_kench.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->checkbox( _( "draw range" ), g_config->tahm_kench.q_draw_range );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->tahm_kench.q_draw_range->get< bool >( ) || g_local->is_dead( ) ) return;

            g_local.update( );

            const auto q = g_local->spell_book.get_spell_slot( ESpellSlot::q );

            if ( q && q->level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color::white( ).alpha( 100 ),
                    m_q_range,
                    2,
                    80,
                    2.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                m_target = CHolder::from_object( g_features->target_selector->get_default_target( ) );
                if ( !m_target ) return;

                IModule::run( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->tahm_kench.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ) return false;

            const auto predicted = g_features->prediction->predict(
                m_target->get( )->index,
                m_q_range,
                2800.f,
                70.f,
                0.25f
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->tahm_kench.q_hitchance->get<
                    int >( ) )
            )
                return false;


            if ( g_features->prediction->minion_in_line( g_local->position, predicted.position, 70.f ) ) return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{ return false; }

        auto spell_r( ) -> bool override{ return false; }

    protected:
    private:
        float m_last_q_time{ };
        float m_q_range{ 900.f };
    };
}
