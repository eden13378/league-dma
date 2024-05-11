#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../tracker.hpp"

namespace features::champion_modules {
    class jinx_module final : public IModule {
    public:
        virtual ~jinx_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "jinx_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Jinx" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "jinx" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->jinx.q_enabled );

            w_settings->checkbox( _( "enable" ), g_config->jinx.w_enabled );
            w_settings->checkbox( _( "only out of aa range" ), g_config->jinx.w_only_out_of_range );
            w_settings->select(
                _( "hitchance" ),
                g_config->jinx.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->jinx.e_enabled );
            e_settings->checkbox( _( "antigapclose" ), g_config->jinx.e_antigapclose );
            e_settings->checkbox( _( "anti-stasis" ), g_config->jinx.e_antistasis );
            e_settings->checkbox( _( "auto-interrupt (?)" ), g_config->jinx.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" )
            );
            e_settings->select(
                _( "hitchance" ),
                g_config->jinx.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "recall ult (?)" ), g_config->jinx.r_recall_ult )->set_tooltip(
                _( "Will R killable targets who are recalling" )
            );
            r_settings->checkbox( _( "prefer delay (?)" ), g_config->jinx.r_recall_ult_delay )->set_tooltip(
                _( "Prefer to wait until last possible time to ult, recommended" )
            );

            drawings->checkbox( _( "draw q range" ), g_config->jinx.q_draw_range );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) {
                m_recall_ult_active = false;
                return;
            }

            g_local.update( );

            if ( g_config->jinx.q_draw_range->get< bool >( ) ) {
                auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );

                if ( slot && slot->level > 0 ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 255, 255, 255 ),
                        g_local->attack_range == m_minigun_range + m_bonus_attackrange
                            ? m_rocket_range + m_bonus_attackrange + 65.f
                            : m_minigun_range + m_bonus_attackrange + 65.f,
                        Renderer::outline,
                        72,
                        3.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_q( );

            if ( g_features->orbwalker->in_action( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );
            antistasis_e( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_e( );
                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                spell_q( );
                break;
            default:
                break;
            }

            //std::cout << "q name: " << m_slot_q->get_name( ) << " | usable: " << ( int )m_slot_q->get_usable_state( )
            //          << " | active: " << (int)m_slot_q->get_active_state( ) << std::endl;
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->jinx.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::laneclear &&
                !m_is_rocket_aa )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ||
                g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::harass
            ) {
                const auto aa_damage = helper::get_aa_damage( target->index );
                bool       should_q{ };

                if ( !m_is_rocket_aa && !g_features->orbwalker->is_attackable( target->index ) && g_features->orbwalker
                    ->is_attackable( target->index, m_rocket_range + m_bonus_attackrange, true )
                    || m_is_rocket_aa && g_features->orbwalker->is_attackable(
                        target->index,
                        m_minigun_range + m_bonus_attackrange,
                        true
                    )
                    && helper::get_real_health(
                        target->index,
                        EDamageType::physical_damage,
                        g_features->orbwalker->get_attack_cast_delay( ),
                        true
                    ) - aa_damage * 1.9f > 0.f )
                    should_q = true;

                if ( !should_q ) return false;
            }

            if ( cast_spell( ESpellSlot::q ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->jinx.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            auto cast_time = 0.6f - 0.02f * std::floor( g_local->bonus_attack_speed / 0.25f );
            if ( cast_time < 0.4f ) cast_time = 0.4f;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->jinx.w_only_out_of_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_w_range,
                3300.f,
                60.f,
                cast_time,
                { },
                true
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->jinx.w_hitchance->get<
                    int >( ) ) ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f )
            )
                return false;


            if ( cast_spell( ESpellSlot::w, predicted.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->jinx.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_e_range,
                0.f,
                115.f,
                0.9f
            );

            if ( !predicted.valid ||
                predicted.hitchance < static_cast< Prediction::EHitchance >( g_config->jinx.e_hitchance->get<
                    int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto antigapclose_e( ) -> void{
            if ( !g_config->jinx.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 0.f, 100.f, 1.f );
            if ( !target ) return;

            const auto aimgr = target->get_ai_manager( );
            if ( !aimgr ) return;

            const auto predicted = g_features->prediction->predict( target->index, m_e_range, 0.f, 100.f, 1.f );
            if ( !predicted.valid || ( int )predicted.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) m_last_e_time = *g_time;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->jinx.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto predicted = g_features->prediction->predict( target->index, m_e_range, 0.f, 100.f, 1.f );
            if ( !predicted.valid || ( int )predicted.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) m_last_e_time = *g_time;
        }

        auto antistasis_e( ) -> bool{
            if ( !g_config->jinx.e_antistasis->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = get_stasis_target(
                m_e_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                115.f
            );
            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict( target->index, m_e_range, 0.f, 115.f, 1.f );
            if ( !predicted.valid || ( int )predicted.hitchance < 3 ) return false;

            if ( cast_spell( ESpellSlot::e, predicted.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                return true;
            }

            return false;
        }

        auto recall_ult( ) -> void{
            if ( !g_config->jinx.r_recall_ult->get< bool >( ) || *g_time - m_last_r_time <= 0.5f || !m_slot_r->is_ready(
                true
            ) ) {
                m_recall_ult_active = false;
                return;
            }


            if ( m_recall_ult_active ) {
                base_ult_tracking( );
                return;
            }

            Object* target{ };
            bool    found_target{ };

            for ( auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) <= 1500.f || !enemy->
                    is_recalling( ) )
                    continue;

                auto recall = g_features->tracker->get_recall( enemy->index );
                if ( !recall ) continue;

                auto recall_time_left = recall->finish_time - *g_time;
                auto travel_time      = 0.6f + 1300.f / 1700.f + ( enemy->dist_to_local( ) - 1300.f ) / 2200.f;

                if ( travel_time >= recall_time_left ) continue;

                float health_regenerated{ };

                if ( enemy->is_invisible( ) ) {
                    auto last_seen_data = g_features->tracker->get_last_seen_data( enemy->index );
                    if ( !last_seen_data ) continue;

                    const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                    const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                    if ( time_missing * enemy->movement_speed >= 140.f ) continue;

                    health_regenerated = ( *g_time - last_seen_data->last_seen_time ) * enemy->total_health_regen;
                    health_regenerated += std::ceil( travel_time ) * enemy->total_health_regen;
                } else health_regenerated = std::ceil( travel_time ) * enemy->total_health_regen;

                auto damage = get_spell_damage( ESpellSlot::r, enemy, health_regenerated );
                if ( damage < enemy->health + health_regenerated ) continue;

                target       = enemy;
                found_target = true;
                break;
            }

            if ( !found_target || !target ) return;

            auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) return;

            m_target_index       = target->index;
            m_baseult_start_time = *g_time;
            m_recall_ult_active  = true;

            base_ult_tracking( );
        }

        auto base_ult_tracking( ) -> void{
            if ( !m_recall_ult_active || !m_slot_r->is_ready( true ) ) {
                m_recall_ult_active = false;
                return;
            }

            const auto& target = g_entity_list.get_by_index( m_target_index );
            if ( !target || !target->is_recalling( ) || target->dist_to_local( ) < 1500.f ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall = g_features->tracker->get_recall( target->index );
            if ( !recall ) {
                m_recall_ult_active = false;
                return;
            }

            const auto recall_time_left = recall->finish_time - *g_time;
            const auto travel_time      = 0.6f + 1300.f / 1700.f + ( target->dist_to_local( ) - 1300.f ) / 2200.f;

            const auto time_until_hit = recall_time_left - travel_time;
            auto       health_regenerated{ std::ceil( time_until_hit ) * target->total_health_regen };
            auto       min_possible_health_regen = std::ceil( travel_time ) * target->total_health_regen;

            if ( target->is_invisible( ) ) {
                const auto last_seen_data = g_features->tracker->get_last_seen_data( target->index );
                if ( !last_seen_data ) {
                    m_recall_ult_active = false;
                    return;
                }

                const auto time_reduction = last_seen_data->total_recall_time + last_seen_data->time_in_recall;
                const auto time_missing   = *g_time - last_seen_data->last_seen_time - time_reduction;
                if ( time_missing * target->movement_speed >= 140.f ) {
                    m_recall_ult_active = false;
                    return;
                }

                health_regenerated += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
                min_possible_health_regen += std::ceil( *g_time - last_seen_data->last_seen_time ) * target->
                    total_health_regen;
            }

            bool should_cast{ };

            m_predicted_cast_time = recall->finish_time - travel_time - 0.5f;

            auto damage = get_spell_damage( ESpellSlot::r, target.get( ), health_regenerated );

            if ( damage > target->health + health_regenerated ) should_cast = time_until_hit <= 0.5f;
            else if ( get_spell_damage( ESpellSlot::r, target.get( ), min_possible_health_regen ) > target->health +
                min_possible_health_regen ) {
                if ( g_config->jinx.r_recall_ult_delay->get< bool >( ) ) {
                    int        max_wait_time{ };
                    const auto seconds_until_recall = static_cast< int >( std::floor( recall_time_left - 1.f ) );

                    for ( auto i = 1; i <= seconds_until_recall; i++ ) {
                        const auto regen_amount = min_possible_health_regen + target->total_health_regen * i;
                        damage                  = get_spell_damage( ESpellSlot::r, target.get( ), regen_amount );

                        if ( target->health + regen_amount >= damage ) break;

                        max_wait_time = i;
                    }

                    m_predicted_cast_time = recall->finish_time - travel_time - ( static_cast< float >( max_wait_time )
                        + 1.f ) - 0.5f;
                    should_cast = max_wait_time <= 1;
                } else should_cast = true;
            } else {
                m_recall_ult_active = false;
                return;
            }

            if ( !should_cast ) return;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                || g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::flee ) {
                m_recall_ult_active = false;
                return;
            }

            if ( cast_spell( ESpellSlot::r, target->position ) ) {
                m_recall_ult_active = false;
                m_last_r_time       = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto get_spell_damage( const ESpellSlot slot, Object* target, const float regen_health ) -> float{
            switch ( slot ) {
            case ESpellSlot::r:
            {
                const auto missing_health = target->max_health - ( target->health + regen_health );
                auto       damage         = m_r_damage[ m_slot_r->level ] + g_local->bonus_attack_damage( );

                damage += missing_health * m_r_missing_health_damage[ m_slot_r->level ];

                return helper::calculate_damage( damage, target->index, true );
            }
            default:
                break;
            }

            return 0.f;
        }

        auto update_q( ) -> void{
            if ( !m_is_rocket_aa && m_slot_q->get_usable_state( ) == 0
                || m_is_rocket_aa && m_slot_q->get_usable_state( ) == 1 ) {
                m_last_q_toggle_time = *g_time;
                m_is_rocket_aa       = m_slot_q->get_usable_state( ) == 0;
            }

            m_minigun_range = 525.f;
            m_rocket_range  = 525.f + m_extra_q_range[ m_slot_q->level ];

            if ( g_local->attack_range < m_rocket_range ) {
                m_bonus_attackrange = g_local->attack_range < m_rocket_range
                                          ? g_local->attack_range - m_minigun_range
                                          : g_local->attack_range - m_rocket_range;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 1.f;
            case ESpellSlot::r:
            {
                const auto tt   = 1.f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 1.f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            default:
                break;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        float m_q_range{ 0.f };
        float m_w_range{ 1450.f };
        float m_e_range{ 900.f };

        float m_rocket_range{ };
        float m_minigun_range{ };
        float m_bonus_attackrange{ };

        bool  m_is_rocket_aa{ };
        float m_last_q_toggle_time{ };

        // silent baseult
        bool    m_recall_ult_active{ };
        int16_t m_target_index{ };
        float   m_baseult_start_time{ };
        float   m_predicted_cast_time{ };

        std::array< float, 6 > m_extra_q_range =
        {
            0.f,
            80.f,
            110.f,
            140.f,
            180.f,
            200.f
        };

        std::vector< float > m_r_damage                = { 0.f, 300.f, 450.f, 600.f };
        std::vector< float > m_r_missing_health_damage = { 0.f, 0.25f, 0.3f, 0.35f };

        float m_base_attack_range{ 525.f };
    };
}
