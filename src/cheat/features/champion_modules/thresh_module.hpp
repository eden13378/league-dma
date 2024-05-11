#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class thresh_module final : public IModule {
    public:
        virtual ~thresh_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "thresh_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Thresh" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "thresh" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->thresh.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->thresh.q_hitchance,
                { ( "Fast" ), ( "Medium" ), ( "High" ), ( "Very high" ), ( "Immobile" ) }
            );
            q_settings->checkbox( _( "enable autointerrupt" ), g_config->thresh.q_autointerrupt );
            q_settings->checkbox( _( "enable antigapclose" ), g_config->thresh.q_antigapclose );

            w_settings->checkbox( _( "enable" ), g_config->thresh.w_enabled );
            w_settings->checkbox(
                _( "cast to ally on incoming damage below percent HP" ),
                g_config->thresh.w_block_inc_damage
            );
            w_settings->slider_int(
                _( "percent hp to cast w to block damage" ),
                g_config->thresh.w_percent_hp_to_block,
                2,
                5,
                1
            );

            e_settings->checkbox( _( "enable" ), g_config->thresh.e_enabled );
            e_settings->checkbox( _( "enable autointerrupt" ), g_config->thresh.e_autointerrupt );
            e_settings->checkbox( _( "enable antigapclose" ), g_config->thresh.e_antigapclose );

            r_settings->checkbox( _( "enable" ), g_config->thresh.r_enabled );
            r_settings->checkbox( _( "full combo only" ), g_config->thresh.r_in_full_combo );

            drawings->checkbox( _( "draw q range" ), g_config->thresh.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->thresh.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->thresh.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->amumu.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->thresh.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->thresh.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            m_q_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "ThreshQPullMissile" ) );

            //antigapclose_q( );
            //autointerrupt_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q( );
            //spell_e( );
            //spell_w( );
            //spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->thresh.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.3f || !m_slot_q->is_ready( true ) || m_q_active )
                return false;

            const auto target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1900.f,
                70.f,
                0.5f,
                { },
                true
            );

            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->thresh.q_hitchance->get< int >( ) ) )
                return false;

            if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->amumu.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) )
                return false;

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->amumu.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f || !m_slot_r->is_ready( true ) )
                return false;

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->amumu.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.2f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 2000.f, 80.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                80.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minions_in_line(
                    g_local->position,
                    pred.position,
                    80.f,
                    0,
                    0.25f,
                    0.25f + g_local->position.dist_to( pred.position ) / 2000.f
                ) > 1 )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->amumu.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 2000.f, 80.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.85f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        bool  m_q_active{ };

        float m_last_w_time{ };


        float m_last_e_time{ };


        float m_last_r_time{ };

        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 100.f, 150.f, 200.f, 250.f, 300.f };

        float m_q_range{ 1040.f };
        float m_w_range{ 950.f };
        float m_e_range{ 537.5f };
        float m_r_range{ 470.f };
    };
} // namespace features::champion_modules
