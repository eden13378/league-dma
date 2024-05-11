#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class kassadin_module final : public IModule {
    public:
        virtual ~kassadin_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "kassadin_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Kassadin" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "kassadin" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->kassadin.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->kassadin.q_enabled );
            q_settings->checkbox( _( "auto interrupt (?)" ), g_config->kassadin.q_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );

            w_settings->checkbox( _( "enable" ), g_config->kassadin.w_enabled );
            w_settings->checkbox( _( "use in laneclear" ), g_config->kassadin.w_laneclear );

            e_settings->checkbox( _( "enable" ), g_config->kassadin.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->kassadin.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->kassadin.r_enabled );
            r_settings->checkbox( _( "only out of aa range" ), g_config->kassadin.r_out_of_range );
            r_settings->select(
                _( "hitchance" ),
                g_config->kassadin.r_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->slider_int( _( "max stacks to use" ), g_config->kassadin.r_max_stacks, 1, 4, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->kassadin.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->kassadin.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->kassadin.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->kassadin.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->kassadin.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->kassadin.q_draw_range->get< bool >( ) &&
                !g_config->kassadin.w_draw_range->get< bool >( ) &&
                !g_config->kassadin.e_draw_range->get< bool >( ) &&
                !g_config->kassadin.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->kassadin.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kassadin.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
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

            if ( g_config->kassadin.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) && !m_w_active || !g_config->kassadin.
                    dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        m_w_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->kassadin.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) && m_e_active || !g_config->kassadin.
                    dont_draw_on_cooldown->get< bool >( ) ) ) {
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

            if ( g_config->kassadin.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->kassadin.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            autointerrupt_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            if ( !g_features->buff_cache->get_buff( g_local->index, ct_hash( "NetherBlade" ) ) ) {
                m_w_range  = g_local->attack_range + 65.f + 50.f;
                m_w_active = false;
            } else m_w_active = true;

            //if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "forcepulsecancast" ) ) ) m_e_active = true;
            //else m_e_active = false;

            m_e_active = true;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_r( );
                spell_e( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->kassadin.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->kassadin.w_laneclear->get< bool >( ) ) spell_w( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->kassadin.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                { },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target || g_features->prediction->windwall_in_line( g_local->position, target->position ) ) {
                return
                    false;
            }

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->kassadin.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) || m_w_active )
                return false;

            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                const auto target = g_features->target_selector->get_default_target( );
                if ( !target ) return false;

                if ( !g_features->orbwalker->should_reset_aa( ) || !g_features->orbwalker->is_attackable(
                    target->index,
                    m_w_range
                ) )
                    return false;
            } else if ( !g_features->orbwalker->should_reset_aa( ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->kassadin.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_e_active || !
                m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1400.f, 0.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kassadin.e_hitchance
                ->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->kassadin.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_r_range + 100.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); }
            );
            if ( !target || g_config->kassadin.r_out_of_range->get< bool >( ) && g_features->orbwalker->is_attackable(
                target->index
            ) )
                return false;

            const auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "RiftWalk" ) );
            if ( buff && buff->stacks( ) >= g_config->kassadin.r_max_stacks->get< int >( ) ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_r_range,
                0.f,
                75.f,
                0.25f,
                g_local->position,
                true
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->kassadin.r_hitchance
                ->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_r_time = *g_time;
                return true;
            }

            return false;
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->kassadin.q_autointerrupt->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target || g_features->prediction->windwall_in_line( g_local->position, target->position ) ) return;

            if ( cast_spell( ESpellSlot::q, target->network_id ) ) m_last_q_time = *g_time;
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

        bool m_w_active{ };
        bool m_e_active{ };

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 650.f };
        float m_w_range{ 0.f };
        float m_e_range{ 600.f };
        float m_r_range{ 500.f };
    };
}
