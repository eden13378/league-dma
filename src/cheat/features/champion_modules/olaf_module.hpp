#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class olaf_module final : public IModule {
    public:
        virtual ~olaf_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "olaf_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Olaf" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "olaf" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->olaf.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->olaf.q_harass );
            q_settings->checkbox( _( "use in jungleclear" ), g_config->olaf.q_jungleclear );
            q_settings->checkbox( _( "use in laneclear(?)" ), g_config->olaf.q_laneclear )->set_tooltip(
                _( "full laneclear only" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->olaf.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->olaf.w_enabled );
            w_settings->checkbox( _( "use in laneclear" ), g_config->olaf.w_laneclear );

            e_settings->checkbox( _( "enable" ), g_config->olaf.e_enabled );
            e_settings->checkbox( _( "weave between attacks" ), g_config->olaf.e_weave_in_aa );

            drawings->checkbox( _( "draw q range" ), g_config->olaf.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->olaf.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->olaf.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->olaf.q_draw_range->get< bool >( ) &&
                !g_config->olaf.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->olaf.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->olaf.dont_draw_on_cooldown->get<
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

            if ( g_config->olaf.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( ) || !g_config->olaf.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_local->is_dead( ) || g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_q( );
                spell_e( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->olaf.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->olaf.w_laneclear->get< bool >( ) ) spell_w( );
                if ( g_config->olaf.q_jungleclear->get< bool >( ) ) jungleclear_q( );
                if ( g_config->olaf.q_laneclear->get< bool >( ) && g_input->
                    is_key_pressed( utils::EKey::control ) )
                    laneclear_q( );
                if ( g_config->olaf.e_laneclear->get< bool >( ) && g_input->
                    is_key_pressed( utils::EKey::control ) )
                    laneclear_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->olaf.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 1600.f, 90.f, 0.25f, { }, true );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->olaf.q_hitchance->get<
                int >( ) )
                return false;

            auto cast_position = pred.position;
            if ( g_local->position.dist_to( pred.position ) > 325.f
                && g_local->position.dist_to( pred.position ) < 950.f ) {
                cast_position = g_local->position.extend(
                    pred.position,
                    g_local->position.dist_to( pred.position ) + 175.f
                );
            }

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->olaf.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time <= 0.4f ||
                !g_features->orbwalker->should_reset_aa( ) ||
                !m_slot_w->is_ready( true )
            )
                return false;

            if ( cast_spell( ESpellSlot::w ) ) {
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->olaf.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > m_e_range ) return false;


            if ( g_config->olaf.e_weave_in_aa->get< bool >( ) && g_features->orbwalker->is_attackable( target->index )
                && !g_features->orbwalker->should_reset_aa( ) &&
                g_features->orbwalker->get_next_possible_aa_time( ) <= *g_time + 0.25f )
                return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
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

        auto laneclear_q( ) -> void{
            if ( *g_time - m_last_e_time <= 0.5f || !m_slot_e->is_ready( true ) ||
                !g_features->orbwalker->should_reset_aa( ) )
                return;

            const auto            laneclear_data = get_line_laneclear_target(
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                90.f,
                false,
                false
            );

            if ( !laneclear_data ) return;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }
        }

        auto laneclear_e( ) -> void{
            if ( *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) ) return;

            const Object* minion_to_attack{ };

            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || !minion->is_alive( ) || !minion->is_targetable( ) || minion->dist_to_local( ) >
                    325 )
                    continue;

                minion_to_attack = minion;

                if ( cast_spell( ESpellSlot::e, minion_to_attack->network_id ) ) {
                    m_last_e_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return;
                }
            }
            /*if ( !minion_to_attack ) return;

            if ( cast_spell( ESpellSlot::e, minion_to_attack->network_id ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
            }*/
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

        std::array< float, 6 > m_q_damage = { 0.f, 70.f, 120.f, 170.f, 220.f, 270.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 1000.f };
        float m_w_range{ 0.f };
        float m_e_range{ 325.f };
        float m_r_range{ 0.f };
    };
}
