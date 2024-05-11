#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class soraka_module final : public IModule {
    public:
        virtual ~soraka_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "soraka_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Soraka" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell, e_spell, r_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "soraka" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->soraka.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->soraka.q_harass );
            q_settings->checkbox( _( "antigapclose" ), g_config->soraka.q_antigapclose );
            q_settings->select(
                _( "hitchance" ),
                g_config->soraka.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->soraka.w_enabled );
            w_settings->slider_int( _( "min local %hp" ), g_config->soraka.w_min_hp, 1, 100, 1 );
            w_settings->slider_int( _( "min ally %hp" ), g_config->soraka.w_min_hp_ally, 1, 100, 1 );

            e_settings->checkbox( _( "enable" ), g_config->soraka.e_enabled );
            e_settings->checkbox( _( "use in harass" ), g_config->soraka.e_harass );
            e_settings->checkbox( _( "antigapclose" ), g_config->soraka.e_antigapclose );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->soraka.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->select(
                _( "hitchance" ),
                g_config->soraka.e_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->soraka.r_enabled );
            r_settings->slider_int( _( "min ally %hp" ), g_config->soraka.r_min_hp, 1, 100, 1 );
            r_settings->slider_int( _( "min ally in danger" ), g_config->soraka.r_min_ally_danger, 1, 5, 1 );
            r_settings->checkbox( _( "danger count override (?)" ), g_config->soraka.r_danger_count_override )->
                        set_tooltip( _( "Will ignore minimum danger count if any Ally is below 5% hp" ) );

            drawings->checkbox( _( "draw q range" ), g_config->soraka.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->soraka.e_draw_range );
            drawings->checkbox( _( "display r counter" ), g_config->soraka.r_draw_counter );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->soraka.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->soraka.q_draw_range->get< bool >( ) &&
                !g_config->soraka.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->soraka.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->soraka.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
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

            if ( g_config->soraka.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->soraka.dont_draw_on_cooldown->get
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

            if ( !g_config->soraka.r_draw_counter->get< bool >( ) ) return;

            Vec2 sp{ };
            if ( !world_to_screen( g_local->position, sp ) ) return;

            const auto text            = std::format( ( "Allies in danger : {}" ), m_r_potential_heals );
            const auto text_size_heals = static_cast< float >( 16 + ( 8 * m_r_potential_heals > 1
                                                                          ? m_r_potential_heals
                                                                          : 0 ) );
            const auto text_size = g_render->get_text_size( text, g_fonts->get_block( ), text_size_heals );

            g_render->text_shadow(
                { sp.x - ( text_size.x / 4.f ), sp.y + text_size.y },
                m_r_potential_heals > 0 ? Color( 25, 255, 25, 220 ) : Color( 255, 25, 25, 220 ),
                g_fonts->get_default_bold( ),
                text.c_str( ),
                text_size_heals
            );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );
            update_r_indicator( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_q( );

            antigapclose_e( );
            autointerrupt_e( );

            spell_r( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_e( );
                spell_q( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( !g_config->soraka.q_harass->get< bool >( ) ) return;
                spell_q( );
                if ( !g_config->soraka.e_harass->get< bool >( ) ) return;
                spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->soraka.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 1250.f, 133.f, 0.25f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->soraka.q_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->soraka.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) ||
                g_local->health / g_local->max_health * 100 < g_config->soraka.w_min_hp->get< int >( ) )
                return false;

            const Object* ally_to_heal   = { };
            const Object* lowest_hp_ally = { };
            const auto    lowest_ally_hp = 101.f;
            float         ally_hp_percent;
            const auto    has_buff = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "sorakaqregen" ) );
            if ( has_buff ) {
                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || ally->dist_to_local( ) > m_w_range || ally->is_local( ) || !ally->
                        is_targetable( ) || ally->health == ally->max_health )
                        continue;

                    ally_hp_percent = ally->health / ally->max_health * 100;
                    if ( ally_hp_percent < lowest_ally_hp ) lowest_hp_ally = ally;
                }
                ally_to_heal = lowest_hp_ally;
            } else {
                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally ||
                        ally->is_dead( ) ||
                        ally->dist_to_local( ) > m_w_range ||
                        ally->is_local( ) )
                        continue;

                    ally_hp_percent = ally->health / ally->max_health * 100;
                    if ( ally_hp_percent < g_config->soraka.w_min_hp_ally->get< int >( ) ) {
                        ally_to_heal = ally;
                        break;
                    }
                }
            }


            if ( !ally_to_heal ) return false;

            if ( cast_spell( ESpellSlot::w, ally_to_heal->network_id ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_w_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->soraka.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_e_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); }
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 0.f, 130.f, 0.25f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->soraka.e_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->set_cast_time( 0.25f );
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->soraka.r_enabled->get< bool >( ) || *g_time - m_last_r_time <= 0.4f ||
                !m_slot_r->is_ready( ) )
                return false;

            if ( g_config->soraka.r_danger_count_override->get< bool >( ) && m_r_critical_danger > 0 ) {
                if ( cast_spell( ESpellSlot::r ) ) {
                    g_features->orbwalker->set_cast_time( 0.25f );
                    m_last_r_time = *g_time;
                    return true;
                }
            }

            if ( m_r_potential_heals > g_config->soraka.r_min_ally_danger->get< int >( ) ) {
                if ( cast_spell( ESpellSlot::r ) ) {
                    g_features->orbwalker->set_cast_time( 0.25f );
                    m_last_r_time = *g_time;
                    return true;
                }
            }
            return false;
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->soraka.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_e_range, 0.f, 130.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->soraka.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred =
                g_features->prediction->predict( target->index, m_e_range, 0.f, 130.f, 0.25f, { }, true );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->soraka.q_antigapclose->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_antigapclose_target( );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1250.f,
                133.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ||
                g_features->prediction->minion_in_line( g_local->position, pred.position, 60.f ) )
                return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_q_time = *g_time;
        }

        auto update_r_indicator( ) -> void{
            if ( !g_config->soraka.r_draw_counter->get< bool >( ) ) return;

            auto danger_counter  = 0;
            auto critical_danger = 0;
            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally ||
                    ally->is_dead( ) ||
                    ally->health / ally->max_health * 100 > g_config->soraka.r_min_hp->get< int >( ) )
                    continue;

                danger_counter++;
                if ( ally->health / ally->max_health * 100 < 5.0f ) critical_danger++;
            }

            m_r_potential_heals = danger_counter;
            m_r_critical_danger = critical_danger;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.35f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.4f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1250.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1250.f;
            }
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position );
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) );
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

        std::array< float, 6 > m_q_damage = { 0.f, 85.f, 120.f, 155.f, 190.f, 225.f };
        std::array< float, 6 > m_e_damage = { 0.f, 70.f, 95.f, 120.f, 145.f, 170.f };

        float m_q_range{ 800.f };
        float m_w_range{ 550.f };
        float m_e_range{ 925.f };

        int m_r_potential_heals{ };
        int m_r_critical_danger{ };
    };
}
