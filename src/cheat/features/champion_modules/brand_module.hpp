#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class brand_module final : public IModule {
    public:
        virtual ~brand_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "brand_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Brand" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, w_spell, q_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "brand" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->brand.q_enabled );
            q_settings->checkbox( _( "only if stunnable" ), g_config->brand.q_only_ablaze );
            q_settings->select(
                _( "hitchance" ),
                g_config->brand.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->brand.w_enabled );
            w_settings->checkbox( _( "multihit" ), g_config->brand.w_multihit );
            w_settings->select(
                _( "hitchance" ),
                g_config->brand.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->brand.e_enabled );

            r_settings->checkbox( _( "enable" ), g_config->brand.r_enabled );
            r_settings->slider_int( _( "min bounce targets" ), g_config->brand.r_min_enemy, 0, 4, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->brand.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->brand.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->brand.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->brand.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->brand.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->brand.q_draw_range->get< bool >( ) &&
                !g_config->brand.w_draw_range->get< bool >( ) &&
                !g_config->brand.e_draw_range->get< bool >( ) &&
                !g_config->brand.r_draw_range->get< bool >( )
                || g_local->is_dead( ) )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->brand.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->brand.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->brand.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->brand.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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

            if ( g_config->brand.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->brand.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->brand.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->brand.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        800.f,
                        Renderer::outline,
                        80,
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
                spell_r( );
                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                spell_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->brand.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || g_config->brand.q_only_ablaze->get< bool >( ) && !g_features->buff_cache->get_buff(
                target->index,
                ct_hash( "BrandAblaze" )
            ) )
                return false;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict(
                target->index,
                1040.f,
                1600.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->brand.q_hitchance->
                    get< int >( ) ) && !will_kill
                || g_features->prediction->minion_in_line( g_local->position, pred.position, 70.f ) )
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
            if ( !g_config->brand.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_w->is_ready( true ) )
                return false;

            if ( g_config->brand.w_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_w_range, 0.f, 250.f, 1.f, false );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::w, multihit.position ) ) {
                        g_features->orbwalker->on_cast( );
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_w_range,
                0.f,
                260.f,
                0.95f,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            ); // 0.875 raw windup

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->brand.w_hitchance->
                get< int >( ) ) && !will_kill )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->brand.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.5f || *g_time -
                m_last_cast_time <= 0.2f
                || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_e_range ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->brand.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) >= m_r_range ) return false;

            int count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->position.dist_to(
                        target->position
                    ) >= 650.f || enemy->network_id == target->network_id
                    || g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                if ( pred && target->position.dist_to( *pred ) > 575.f ) continue;

                ++count;
            }

            if ( count < g_config->brand.r_min_enemy->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        static auto get_target( ESpellSlot slot, float range ) -> Object*{ return { }; }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            constexpr auto q_ap_ratio = 0.85f;
            constexpr auto w_ap_ratio = 0.55f;
            constexpr auto e_ap_ratio = 0.45f;
            constexpr auto r_ap_ratio = 0.25f;

            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * q_ap_ratio,
                    target->index,
                    false
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->ability_power( ) * w_ap_ratio,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * e_ap_ratio,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * r_ap_ratio,
                    target->index,
                    false
                );
            }

            return 0.f;
        }

    private:
        float                  m_q_range{ 1040.f };
        float                  m_q_radius{ 60.f };
        float                  m_q_cast_time{ 0.25f };
        float                  m_q_speed{ 1600.f };
        float                  m_last_q_time{ };
        std::array< float, 6 > m_q_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        float                  m_w_range{ 900.f };
        float                  m_w_radius{ 130.f };
        float                  m_w_cast_time{ 0.25f };
        float                  m_w_damage_delay{ 0.625f };
        float                  m_last_w_time{ };
        std::array< float, 6 > m_w_damage = { 0.f, 75.f, 120.f, 165.f, 210.f, 255.f };

        float                  m_e_range{ 675.f };
        float                  m_e_cast_time{ 0.25f };
        float                  m_last_e_time{ };
        std::array< float, 6 > m_e_damage = { 0.f, 70.f, 95.f, 120.f, 145.f, 170.f };

        float                  m_r_range{ 725.f };
        float                  m_r_cast_time{ 0.25f };
        float                  m_last_r_time{ };
        std::array< float, 6 > m_r_damage = { 0.f, 100.f, 200.f, 300.f };

        float m_last_cast_time{ };
    };
}
