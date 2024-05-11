#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class ryze_module final : public IModule {
    public:
        virtual ~ryze_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "ryze_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Ryze" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "ryze" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->ryze.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->ryze.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->ryze.w_enabled );
            w_settings->checkbox( _( "only use to root" ), g_config->ryze.w_only_root );

            e_settings->checkbox( _( "enable" ), g_config->ryze.e_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->ryze.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->ryze.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ryze.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->ryze.q_draw_range->get< bool >( ) &&
                !g_config->ryze.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->ryze.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ryze.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        70,
                        3.f
                    );
                }
            }

            if ( g_config->ryze.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ryze.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range + 65.f,
                        Renderer::outline,
                        70,
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
                spell_e( );
                spell_w( );
                spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->ryze.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1700.f,
                50.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ryze.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( g_features->prediction->minion_in_line_predicted(
                g_local->position,
                pred.position,
                55.f,
                0.25f,
                1700.f
            ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->ryze.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) || m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( !g_features->buff_cache->get_buff( target->index, ct_hash( "RyzeE" ) ) && g_config->ryze.w_only_root->
                get< bool >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->ryze.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) || m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->buff_cache->get_buff( target->index, ct_hash( "RyzeE" ) ) ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1700.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1700.f;
            }
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 75.f, 100.f, 125.f, 150.f, 175.f };

        float m_q_range{ 1000.f };
        float m_w_range{ 550.f };
        float m_e_range{ 550.f };
    };
}
