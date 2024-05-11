#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class fiora_module final : public IModule {
    public:
        struct vital_instance_t {
            int16_t  index{ };
            unsigned network_id{ };

            float start_time{ };
            float end_time{ };

            bool is_active{ };

            bool  is_direction_saved{ };
            float last_direction_update_time{ };
            Vec3  direction{ };
        };

        virtual ~fiora_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "fiora_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Fiora" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "fiora" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto vitals     = navigation->add_section( _( "vital settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->fiora.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->fiora.q_harass );
            //q_settings->checkbox( _( "only on vitals" ), g_config->fiora.q_only_vitals );
            q_settings->select(
                _( "vital hitchance" ),
                g_config->fiora.q_vital_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->fiora.e_enabled );
            e_settings->checkbox( _( "only to reset aa" ), g_config->fiora.e_aa_reset );
            e_settings->checkbox( _( "turretclear e" ), g_config->fiora.e_turretclear );

            vitals->slider_int( _( "max range %" ), g_config->fiora.vital_max_range, 50, 100, 1 );
            vitals->slider_int( _( "max angle %" ), g_config->fiora.vital_max_angle, 50, 100, 1 );

            drawings->checkbox( _( "draw q range" ), g_config->fiora.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->fiora.w_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->fiora.dont_draw_on_cooldown );
            drawings->checkbox( _( "draw vitals" ), g_config->fiora.draw_vitals );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };


            if ( g_config->fiora.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->fiora.dont_draw_on_cooldown->get< bool >( ) ) ) {
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


            for ( auto vital : m_vitals ) {
                const auto target = g_entity_list.get_by_index( vital.index );
                if ( !target || target->is_dead( ) ) continue;

                // g_render->circle_3d( target->position, color( 255, 255, 0 ), 65.f, c_renderer::outline, 32, 2.f );

                if ( !vital.is_direction_saved ) continue;

                auto temp            = target->position + vital.direction;
                auto vital_direction = target->position.extend( temp, 100.f );

                auto sect = Sector(
                    target->position,
                    vital_direction,
                    90.f * ( g_config->fiora.vital_max_angle->get< int >( ) / 100.f ),
                    400.f * ( g_config->fiora.vital_max_range->get< int >( ) / 100.f )
                );
                const auto poly = sect.to_polygon_new( );

                g_render->polygon_3d(
                    poly,
                    g_features->orbwalker
                              ->animate_color(
                                  !vital.is_active ? Color( 255, 255, 25 ) : Color( 50, 100, 250 ),
                                  EAnimationType::pulse,
                                  2
                              ).alpha( 25 ),
                    Renderer::outline | Renderer::filled,
                    2.f
                );
            }

            //for ( auto point : m_dash_points ) { g_render->circle_3d( point, Color::white( ), 5.f, Renderer::outline, 8, 1.f ); }
        }

        // 1100 q dash speed

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_vitals( );

            if ( g_features->orbwalker->in_action( ) || g_features->orbwalker->in_attack( ) ||
                g_features->evade->is_active( ) )
                return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_q( );
                spell_e( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->fiora.e_turretclear->get< bool >( ) &&
                    g_features->orbwalker->get_last_target( ) == ETargetType::turret && spell_e( ) )
                    break;

                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->fiora.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;


            if ( vital_q( ) ) return true;


            return false;
        }

        auto vital_q( ) -> bool{
            auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            auto vital_optional = get_vital( target->network_id );
            if ( !vital_optional ) return false;

            auto vital = vital_optional.value( );
            if ( !vital.is_direction_saved ) return false;

            auto pred = g_features->prediction->predict( target->index, 1000.f, 0.f, 0.f, 0.15f );
            if ( !pred.valid || ( int )pred.hitchance < g_config->fiora.q_vital_hitchance->get< int >( ) ) return false;

            auto target_position = pred.position;


            auto temp            = target_position + vital.direction;
            auto vital_direction = target_position.extend( temp, 375.f );

            auto sect = sdk::math::Sector(
                target_position,
                vital_direction,
                80.f * ( g_config->fiora.vital_max_angle->get< int >( ) / 100.f ),
                400.f * ( g_config->fiora.vital_max_range->get< int >( ) / 100.f )
            );
            auto poly = sect.to_polygon_new( );

            std::vector< Vec3 > available_points{ };

            for ( auto point : poly.points ) {
                for ( auto i = 0; i <= 3; i++ ) {
                    if ( point.dist_to( target_position ) <= 10.f ) continue;

                    available_points.push_back(
                        point.extend( target_position, point.dist_to( target_position ) / 5.f * i )
                    );
                }
            }

            if ( available_points.size( ) > 0 ) m_dash_points = available_points;

            Vec3  best_point{ };
            float best_weight{ };

            for ( auto point : available_points ) {
                if ( g_navgrid->is_wall( point ) ) continue;

                auto distance = g_local->position.dist_to( point );
                if ( distance > m_q_range + 50.f || distance <= 75.f ) continue;

                auto travel_time = g_features->orbwalker->get_ping( ) + g_local->position.dist_to( point ) / 1100.f;
                auto predict     = g_features->prediction->predict_default( target->index, travel_time );
                if ( !predict ) break;

                auto sim_temp       = predict.value( ) + vital.direction;
                auto sim_direction  = predict.value( ).extend( sim_temp, 375.f );
                auto simulated_poly =
                    sdk::math::Sector(
                        *predict,
                        sim_direction,
                        80.f * ( g_config->fiora.vital_max_angle->get< int >( ) / 100.f ),
                        400.f * ( g_config->fiora.vital_max_range->get< int >( ) / 100.f )
                    )
                    .to_polygon_new( );
                if ( simulated_poly.is_outside( point ) || point.dist_to( *predict ) > 200.f ) continue;

                auto weight{ std::min( 1.f - distance / 360.f, 0.5f ) };

                auto       v1            = sim_direction - *predict;
                auto       v2            = point - *predict;
                auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                weight += 1.f - current_angle / 180.f;
                if ( weight < best_weight ) continue;

                best_point  = point;
                best_weight = weight;
            }

            if ( best_point.length( ) <= 0.f || !vital.is_active ) return false;

            const auto cast_position = best_point;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{ return false; }

        auto spell_e( ) -> bool override{
            if ( !g_config->fiora.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f ||
                *g_time - m_last_e_time <= 0.5f || !g_features->orbwalker->should_reset_aa( ) || !m_slot_e->is_ready(
                    true
                ) ||
                g_features->buff_cache->get_buff( g_local->index, ct_hash( "FioraE" ) ) || g_features->buff_cache->
                get_buff( g_local->index, ct_hash( "fiorae2" ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{ return false; }

        auto update_vitals( ) -> void{
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 1200.f ||
                    is_vital_tracked( enemy->network_id ) )
                    continue;

                const auto buff = g_features->buff_cache->get_buff( enemy->index, ct_hash( "fiorapassivemanager" ) );
                if ( !buff ) continue;

                vital_instance_t inst{
                    enemy->index,
                    enemy->network_id,
                    buff->buff_data->start_time,
                    buff->buff_data->end_time,
                    false,
                    false,
                    0.f,
                    Vec3( )
                };

                std::cout << "Added new vital for " << enemy->champion_name.text << " | start time: " << inst.start_time
                    << " | T: " << *g_time << std::endl;

                m_vitals.push_back( inst );
            }

            for ( auto& vital : m_vitals ) {
                if ( vital.end_time <= *g_time ) {
                    remove_vital( vital.network_id );
                    continue;
                }

                const auto target = g_entity_list.get_by_index( vital.index );
                if ( !target || target->is_dead( ) || target->is_invisible( ) ) {
                    remove_vital( vital.network_id );
                    continue;
                }

                const auto buff = g_features->buff_cache->get_buff( vital.index, ct_hash( "fiorapassivemanager" ) );
                if ( !buff || buff->buff_data->start_time > vital.start_time + 1.f ) {
                    remove_vital( vital.network_id );
                    continue;
                }

                if ( vital.is_direction_saved && *g_time - vital.last_direction_update_time <= 0.5f ) {
                    if ( !vital.is_active && *g_time - vital.start_time >= 1.75f ) vital.is_active = true;

                    continue;
                }

                bool found_vital{ };
                Vec3 detected_direction{ };

                for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                    if ( !particle || particle->position.dist_to( target->position ) > 50.f ) continue;

                    auto name = particle->get_alternative_name( );
                    if ( name.find( "Fiora_Base_Passive" ) == std::string::npos ) continue;

                    const auto name_hash = rt_hash( name.data( ) );

                    switch ( name_hash ) {
                    case ct_hash( "Fiora_Base_Passive_NE" ): // North
                    case ct_hash( "Fiora_Base_Passive_NE_Warning" ):
                        detected_direction = Vec3( 0, 0, 1 );
                        break;
                    case ct_hash( "Fiora_Base_Passive_NW" ): // West
                    case ct_hash( "Fiora_Base_Passive_NW_Warning" ):
                        detected_direction = Vec3( 1, 0, 0 );
                        break;
                    case ct_hash( "Fiora_Base_Passive_SW" ): // South
                    case ct_hash( "Fiora_Base_Passive_SW_Warning" ):
                        detected_direction = Vec3( 0, 0, -1 );
                        break;
                    case ct_hash( "Fiora_Base_Passive_SE" ): // East
                    case ct_hash( "Fiora_Base_Passive_SE_Warning" ):
                        detected_direction = Vec3( -1, 0, 0 );
                        break;
                    default:
                        break;
                    }

                    if ( detected_direction.length( ) > 0.f ) found_vital = true;
                    break;
                }

                if ( !found_vital ) continue;

                if ( vital.is_direction_saved && detected_direction != vital.direction ) {
                    std::cout << "New vital direction found? "
                        << "last update: " << *g_time - vital.last_direction_update_time
                        << "| time since start: " << *g_time - vital.start_time << std::endl;
                }

                vital.is_direction_saved         = true;
                vital.last_direction_update_time = *g_time;
                vital.direction                  = detected_direction;
            }
        }

        auto get_vital( const unsigned network_id ) const -> std::optional< vital_instance_t >{
            for ( const auto vital : m_vitals )
                if ( vital.network_id == network_id )
                    return
                        std::make_optional( vital );

            return std::nullopt;;
        }

        auto remove_vital( const unsigned network_id ) -> void{
            if ( m_vitals.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_vitals,
                    [ & ]( const vital_instance_t& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_vitals.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_vital_tracked( const unsigned network_id ) const -> bool{
            for ( const auto vital : m_vitals ) if ( vital.network_id == network_id ) return true;

            return false;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage( m_q_damage[ get_slot_q( )->level ], target->index, false );
            default:
                break;
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 2000.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 2000.f;
            }
            case ESpellSlot::w:
                return 0.25f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        // fiorapassivemanager - vital buff

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        std::vector< vital_instance_t > m_vitals{ };
        std::vector< float >            m_q_damage = { 0.f, 80.f, 130.f, 180.f, 230.f, 280.f };

        std::vector< Vec3 > m_dash_points{ };

        float m_q_range{ 360.f };
        float m_w_range{ 900.f };
        float m_e_range{ 0.f };
    };
} // namespace features::champion_modules
