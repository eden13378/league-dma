#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class gnar_module final : public IModule {
    public:
        virtual ~gnar_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "gnar_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Gnar" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "gnar" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->gnar.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->gnar.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->gnar.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->gnar.w_enabled );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->gnar.w_autointerrupt )->set_tooltip(
                _( "Only in mega form. Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->select(
                _( "hitchance" ),
                g_config->gnar.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->gnar.e_enabled );
            e_settings->checkbox( _( "disable in aa range" ), g_config->gnar.e_disable_aa_range );
            e_settings->select(
                _( "hitchance" ),
                g_config->gnar.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->gnar.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->gnar.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->gnar.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->gnar.q_draw_range->get< bool >( ) &&
                !g_config->gnar.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->gnar.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->gnar.dont_draw_on_cooldown->get<
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

            if ( g_config->gnar.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->gnar.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            autointerrupt_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            m_big_gnar = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "gnartransform" ) ) || !!
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "gnartransformsoon" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_w( );
                spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->gnar.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->gnar.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( m_big_gnar ) {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2100.f, 80.f, 0.5f, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.q_hitchance->get< int >( )
                    || g_features->prediction->minion_in_line( g_local->position, pred.position, 90.f ) )
                    return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->on_cast( );
                    m_last_q_time = *g_time;
                    return true;
                }
            } else {
                auto pred = g_features->prediction->predict( target->index, m_q_range, 2500.f, 50.f, 0.25f, { }, true );
                if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.q_hitchance->get< int >( ) ||
                    g_features->prediction->minion_in_line_predicted(
                        g_local->position,
                        pred.position,
                        55.f,
                        0.25f,
                        2500.f
                    ) )
                    return false;

                if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                    g_features->orbwalker->on_cast( );
                    m_last_q_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->gnar.w_enabled->get< bool >( ) || !m_big_gnar || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 100.f, 0.6f, { } );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->gnar.w_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->gnar.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->gnar.e_disable_aa_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            if ( m_big_gnar ) {
                const auto pred = g_features->prediction->predict( target->index, m_e_range, 800.f, 100.f, 0.f );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->gnar.e_hitchance
                    ->get< int >( ) ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            } else {
                const auto pred = g_features->prediction->predict( target->index, m_e_range, 900.f, 10.f, 0.f );
                if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->gnar.e_hitchance
                    ->get< int >( ) ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->gnar.w_autointerrupt->get< bool >( ) || !m_big_gnar || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 100.f, 0.6f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time = *g_time;
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.3f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return g_features->orbwalker->get_ping( );
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1750.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1750.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        bool m_big_gnar{ };

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 90.f, 115.f, 140.f, 165.f };
        std::array< float, 6 > m_e_damage = { 0.f, 50.f, 70.f, 90.f, 110.f, 130.f };

        float m_q_range{ 1125.f };
        float m_w_range{ 575.f };
        float m_e_range{ 475.f };
    };
}
