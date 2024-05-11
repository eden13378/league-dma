#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class syndra_module final : public IModule {
    public:
        virtual ~syndra_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "syndra_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Syndra" ); }

        auto initialize( ) -> void override{ m_priority_list = { r_spell, w_spell, q_spell, e_spell }; }

        auto on_draw( ) -> void override{
            if ( !g_config->syndra.q_draw_range->get< bool >( ) &&
                !g_config->syndra.eq_draw_range->get< bool >( ) &&
                !g_config->syndra.r_draw_range->get< bool >( ) &&
                !g_config->syndra.r_draw_damage->get< bool >( ) ||
                g_local->is_dead( )
            )
                return;

            g_local.update( );

            if ( g_config->syndra.q_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::q )->
                                                                           level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 47, 79, 173, 255 ),
                    800.f,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            if ( g_config->syndra.eq_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::q )->
                                                                            level > 0 && g_local->spell_book.
                get_spell_slot( ESpellSlot::e )->level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 47, 79, 173, 255 ),
                    1250.f,
                    Renderer::outline,
                    80,
                    2.f
                );
            }

            if ( g_config->syndra.r_draw_range->get< bool >( ) && g_local->spell_book.get_spell_slot( ESpellSlot::r )->
                                                                           level > 0 ) {
                g_render->circle_3d(
                    g_local->position,
                    Color( 173, 47, 68, 255 ),
                    m_r_range,
                    Renderer::outline,
                    80,
                    2.f
                );
            }


            if ( !g_config->syndra.r_draw_damage->get< bool >( ) || !m_slot_r->is_ready( true ) ) return;

            for ( const auto index : g_features->tracker->get_enemies( ) ) {
                auto& enemy = g_entity_list.get_by_index( index );
                if ( !enemy || enemy->dist_to_local( ) > 1200.f ) continue;

                Vec2 sp{ };
                if ( !world_to_screen( enemy->position, sp ) ) continue;

                const auto width_ratio  = 1920.f / static_cast< float >( g_render_manager->get_width( ) );
                const auto height_ratio = 1080.f / static_cast< float >( g_render_manager->get_height( ) );

                const auto bar_length = width_ratio * 0.055f;
                const auto bar_height = height_ratio * 0.0222f;

                auto base_position = enemy->get_hpbar_position( );

                base_position.x -= bar_length * 0.43f;
                base_position.y -= bar_height;

                const auto damage = get_spell_damage( ESpellSlot::r, enemy.get( ) ) + predict_q_damage( enemy->index );
                const auto modifier = enemy->health / enemy->max_health;
                const auto damage_mod = damage / enemy->max_health;

                const Vec2 box_start{ base_position.x + bar_length * modifier, base_position.y };
                const Vec2 box_size{
                    damage_mod * bar_length > box_start.x - base_position.x
                        ? base_position.x - box_start.x
                        : -( bar_length * damage_mod ),
                    bar_height * 0.5f
                };

                g_render->filled_box( box_start, box_size, Color( 94, 235, 52, 180 ) );
            }
        }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "syndra" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );
            const auto spellclear = navigation->add_section( _( "spellclear" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );

            q_settings->checkbox( _( "enable" ), g_config->syndra.q_enabled );
            q_settings->checkbox( _( "q harass" ), g_config->syndra.q_harass );
            q_settings->select(
                _( "hitchance" ),
                g_config->syndra.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->syndra.w_enabled );
            w_settings->select(
                _( "hitchance" ),
                g_config->syndra.w_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->syndra.e_enabled );
            e_settings->checkbox( _( "antigapclose" ), g_config->syndra.e_antigapclose );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->syndra.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            e_settings->checkbox( _( "allow qe combo" ), g_config->syndra.e_snipe_enabled );
            e_settings->select(
                _( "hitchance" ),
                g_config->syndra.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            r_settings->checkbox( _( "enable" ), g_config->syndra.r_enabled );
            r_settings->checkbox( _( "predict q damage" ), g_config->syndra.r_predict_q_damage );

            spellclear->checkbox( _( "use q to lasthit" ), g_config->syndra.lasthit_q );
            spellclear->checkbox( _( "use q to laneclear" ), g_config->syndra.laneclear_q );

            drawings->checkbox( _( "draw q range" ), g_config->syndra.q_draw_range );
            drawings->checkbox( _( "draw eq range" ), g_config->syndra.eq_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->syndra.r_draw_range );
            drawings->checkbox( _( "draw r damage" ), g_config->syndra.r_draw_damage );
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_spheres( );

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );

                if ( g_features->orbwalker->in_attack( ) || m_snipe_start > 0.f && cast_delayed_e( ) ) return;

                spell_e( );
                spell_q( );
                spell_w( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->syndra.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->syndra.lasthit_q->get< bool >( ) ) {
                    if ( g_config->syndra.laneclear_q->get< bool >( ) && GetAsyncKeyState( VK_CONTROL ) &&
                        laneclear_q( ) )
                        return;

                    if ( lasthit_q( ) ) return;
                }

                if ( g_config->syndra.q_harass->get< bool >( ) && !is_position_in_turret_range( ) ) spell_q( );

                break;
            case Orbwalker::EOrbwalkerMode::lasthit:
                if ( g_features->orbwalker->in_attack( ) ) return;

                if ( g_config->syndra.lasthit_q->get< bool >( ) && lasthit_q( ) ) return;
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->syndra.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f || *g_time -
                m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;


            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( combo_q( target ) ) break;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                200.f,
                0.8f,
                { },
                false,
                Prediction::include_ping,
                Prediction::ESpellType::circle
            );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->syndra.q_hitchance->
                get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->syndra.w_enabled->get< bool >( ) ||
                *g_time - m_last_w_time < 0.3f ||
                *g_time - m_last_cast_time <= 0.1f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            if ( rt_hash( m_slot_w->get_name( ).c_str( ) ) == ct_hash( "SyndraWCast" ) ) {
                if ( m_grabbed_unit == 0 ) {
                    const auto pred = g_features->prediction->predict( target->index, 950.f, 1450.f, 200.f, 0.25f );
                    if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->syndra.
                        w_hitchance->get< int >( ) ) )
                        return false;

                    if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;

                        debug_log( "[ syndra ] casted W without knowing unit" );
                        return true;
                    }
                } else {
                    const auto pred = g_features->prediction->predict(
                        target->index,
                        950.f,
                        1450.f,
                        200.f,
                        0.25f
                    );

                    if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->syndra.
                        w_hitchance->get< int >( ) ) )
                        return false;

                    if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                        m_last_w_time    = *g_time;
                        m_last_cast_time = *g_time;
                        m_grabbed_unit   = 0;

                        debug_log( "[ syndra ] casted W" );
                        return true;
                    }
                }
            } else {
                Vec3    grab_position{ };
                int16_t candidate_index{ };

                if ( target->dist_to_local( ) > 950.f ) return false;

                for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
                    if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->dist_to_local( ) >= 925.f || obj->
                        get_name( ) != "SyndraSphere" )
                        continue;

                    grab_position   = obj->position;
                    candidate_index = obj->index;
                    break;
                }

                if ( grab_position.length( ) <= 0.f || candidate_index == 0 ) {
                    auto highest_hp{ 0.f };

                    for ( const auto obj : g_entity_list.get_enemy_minions( ) ) {
                        if ( !obj || obj->is_dead( ) || obj->is_invisible( ) || obj->dist_to_local( ) >= 925.f || !obj->
                            is_normal_minion( ) )
                            continue;

                        if ( obj->is_major_monster( ) || obj->is_minion_only_autoattackable( ) ) continue;

                        if ( obj->health < highest_hp ) continue;

                        grab_position   = obj->position;
                        highest_hp      = obj->health;
                        candidate_index = obj->index;
                    }

                    if ( grab_position.length( ) == 0.f || candidate_index == 0 ) return false;
                }

                if ( cast_spell( ESpellSlot::w, grab_position ) ) {
                    g_features->orbwalker->on_cast( );
                    m_last_cast_time = *g_time;
                    m_grabbed_unit   = candidate_index;
                    return true;
                }
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->syndra.e_enabled->get< bool >( ) ||
                !m_slot_e->is_ready( true ) ||
                *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f
            )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target( 1200.f );
            if ( !target ) return false;

            bool should_e{ };
            Vec3 cast_position{ };

            for ( const auto sphere : m_spheres ) {
                auto& obj = g_entity_list.get_by_index( sphere );
                if ( !obj ) continue;

                obj.update( );

                if ( obj->is_dead( ) || obj->dist_to_local( ) > 800.f || m_grabbed_unit == obj->index ) continue;

                auto rect = sdk::math::Rectangle(
                    g_local->position,
                    g_local->position.extend( obj->position, 1250.f ),
                    100.f
                );
                auto poly = rect.to_polygon( );

                if ( obj->dist_to_local( ) < target->dist_to_local( ) ) {
                    auto pred = g_features->prediction->predict(
                        target->index,
                        1200.f - obj->dist_to_local( ),
                        2000.f,
                        100.f,
                        0.25f,
                        obj->position
                    );
                    if ( !pred.valid ||
                        pred.position.length( ) == 0 ||
                        pred.hitchance < static_cast< Prediction::EHitchance >( g_config->syndra.e_hitchance->get<
                            int >( ) )
                    )
                        return false;

                    if ( !poly.is_outside( pred.position ) ) {
                        should_e      = true;
                        cast_position = obj->position;
                        break;
                    }
                } else if ( obj->dist_to_local( ) <= 1100.f && target->dist_to_local( ) <= 700.f ) {
                    auto pred = g_features->prediction->predict_default( target->index, 0.25f );
                    if ( !pred || pred.value( ).dist_to( g_local->position ) > 775.f ) return false;

                    if ( !poly.is_outside( g_local->position.extend( pred.value( ), obj->dist_to_local( ) ) ) ) {
                        should_e      = true;
                        cast_position = g_local->position.extend( pred.value( ), obj->dist_to_local( ) );
                        break;
                    }
                }
            }

            if ( !should_e && g_config->syndra.e_snipe_enabled->get< bool >( ) && m_slot_q->is_ready( true ) && target->
                dist_to_local( ) >= 825.f ) {
                const auto pred = g_features->prediction->predict(
                    target->index,
                    500.f,
                    2000.f,
                    100.f,
                    0.6f,
                    g_local->position.extend( target->position, 775.f )
                );
                if ( !pred.valid || pred.position.length( ) == 0 || pred.hitchance < static_cast<
                    Prediction::EHitchance >( g_config->syndra.e_hitchance->get< int >( ) ) )
                    return false;

                if ( cast_spell( ESpellSlot::q, g_local->position.extend( pred.position, 775.f ) ) ) {
                    m_last_q_time        = *g_time;
                    m_last_cast_time     = *g_time;
                    m_delayed_e_position = g_local->position.extend( pred.position, 775.f );
                    m_snipe_start        = *g_time;

                    return true;
                }

                return false;
            }

            if ( !should_e ) return false;

            if ( cast_spell( ESpellSlot::e, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->syndra.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.1f
                || *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_spell_specific_target(
                m_r_range + 10.f,
                { },
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::r, unit ); }
            );
            if ( !target ) return false;

            const auto damage      = get_spell_damage( ESpellSlot::r, target ) + predict_q_damage( target->index );
            const auto health      = g_features->prediction->predict_health( target, 0.5f );
            const auto is_upgraded = m_slot_r->get_usable_state( ) == 1;

            const auto can_kill    = damage >= health;
            const auto can_execute = is_upgraded && health - damage < target->max_health * 0.15f;

            if ( !can_kill && !can_execute ) return false;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_r_time    = *g_time;

                return true;
            }

            return false;
        }

        auto update_spheres( ) -> void{
            for ( const auto obj : g_entity_list.get_ally_minions( ) ) {
                if ( !obj || obj->dist_to_local( ) > 1500.f ) continue;

                const auto hashed_name = rt_hash( obj->get_name( ).data( ) );
                if ( hashed_name == ct_hash( "SyndraSphere" ) || hashed_name == ct_hash( "TestCubeRender10Vision" ) ) {
                    bool skip;
                    for ( const auto index : m_spheres ) {
                        if ( index == obj->index ) {
                            skip = true;
                            break;
                        }
                    }

                    m_spheres.push_back( obj->index );
                }
            }

            for ( const auto x : m_spheres ) {
                auto& obj = g_entity_list.get_by_index( x );
                if ( !obj ) {
                    for ( size_t i = 0u; i < m_spheres.size( ); i++ ) {
                        if ( m_spheres[ i ] == x ) {
                            m_spheres.erase( m_spheres.begin( ) + i );
                            break;
                        }
                    }

                    continue;
                }

                obj.update( );

                if ( !obj || obj->is_dead( ) ) {
                    for ( size_t i = 0u; i < m_spheres.size( ); i++ ) {
                        if ( m_spheres[ i ] == x ) {
                            m_spheres.erase( m_spheres.begin( ) + i );
                            break;
                        }
                    }
                }
            }
        }

        auto cast_delayed_e( ) -> bool{
            if ( *g_time - m_snipe_start >= 0.3f || !g_config->syndra.e_snipe_enabled->get< bool >( ) ) {
                m_delayed_e_position = Vec3( 0, 0, 0 );
                m_snipe_start        = 0.f;
                return false;
            }

            if ( !m_slot_e->is_ready( true ) || *g_time - m_last_cast_time < 0.05f ) return false;

            if ( m_delayed_e_position.dist_to( g_local->position ) > 800.f ) {
                m_delayed_e_position = Vec3( 0, 0, 0 );
                m_snipe_start        = 0.f;
                return false;
            }

            if ( cast_spell( ESpellSlot::e, m_delayed_e_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time        = *g_time;
                m_last_cast_time     = *g_time;
                m_snipe_start        = 0.f;
                m_delayed_e_position = Vec3( 0, 0, 0 );
                return true;
            }

            return false;
        }

        auto lasthit_q( ) -> bool{
            if ( *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto          farm_data = get_circle_lasthit_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                0.f,
                0.6f,
                90.f
            );

            if ( farm_data.position.length( ) <= 0.f ) return false;

            if ( cast_spell( ESpellSlot::q, farm_data.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->prediction->add_special_attack(
                    farm_data.target_data[ 0 ].index,
                    farm_data.target_data[ 0 ].damage,
                    farm_data.target_data[ 0 ].travel_time
                );
                return true;
            }

            return false;
        }

        auto laneclear_q( ) -> bool{
            if ( *g_time - m_last_q_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_q->
                is_ready( true ) )
                return false;

            const auto          farm_pos = get_circle_laneclear_target(
                [this]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                m_q_range,
                0.f,
                0.6f,
                90.f
            );
            if ( farm_pos.value < 4 ) {
                // std::cout << "pts: " <<  std::dec << farm_pos.value << std::endl;
                return false;
            }
            // else  std::cout << "pts: " << std::dec << farm_pos.value << std::endl;

            if ( cast_spell( ESpellSlot::q, farm_pos.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->set_cast_time( 0.f );

                for ( const auto inst : farm_pos.target_data ) {
                    g_features->prediction->add_special_attack(
                        inst.index,
                        inst.damage,
                        inst.travel_time
                    );
                }

                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->syndra.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( 700.f, 0.f, 0.f, 0.25f );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, 700.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_e_time = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->syndra.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict( target->index, 700.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) m_last_e_time = *g_time;
        }

        auto predict_q_damage( const int16_t index ) -> float{
            if ( !g_config->syndra.r_predict_q_damage->get< bool >( ) ) return 0.f;

            auto& enemy = g_entity_list.get_by_index( index );
            if ( !enemy ) return 0.f;

            enemy.update( );

            const Object* missile{ };

            for ( const auto mis : g_entity_list.get_ally_missiles( ) ) {
                if ( !mis || mis->dist_to_local( ) > 1000.f ) continue;

                auto info = mis->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto name = data->get_name( );
                if ( name.find( "SyndraQSpell" ) == std::string::npos ) continue;

                missile = mis;
                break;
            }

            if ( !missile ) return 0.f;

            const auto spawn_time     = missile->missile_spawn_time( );
            const auto time_until_hit = 0.625f - ( *g_time - spawn_time );

            const auto pred = g_features->prediction->predict_default( enemy->index, time_until_hit );
            if ( !pred || pred.value( ).dist_to( missile->missile_end_position ) >= 210.f ) return 0.f;

            const auto possible_movement = time_until_hit * enemy->movement_speed;
            const auto q_distance        = enemy->position.dist_to( missile->missile_end_position );
            const auto safe_distance     = 210.f - q_distance;

            if ( possible_movement - safe_distance > 5.f ) return 0.f;

            const auto damage = get_spell_damage( ESpellSlot::q, enemy.get( ) );

            return damage;
        }

        auto get_available_balls( ) -> int{
            const Object* missile{ };

            for ( const auto mis : g_entity_list.get_ally_missiles( ) ) {
                if ( !mis || mis->dist_to_local( ) > 1000.f ) continue;

                auto info = mis->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                auto name = data->get_name( );
                if ( name.find( "SyndraQSpell" ) == std::string::npos ) continue;

                missile = mis;
                break;
            }

            if ( !missile || m_slot_r->charges == 7 ) return m_slot_r->charges;

            return m_slot_r->charges + 1;
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
            case ESpellSlot::r:
            {
                const auto damage = m_sphere_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.17f;
                return helper::calculate_damage(
                    damage * static_cast< float >( get_available_balls( ) ),
                    target->index,
                    false
                );
            }
            }

            return 0.f;
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.6f;
            }

            return IModule::get_spell_travel_time( slot, target );
        }

    private:
        std::vector< float >   m_sphere_damage = { 0.f, 90.f, 130.f, 170.f };
        std::vector< float >   m_q_damage      = { 0.f, 70.f, 105.f, 140.f, 175.f, 210.f };
        std::vector< int16_t > m_spheres{ };

        Vec3  m_delayed_e_position{ };
        float m_snipe_start{ };

        int16_t m_grabbed_unit{ };

        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };
        float m_last_cast_time{ };

        float m_q_range{ 800.f };
        float m_w_range{ 950.f };
        float m_e_range{ 800.f };
        float m_r_range{ 675.f };
    };
}
