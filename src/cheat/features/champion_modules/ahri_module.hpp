#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../menu/menu.hpp"

namespace features::champion_modules {
    class ahri_module final : public IModule {
    public:
        virtual ~ahri_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "ahri_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Ahri" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "ahri" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->ahri.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->ahri.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->ahri.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->ahri.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->ahri.e_enabled );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->ahri.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "antigapclose" ), g_config->ahri.e_antigapclose );
            e_settings->select(
                _( "hitchance" ),
                g_config->ahri.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            drawings->checkbox( _( "draw q range" ), g_config->ahri.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->ahri.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->ahri.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ahri.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->ahri.q_draw_range->get< bool >( ) &&
                !g_config->ahri.w_draw_range->get< bool >( ) &&
                !g_config->ahri.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->ahri.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ahri.dont_draw_on_cooldown->get<
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

            if ( g_config->ahri.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ahri.dont_draw_on_cooldown->get<
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

            if ( g_config->ahri.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->ahri.dont_draw_on_cooldown->get<
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->ahri.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->ahri.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1550.f, 100.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ahri.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->ahri.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                650.f,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::w, unit ); }
            );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->ahri.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1550.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->ahri.e_hitchance->get
                    < int >( ) )
                || g_features->prediction->minion_in_line_predicted(
                    g_local->position,
                    pred.position,
                    60.f,
                    0.25f,
                    1550.f
                ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto antigapclose_e( ) -> void{
            if ( !g_config->ahri.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1550.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );

            if ( !pred.valid || ( int )pred.hitchance < 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                60.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->ahri.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1550.f,
                60.f,
                0.25f,
                { },
                true,
                Prediction::include_ping,
                Prediction::ESpellType::linear
            );
            if ( !pred.valid || ( int )pred.hitchance <= 3 || g_features->prediction->minion_in_line(
                g_local->position,
                pred.position,
                60.f
            ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            }
            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1550.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1550.f;
            }
            case ESpellSlot::w:
                return 0.25f;
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

        std::array< float, 6 > m_q_damage = { 0.f, 40.f, 65.f, 90.f, 115.f, 140.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        float m_q_range{ 900.f };
        float m_w_range{ 725.f };
        float m_e_range{ 1000.f };
    };
}
