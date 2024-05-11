#pragma once
#include "module.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../evade.hpp"
#include "../../menu/menu.hpp"


#include "../../sdk/game/spell_cast_info.hpp"

namespace features::champion_modules {
    class jax_module final : public IModule {
    public:
        virtual ~jax_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "jax_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Jax" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "jax" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->jax.q_enabled );
            q_settings->checkbox( _( "only out of aa range" ), g_config->jax.q_only_out_of_range );

            w_settings->checkbox( _( "enable" ), g_config->jax.w_enabled );
            w_settings->checkbox( _( "use in laneclear" ), g_config->jax.w_laneclear );

            e_settings->checkbox( _( "enable" ), g_config->jax.e_enabled );
            e_settings->checkbox( _( "block autoattack" ), g_config->jax.e_only_on_autoattack );
            e_settings->checkbox( _( "allow E2" ), g_config->jax.e_recast );

            drawings->checkbox( _( "draw q range" ), g_config->jax.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->jax.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->jax.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->jax.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->jax.q_draw_range->get< bool >( ) &&
                !g_config->jax.w_draw_range->get< bool >( ) &&
                !g_config->jax.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );

            auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
            if ( g_config->jax.q_draw_range->get< bool >( ) && slot && slot->level > 0 && ( slot->is_ready( true ) || !
                g_config->jax.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 31, 88, 255, 255 ),
                    m_q_range,
                    Renderer::outline,
                    m_circle_segments,
                    2.f
                );
            }

            slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            if ( g_config->jax.w_draw_range->get< bool >( ) && slot && slot->level > 0 && ( slot->is_ready( true ) && !
                m_w_active || !g_config->jax.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 132, 245, 66, 255 ),
                    m_w_range,
                    Renderer::outline,
                    m_circle_segments,
                    2.f
                );
            }

            slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( g_config->jax.e_draw_range->get< bool >( ) && slot && slot->level > 0 && ( slot->is_ready( true ) && !
                m_e_active || !g_config->jax.dont_draw_on_cooldown->get< bool >( ) ) ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 173, 47, 68, 255 ),
                    m_e_range,
                    Renderer::outline,
                    m_circle_segments,
                    2.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxEmpowerTwo" ) ) ) {
                m_w_range  = g_local->attack_range + 65.f + 50.f;
                m_w_active = false;
            } else m_w_active = true;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxCounterStrike" ) ) ) m_e_active = true;
            else m_e_active                                                                                     = false;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                if ( spell_e( ) ) return;
                if ( spell_w( ) ) return;
                if ( spell_q( ) ) return;
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->jax.w_laneclear->get< bool >( ) ) spell_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->jax.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || helper::is_position_under_turret( target->position ) ) return false;

            bool allow_cast{ };

            if ( !g_features->orbwalker->is_attackable( target->index, m_q_range, false, false ) ||
                g_config->jax.q_only_out_of_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                    target->index,
                    g_local->attack_range + 125.f
                ) )
                return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxCounterStrike" ) ) && !m_slot_e->
                is_ready( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->jax.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            if ( m_w_active ) {
                std::cout << "w active??\n";
                return false;
            }

            std::cout << "[ W ]: " << g_features->orbwalker->should_reset_aa( ) << std::endl;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || g_features->orbwalker->get_last_target( ) !=
                    ETargetType::hero )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;

                std::cout << "[ Jax: W ]: AA reset | should_reset: " << g_features->orbwalker->should_reset_aa( )
                    << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->jax.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "JaxCounterStrike" ) ) ) {
                if ( !g_config->jax.e_recast->get< bool >( ) || target->dist_to_local( ) < 350.f ) return false;

                auto pred = g_features->prediction->predict_default(
                    g_local->index,
                    g_features->orbwalker->get_ping( ),
                    false
                );
                if ( !pred ) return false;

                const auto position_on_cast{ *pred };

                pred = g_features->prediction->predict_default(
                    target->index,
                    g_features->orbwalker->get_ping( ),
                    false
                );
                if ( !pred ) return false;

                if ( position_on_cast.dist_to( *pred ) >= m_e_range - 5.f ) return false;

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }

                return false;
            }

            if ( !m_slot_e->is_ready( true ) ) return false;

            bool allow_cast{ };

            if ( g_config->jax.e_only_on_autoattack->get< bool >( ) ) {
                auto target_sci = target->spell_book.get_spell_cast_info( );
                if ( !target_sci || !target_sci->is_autoattack && !target_sci->is_special_attack || target_sci->
                    server_cast_time <= *g_time - g_features->orbwalker->get_ping( ) || target_sci->get_target_index( )
                    != g_local->index )
                    return false;

                allow_cast = true;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_cast_time{ };

        float m_q_range{ 700.f };
        float m_w_range{ 0.f };
        float m_e_range{ 375.f };

        bool m_e_active{ };
        bool m_w_active{ };

        int32_t m_circle_segments{ 50 };
    };
}
