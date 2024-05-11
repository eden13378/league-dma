#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class illaoi_module final : public IModule {
    public:
        virtual ~illaoi_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "illaoi_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Illaoi" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "illaoi" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->illaoi.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->illaoi.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->illaoi.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max range %" ), g_config->illaoi.q_max_range, 75, 100, 1 );

            w_settings->checkbox( _( "enable" ), g_config->illaoi.w_enabled );
            w_settings->checkbox( _( "only to reset aa" ), g_config->illaoi.w_aa_reset );

            e_settings->checkbox( _( "enable" ), g_config->illaoi.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->illaoi.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max range %" ), g_config->illaoi.e_max_range, 75, 100, 1 );

            r_settings->checkbox( _( "enable" ), g_config->illaoi.r_enabled );
            r_settings->slider_int( "min targets", g_config->illaoi.r_min_targets, 1, 5, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->illaoi.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->illaoi.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->illaoi.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->illaoi.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->illaoi.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->illaoi.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->illaoi.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
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

                spell_r( );
                spell_e( );
                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->illaoi.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->illaoi.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict(
                target->index,
                m_q_range * ( g_config->illaoi.q_max_range->get< int >( ) / 100.f ),
                0.f,
                100.f,
                0.75f,
                { },
                true
            );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->illaoi.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->illaoi.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            if ( g_config->illaoi.w_aa_reset->get< bool >( )
                && ( !g_features->orbwalker->should_reset_aa( ) || g_features->orbwalker->get_last_target( ) !=
                    ETargetType::hero ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->
                                         is_attackable( target->index, g_local->attack_range + 225.f ) )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->illaoi.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_e_range * ( g_config->illaoi.e_max_range->get< int >( ) / 100.f ),
                    1900.f,
                    50.f,
                    0.25f,
                    { },
                    true
                );

            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->illaoi.e_hitchance->get< int >( )
                || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    60.f,
                    0.25f,
                    1900.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->illaoi.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f || *g_time -
                m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            int hit_count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy ||
                    enemy->is_dead( ) ||
                    enemy->is_invisible( ) ||
                    enemy->dist_to_local( ) > 600.f ||
                    g_features->target_selector->is_bad_target( enemy->index )
                )
                    continue;

                auto pred = g_features->prediction->predict_default( enemy->index, 0.5f );
                if ( !pred || g_local->position.dist_to( *pred ) > 475.f ) continue;


                ++hit_count;
            }

            if ( hit_count < g_config->illaoi.r_min_targets->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
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
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
                return 0.25f;
            case ESpellSlot::r:
                return 0.25f;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 850.f };
        float m_w_range{ 0.f };
        float m_e_range{ 950.f };
        float m_r_range{ 500.f };
    };
}
