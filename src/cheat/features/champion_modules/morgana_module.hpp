#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class morgana_module final : public IModule {
    public:
        virtual ~morgana_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "morgana_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Morgana" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "morgana" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->morgana.q_enabled );
            q_settings->checkbox( _( "antigapclose" ), g_config->morgana.q_antigapclose );
            q_settings->checkbox( _( "auto interrupt (?)" ), g_config->morgana.q_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            q_settings->checkbox( _( "use in harass" ), g_config->morgana.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->morgana.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->morgana.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->morgana.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->morgana.r_enabled );
            r_settings->slider_int( _( "minimum hitcount" ), g_config->morgana.r_minimum_hitcount, 1, 5, 1 );

            spellclear->checkbox( _( "w spellclear" ), g_config->morgana.w_laneclear );
            spellclear->slider_int( _( "^ minimum mana %" ), g_config->morgana.spellclear_min_mana, 10, 90, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->morgana.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->morgana.w_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->morgana.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->morgana.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->morgana.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->morgana.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
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

            if ( g_config->morgana.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->morgana.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
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

            if ( g_config->morgana.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->morgana.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
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

            antigapclose_q( );
            autointerrupt_q( );
            spell_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->morgana.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->morgana.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->morgana.q_hitchance->get< int >( ) )
                ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    70.f,
                    0.25f,
                    1200.f
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
            if ( !g_config->morgana.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 100.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->morgana.w_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->morgana.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            unsigned target_nid{ };
            bool     found_target{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > 800.f ) continue;

                if ( should_spellshield_ally( ally ) ) {
                    found_target = true;
                    target_nid   = ally->network_id;
                    break;
                }
            }

            if ( !found_target ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->morgana.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready(
                true
            ) )
                return false;

            int enemy_count{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy ||
                    g_features->target_selector->is_bad_target( enemy->index ) ||
                    enemy->dist_to_local( ) > m_r_range
                )
                    continue;

                auto predict = g_features->prediction->predict_default( enemy->index, 0.25f );
                if ( !predict || predict.value( ).dist_to( g_local->position ) > m_r_range ) continue;

                enemy_count++;
            }

            if ( enemy_count < g_config->morgana.r_minimum_hitcount->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto laneclear_w( ) -> bool{
            if ( *g_time - m_last_w_time <= 0.4f ||
                g_local->mana < g_local->max_mana / 100.f * g_config->morgana.spellclear_min_mana->get< int >( ) ||
                !m_slot_w->is_ready( true ) || !GetAsyncKeyState( VK_CONTROL )
            )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_w_range, 135.f );
            if ( farm_pos.value < 3 ) return false;

            if ( cast_spell( ESpellSlot::w, farm_pos.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );

                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->morgana.q_antigapclose->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    70.f,
                    0.25f,
                    1200.f
                ) )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) m_last_q_time = *g_time;
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->morgana.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1200.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ||
                g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    70.f,
                    0.25f,
                    1200.f
                ) )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) m_last_q_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1200.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1200.f;
            }
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

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 150.f, 225.f, 300.f };

        float m_q_range{ 1300.f };
        float m_w_range{ 900.f };
        float m_e_range{ 800.f };
        float m_r_range{ 575.f };
    };
}
