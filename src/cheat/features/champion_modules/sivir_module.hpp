#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class sivir_module final : public IModule {
    public:
        virtual ~sivir_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "sivir_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Sivir" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "sivir" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->sivir.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->sivir.q_enabled );
            q_settings->select(
                _( "hitchance" ),
                g_config->sivir.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->slider_int( _( "max minions hit" ), g_config->sivir.q_max_minions, 0, 4, 1 );

            w_settings->checkbox( _( "enable" ), g_config->sivir.w_enabled );
            w_settings->checkbox( _( "only if can reset aa" ), g_config->sivir.w_aa_reset );

            e_settings->checkbox( _( "enable" ), g_config->sivir.e_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->sivir.q_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->sivir.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->sivir.q_draw_range->get< bool >( ) || g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->sivir.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->sivir.dont_draw_on_cooldown->get<
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

            //  if(found_buff) g_render->circle_3d(g_local->position, color(255, 50, 50, 255), 50.f, c_renderer::outline, 50, 2.f);
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            spell_e( );


            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) || g_features->evade->
                is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->sivir.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1450.f, 90.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->sivir.q_hitchance->get<
                int >( ) )
                return false;

            if ( g_config->sivir.q_max_minions->get< int >( ) < 4 &&
                g_features->prediction->count_minions_in_line( g_local->position, pred.position, 90.f ) > g_config->
                sivir.q_max_minions->get< int >( )
            )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->sivir.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                g_config->sivir.w_aa_reset->get< bool >( ) && !g_features->orbwalker->should_reset_aa( ) || !m_slot_w->
                is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->sivir.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto allow_e{ should_spellshield_ally( g_local.get( ) ) };

            if ( !allow_e ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto jungleclear_q( ) -> void{
            if ( *g_time - m_last_q_time <= 0.5f || !m_slot_q->is_ready( true ) || !g_features->orbwalker->
                should_reset_aa( ) )
                return;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                90.f,
                false,
                true
            );

            if ( !laneclear_data ) return;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ),
                    target->index,
                    true
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
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1600.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1600.f;
            }
            case ESpellSlot::r:
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

        bool found_buff{ };

        std::array< float, 6 > m_q_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 1250.f };
        float m_w_range{ 0.f };
        float m_e_range{ 0.f };
        float m_r_range{ 0.f };
    };
}
