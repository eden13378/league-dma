#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class belveth_module final : public IModule {
    public:
        virtual ~belveth_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "belveth_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Belveth" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "belveth" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );

            q_settings->checkbox( _( "enable" ), g_config->belveth.q_enabled );
            q_settings->checkbox( _( "use in harass" ), g_config->belveth.q_harass );
            q_settings->select(
                _( "mode" ),
                g_config->belveth.q_mode,
                { _( "Gapclose" ), _( "Gapclose + Stack passive" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->belveth.w_enabled );
            w_settings->checkbox( _( "auto interrupt (?)" ), g_config->belveth.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->checkbox( _( "only out of aa range" ), g_config->belveth.w_only_out_of_range );
            w_settings->select(
                _( "hitchance" ),
                g_config->belveth.w_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->belveth.e_enabled );
            e_settings->checkbox( _( "dont use under turret" ), g_config->belveth.e_disable_under_turret );
            e_settings->slider_int( _( "enemy min health %" ), g_config->belveth.e_min_health_percent, 20, 80, 1 );

            r_settings->checkbox( _( "enable" ), g_config->belveth.r_enabled );
            r_settings->checkbox( _( "only if not already in r" ), g_config->belveth.r_disable_if_buffed );

            spellclear->checkbox( _( "jungleclear q" ), g_config->belveth.q_laneclear );
            spellclear->checkbox( _( "jungleclear w" ), g_config->belveth.w_laneclear );

            //r_disable_if_buffed
            //e_disable_under_turret
            drawings->checkbox( _( "draw q range" ), g_config->belveth.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->belveth.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->belveth.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->belveth.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->belveth.q_draw_range->get< bool >( ) &&
                !g_config->belveth.w_draw_range->get< bool >( ) &&
                !g_config->belveth.e_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->belveth.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->belveth.dont_draw_on_cooldown->
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

            if ( g_config->belveth.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->belveth.dont_draw_on_cooldown->
                    get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 132, 245, 66, 255 ),
                        700.f,
                        Renderer::outline,
                        50,
                        2.f
                    );
                }
            }

            if ( g_config->belveth.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) && !m_e_active || !g_config->belveth.
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

            // g_render->circle_3d(g_local->position, color(255, 66, 50, 255), 340.f, c_renderer::outline, 50, 2.f);

            return;

            auto value{ 50.f };
            auto north_east{ g_local->position + Vec3( 50.f, 0, 50.f ) };
            auto north_west{ g_local->position + Vec3( -50.f, 0, 50.f ) };
            auto south_east{ g_local->position + Vec3( 50.f, 0, -50.f ) };
            auto south_west{ g_local->position + Vec3( -50.f, 0, -50.f ) };

            //North west = 1 |
            // North east = 2 |
            // South east = 4 |
            // South west = 8 |
            if ( m_flag == 0 ) {
                g_render->circle_3d( north_west, Color( 255, 255, 255 ), 30, 2 );
                g_render->circle_3d( north_east, Color( 255, 255, 255 ), 30, 2 );
                g_render->circle_3d( south_east, Color( 255, 255, 255 ), 30, 2 );
                g_render->circle_3d( south_west, Color( 255, 255, 255 ), 30, 2 );
            } else {
                if ( m_flag & 1 ) g_render->circle_3d( north_west, Color( 255, 255, 255 ), 30, 2 );
                if ( m_flag & 2 ) g_render->circle_3d( north_east, Color( 255, 255, 255 ), 30, 2 );
                if ( m_flag & 4 ) g_render->circle_3d( south_east, Color( 255, 255, 255 ), 30, 2 );
                if ( m_flag & 8 ) g_render->circle_3d( south_west, Color( 255, 255, 255 ), 30, 2 );
            }

            // 14, 6, 2, 1
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            m_e_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "BelvethE" ) );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            autointerrupt_w( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            m_flag     = m_slot_q->get_usable_state( ) == 14 ? 0 : m_slot_q->get_usable_state( ) + 1;
            m_r_active = !!g_features->buff_cache->get_buff( g_local->index, ct_hash( "BelvethRSteroid" ) );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_q( );
                spell_e( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->belveth.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                laneclear_q( );
                laneclear_w( );
                break;
            default:
                break;
            }

            // BelvethSpore
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->belveth.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || !m_slot_q->
                is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                g_config->belveth.q_mode->get< int >( ) == 0 &&
                g_features->orbwalker->is_attackable( target->index ) ||
                g_config->belveth.q_mode->get< int >( ) == 1 &&
                g_features->orbwalker->is_attackable( target->index ) &&
                g_local->mana > 0
            )
                return false;

            if ( m_e_active && target->dist_to_local( ) < 375.f ) return false;

            const auto raw_pred = g_features->prediction->predict(
                target->index,
                m_q_range + 200.f,
                m_q_speed[ m_slot_q->level ],
                0.f,
                0.f
            );
            if ( !raw_pred.valid ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range + 200.f,
                m_q_speed[ m_slot_q->level ],
                100.f,
                0.f
            );
            if ( !pred.valid ) return false;

            const auto dash_position = g_local->position.extend( pred.position, 400.f );
            if ( !can_dash( dash_position ) ||
                is_wall_blocking( pred.position ) ||
                is_position_in_turret_range( dash_position ) &&
                !is_position_in_turret_range( g_local->position )
            )
                return false;

            switch ( g_config->belveth.q_mode->get< int >( ) ) {
            case 0:
                if ( dash_position.dist_to( raw_pred.position ) > g_local->attack_range + 100.f ) return false;
                break;
            case 1:
                if ( !g_features->orbwalker->is_attackable( target->index ) &&
                    dash_position.dist_to( raw_pred.position ) > g_local->attack_range + 100.f ||
                    g_features->orbwalker->is_attackable( target->index ) &&
                    dash_position.dist_to( raw_pred.position ) > g_local->attack_range
                )
                    return false;
                break;
            default:
                return false;
            }

            if ( cast_spell( ESpellSlot::q, dash_position ) ) {
                m_last_q_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->belveth.w_enabled->get< bool >( ) || m_e_active || *g_time - m_last_w_time <= 0.4f || !
                m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                g_config->belveth.w_only_out_of_range->get< bool >( ) &&
                g_features->orbwalker->is_attackable( target->index )
            )
                return false;

            auto pred = g_features->prediction->predict( target->index, 650.f, 0.f, 110.f, 0.5f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->belveth.w_hitchance->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.4f );

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->belveth.e_enabled->get< bool >( ) || m_e_active || *g_time - m_last_e_time <= 0.4f || !
                m_slot_e->is_ready( ) ||
                g_config->belveth.e_disable_under_turret->get< bool >( ) && is_position_in_turret_range(
                    g_local->position
                ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ||
                target->dist_to_local( ) > 350.f ||
                target->health / target->max_health * 100.f > g_config->belveth.e_min_health_percent->get< int >( )
            )
                return false;

            auto should_cast{ true };
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    minion->dist_to_local( ) > 475.f ||
                    !minion->is_normal_minion( )
                )
                    continue;

                if ( minion->health < target->health ) {
                    should_cast = false;
                    break;
                }
            }

            if ( !should_cast ) return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.25f );
            if ( !pred || g_local->position.dist_to( *pred ) > 210.f ) return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->belveth.r_enabled->get< bool >( ) || m_e_active || g_config->belveth.r_disable_if_buffed->
                get< bool >( ) && m_r_active || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( ) ) {
                return
                    false;
            }

            std::vector< Object* > spores{ };

            for ( auto unit : g_entity_list.get_ally_minions( ) ) {
                if ( !unit || unit->is_dead( ) || unit->dist_to_local( ) >= 340.f || !unit->is_spore( ) ) continue;

                spores.push_back( unit );
            }

            if ( spores.empty( ) ) return false;

            const Object* target_spore{ };
            int           target_hit_count{ };

            for ( const auto spore : spores ) {
                if ( !spore ) continue;

                int hit_count{ };

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy ||
                        enemy->is_dead( ) ||
                        enemy->is_invisible( ) ||
                        enemy->position.dist_to( spore->position ) > 500.f ||
                        g_features->target_selector->is_bad_target( enemy->index )
                    )
                        continue;

                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f );
                    if ( !pred || spore->position.dist_to( *pred ) > 450.f ) continue;

                    ++hit_count;
                }

                if ( hit_count > target_hit_count ) {
                    target_spore     = spore;
                    target_hit_count = hit_count;
                }
            }

            if ( !target_spore || target_hit_count == 0 ) return false;

            if ( cast_spell( ESpellSlot::r, target_spore->network_id ) ) {
                m_last_r_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.25f );
                return true;
            }

            return false;
        }

        auto can_dash( const Vec3& position ) const -> bool{
            const auto value{ 10.f };
            const auto north_east{ g_local->position + Vec3( value, 0, value ) };
            const auto north_west{ g_local->position + Vec3( -value, 0, value ) };
            const auto south_east{ g_local->position + Vec3( value, 0, -value ) };
            const auto south_west{ g_local->position + Vec3( -value, 0, -value ) };

            const auto dash_direction = g_local->position.extend( position, 10.f );


            auto lowest_distance{ 999.f };
            int  number{ };

            for ( auto i = 0; i < 4; i++ ) {
                float dist{ };
                switch ( i ) {
                case 0:
                    dist = dash_direction.dist_to( north_east );
                    break;
                case 1:
                    dist = dash_direction.dist_to( north_west );
                    break;
                case 2:
                    dist = dash_direction.dist_to( south_west );
                    break;
                case 3:
                    dist = dash_direction.dist_to( south_east );
                    break;
                default:
                    return false;
                }

                if ( dist < lowest_distance ) {
                    number          = i;
                    lowest_distance = dist;
                }
            }

            if ( m_flag == 0 ) return true;

            switch ( number ) {
            case 0:
                return m_flag & 2;
            case 1:
                return m_flag & 1;
            case 2:
                return m_flag & 8;
            case 3:
                return m_flag & 4;
            default:
                break;
            }

            // North west = 1 |
            // North east = 2 |
            // South east = 4 |
            // South west = 8 |

            return false;
        }

        auto is_wall_blocking( const Vec3& end_position ) const -> bool{
            const auto max_distance = g_local->position.dist_to( end_position );

            if ( m_r_active ) {
                if ( g_navgrid->is_wall( g_local->position.extend( end_position, 400.f ) ) ) return true;

                return false;
            }

            for ( auto i = 1; i < 40; i++ ) {
                const auto distance = i * 10.f;
                if ( distance >= max_distance ) break;

                if ( g_navgrid->is_wall( g_local->position.extend( end_position, distance ) ) ) return true;
            }

            return false;
        }

        auto laneclear_q( ) -> void{
            if ( !g_config->belveth.q_laneclear->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.5f ||
                !m_slot_q->is_ready( ) ||
                g_local->mana > 0 ||
                !g_features->orbwalker->should_reset_aa( )
            )
                return;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                m_q_range,
                100.f,
                false,
                true
            );

            if ( !laneclear_data || !can_dash( laneclear_data->cast_position ) ) return;

            if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) m_last_q_time = *g_time;

            return;
        }

        auto laneclear_w( ) -> void{
            if ( !g_config->belveth.w_laneclear->get< bool >( ) || *g_time - m_last_w_time <= 0.5f || !m_slot_w->
                is_ready( ) || !g_features->orbwalker->should_reset_aa( ) )
                return;

            const auto          laneclear_data = get_line_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::w, unit ); },
                [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::w, unit ); },
                m_w_range,
                100.f,
                false,
                true
            );

            if ( !laneclear_data ) return;

            if ( cast_spell( ESpellSlot::w, laneclear_data->cast_position ) ) {
                m_last_w_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.45f );
            }
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->belveth.w_autointerrupt->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            auto pred = g_features->prediction->predict( target->index, 650.f, 0.f, 110.f, 0.5f );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) m_last_w_time = *g_time;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->attack_damage( ) * 1.1f,
                    target->index,
                    true
                );
            case ESpellSlot::w:
                return helper::calculate_damage(
                    m_w_damage[ get_slot_w( )->level ] + g_local->bonus_attack_damage( ) + g_local->ability_power( ) *
                    1.25f,
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
                const auto tt   = g_local->position.dist_to( target->position ) / m_q_speed[ get_slot_q( )->level ];
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return g_local->position.dist_to( pred.value( ) ) / m_q_speed[ get_slot_q( )->level ];
            }
            case ESpellSlot::w:
                return 0.5f;
            default:
                break;
            }

            return std::numeric_limits< float >::max( );
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        int m_flag{ };

        bool m_r_active{ };
        bool m_e_active{ };

        std::vector< float > m_q_damage = { 0.f, 10.f, 15.f, 20.f, 25.f, 30.f };
        std::vector< float > m_w_damage = { 0.f, 70.f, 110.f, 150.f, 190.f, 230.f };
        std::vector< float > m_r_damage = { 0.f, 80.f, 100.f, 120.f };

        std::vector< float > m_q_speed = { 0.f, 800.f, 850.f, 900.f, 950.f, 1000.f };

        float m_q_range{ 400.f };
        float m_w_range{ 650.f };
        float m_e_range{ 500.f };
        float m_r_range{ 500.f };
    };
}
