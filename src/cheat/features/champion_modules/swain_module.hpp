#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class swain_module final : public IModule {
    public:
        virtual ~swain_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "swain_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Swain" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation     = g_window->push( _( "swain" ), menu_order::champion_module );
            const auto q_settings     = navigation->add_section( _( "q settings" ) );
            const auto drawings       = navigation->add_section( _( "drawings" ) );
            const auto w_settings     = navigation->add_section( _( "w settings" ) );
            const auto e_settings     = navigation->add_section( _( "e settings" ) );
            const auto r_settings     = navigation->add_section( _( "r settings" ) );
            const auto combo_settings = navigation->add_section( _( "combo settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->swain.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->swain.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->swain.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->swain.w_enabled );
            w_settings->checkbox( _( "allow lower hitchance (?)" ), g_config->swain.w_allow_lower_hitchance )->
                        set_tooltip( _( "Will allow lower enemy hitchance if enemy in combat with ally" ) );
            w_settings->select(
                _( "hitchance" ),
                g_config->swain.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->swain.e_enabled );
            e_settings->checkbox( _( "harass e" ), g_config->swain.e_harass );
            e_settings->checkbox( _( "flee e" ), g_config->swain.e_flee );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->swain.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" )
            );
            e_settings->select(
                _( "hitchance" ),
                g_config->swain.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->swain.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->swain.r_full_combo );
            r_settings->checkbox( _( "R2 if killable" ), g_config->swain.r_execute );
            r_settings->slider_int( _( "min enemy in range" ), g_config->swain.r_min_enemy_count, 1, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->swain.q_draw_range );
            drawings->checkbox( _( "draw w range minimap" ), g_config->swain.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->swain.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->swain.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->swain.q_draw_range->get< bool >( ) &&
                !g_config->swain.w_draw_range->get< bool >( ) &&
                !g_config->swain.e_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->swain.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->swain.dont_draw_on_cooldown->get<
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

            if ( g_config->swain.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->swain.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_minimap(
                        g_local->position,
                        Color( 255, 255, 255, 155 ),
                        m_w_range,
                        70,
                        1.f
                    );
                }
            }

            if ( g_config->swain.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->swain.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            m_w_range = 5000.f + 500.f * m_slot_w->level;
            m_e_delay = rt_hash( m_slot_e->get_name().c_str() ) == ct_hash( "SwainE2" );

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::recalling && g_features->orbwalker->
                get_mode( ) != Orbwalker::EOrbwalkerMode::flee )
                spell_w( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_e( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->swain.q_harass->get< bool >( ) ) spell_q( );
                if ( g_config->swain.e_harass->get< bool >( ) ) spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->swain.e_flee->get< bool >( ) ) spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->swain.q_enabled->get< bool >( ) || m_e_delay || *g_time - m_last_q_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, 700.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->swain.q_hitchance->get< int >( ) ) return false;


            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->swain.w_enabled->get< bool >( ) || m_e_delay || *g_time - m_last_w_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.15f || !m_slot_w->is_ready( true ) )
                return false;

            const Object* target{ };
            auto          target_priority{ -1 };
            Vec3          cast_position{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_w_range || g_features->target_selector->is_bad_target(
                    enemy->index
                ) )
                    continue;

                auto       hitchance = g_config->swain.w_hitchance->get< int >( );
                const auto health    = g_features->prediction->predict_health( enemy, 1.f, false, true, true );
                const auto allow_lower_hitchance{
                    g_config->swain.w_allow_lower_hitchance->get< bool >( ) && enemy->dist_to_local( ) > 1000.f &&
                    health < enemy->health
                };

                if ( allow_lower_hitchance ) hitchance = 2;

                auto pred = g_features->prediction->predict( enemy->index, m_w_range, 0.f, 325.f, 1.5f );
                if ( !pred.valid || ( int )pred.hitchance < hitchance ) return false;

                const auto priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );

                if ( priority < target_priority || target && priority == target_priority && enemy->health > target->
                    health )
                    continue;

                target          = enemy;
                target_priority = priority;
                cast_position   = pred.position;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->swain.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f )
                return false;

            if ( rt_hash( m_slot_e->get_name( ).c_str( ) ) == ct_hash( "SwainE" ) ) {
                if ( !m_slot_e->is_ready( true ) ) return false;

                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                auto pred = g_features->prediction->predict( target->index, m_e_range, 935.f, 80.f, 0.25f );
                if ( !pred.valid || ( int )pred.hitchance < g_config->swain.e_hitchance->get< int >( ) ) return false;

                const auto return_position = g_local->position.extend( pred.position, m_e_range );
                if ( g_features->prediction->minion_in_line( return_position, pred.position, 85.f ) ) return false;


                if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }
            } else {
                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) return false;

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    return true;
                }
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->swain.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( ) )
                return false;

            if ( rt_hash( m_slot_r->get_name().c_str() ) == ct_hash( "SwainRSoulFlare" ) ) {
                if ( !g_config->swain.r_execute->get< bool >( ) ) return false;

                bool execute_found{ };
                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->dist_to_local( ) > m_r_range || g_features->target_selector->is_bad_target(
                        enemy->index
                    ) )
                        continue;

                    auto predict = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !predict || predict.value( ).dist_to( g_local->position ) > m_r_range ) continue;

                    const auto health = g_features->prediction->predict_health( enemy, 0.25f, false, true, true );
                    const auto damage = get_spell_damage( ESpellSlot::r, enemy );

                    if ( health >= damage ) continue;

                    execute_found = true;
                    break;
                }

                if ( !execute_found ) return false;

                if ( cast_spell( ESpellSlot::r ) ) {
                    m_last_r_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->on_cast( );
                    return true;
                }

                return false;
            }

            if ( g_config->swain.r_full_combo->get< bool >( ) && !GetAsyncKeyState( VK_CONTROL ) || !m_slot_r->is_ready(
                true
            ) )
                return false;

            int enemy_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || g_features->target_selector->is_bad_target( enemy->index ) || enemy->dist_to_local( ) >
                    m_r_range )
                    continue;

                auto predict = g_features->prediction->predict_default( enemy->index, 0.5f );
                if ( !predict || predict.value( ).dist_to( g_local->position ) > m_r_range ) continue;

                enemy_count++;
            }

            if ( enemy_count < g_config->swain.r_min_enemy_count->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->swain.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, m_e_range, 935.f, 40.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;


            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
            {
                const auto damage = m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 0.6f;

                return helper::calculate_damage( damage, target->index, false );
            }
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        bool m_e_delay{ };

        float m_last_cast_time{ };

        std::vector< float > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::vector< float > m_r_damage = { 0.f, 150.f, 225.f, 300.f };

        float m_q_range{ 725.f };
        float m_w_range{ 5500.f };
        float m_e_range{ 850.f };
        float m_r_range{ 650.f };
    };
}
