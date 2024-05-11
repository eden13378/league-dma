#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class chogath_module final : public IModule {
    public:
        virtual ~chogath_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "chogath_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Chogath" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "chogath" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->chogath.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->chogath.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->chogath.q_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->chogath.w_enabled );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->chogath.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->select(
                _( "hitchance" ),
                g_config->chogath.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->chogath.e_enabled );

            //r_settings->checkbox( _( "enable" ), g_config->chogath.r_enabled );
            //r_settings->checkbox( _( "chomp drags and baron" ), g_config->chogath.r_objs );

            drawings->checkbox( _( "draw q range" ), g_config->chogath.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->chogath.w_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->chogath.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->chogath.q_draw_range->get< bool >( ) &&
                !g_config->chogath.w_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->chogath.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->chogath.dont_draw_on_cooldown->
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

            if ( g_config->chogath.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->chogath.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
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
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            autointerrupt_w( );
            //r_monsters( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_w( );
            //spell_r( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->chogath.q_harass->get< bool >( ) ) spell_q( );
                break;
            default: ;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->chogath.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 250.f, 1.25f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->chogath.q_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->chogath.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.5f );
            if ( !pred.valid || static_cast< int >( pred.hitchance ) < g_config->chogath.w_hitchance->get<
                int >( ) )
                return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->chogath.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || m_slot_e->charges >
                0 || !m_slot_e->is_ready( true ) )
                return false;
            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || !g_features->orbwalker->is_attackable( target->index ) ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->chogath.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, m_w_range, 0.f, 0.f, 0.5f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) m_last_w_time = *g_time;
        }

        /*auto r_monsters( ) -> boolean {
            if ( !g_config->activator.smite_toggle->get< bool >( ) ) return false;

            Object *target{ };
            bool    found{ };

            for ( const auto obj : g_entity_list->get_enemy_minions( ) ) {

                if ( !obj || obj->is_dead( ) || obj->dist_to_local( ) > 550.f + obj->get_bounding_radius( ) ||
                     obj->is_invisible( ) || !obj->is_jungle_monster( ) )
                    continue;

                auto name_hash = rt_hash( obj->get_name( ).c_str( ) );

                found = name_hash == ct_hash( "SRU_RiftHerald" ) || name_hash == ct_hash( "SRU_Baron" ) ||
                    name_hash == ct_hash( "SRU_Dragon_Earth" ) || name_hash == ct_hash( "SRU_Dragon_Air" ) ||
                    name_hash == ct_hash( "SRU_Dragon_Water" ) || name_hash == ct_hash( "SRU_Dragon_Fire" ) ||
                    name_hash == ct_hash( "SRU_Dragon_Hextech" ) || name_hash == ct_hash( "SRU_Dragon_Chemtech" ) ||
                    name_hash == ct_hash( "SRU_Dragon_Elder" ) || g_config->activator.smite_buffs->get< bool >( );


                if ( !found ) continue;

                target = obj;
                break;
            }

            if ( !target ) return false;

            float damage = 1200.f + g_local->ability_power(  ) * 0.5f + g_local->health
        }*/

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

        std::array< float, 6 > m_q_damage = { 0.f, 65.f, 95.f, 125.f, 155.f, 185.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 105.f, 130.f, 155.f, 180.f };
        std::array< float, 6 > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        float m_q_range{ 950.f };
        float m_w_range{ 650.f };
        float m_e_range{ 0.f };
        float m_r_range{ 0.f };
    };
}
