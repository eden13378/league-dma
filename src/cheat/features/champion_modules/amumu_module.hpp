#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class amumu_module final : public IModule {
    public:
        virtual ~amumu_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "amumu_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Amumu" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation  = g_window->push( _( "amumu" ), menu_order::champion_module );
            const auto q_settings  = navigation->add_section( _( "q settings" ) );
            const auto drawings    = navigation->add_section( _( "drawings" ) );
            const auto w_settings  = navigation->add_section( _( "w settings" ) );
            const auto e_settings  = navigation->add_section( _( "e settings" ) );
            const auto r_settings  = navigation->add_section( _( "r settings" ) );
            const auto jungleclear = navigation->add_section( _( "jungleclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->amumu.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->amumu.q_hitchance,
                { ( "Fast" ), ( "Medium" ), ( "High" ), ( "Very high" ), ( "Immobile" ) }
            );
            q_settings->checkbox( _( "enable autointerrupt" ), g_config->amumu.q_autointerrupt );
            q_settings->checkbox( _( "enable antigapclose" ), g_config->amumu.q_antigapclose );
            q_settings->checkbox( _( "enable killsteal" ), g_config->amumu.q_killsteal );

            w_settings->checkbox( _( "enable" ), g_config->amumu.w_enabled );
            w_settings->slider_int( _( "min enemies" ), g_config->amumu.w_min_enemy, 1, 5, 1 );
            w_settings->checkbox( _( "disable w below min mana" ), g_config->amumu.w_disable_under_min_mana );
            w_settings->slider_int( _( "min mana" ), g_config->amumu.w_min_mana, 5, 20, 1 );

            e_settings->checkbox( _( "enable" ), g_config->amumu.e_enabled );

            r_settings->checkbox( _( "enable" ), g_config->amumu.r_enabled );
            r_settings->checkbox( _( "ignore min count in full combo" ), g_config->amumu.r_ignore_multi_FC );
            r_settings->slider_int( _( "min enemies" ), g_config->amumu.r_min_enemy, 2, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->amumu.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->amumu.dont_draw_on_cooldown );

            jungleclear->checkbox( _( "enable w" ), g_config->amumu.w_jungleclear );
            jungleclear->slider_int( _( "min monsters" ), g_config->amumu.w_min_jungle, 1, 5, 1 );
            jungleclear->checkbox( _( "enable e" ), g_config->amumu.e_jungleclear );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->amumu.q_draw_range->get< bool >( ) ||
                g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->amumu.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->amumu.dont_draw_on_cooldown->get< bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            m_w_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "AuraofDespair" ) );

            killsteal_q( );
            antigapclose_q( );
            autointerrupt_q( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:
                spell_q( );
                spell_e( );
                spell_w( );
                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->amumu.w_jungleclear->get< bool >( ) && jungleclear_w( ) ) return;
                if ( g_config->amumu.e_jungleclear->get< bool >( ) && jungleclear_e( ) ) return;
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->amumu.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.3f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                80.f,
                0.25f,
                { },
                true
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->amumu.q_hitchance->
                get< int >( ) ) )
                return false;

            if ( g_features->prediction->minion_in_line( g_local->position, pred.position, 80.f ) ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_cast_time = *g_time;
                m_last_q_time    = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->amumu.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f )
                return false;

            const auto mana = g_local->mana / g_local->max_mana * 100;

            if ( g_config->amumu.w_min_mana->get< int >( ) > mana ) {
                if ( g_config->amumu.w_disable_under_min_mana->get< bool >( ) && m_w_active ) {
                    if ( cast_spell( ESpellSlot::w ) ) {
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->set_cast_time( 0.0f );
                        return true;
                    }

                    return false;
                }
            }

            const auto min_enemy = g_config->amumu.w_min_enemy->get< int >( );

            auto enemy_count = 0;
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->dist_to_local( ) > m_w_range ) continue;

                enemy_count++;
            }

            if ( enemy_count >= min_enemy && !m_w_active ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.0f );
                    return true;
                }
            }

            if ( ( enemy_count < min_enemy || enemy_count == 0 ) && m_w_active ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.0f );
                    return true;
                }
            }
            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->amumu.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( m_e_range );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( pred.value( ) ) >= m_e_range ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->amumu.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f || !m_slot_r->is_ready( true ) )
                return false;

            int enemy_count{ };

            if ( GetAsyncKeyState( VK_CONTROL ) && g_config->amumu.r_ignore_multi_FC->get< bool >( ) ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target || !target->is_targetable( ) ) return false;

                const auto predict = g_features->prediction->predict_default( target->index, 0.25f );
                if ( !predict || predict.value( ).dist_to( g_local->position ) > m_r_range ) return false;

                if ( cast_spell( ESpellSlot::r ) ) {
                    m_last_r_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );

                    return true;
                }
            }

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

            if ( enemy_count < g_config->amumu.r_min_enemy->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );

                return true;
            }

            return false;
        }

        auto jungleclear_w( ) -> bool{
            if ( !m_slot_w->is_ready( true ) || *g_time - m_last_w_time <= 0.5f ) return false;

            int hit_count{ };
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_w_range
                )
                    continue;

                hit_count++;
            }

            const auto should_w = hit_count < g_config->amumu.w_min_jungle->get< int >( ) && m_w_active || hit_count >=
                g_config->amumu.w_min_jungle->get< int >( ) && !m_w_active;
            if ( !should_w ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto jungleclear_e( ) -> bool{
            if ( !g_config->amumu.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.3f ||
                *g_time - m_last_cast_time <= 0.3f || !m_slot_e->is_ready( true ) )
                return false;

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || minion->is_dead( ) || minion->is_invisible( ) || !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_e_range )
                    continue;

                if ( cast_spell( ESpellSlot::e ) ) {
                    m_last_e_time    = *g_time;
                    m_last_cast_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return true;
                }
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->amumu.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.3f ||
                !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, 1200.f, 2000.f, 60.f, 0.25f, { }, true );

            if ( !predicted.valid ||
                g_features->prediction->minion_in_line( g_local->position, predicted.position, 60.f ) )
                return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ KS Q ] Order cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->amumu.q_antigapclose->get< bool >( ) || *g_time - m_last_cast_time <= 0.2f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 2000.f, 80.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                2000.f,
                80.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minions_in_line(
                    g_local->position,
                    pred.position,
                    80.f,
                    0,
                    0.25f,
                    0.25f + g_local->position.dist_to( pred.position ) / 2000.f
                ) > 1 )
                return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->amumu.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_q_range, 2000.f, 80.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.85f,
                    target->index,
                    false
                );
            default: ;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            default: ;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };

        float m_last_w_time{ };
        bool  m_w_active{ };

        float m_last_e_time{ };


        float m_last_r_time{ };

        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 70.f, 95.f, 120.f, 145.f, 170.f };

        float m_q_range{ 1100.f };
        float m_w_range{ 350.f };
        float m_e_range{ 350.f };
        float m_r_range{ 550.f };

        ESpellSlot ward_slot{ };
        float      m_last_ward_time{ };
        Vec3       m_last_ward_pos{ };
    };
} // namespace features::champion_modules
