#pragma once

#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class sylas_module final : public IModule {
    public:
        virtual ~sylas_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "sylas_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Sylas" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "sylas" ), menu_order::champion_module );

            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );


            q_settings->checkbox( _( "enable" ), g_config->sylas.q_enabled );
            q_settings->checkbox( _( "prefer q2 over q1" ), g_config->sylas.q_prefer_q2 );
            q_settings->select(
                _( "hitchance" ),
                g_config->sylas.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->sylas.w_enabled );
            w_settings->slider_int( _( "min hp% threshold" ), g_config->sylas.w_min_health_percent, 30, 90, 1 );

            e_settings->checkbox( _( "enable" ), g_config->sylas.e_enabled );
            e_settings->checkbox( _( "use auto dash" ), g_config->sylas.e_use_dash );
            e_settings->select(
                _( "hitchance" ),
                g_config->sylas.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->sylas.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->sylas.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->sylas.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->sylas.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->sylas.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->sylas.q_draw_range->get< bool >( ) &&
                !g_config->sylas.w_draw_range->get< bool >( ) &&
                !g_config->sylas.e_draw_range->get< bool >( ) &&
                !g_config->sylas.r_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            // const auto obj = g_entity_list->get_by_index( g_local->index );
            // if ( !obj ) return;
            // auto local = obj->create_copy( );
            // local.update( );
            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->sylas.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sylas.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->sylas.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sylas.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->sylas.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sylas.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->sylas.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sylas.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_local->is_dead( ) || g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "SylasPassiveAttack" ) );
            if ( !buff ) m_passive_stacks = 0;
            else m_passive_stacks         = buff->stacks( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_q( );
                break;
            default:
                break;
            }

            //SylasPassiveAttack
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->sylas.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) && m_passive_stacks > 2 ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                45.f,
                g_config->sylas.q_prefer_q2->get< bool >( ) ? 1.f : 0.4f
            );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sylas.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->sylas.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_w_range ) return false;

            if ( g_features->orbwalker->is_attackable( target->index ) &&
                g_local->health / g_local->max_health * 100 > g_config->sylas.w_min_health_percent->get< int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->sylas.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.25f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto info = m_slot_e->get_spell_info( );
            if ( !info ) return false;

            auto data = info->get_spell_data( );
            if ( !data ) return false;

            const auto name = data->get_name( );

            if ( rt_hash( name.c_str() ) != ct_hash( "SylasE2" ) ) {
                if ( !g_config->sylas.e_use_dash->get< bool >( ) || !m_slot_e->is_ready( true ) ) return false;

                auto dash_end_position = g_local->position.extend( target->position, m_e_dash_range );

                if ( g_features->orbwalker->is_attackable( target->index ) ) {
                    if ( m_passive_stacks > 0 ) return false;

                    const auto pred = g_features->prediction->predict_default( target->index, 0.2f );
                    if ( !pred ) return false;

                    if ( g_local->position.dist_to( *pred ) <= target->position.
                                                                       dist_to( g_local->position ) ) {
                        dash_end_position
                            = target->position;
                    } else dash_end_position = *pred;
                } else if ( target->position.dist_to( g_local->position ) <= 400.f ) {
                    const auto pred = g_features->prediction->predict_default( target->index, 0.1f );
                    if ( !pred ) return false;

                    dash_end_position = *pred;
                }

                if ( dash_end_position.dist_to( target->position ) >= 500.f || !is_position_in_turret_range(
                        g_local->position
                    ) && is_position_in_turret_range( dash_end_position ) ||
                    !g_features->evade->is_position_safe( dash_end_position ) || g_features->prediction->minion_in_line(
                        dash_end_position,
                        target->position,
                        60.f
                    ) )
                    return false;

                if ( cast_spell( ESpellSlot::e, dash_end_position ) ) {
                    m_last_e_time = *g_time;
                    return true;
                }

                return false;
            }

            auto pred = g_features->prediction->predict( target->index, m_e_range, 1600.f, 60.f, 0.25f, { }, true );
            if ( !pred.valid ||
                static_cast< int >( pred.hitchance ) < g_config->sylas.e_hitchance->get< int >( ) ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f )
            )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.1f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( ESpellSlot slot, Object* target ) -> float override{ return 0.f; }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        int m_passive_stacks{ };

        float m_q_range{ 775.f };
        float m_w_range{ 400.f };
        float m_e_dash_range{ 400.f };
        float m_e_range{ 800.f };
        float m_r_range{ 950.f };
    };
}
