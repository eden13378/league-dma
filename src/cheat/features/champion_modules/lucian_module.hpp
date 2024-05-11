#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../../sdk/game/pw_hud.hpp"
#include "../../sdk/game/hud_manager.hpp"

namespace features::champion_modules {
    class lucian_module final : public IModule {
    public:
        virtual ~lucian_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "lucian_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Lucian" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "lucian" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->lucian.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->lucian.q_harass );
            q_settings->checkbox( _( "only if can reset aa" ), g_config->lucian.q_aa_reset );
            q_settings->select(
                _( "hitchance" ),
                g_config->lucian.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->lucian.w_enabled );
            w_settings->checkbox( _( "only use after q" ), g_config->lucian.w_only_use_after_q );
            w_settings->checkbox( _( "only if can reset aa" ), g_config->lucian.w_aa_reset );
            w_settings->select(
                _( "hitchance" ),
                g_config->lucian.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->lucian.e_enabled );
            e_settings->checkbox( _( "use last in combo" ), g_config->lucian.e_last_in_combo );
            e_settings->checkbox( _( "use only to reset aa" ), g_config->lucian.e_only_reset_aa );

            drawings->checkbox( _( "draw q range" ), g_config->lucian.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->lucian.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->lucian.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->lucian.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->lucian.q_draw_range->get< bool >( ) &&
                !g_config->lucian.w_draw_range->get< bool >( ) &&
                !g_config->lucian.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->lucian.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lucian.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_max_range,
                        Renderer::outline,
                        60,
                        2.f
                    );
                }
            }

            if ( g_config->lucian.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lucian.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->lucian.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lucian.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( !m_passive_logged ) {
                if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "LucianPassiveBuff" ) ) && *g_time -
                    m_last_cast_time <= 0.8f )
                    return;

                m_passive_logged = true;
            }

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( )
                || g_features->buff_cache->get_buff( g_local->index, ct_hash( "LucianPassiveBuff" ) ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_e( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->lucian.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->lucian.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = get_target_q( );
            if ( !target ) return false;

            if ( target->is_hero( ) && g_config->lucian.q_aa_reset->get< bool >( ) && !g_features->orbwalker->
                should_reset_aa( ) )
                return false;

            auto cast_time = 0.4f - 0.15f / 17.f * static_cast< float >( ( m_slot_q->level - 1 ) );
            if ( cast_time < 0.25f ) cast_time = 0.25f;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                g_features->orbwalker->set_cast_time( cast_time );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->lucian.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->lucian.w_only_use_after_q->get< bool >( ) &&
                m_slot_q->is_ready( true ) ||
                g_config->lucian.w_aa_reset->get< bool >( ) &&
                !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            const auto damage    = get_spell_damage( ESpellSlot::w, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 120.f, 1.2f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lucian.w_hitchance->get< int >( ) )
                &&
                !will_kill ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 55.f ) &&
                !g_features->orbwalker->is_attackable( target->index )
            )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->lucian.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( g_config->lucian.e_last_in_combo->get< bool >( ) &&
                ( m_slot_q->is_ready( true ) || m_slot_w->is_ready( true ) ) ||
                g_config->lucian.e_only_reset_aa->get< bool >( ) && !g_features->orbwalker->should_reset_aa( )
            )
                return false;

            auto hud = g_pw_hud->get_hud_manager( );
            if ( !hud ) return false;
            const auto cursor_position{ hud->cursor_position_unclipped };

            if ( cast_spell( ESpellSlot::e, cursor_position ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                m_passive_logged = false;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto get_target_q( ) -> Object*{
            const auto target{ g_features->target_selector->get_default_target( ) };
            if ( !target ) return { };
            if ( g_features->orbwalker->is_attackable( target->index, m_q_range ) ) return target;

            auto cast_time = 0.4f - 0.15f / 17.f * static_cast< float >( ( m_slot_q->level - 1 ) );
            if ( cast_time < 0.25f ) cast_time = 0.25f;

            const auto damage    = get_spell_damage( ESpellSlot::q, target );
            const auto will_kill = damage > target->health + target->total_health_regen;

            const auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, cast_time );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lucian.q_hitchance->
                get< int >( ) ) && !will_kill )
                return { };

            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero ) continue;
                if ( target->network_id == hero->network_id ||
                    !g_features->orbwalker->is_attackable( hero->index, m_q_range ) ||
                    g_features->target_selector->is_bad_target( hero->index )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( hero->position, m_q_max_range ),
                    60.f
                );
                auto polygon = rect.to_polygon( 55 );

                // make a box around the minion
                // see if they overlap

                if ( polygon.is_inside( pred.position ) ) return hero;
            }

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_lane_minion( ) &&
                    !minion->is_jungle_monster( ) &&
                    !minion->is_misc_minion( ) ||
                    minion->is_ward( ) ||
                    g_features->orbwalker->is_ignored( minion->index ) ||
                    !g_features->orbwalker->is_attackable( minion->index, m_q_range )
                )
                    continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( minion->position, m_q_max_range ),
                    60.f
                );
                auto polygon = rect.to_polygon( 55 );

                // make a box around the minion
                // see if they overlap

                if ( polygon.is_inside( pred.position ) ) return minion;
            }

            return { };
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * m_q_bonus_ad_modifier[
                        get_slot_q( )->level ],
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->ability_power( ) * 0.9f,
                    target->index,
                    false
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

        std::array< float, 6 > m_q_damage            = { 0.f, 95.f, 130.f, 165.f, 200.f };
        std::array< float, 6 > m_q_bonus_ad_modifier = { 0.f, .6f, .75f, .9f, 1.05f, 1.2f };

        std::array< float, 6 > m_w_damage = { 0.f, 75.f, 110.f, 145.f, 180.f, 215.f };
        std::array< float, 6 > m_r_damage = { 0.f, 175.f, 250.f, 325.f };

        float m_q_range{ 500.f };
        float m_q_max_range{ 1000.f };

        float m_w_range{ 900.f };
        float m_e_range{ 425.f };
    };
}
