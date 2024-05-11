#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class mundo_module final : public IModule {
    public:
        virtual ~mundo_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "mundo_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "DrMundo" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "mundo" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->mundo.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->mundo.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->mundo.q_harass );
            q_settings->checkbox( _( "lasthit q" ), g_config->mundo.q_lasthit );
            q_settings->select(
                _( "hitchance" ),
                g_config->mundo.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->mundo.e_enabled );
            e_settings->checkbox( _( "use in laneclear" ), g_config->mundo.e_laneclear );

            drawings->checkbox( _( "draw q range" ), g_config->mundo.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->mundo.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->mundo.q_draw_range->get< bool >( ) || g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->mundo.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->mundo.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                for ( auto i = 0; i < 2; i++ ) {
                    if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                    const auto target = g_features->target_selector->get_default_target( i > 0 );
                    if ( !target ) break;

                    if ( combo_q( target ) ) break;
                }

                spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->mundo.q_harass->get< bool >( ) ) {
                    for ( auto i = 0; i < 2; i++ ) {
                        if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                        const auto target = g_features->target_selector->get_default_target( i > 0 );
                        if ( !target ) break;

                        if ( combo_q( target ) ) break;
                    }
                }

            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_config->mundo.q_harass->get< bool >( ) ) {
                    for ( auto i = 0; i < 2; i++ ) {
                        if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                        const auto target = g_features->target_selector->get_default_target( i > 0 );
                        if ( !target ) break;

                        if ( combo_q( target ) ) break;
                    }
                }

                lasthit_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:

                if ( g_config->mundo.q_harass->get< bool >( ) ) {
                    for ( auto i = 0; i < 2; i++ ) {
                        if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                        const auto target = g_features->target_selector->get_default_target( i > 0 );
                        if ( !target ) break;

                        if ( combo_q( target ) ) break;
                    }
                }

                if ( g_config->mundo.e_laneclear->get< bool >( ) ) spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::flee:

                if ( g_config->mundo.q_flee->get< bool >( ) ) {
                    for ( auto i = 0; i < 2; i++ ) {
                        if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                        const auto target = g_features->target_selector->get_default_target( i > 0 );
                        if ( !target ) break;

                        if ( combo_q( target ) ) break;
                    }
                }

                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{ return false; }

        auto combo_q( Object* target ) -> bool{
            if ( !g_config->mundo.q_enabled->get< bool >( ) || !target || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->mundo.q_hitchance->get< int >( ) ) ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    60.f,
                    0.25f,
                    2000.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->mundo.e_enabled->get< bool >( ) ||
                *g_time - m_last_e_time <= 0.5f ||
                !g_features->orbwalker->should_reset_aa( ) ||
                !m_slot_e->is_ready( ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "DrMundoE" ) ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( !g_config->mundo.q_lasthit->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto          lasthit_data = get_line_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                2000.f,
                0.25f,
                60.f,
                true
            );
            if ( !lasthit_data ) return false;

            if ( cast_spell( ESpellSlot::q, lasthit_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                g_features->orbwalker->ignore_minion( lasthit_data->index, lasthit_data->travel_time );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage( m_q_damage[ get_slot_q( )->level ], target->index, false );
            default:
                break;
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
            case ESpellSlot::w:
                return 0.25f;
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

        std::vector< float > m_q_damage = { 0.f, 80.f, 130.f, 180.f, 230.f, 280.f };


        float m_q_range{ 1050.f };
        float m_w_range{ 0.f };
        float m_e_range{ 0.f };
    };
}
