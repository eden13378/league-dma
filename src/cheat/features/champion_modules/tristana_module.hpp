#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class tristana_module final : public IModule {
    public:
        virtual ~tristana_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "tristana_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Tristana" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "tristana" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->tristana.q_enabled );
            q_settings->checkbox( _( "use in laneclear" ), g_config->tristana.q_laneclear );

            e_settings->checkbox( _( "enable" ), g_config->tristana.e_enabled );

            r_settings->checkbox( _( "enable" ), g_config->tristana.r_enabled );
            r_settings->checkbox( _( "antigapclose" ), g_config->tristana.r_antigapclose );
            r_settings->checkbox( _( "auto interrupt (?)" ), g_config->tristana.r_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            m_in_dash = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "TristanaW" ) );

            antigapclose_r( );
            autointerrupt_r( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->tristana.q_laneclear->get< bool >( ) ) spell_q( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->tristana.q_enabled->get< bool >( ) ||
                !m_slot_q->is_ready( true ) ||
                *g_time - m_last_q_time <= 0.4f ||
                !g_features->orbwalker->should_reset_aa( ) ||
                m_in_dash
            )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->tristana.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "tristanaecharge" ) );
            if ( buff ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->tristana.r_enabled->get< bool >( ) ||
                !m_slot_r->is_ready( true ) ||
                *g_time - m_last_r_time <= 0.4f ||
                m_in_dash
            )
                return false;

            const Object* target{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || !g_features->orbwalker->is_attackable( enemy->index ) || g_features->target_selector->
                    is_bad_target( enemy->index ) )
                    continue;

                auto damage{ get_spell_damage( ESpellSlot::r, enemy ) };

                damage += get_spell_damage( ESpellSlot::e, enemy );

                if ( damage <= enemy->health + enemy->total_health_regen ) continue;

                target = enemy;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                m_last_r_time = *g_time;
                if ( g_features->orbwalker->get_next_aa_time( ) <= *g_time + g_features->orbwalker->
                    get_attack_cast_delay( ) )
                    g_features->orbwalker->set_cast_time( g_features->orbwalker->get_attack_cast_delay( ) * 2 );
                else g_features->orbwalker->set_cast_time( g_features->orbwalker->get_attack_cast_delay( ) );
                return true;
            }

            return false;
        }

        auto antigapclose_r( ) -> void{
            if ( !g_config->tristana.r_antigapclose->get< bool >( ) || m_in_dash || *g_time - m_last_r_time <= 0.4f || !
                m_slot_r->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( 300.f );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) m_last_r_time = *g_time;
        }

        auto autointerrupt_r( ) -> void{
            if ( !g_config->tristana.r_autointerrupt->get< bool >( ) || m_in_dash || *g_time - m_last_r_time <= 0.4f ||
                !m_slot_r->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( g_local->attack_range + 65.f );
            if ( !target || !g_features->orbwalker->is_attackable( target->index )
                || g_features->prediction->windwall_in_line( g_local->position, target->position ) )
                return;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) m_last_r_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                auto& enemy = g_entity_list.get_by_index( target->index );
                if ( !enemy ) return 0.f;

                enemy.update( );
                if ( enemy->is_dead( ) ) return 0.f;

                const auto buff     = g_features->buff_cache->get_buff( enemy->index, ct_hash( "tristanaecharge" ) );
                const auto alt_buff = g_features->buff_cache->get_buff(
                    enemy->index,
                    ct_hash( "tristanaechargesound" )
                );

                if ( !buff && !alt_buff ) return 0.f;

                auto e_damage = m_e_base_damage[ m_slot_e->level ] + m_e_bonus_ad_modifier[ m_slot_e->level ] * g_local
                    ->bonus_attack_damage( ) + g_local->ability_power( ) * 0.5f;

                if ( alt_buff ) {
                    const auto stacks = alt_buff->stacks( );

                    e_damage += ( m_e_stack_base_damage[ m_slot_e->level ] + m_e_stack_bonus_ad_modifier[ m_slot_e->
                        level ] * g_local->bonus_attack_damage( ) + g_local->ability_power( ) * 0.15f ) * stacks;
                }

                return helper::calculate_damage( e_damage, target->index, true );
            }
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 1.f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

    private:
        float m_last_q_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_q_range{ 0.f };
        float m_w_range{ 0.f };

        bool m_in_dash{ };

        std::vector< float > m_r_damage{
            0.f,
            300.f,
            400.f,
            500.f
        };

        std::array< float, 6 > m_e_base_damage = {
            0.f,
            70.f,
            80.f,
            90.f,
            100.f,
            110.f
        };

        std::array< float, 6 > m_e_bonus_ad_modifier = {
            0.f,
            0.5f,
            0.75f,
            1.f,
            1.25f,
            1.5f
        };

        std::array< float, 6 > m_e_stack_base_damage = {
            0.f,
            21.f,
            24.f,
            27.f,
            30.f,
            33.f
        };

        std::array< float, 6 > m_e_stack_bonus_ad_modifier = {
            0.f,
            .15f,
            .225f,
            .30f,
            .375f,
            .45f
        };
    };
}
