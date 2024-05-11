#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class jarvan_module final : public IModule {
    public:
        virtual ~jarvan_module( ) = default;

        struct flag_instance_t {
            int16_t  index{ };
            unsigned network_id{ };

            Object* object{ };

            float start_time{ };
            float end_time{ };

            Vec3 position{ };
        };

        auto get_name( ) -> hash_t override{ return ct_hash( "jarvan_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "JarvanIV" ); }

        auto initialize( ) -> void override{ m_priority_list = { e_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation           = g_window->push( _( "jarvan" ), menu_order::champion_module );
            const auto q_settings           = navigation->add_section( _( "q settings" ) );
            const auto drawings             = navigation->add_section( _( "drawings" ) );
            const auto e_settings           = navigation->add_section( _( "e settings" ) );
            const auto w_settings           = navigation->add_section( _( "w settings" ) );
            const auto jungleclear_settings = navigation->add_section( _( "jungle clear" ) );

            q_settings->checkbox( _( "enable" ), g_config->jarvan.q_enabled );

            w_settings->checkbox( _( "enable" ), g_config->jarvan.w_enabled );

            e_settings->checkbox( _( "enable" ), g_config->jarvan.e_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->jarvan.eq_hitchance,
                { _( "Low" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            //e_settings->checkbox( _( "auto interrupt (?)" ), g_config->jarvan.eq_autointerrupt )
            //->set_tooltip( _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" ) );

            drawings->checkbox( _( "draw eq range" ), g_config->jarvan.eq_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->jarvan.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->ahri.dont_draw_on_cooldown );

            jungleclear_settings->checkbox( _( "enable q" ), g_config->jarvan.q_jungleclear );
            jungleclear_settings->checkbox( _( "enable w" ), g_config->jarvan.w_jungleclear );
            jungleclear_settings->checkbox( _( "enable e" ), g_config->jarvan.e_jungleclear );
        }

        auto on_draw( ) -> void override{
            if ( !g_config->jarvan.eq_draw_range->get< bool >( ) &&
                !g_config->jarvan.r_draw_range->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->jarvan.eq_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->jarvan.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        60,
                        3.f
                    );
                }
            }

            if ( g_config->jarvan.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->jarvan.dont_draw_on_cooldown->get
                    < bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_r_range,
                        Renderer::outline,
                        60,
                        3.f
                    );
                }
            }

            Vec3 flag_pos{ };
            auto flag_found = false;
            for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
                if ( !minion || minion->get_name( ) != "JarvanIVStandard" ) continue;
                flag_pos   = minion->position;
                flag_found = true;
            }

            //DEBUG

            /*if ( !flag_found ) return;

            auto rect =
                sdk::math::Rectangle( g_local->position, g_local->position.extend( flag_pos, m_q_range ), 150.f );
            auto polygon = rect.to_polygon( );

            const auto target = g_features->target_selector->get_default_target( );

            if ( !polygon.is_inside( target->position ) || !target) return;

            for ( size_t i = 0u; i < polygon.points.size( ); i++ ) {
                const auto start_point = polygon.points[ i ];
                const auto end_point = i == polygon.points.size( ) - 1 ? polygon.points[ 0 ] : polygon.points[ i + 1 ];

                Vec2 sp_start;
                Vec2 sp_end;

                if ( !sdk::math::world_to_screen( start_point, sp_start ) ||
                     !sdk::math::world_to_screen( end_point, sp_end ) )
                    continue;

                g_render->line( sp_start, sp_end, color( 25, 255, 25 ) , 1.f );
            }*/

            /*const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return;

            

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 900.f, 75.f, 0.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::e_hitchance >( g_config->jarvan.eq_hitchance->get< int >( ) ) ) return;

            //g_render->circle_3d( pred.position, color( 144, 66, 245, 255 ), 50.f, c_renderer::outline, 60, 3.f ); */
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_flag( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            //autointerrupt_eq( );

            if ( g_features->orbwalker->in_attack( ) || g_features->evade->is_active( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_e( );
                spell_w( );
                break;

            case Orbwalker::EOrbwalkerMode::harass:
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                // *********** jarvanivmartialcadencecheck ******************
                // need to add AA switching
                if ( g_config->jarvan.e_jungleclear->get< bool >( ) ) jungleclear_e( );
                if ( g_config->jarvan.w_jungleclear->get< bool >( ) ) jungleclear_w( );
                if ( g_config->jarvan.q_jungleclear->get< bool >( ) ) jungleclear_q( );

            default:
                break;
            }
        }

    private:
        //JarvanIVStandard
        auto spell_q( ) -> bool override{
            if ( !g_config->jarvan.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            if ( m_flag.empty( ) ) return false;

            const auto rect = sdk::math::Rectangle(
                g_local->position,
                g_local->position.extend( m_flag.front( ).position, m_q_range ),
                175.f
            );
            auto polygon = rect.to_polygon( );

            const auto target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            //float travel_time = 0.4f + m_flag.front(  ).object->position.dist_to( g_local->position ) / 1000.f;
            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1000.f, 75.f, 0.4f );

            if ( !pred.valid ) return false;

            if ( !polygon.is_inside( pred.position ) ) return false;

            //debug_log( "before cast" );

            if ( cast_spell( ESpellSlot::q, m_flag.front( ).position ) ) {
                g_features->orbwalker->set_cast_time( 0.4f );
                m_last_q_time = *g_time;
                //m_predpose    = { };
                return true;
            }

            return false;
        }

        auto jungleclear_q( ) -> bool{
            if ( !g_config->jarvan.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                !m_slot_q->is_ready( true ) )
                return false;

            if ( m_flag.empty( ) ) {
                debug_log( "we should be here" );
                const auto          laneclear_data = get_line_laneclear_target(
                    [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                    [this]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                    m_q_range,
                    90.f,
                    false,
                    true
                );

                if ( !laneclear_data ) return false;

                if ( cast_spell( ESpellSlot::q, laneclear_data->cast_position ) ) {
                    g_features->orbwalker->set_cast_time( 0.4f );
                    m_last_q_time = *g_time;
                    // m_predpose    = { };
                    return true;
                }
            } else {
                const auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( m_flag.front( ).position, m_q_range ),
                    200.f
                );
                auto          polygon = rect.to_polygon( );
                const Object* target  = { };

                for ( const auto monster : g_entity_list.get_enemy_minions( ) ) {
                    if ( !monster ||
                        monster->is_lane_minion( ) ||
                        !monster->is_alive( ) ||
                        !monster->is_visible( )
                    )
                        continue;

                    target = monster;
                }

                //if ( !polygon.is_inside( target->position ) || !target ) return false;

                if ( cast_spell( ESpellSlot::q, m_flag.front( ).position ) ) {
                    g_features->orbwalker->set_cast_time( 0.4f );
                    m_last_q_time = *g_time;
                    // m_predpose    = { };
                    return true;
                }
            }


            return false;
        }


        auto spell_w( ) -> bool override{
            if ( !g_config->jarvan.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready(
                true
            ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            if ( target->dist_to_local( ) < 600.f ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_w_time = *g_time;
                    return true;
                }
            }
            return false;
        }

        auto jungleclear_w( ) -> bool{
            if ( !m_slot_w->is_ready( true ) || *g_time - m_last_w_time <= 0.4f ) return false;
            auto in_range = false;
            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion ||
                    minion->is_dead( ) ||
                    minion->is_invisible( ) ||
                    !minion->is_jungle_monster( ) ||
                    minion->position.dist_to( g_local->position ) > m_w_range
                )
                    continue;

                in_range = true;
            }

            if ( in_range ) {
                if ( cast_spell( ESpellSlot::w ) ) {
                    m_last_q_time = *g_time;
                    g_features->orbwalker->set_cast_time( 0.25f );
                    return true;
                }
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->jarvan.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );

            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_e_range, 1000.f, 75.f, 0.f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->jarvan.eq_hitchance->
                get< int >( ) ) )
                return false;

            Vec3 cast_pos{ };
            //cast_pos = target->position.extend( pred.position, +15 );
            /*for ( int i = 50; i < 120; i += 2 ) {
                Vec3 temp = 
            }*/
            if ( target->dist_to_local( ) <= 200 ) cast_pos = pred.position;
            else cast_pos = g_local->position.extend( pred.position, g_local->position.dist_to( pred.position ) + 130 );


            if ( cast_spell( ESpellSlot::e, cast_pos ) ) {
                m_last_e_time = *g_time;
                // m_predpose    = { };
                return true;
            }
            return false;
        }

        auto jungleclear_e( ) -> bool{
            if ( !g_config->jarvan.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                !m_slot_e->is_ready( true ) )
                return false;

            const auto farm_pos = get_best_laneclear_position( m_e_range, 200, false, true, 0.25f );

            if ( farm_pos.value < 1 ) return false;

            if ( cast_spell( ESpellSlot::e, farm_pos.position ) ) {
                m_last_e_time = *g_time;
                return true;
            }
            return false;
        }

        //Thank you SX
        auto update_flag( ) -> void{
            for ( const auto unit : g_entity_list.get_ally_minions( ) ) {
                if ( !unit || unit->is_dead( ) || !unit->is_jarvan_flag( ) || is_tracked( unit->network_id ) ) continue;


                flag_instance_t inst{ unit->index, unit->network_id, unit, *g_time, *g_time + 8.f, unit->position };


                m_flag.push_back( inst );
            }

            for ( auto& inst : m_flag ) {
                if ( !inst.object || inst.object->is_dead( ) || !inst.object->is_jarvan_flag( ) ) {
                    remove_flag( inst.network_id );
                    continue;
                }

                inst.position = inst.object->position;
            }
        }

        auto remove_flag( const unsigned network_id ) -> void{
            if ( m_flag.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_flag,
                    [ & ]( const flag_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_flag.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_flag ) if ( inst.network_id == network_id ) return true;

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->bonus_attack_damage( ) * 1.40,
                    target->index,
                    true
                );
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.4f + g_local->position.dist_to( target->position ) / 800.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.4f + g_local->position.dist_to( pred.value( ) ) / 800.f;
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

        std::array< float, 6 > m_q_damage = { 0.f, 90.f, 130.f, 170.f, 210.f, 250.f };
        std::array< float, 6 > m_e_damage = { 0.f, 80.f, 110.f, 140.f, 170.f, 200.f };

        float m_q_range{ 850.f };
        float m_w_range{ 600.f };
        float m_e_range{ 860.f };
        float m_r_range{ 650.f };

        Vec3                           m_flag_pos{ };
        bool                           m_flag_found{ };
        std::vector< flag_instance_t > m_flag{ };
    };
} // namespace features::champion_modules
