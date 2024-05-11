#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class tryndamere_module final : public IModule {
    public:
        virtual ~tryndamere_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "tryndamere_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Tryndamere" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "tryndamere" ), menu_order::champion_module );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            w_settings->checkbox( _( "enable" ), g_config->trynda.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->trynda.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->trynda.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->trynda.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw w range" ), g_config->trynda.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->trynda.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->trynda.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->trynda.w_draw_range->get< bool >( ) &&
                !g_config->trynda.e_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->trynda.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->trynda.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->trynda.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->trynda.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
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
                spell_w( );
                spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto spell_w( ) -> bool override{
            if ( !g_config->trynda.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 800.f ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.3f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->trynda.w_hitchance->
                get< int >( ) ) )
                return false;

            const auto dir_pred = g_features->prediction->predict_default( target->index, 0.1f );
            if ( !dir_pred || target->dist_to_local( ) >= g_local->position.dist_to( dir_pred.value( ) ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.3f );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->trynda.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 900.f, 0.f, 0.1f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->trynda.e_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( g_local->position.dist_to( pred.position ) / 900.f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( ESpellSlot slot, Object* target ) -> float override{
            // switch ( slot ) {
            // default:
            //     return 0.f;
            // }
            return 0.f;
        }

        auto get_spell_travel_time( ESpellSlot slot, Object* target ) -> float override{
            // switch ( slot ) {
            // default:
            //     return 0.f;
            // }
            //
            // return std::numeric_limits< float >::max( );
            return 0.f;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };


        float m_q_range{ 0.f };
        float m_w_range{ 850.f };
        float m_e_range{ 660.f };
    };
}
