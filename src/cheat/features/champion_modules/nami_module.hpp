#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class nami_module final : public IModule {
    public:
        virtual ~nami_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "nami_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Nami" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "nami" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->nami.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->nami.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->nami.q_flee );
            q_settings->checkbox( _( "antigapclose q" ), g_config->nami.q_antigapclose );
            q_settings->checkbox( _( "auto-interrupt q (?)" ), g_config->nami.q_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->nami.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->nami.w_enabled );
            w_settings->select(
                _( "bounce prefer" ),
                g_config->nami.w_bounce_logic,
                { _( "Damage" ), _( "Healing" ) }
            );
            w_settings->slider_int( _( "heal threshold (?)" ), g_config->nami.w_heal_threshold, 10, 100, 1 )->
                        set_tooltip( _( "Ally HP% must be less than X to directly heal them" ) );
            w_settings->slider_int(
                _( "simulated bounce max range %" ),
                g_config->nami.w_simulated_bounce_max_range,
                25,
                100,
                1
            );

            e_settings->checkbox( _( "enable" ), g_config->nami.e_enabled );
            e_settings->multi_select(
                _( "cast on " ),
                { g_config->nami.e_on_ally_autoattack, g_config->nami.e_on_self_autoattack },
                { _( "Ally autoattack" ), _( "Self autoattack" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->nami.r_enabled );
            r_settings->slider_int( _( "minimum hitcount" ), g_config->nami.r_minimum_hitcount, 2, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->nami.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->nami.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->nami.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->nami.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->nami.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->nami.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->nami.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->nami.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->nami.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->nami.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->nami.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
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
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->nami.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->nami.q_flee->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->nami.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                200.f,
                1.f,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->nami.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Nami ] Q spellcast, target " << target->champion_name.text << " | " << *g_time <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->nami.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.4f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            const auto max_bounce_range = 800.f * ( g_config->nami.w_simulated_bounce_max_range->get< int >( ) /
                100.f );
            const auto heal_threshold_percent = static_cast< float >( g_config->nami.w_heal_threshold->get< int >( ) )
                / 100.f;

            bool     allow_cast{ };
            unsigned target_nid{ };
            float    target_heal_weight{ };
            bool     found_heal_target{ };


            auto target_bounces{ 0 };
            auto cast_reason = 0;

            switch ( g_config->nami.w_bounce_logic->get< int >( ) ) {
            case 1:
                for ( auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) >
                        m_w_range )
                        continue;

                    auto heal_weight{ 1.f - ally->health / ally->max_health };

                    if ( ally->health <= ally->max_health * heal_threshold_percent &&
                        heal_weight > target_heal_weight ) {
                        allow_cast         = true;
                        target_nid         = ally->network_id;
                        target_heal_weight = heal_weight;
                        found_heal_target  = true;
                        cast_reason        = 1;
                        continue;
                    }

                    if ( found_heal_target ) continue;

                    auto ally_pred = g_features->prediction->predict_default( ally->index, 0.25f );
                    if ( !ally_pred ) continue;

                    auto ally_future_position = *ally_pred;

                    int most_bounces{ };

                    for ( auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || enemy->position.dist_to( ally->position ) > max_bounce_range ||
                            enemy->position.dist_to( ally_future_position ) > max_bounce_range ||
                            g_features->target_selector->is_bad_target( enemy->index ) )
                            continue;

                        auto bounces{ 1 };

                        for ( auto ally_hero : g_entity_list.get_allies( ) ) {
                            if ( !ally_hero || ally_hero->position.dist_to( enemy->position ) > max_bounce_range ||
                                ally_hero->network_id == ally->network_id ||
                                g_features->target_selector->is_bad_target( ally_hero->index ) )
                                continue;

                            ++bounces;
                            break;
                        }

                        if ( bounces <= most_bounces ) continue;

                        most_bounces = bounces;
                    }

                    if ( most_bounces <= target_bounces ) continue;

                    allow_cast     = true;
                    target_nid     = ally->network_id;
                    target_bounces = most_bounces;

                    cast_reason = 2;
                }

                if ( allow_cast && found_heal_target ) break;

                for ( auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                        m_w_range )
                        continue;

                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !pred ) continue;

                    auto future_position = *pred;

                    int most_bounces{ };

                    for ( auto ally : g_entity_list.get_allies( ) ) {
                        if ( !ally || ally->position.dist_to( enemy->position ) > max_bounce_range ||
                            ally->position.dist_to( future_position ) > max_bounce_range ||
                            g_features->target_selector->is_bad_target( ally->index ) )
                            continue;

                        auto bounces{ 1 };

                        auto ally_pred = g_features->prediction->predict_default( ally->index, 0.25f );
                        if ( !ally_pred ) continue;

                        auto predict_point{ ally_pred.value( ) };

                        for ( auto hero : g_entity_list.get_enemies( ) ) {
                            if ( !hero || hero->position.dist_to( ally->position ) > max_bounce_range ||
                                hero->position.dist_to( predict_point ) > max_bounce_range ||
                                hero->network_id == enemy->network_id ||
                                g_features->target_selector->is_bad_target( hero->index ) )
                                continue;

                            ++bounces;
                        }

                        if ( bounces < most_bounces ) continue;

                        most_bounces = bounces;
                    }

                    if ( most_bounces <= target_bounces ) continue;

                    allow_cast     = true;
                    target_nid     = enemy->network_id;
                    target_bounces = most_bounces;

                    cast_reason = 2;
                }
                break;
            case 0:
                for ( auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                        m_w_range )
                        continue;

                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !pred ) continue;

                    auto future_position = *pred;

                    int most_bounces{ };

                    for ( auto ally : g_entity_list.get_allies( ) ) {
                        if ( !ally || ally->position.dist_to( enemy->position ) > max_bounce_range ||
                            ally->position.dist_to( future_position ) > max_bounce_range ||
                            g_features->target_selector->is_bad_target( ally->index ) )
                            continue;

                        auto bounces{ 1 };

                        auto ally_pred = g_features->prediction->predict_default( ally->index, 0.25f );
                        if ( !ally_pred ) continue;

                        auto predict_point{ ally_pred.value( ) };

                        for ( auto hero : g_entity_list.get_enemies( ) ) {
                            if ( !hero || hero->position.dist_to( ally->position ) > max_bounce_range ||
                                hero->position.dist_to( predict_point ) > max_bounce_range ||
                                hero->network_id == enemy->network_id ||
                                g_features->target_selector->is_bad_target( hero->index ) )
                                continue;

                            ++bounces;
                            break;
                        }

                        if ( bounces < most_bounces ) continue;

                        most_bounces = bounces;
                    }

                    if ( most_bounces <= target_bounces ) continue;

                    allow_cast     = true;
                    target_nid     = enemy->network_id;
                    target_bounces = most_bounces;

                    cast_reason = 2;
                }

                for ( auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) >
                        m_w_range )
                        continue;

                    auto heal_weight{ 1.f - ally->health / ally->max_health };

                    if ( ally->health <= ally->max_health * heal_threshold_percent &&
                        heal_weight > target_heal_weight ) {
                        allow_cast         = true;
                        target_nid         = ally->network_id;
                        target_heal_weight = heal_weight;
                        found_heal_target  = true;
                        cast_reason        = 1;
                        continue;
                    }

                    if ( found_heal_target ) continue;

                    auto ally_pred = g_features->prediction->predict_default( ally->index, 0.25f );
                    if ( !ally_pred ) continue;

                    auto ally_future_position = *ally_pred;

                    int most_bounces{ };

                    for ( auto enemy : g_entity_list.get_enemies( ) ) {
                        if ( !enemy || enemy->position.dist_to( ally->position ) > max_bounce_range ||
                            enemy->position.dist_to( ally_future_position ) > max_bounce_range ||
                            g_features->target_selector->is_bad_target( enemy->index ) )
                            continue;

                        auto bounces{ 1 };

                        for ( auto ally_hero : g_entity_list.get_allies( ) ) {
                            if ( !ally_hero || ally_hero->position.dist_to( enemy->position ) > max_bounce_range ||
                                ally_hero->network_id == ally->network_id ||
                                g_features->target_selector->is_bad_target( ally_hero->index ) )
                                continue;

                            ++bounces;
                            break;
                        }

                        if ( bounces <= most_bounces ) continue;

                        most_bounces = bounces;
                    }

                    if ( most_bounces <= target_bounces ) continue;

                    allow_cast     = true;
                    target_nid     = ally->network_id;
                    target_bounces = most_bounces;

                    cast_reason = 2;
                }
                break;
            default:
                break;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::w, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Nami: W ] Reason: " << cast_reason << std::endl;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->nami.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            unsigned    target_nid{ };
            std::string target_name{ };
            bool        allow_cast{ };

            if ( g_config->nami.e_on_ally_autoattack->get< bool >( ) ) {
                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->dist_to_local( ) > m_e_range || ally->is_dead( ) || ally->is_invisible( ) ||
                        ally->get_selectable_flag( ) != 1 || !g_config->nami.e_on_self_autoattack->get< bool >( ) &&
                        ally->network_id == g_local->network_id )
                        continue;

                    auto sci = ally->spell_book.get_spell_cast_info( );
                    if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                        sci->server_cast_time < *g_time + g_features->orbwalker->get_ping( ) )
                        continue;

                    const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                    if ( !target || !target->is_hero( ) ) continue;

                    target_nid  = ally->network_id;
                    target_name = ally->champion_name.text;
                    allow_cast  = true;
                    break;
                }
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Nami ] E on autoattack, buff target: " << target_name << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->nami.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto multihit = get_multihit_position( m_r_range, 850.f, 260.f, 0.5f, true );
            if ( multihit.hit_count < g_config->nami.r_minimum_hitcount->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Nami ] R Multihit, count: " << multihit.hit_count << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->nami.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 0.f, 200.f, 0.95f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Nami ] Q Antigapclose: target " << target->champion_name.text << " | " << *g_time
                    << std::endl;
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->nami.q_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f || *g_time -
                m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 0.f, 200.f, 0.95f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Nami ] Q Auto-interrupt: target " << target->champion_name.text << " | " << *g_time <<
                    std::endl;
            }
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
        float m_last_cast_time{ };

        std::vector< float > m_q_damage{ 0.f, 80.f, 135.f, 190.f, 245.f, 300.f };
        std::vector< float > m_r_damage{ 0.f, 150.f, 225.f, 300.f };

        float m_q_range{ 850.f };
        float m_w_range{ 725.f };
        float m_e_range{ 800.f };
        float m_r_range{ 2750.f };
    };
} // namespace features::champion_modules
