#pragma once
#include "../../sdk/game/hud_manager.hpp"
#include "../../sdk/game/pw_hud.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class evelynn_module final : public IModule {
    public:
        virtual ~evelynn_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "evelynn_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Evelynn" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "evelynn" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto logic      = navigation->add_section( _( "combo logic" ) );

            q_settings->checkbox( _( "enable" ), g_config->evelynn.q_enabled );
            q_settings->checkbox( _( "killsteal q" ), g_config->evelynn.q_killsteal );
            q_settings->checkbox( _( "spellclear q" ), g_config->evelynn.q_spellclear );
            q_settings->select(
                _( "hitchance" ),
                g_config->evelynn.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->evelynn.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->evelynn.e_enabled );
            e_settings->checkbox( _( "killsteal e" ), g_config->evelynn.e_killsteal );
            e_settings->checkbox( _( "jungleclear e" ), g_config->evelynn.e_spellclear );

            r_settings->checkbox( _( "enable" ), g_config->evelynn.r_enabled );
            r_settings->checkbox( _( "killsteal r" ), g_config->evelynn.r_killsteal );

            logic->checkbox( _( "delay spell/aa to stun" ), g_config->evelynn.delay_qe_for_stun );
            logic->checkbox( _( "^ ignore rule in fullcombo" ), g_config->evelynn.full_combo_ignore_delay );

            drawings->checkbox( _( "draw q range" ), g_config->evelynn.q_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->evelynn.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->evelynn.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->kindred.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->kindred.dont_draw_on_cooldown->get< bool >( ) ) ) {
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

            m_can_recast_q = rt_hash( m_slot_q->get_name( ).data( ) ) != ct_hash( "EvelynnQ" );

            cast_tracking( );
            recast_q( );

            if ( g_features->orbwalker->in_action( ) ) return;

            if ( g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                //if ( g_config->kindred.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                //laneclear_q( );
                //laneclear_e( );
                break;
            default:
                break;
            }

            //std::cout << "Q: " << m_slot_q->get_name( ) << " | States " << std::dec << ( int )m_slot_q->get_usable_state( ) << ":"
            //          << (int)m_slot_q->get_active_state( ) << " | 0x" << std::hex << m_slot_q.get_address( ) << std::endl;
        }

    private:
        auto recast_q( ) -> bool{
            if ( !m_can_recast_q || !m_slot_q->is_ready( ) && m_slot_q->cooldown_expire - *g_time > g_features->
                orbwalker->get_ping( )
                || *g_time - m_last_q_time <= 0.15f )
                return false;

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;

                std::cout << "[ Evelynn: Q2 ] Stacks: " << m_slot_q->charges << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( !g_config->kindred.q_laneclear->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) || !g_features->orbwalker->should_reset_aa( ) )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const auto cursor_position{ hud->cursor_position_unclipped };
            const auto after_dash_position{
                g_local->position.extend(
                    cursor_position,
                    std::min( 300.f, g_local->position.dist_to( cursor_position ) )
                )
            };

            if ( cast_spell( ESpellSlot::q, cursor_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->reset_aa_timer( );
                return true;
            }

            return false;
        }

        auto laneclear_e( ) -> bool{
            if ( !g_config->kindred.e_jungleclear->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) || !g_features->orbwalker->should_reset_aa( ) )
                return false;

            const auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;

            const Object* target{ };
            int           priority{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->dist_to_local( ) > m_e_range ||
                    minion->get_monster_priority( ) < 2
                )
                    continue;
                // good for jungle clear, want to ignore for OBJs and use HP threshold
                if ( minion->get_monster_priority( ) > priority ) {
                    target   = minion;
                    priority = minion->get_monster_priority( );
                }
            }

            if ( !target ) return false;

            if ( target->health / target->max_health * 100 > 50.f &&
                g_config->kindred.e_execute_camp_threshold->get< bool >( ) )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->reset_aa_timer( );
                return true;
            }

            return false;
        }

        auto spell_q( ) -> bool override{
            if ( !g_config->evelynn.q_enabled->get< bool >( ) || m_can_recast_q || are_spells_disabled( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_q_time <= 0.25f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || is_target_ignored( target->index ) ) return false;

            auto       minimum_impact_time{ -1.f };
            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "EvelynnW" ) );
            if ( buff ) minimum_impact_time = buff->buff_data->start_time + 2.5f;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 2400.f, 60.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < g_config->evelynn.q_hitchance->get< int >( )
                || minimum_impact_time > 0.f && *g_time + 0.25f + g_features->orbwalker->get_ping( ) / 2.f + g_local->
                position.dist_to( pred.default_position ) / 2400.f < minimum_impact_time )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Evelynn: Q ] Combo cast at " << std::dec << target->index << std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->evelynn.e_enabled->get< bool >( ) || are_spells_disabled( ) ||
                *g_time - m_last_cast_time <= 0.025f || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || is_target_ignored( target->index ) || !g_features->orbwalker->is_attackable(
                target->index,
                210.f,
                true,
                false
            ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "EvelynnW" ) );
            if ( buff && buff->buff_data->start_time + 2.5f > *g_time + 0.25f ) return false;


            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->kindred.r_enabled->get< bool >( ) ||
                g_config->kindred.r_only_full_combo->get< bool >( ) && !GetAsyncKeyState( VK_CONTROL ) ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto predicted_health = g_features->prediction->predict_health( g_local.get( ), 0.3f );
            const auto hp_threshold     = g_local->max_health * ( g_config->kindred.r_health_threshold->get< int >( ) *
                0.01f );
            if ( predicted_health > hp_threshold ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto cast_tracking( ) -> void{
            if ( !m_w_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 52 || sci->server_cast_time < *g_time ) {
                    //if ( sci ) std::cout << sci->get_spell_name( ) << " | SLOT: " << std::dec << sci->slot << std::endl;
                    return;
                }

                m_w_active           = true;
                m_w_server_cast_time = sci->server_cast_time;
                m_w_target_index     = g_pw_hud->get_hud_manager( )->last_target_handle;

                m_ignored_index      = m_w_target_index;
                m_ignore_expire_time = m_w_server_cast_time + 0.25f;

                std::cout << "[ Spell W ] Detected cast | " << *g_time << std::endl;

                //disable_spells( 0.4f );
            } else {
                if ( false && m_ignore_expire_time < m_w_server_cast_time ) {
                    bool found_particle{ };
                    Vec3 particle_position{ };

                    for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                        if ( !particle || particle->position.dist_to( g_local->position ) > 1600.f ) continue;

                        auto name = particle->get_alternative_name( );
                        if ( name.find( "Evelynn" ) == std::string::npos ||
                            name.find( "Mark_Cook_Ground" ) == std::string::npos )
                            continue;

                        found_particle    = true;
                        particle_position = particle->position;
                        break;
                    }

                    if ( !found_particle ) return;

                    int16_t target_index{ };
                    auto    lowest_distance{ 1600.f };
                    for ( auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || enemy->is_dead( ) ||
                            enemy->position.dist_to( particle_position ) > lowest_distance )
                            continue;

                        target_index    = enemy->index;
                        lowest_distance = enemy->position.dist_to( particle_position );
                    }

                    if ( target_index == 0 ) return;

                    m_ignore_expire_time = m_w_server_cast_time + 0.25f;
                    m_ignored_index      = target_index;
                    disable_spells( 0.f );

                    std::cout << "[ W: Found target ]: " << std::dec << target_index
                        << " | distance: " << lowest_distance << std::endl;
                }

                if ( *g_time > m_w_server_cast_time ) m_w_active = false;
            }
        }

        auto is_target_ignored( const int16_t index ) const -> bool{
            return m_ignore_expire_time > *g_time && m_ignored_index == index;
        }

        auto are_spells_disabled( ) const -> bool{ return m_disable_spells && m_disable_expire_time > *g_time; }

        auto disable_spells( const float duration ) -> void{
            m_disable_spells      = true;
            m_disable_expire_time = *g_time + duration;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 0.4f,
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->bonus_attack_damage( ) * 0.7f,
                    target->index,
                    true
                );
            }
            return 0.f;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_cast_time{ };
        bool  m_passive_logged{ };

        // ignored target
        int16_t m_ignored_index{ };
        float   m_ignore_expire_time{ };

        // cast tracking
        bool    m_w_active{ };
        float   m_w_server_cast_time{ };
        int16_t m_w_target_index{ };

        // disabling
        bool  m_disable_spells{ };
        float m_disable_expire_time{ };

        // Q stuff
        bool m_can_recast_q{ };

        std::vector< float > m_q_damage = { 0.f, 40.f, 80.f, 100.f, 130.f, 160.f };
        std::vector< float > m_w_damage = { 0.f, 70.f, 115.f, 160.f, 205.f, 250.f };
        std::vector< float > m_r_damage = { 0.f, 175.f, 250.f, 325.f };

        float m_q_range{ 925.f };
        float m_q_dash_range{ 300.f };

        float m_w_range{ 1300.f };
        float m_e_range{ 425.f };
    };
} // namespace features::champion_modules
