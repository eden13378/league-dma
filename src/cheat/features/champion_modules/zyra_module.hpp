#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "../utils/c_function_caller.hpp"

namespace features::champion_modules {
    class ZyraModule final : public IModule {
        enum class EPlantType {
            unknown = 0,
            seed,
            thornspitter,
            vinelasher
        };

        struct PlantInstance {
            int16_t  index{ };
            unsigned network_id{ };

            Vec3       position{ };
            EPlantType type{ };
            float      attack_range{ };

            int16_t last_target_index{ };
            float   last_server_cast_time{ };

            bool  is_glowing{ };
            float last_glow_update_time{ };

            std::vector< Vec3 > attackrange_circle{ };

            float start_time{ };
            float end_time{ };
        };

        struct SpawningSeedInstance {
            int16_t  index{ };
            unsigned network_id{ };

            Vec3 position{ };

            float start_time{ };
            float end_time{ };
        };

    public:
        virtual ~ZyraModule( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "zyra_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Zyra" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "zyra" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->zyra.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->zyra.q_harass );
            q_settings->checkbox( _( "fertilizer q (keybind)" ), g_config->zyra.q_poke_toggle );
            q_settings->select(
                _( "hitchance" ),
                g_config->zyra.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->zyra.w_enabled );
            w_settings->checkbox( _( "dont place under turret" ), g_config->zyra.w_turret_check );
            w_settings->checkbox( _( "save charge (?)" ), g_config->zyra.w_save_charge )->set_tooltip(
                _( "Will save 1 charge if cannot spawn preferred plant" )
            );
            w_settings->select(
                _( "preferred plant" ),
                g_config->zyra.w_preferred_plant,
                { _( "Thorn spitter" ), _( "Vine lasher" ) }
            );

            e_settings->checkbox( _( "enable" ), g_config->zyra.e_enabled );
            e_settings->checkbox( _( "flee e" ), g_config->zyra.e_flee );
            e_settings->checkbox( _( "auto interrupt (?)" ), g_config->zyra.e_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Shen R, Fiddlesticks R, etc" )
            );
            e_settings->checkbox( _( "antigapclose e" ), g_config->zyra.e_antigapclose );
            e_settings->checkbox( _( "multihit" ), g_config->zyra.e_multihit );
            e_settings->select(
                _( "hitchance" ),
                g_config->zyra.e_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            e_settings->slider_int( _( "max range %" ), g_config->zyra.e_max_range, 70, 100, 1 );

            r_settings->checkbox( _( "enable" ), g_config->zyra.r_enabled );
            r_settings->checkbox( _( "only in full combo" ), g_config->zyra.r_only_full_combo );
            r_settings->select(
                _( "hitchance" ),
                g_config->zyra.r_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            r_settings->checkbox( _( "standalone multihit" ), g_config->zyra.r_multihit );

            drawings->checkbox( _( "draw q range" ), g_config->zyra.q_draw_range );
            drawings->checkbox( _( "draw w range" ), g_config->zyra.w_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->zyra.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->zyra.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->zyra.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->zyra.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zyra.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->zyra.w_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->zyra.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 50, 255, 75, 255 ),
                        m_w_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->zyra.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zyra.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 144, 66, 245, 255 ),
                        m_e_range * ( g_config->zyra.e_max_range->get< int >( ) / 100.f ),
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            if ( g_config->zyra.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->zyra.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        70,
                        2.f
                    );
                }
            }

            draw_plants( );


            if ( g_config->zyra.q_poke_toggle->get< bool >( ) ) {
                Vec2 sp{ };
                if ( !world_to_screen( g_local->position, sp ) ) return;

                sp.x += 30.f;
                sp.y -= 8.f;

                const std::string text{ "LONG HARASS: " };
                const auto        text_size = g_render->get_text_size( text, g_fonts->get_block( ), 8.f );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_block( ), text.c_str( ), 8 );
                g_render->text_shadow(
                    { sp.x + text_size.x, sp.y },
                    Color( 50, 255, 75 ),
                    g_fonts->get_block( ),
                    "ENABLED",
                    8
                );
            }

            return;

            if ( !m_is_active_seed_area ) return;

            const auto hitbox = m_seed_area;

            for ( auto i = 0; i < hitbox.points.size( ); i++ ) {
                auto start = hitbox.points[ i ];
                auto end   = i == hitbox.points.size( ) - 1 ? hitbox.points[ 0 ] : hitbox.points[ i + 1 ];

                auto line_length = start.dist_to( end );

                auto draw_start = start.extend( end, line_length / 2.f );
                auto box_color  = Color( 255, 255, 255, 255 );

                g_render->line_3d(
                    draw_start.extend( start, line_length / 2.f ),
                    draw_start.extend( end, line_length / 2.f ),
                    box_color,
                    3.f
                );
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            cast_tracking( );
            update_plants( );
            update_seed_area( );

            if ( m_is_active_seed_area && *g_time > m_seed_area_expire_time ) m_is_active_seed_area = false;

            if ( g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::recalling ) {
                spell_w( );
                poke_w( );
            }

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            antigapclose_e( );
            autointerrupt_e( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_r( );
                spell_e( );
                spell_q( );
                poke_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->zyra.q_harass->get< bool >( ) ) {
                    spell_q( );
                    poke_q( );
                }

                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->zyra.e_flee->get< bool >( ) ) spell_e( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->zyra.q_enabled->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_q_range, 0.f, 160.f, 0.875f );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zyra.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto poke_q( ) -> bool{
            if ( !g_config->zyra.q_fertilizer->get< bool >( ) || !g_config->zyra.q_poke_toggle->get< bool >( ) ||
                *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range + 500.f, 0.f, 0.f, 0.8f );
            if ( !pred.valid || pred.position.dist_to( g_local->position ) <= m_w_range ) return false;

            bool allow_q{ };

            const auto extend_point = g_local->position.extend( pred.position, 790.f );

            const auto base_position = extend_point;
            const auto delta         = ( base_position - g_local->position ).normalize( );
            const auto start_point   = base_position.extend( base_position + delta.rotated( 1.58f ), 500.f );
            const auto end_point     = start_point.extend( extend_point, 1000.f );

            const auto rect  = sdk::math::Rectangle( start_point, end_point, 300.f );
            const auto seeds = get_seeds_in_area( rect.to_polygon( 20 ) );
            if ( seeds.size( ) > 0 ) {
                auto lowest_distance{ FLT_MAX };
                Vec3 closest_seed{ };

                for ( auto seed : seeds ) {
                    if ( seed.dist_to( pred.position ) > lowest_distance ) continue;

                    lowest_distance = seed.dist_to( pred.position );
                    closest_seed    = seed;
                }

                const auto minion_distance = get_distance_to_nearest_minion( closest_seed );
                allow_q                    = lowest_distance <= 350.f && minion_distance > lowest_distance &&
                ( !g_config->zyra.w_turret_check->get< bool >( ) ||
                    !is_position_in_turret_range( closest_seed ) );
            }

            if ( !allow_q ) {
                const auto minion_distance = get_distance_to_nearest_minion( extend_point );

                const auto nearest = get_nearest_plant( extend_point, true );
                if ( nearest.length( ) > 0.f && nearest.dist_to( extend_point ) <= 400.f ) return false;

                allow_q = m_slot_w->charges > 0 && m_slot_w->is_ready( ) &&
                    pred.position.dist_to( g_local->position ) < m_w_range + 350.f &&
                    minion_distance > extend_point.dist_to( target->position ) &&
                    ( !g_config->zyra.w_turret_check->get< bool >( ) || !is_position_in_turret_range( pred.position ) );
            }

            if ( !allow_q ) return false;

            if ( cast_spell( ESpellSlot::q, extend_point ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ poke q ] spellcast\n";
                return true;
            }

            return false;
        }

        auto poke_w( ) -> bool{
            if ( !g_config->zyra.w_enabled->get< bool >( ) || !g_config->zyra.q_poke_toggle->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || !m_is_active_seed_area || m_slot_w->charges == 0 ||
                !m_slot_w->is_ready( ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_w_range + 500.f, 0.f, 0.f, 0.25f );
            if ( !pred.valid || pred.position.dist_to( g_local->position ) > m_w_range + 400.f || pred.position.dist_to(
                g_local->position
            ) < m_w_range )
                return false;

            const auto extend_point = g_local->position.extend( pred.position, 840.f );
            if ( extend_point.dist_to( m_last_w_position ) <= 100.f && *g_time - m_last_w_time <= 1.f ) return false;

            const auto minion_distance = get_distance_to_nearest_minion( extend_point );
            if ( minion_distance <= pred.position.dist_to( extend_point ) ) return false;

            const auto nearest = get_nearest_plant( extend_point, true );
            if ( nearest.length( ) > 0.f && nearest.dist_to( extend_point ) <= 400.f
                || m_seed_area.is_outside( extend_point ) || m_seed_area_expire_time - *g_time > 0.65f )
                return false;

            if ( cast_spell( ESpellSlot::w, extend_point ) ) {
                m_last_cast_time  = *g_time;
                m_last_w_time     = *g_time;
                m_last_w_position = extend_point;

                std::cout << "[ poke w ] spellcast\n";
                return true;
            }

            return false;
        }


        auto spell_w( ) -> bool override{
            if ( !g_config->zyra.w_enabled->get< bool >( ) || *g_time - m_last_w_time <= 0.05f ||
                *g_time - m_last_cast_time <= 0.05f || !m_is_active_seed_area || m_slot_w->charges == 0 || !m_slot_w->
                is_ready( ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( w_logic( target ) ) return true;
            }

            return false;
        }

        auto w_logic( Object* target ) -> bool{
            if ( !target ) return false;

            if ( g_config->zyra.w_save_charge->get< bool >( ) && m_slot_w->charges == 1 &&
                ( g_config->zyra.w_preferred_plant->get< int >( ) == 0 && m_seed_area_slot != 0 ||
                    g_config->zyra.w_preferred_plant->get< int >( ) == 1 && m_seed_area_slot != 2 ) )
                return false;

            const auto pred = g_features->prediction->predict_default( target->index, 0.125f );
            if ( !pred ) return false;

            auto seed_point = get_best_seed_point( target->position, *pred, target->index );
            if ( seed_point.length( ) <= 0.f || !m_seed_area.is_inside( seed_point ) ) {
                if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo ) {
                    seed_point = get_best_seed_point( target->position, *pred, target->index, true );
                    if ( seed_point.length( ) <= 0.f || !m_seed_area.is_inside( seed_point ) ) return false;
                } else return false;
            }

            const auto cast_position = seed_point;
            if ( g_local->position.dist_to( cast_position ) > m_w_range + 5.f ||
                cast_position.dist_to( m_last_w_position ) < 50.f && *g_time - m_last_w_time <= 1.f ||
                g_config->zyra.w_turret_check->get< bool >( ) && is_position_in_turret_range( cast_position ) )
                return false;

            if ( cast_spell( ESpellSlot::w, cast_position ) ) {
                m_last_cast_time  = *g_time;
                m_last_w_time     = *g_time;
                m_last_w_position = cast_position;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->zyra.e_enabled->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.1f || !m_slot_e->is_ready( true ) )
                return false;

            if ( g_config->zyra.e_multihit->get< bool >( ) ) {
                const auto multihit = get_multihit_position( m_e_range, 1150.f, 70.f, 0.25f, true );

                if ( multihit.hit_count > 1 ) {
                    if ( cast_spell( ESpellSlot::e, multihit.position ) ) {
                        m_last_e_time    = *g_time;
                        m_last_cast_time = *g_time;
                        g_features->orbwalker->on_cast( );
                        return true;
                    }
                }
            }

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_e_range * ( g_config->zyra.e_max_range->get< int >( ) / 100.f ),
                    1150.f,
                    70.f,
                    0.25f,
                    { },
                    true,
                    Prediction::include_ping,
                    Prediction::ESpellType::linear
                );

            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zyra.e_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->zyra.r_enabled->get< bool >( ) && !g_config->zyra.r_multihit->get< bool >( ) ||
                *g_time - m_last_r_time <= 0.4f || *g_time - m_last_cast_time <= 0.1f || !m_slot_r->is_ready( true ) )
                return false;

            if ( multihit_r( ) ) return true;

            if ( !g_config->zyra.r_enabled->get< bool >( ) || g_config->zyra.r_only_full_combo->get< bool >( ) && !
                GetAsyncKeyState( VK_CONTROL ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict( target->index, m_r_range, 0.f, 500.f, 2.f );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->zyra.r_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::r, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                return true;
            }

            return false;
        }

        auto multihit_r( ) -> bool{
            if ( !g_config->zyra.r_multihit->get< bool >( ) ) return false;

            const auto            multihit = get_circle_multihit(
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::r, unit ); },
                m_r_range,
                500.f
            );

            auto allow_cast{ multihit.hit_count > 2 };

            int plant_count{ };

            if ( !allow_cast && multihit.hit_count > 1 ) {
                plant_count = get_active_plants_in_circle( multihit.position, 500.f, 0.75f );

                allow_cast = plant_count > 1;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                std::cout << "[ multihit r ] count: " << multihit.hit_count << " | plant count: " << plant_count
                    << std::endl;

                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );
                return true;
            }

            return false;
        }

        auto antigapclose_e( ) -> void{
            if ( !g_config->zyra.e_antigapclose->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_e_range, 1150.f, 70.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1150.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_cast_time = *g_time;
                m_last_e_time    = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto autointerrupt_e( ) -> void{
            if ( !g_config->zyra.e_autointerrupt->get< bool >( ) || *g_time - m_last_e_time <= 0.4f || !m_slot_e->
                is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_e_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_e_range,
                1150.f,
                70.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance <= 3 ) return;

            if ( cast_spell( ESpellSlot::e, pred.position ) ) {
                m_last_cast_time = *g_time;
                m_last_e_time    = *g_time;
                g_features->orbwalker->on_cast( );
            }
        }

        auto get_best_seed_point(
            const Vec3&   target_position,
            const Vec3&   future_position,
            const int16_t target_index,
            const bool    low_requirements = false
        ) -> Vec3{
            const auto seed_points = g_render->get_3d_circle_points(
                future_position,
                low_requirements ? ( m_seed_area_slot == 2 ? 150.f : 200.f ) : m_seed_area_slot == 2 ? 100.f : 150.f,
                20
            );

            const auto nearest_plant = get_nearest_plant( target_position );
            const auto is_plant_near = nearest_plant.length( ) > 0.f;

            Vec3  best_point{ };
            float best_weight{ };

            const auto attack_range = m_seed_area_slot == 0 ? 575.f : 400.f;

            possible_seed_points = seed_points;

            std::vector< Vec3 > selected_points{ };

            for ( auto point : seed_points ) {
                if ( point.dist_to( g_local->position ) > m_w_range || g_navgrid->is_wall( point ) || m_seed_area.
                    is_outside( point ) )
                    continue;

                const auto distance = point.dist_to( target_position ) + point.dist_to( future_position );
                auto       weight   = 1.f - std::min( distance / 500.f, 1.f );

                if ( future_position.dist_to( g_local->position ) + 10.f <
                    target_position.dist_to( g_local->position ) )
                    weight += 0.5f;

                if ( is_plant_near ) weight -= 1.f - std::min( point.dist_to( nearest_plant ) / 400.f, 1.f );

                const auto nearby_enemies = helper::get_nearby_champions_count( point, false, attack_range );
                const auto enemy_weight   = ( nearby_enemies - 1 ) / 4.f;
                weight += enemy_weight;

                if ( get_plant_targeting_count( target_position, target_index ) > 0 ) weight -= 0.25f;

                if ( weight < best_weight ) continue;

                best_point  = point;
                best_weight = weight;
                selected_points.push_back( point );
            }

            m_selected_seed_points = selected_points;

            return best_point;
        }

        auto get_plant_targeting_count( const Vec3& target_position, const int16_t index ) const -> int{
            int count{ };

            for ( auto plant : m_plants ) {
                if ( plant.type == EPlantType::seed ) continue;

                if ( plant.position.dist_to( target_position ) > plant.attack_range ||
                    plant.last_target_index != index )
                    continue;

                ++count;
            }

            return count;
        }

        auto get_nearest_plant( const Vec3& position, const bool ignore_seed_area = false ) const -> Vec3{
            auto lowest_distance{ FLT_MAX };
            Vec3 nearest_position{ };

            for ( auto plant : m_plants ) {
                if ( plant.type == EPlantType::seed && !ignore_seed_area && m_seed_area.
                    is_outside( plant.position ) )
                    continue;

                const auto attack_range = plant.type == EPlantType::seed
                                              ? ( m_seed_area_slot == 2 && !ignore_seed_area ? 400.f : 575.f )
                                              : plant.attack_range;

                if ( plant.position.dist_to( position ) > lowest_distance ||
                    plant.position.dist_to( position ) > attack_range )
                    continue;


                nearest_position = plant.position;
                lowest_distance  = plant.position.dist_to( position );
            }

            return nearest_position;
        }

        auto get_seeds_in_area( const sdk::math::Polygon& area, float delay = 0.5f ) const -> std::vector< Vec3 >{
            int                 seed_count{ };
            std::vector< Vec3 > seed_points{ };

            for ( auto inst : m_spawning_seeds ) {
                if ( area.is_outside( inst.position ) ) continue;

                seed_points.push_back( inst.position );
            }

            for ( auto inst : m_plants ) {
                if ( inst.type != EPlantType::seed || area.is_outside( inst.position ) ) continue;

                seed_points.push_back( inst.position );
            }

            return seed_points;
        }

        auto get_active_plants_in_circle(
            const Vec3& source_position,
            const float radius,
            const float delay = 0.75f
        ) const -> int{
            int count{ };

            for ( auto plant : m_plants ) {
                if ( plant.type == EPlantType::seed || plant.end_time - *g_time <= delay || source_position.dist_to(
                    plant.position
                ) > radius )
                    continue;

                ++count;
            }

            return count;
        }

        auto cast_tracking( ) -> void{
            if ( !m_cast_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->is_autoattack || *g_time > sci->server_cast_time || sci->slot != 0 && sci->slot !=
                    2 )
                    return;

                m_cast_active      = true;
                m_start_position   = sci->start_position;
                m_end_position     = sci->end_position;
                m_cast_slot        = sci->slot;
                m_server_cast_time = sci->server_cast_time;

                return;
            }

            if ( *g_time > m_server_cast_time ) {
                m_cast_active = false;
                return;
            }

            if ( m_is_active_seed_area && m_cast_slot == m_seed_area_slot ) return;

            switch ( m_cast_slot ) {
            case 0:
            { // Q

                const auto base_position = m_end_position;
                const auto delta         = ( base_position - m_start_position ).normalize( );
                const auto start_point   = base_position.extend( base_position + delta.rotated( 1.58f ), 500.f );
                const auto end_point     = start_point.extend( m_end_position, 1000.f );

                const auto rect         = sdk::math::Rectangle( start_point, end_point, 300.f );
                m_seed_area.points      = rect.to_polygon( ).points;
                m_seed_area_expire_time = m_server_cast_time + 0.8f;
                m_seed_area_slot        = m_cast_slot;
                m_is_active_seed_area   = true;

                break;
            }
            case 2:
            { // E
                m_seed_area_slot = m_cast_slot;
                break;
            }
            default:
                break;
            }

            update_seed_area( );
        }

        auto update_seed_area( ) -> void{
            if ( m_cast_slot == 2 ) {
                auto distance_traveled = m_server_cast_time > *g_time ? 0.f : ( *g_time - m_server_cast_time ) * 1150.f;
                if ( distance_traveled > 1100.f ) distance_traveled = 1100.f;

                const auto missile_position = m_start_position.extend( m_end_position, distance_traveled );

                const auto rect = sdk::math::Rectangle(
                    missile_position,
                    missile_position.extend( m_start_position, -150.f ),
                    150.f
                );
                m_seed_area.points      = rect.to_polygon( 150.f ).points;
                m_seed_area_expire_time = m_server_cast_time + 1150.f / 1100.f;
                m_seed_area_slot        = m_cast_slot;
                m_is_active_seed_area   = true;
            }
        }

        // ZyraQPlant - thornspitter duration buff
        // ZyraW - seed duration buff
        // ZyraEPlant - vinelasher duration buff
        // zyraplantenrage - r empowered plant buff

        auto update_plants( ) -> void{
            for ( const auto missile : g_entity_list.get_ally_missiles( ) ) {
                if ( !missile || missile->missile_start_position.dist_to( g_local->position ) > 100.f
                    || is_spawning_seed_tracked( missile->network_id ) || *g_time >= missile->missile_spawn_time( ) +
                    0.5f )
                    continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                const auto missile_hash = rt_hash( data->get_name( ).data( ) );
                if ( missile_hash != ct_hash( "ZyraPSeedMis" ) ) continue;

                m_spawning_seeds.push_back(
                    {
                        missile->index,
                        missile->network_id,
                        missile->missile_end_position,
                        missile->missile_spawn_time( ),
                        missile->missile_spawn_time( ) + 1.f
                    }
                );
            }

            for ( const auto inst : m_spawning_seeds ) {
                if ( inst.end_time + 0.1f <= *g_time ) {
                    remove_spawning_seed( inst.network_id );
                    continue;
                }
            }

            for ( const auto object : g_entity_list.get_ally_minions( ) ) {
                if ( !object || object->is_dead( ) || object->is_invisible( ) || object->dist_to_local( ) > 1500.f ||
                    !object->is_zyra_plant( ) || is_plant_tracked( object->network_id ) )
                    continue;

                PlantInstance instance{ };

                instance.index      = object->index;
                instance.network_id = object->network_id;
                instance.position   = object->position;

                const auto name_hash = rt_hash( object->get_name( ).data( ) );
                switch ( name_hash ) {
                case ct_hash( "ZyraThornPlant" ):
                {
                    instance.type = EPlantType::thornspitter;

                    const auto buff = g_features->buff_cache->get_buff( object->index, ct_hash( "ZyraQPlant" ) );
                    if ( !buff || !buff->buff_data ) continue;

                    instance.start_time = buff->buff_data->start_time;
                    instance.end_time   = buff->buff_data->end_time;

                    instance.attack_range = 575.f;

                    break;
                }
                case ct_hash( "ZyraGraspingPlant" ):
                {
                    instance.type = EPlantType::vinelasher;

                    const auto buff = g_features->buff_cache->get_buff( object->index, ct_hash( "ZyraEPlant" ) );
                    if ( !buff || !buff->buff_data ) continue;

                    instance.start_time = buff->buff_data->start_time;
                    instance.end_time   = buff->buff_data->end_time;

                    instance.attack_range = 400.f;
                    break;
                }
                case ct_hash( "ZyraSeed" ):
                {
                    instance.type = EPlantType::seed;

                    const auto buff = g_features->buff_cache->get_buff( object->index, ct_hash( "ZyraW" ) );
                    if ( !buff || !buff->buff_data ) continue;

                    instance.start_time = buff->buff_data->start_time;
                    instance.end_time   = buff->buff_data->end_time;

                    if ( instance.end_time - instance.start_time < 60.f ) remove_spawning_seed( instance.position );
                    break;
                }
                default:
                    continue;
                }

                m_plants.push_back( instance );
            }

            if ( m_plants.empty( ) ) return;

            for ( auto& inst : m_plants ) {
                if ( inst.end_time <= *g_time ) {
                    if ( inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            inst.network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                            6,
                            1,
                            true
                        );

                        inst.is_glowing = false;
                    }

                    if ( inst.is_glowing ) continue;

                    remove_plant( inst.network_id );
                    continue;
                }

                auto object = g_entity_list.get_by_index( inst.index );
                if ( !object ) {
                    if ( inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            inst.network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                            6,
                            1,
                            true
                        );
                        inst.is_glowing = false;
                    }

                    if ( inst.is_glowing ) continue;
                    remove_plant( inst.network_id );
                    continue;
                }


                switch ( inst.type ) {
                case EPlantType::thornspitter:
                case EPlantType::vinelasher:
                {
                    auto sci = object->spell_book.get_spell_cast_info( );
                    if ( sci && sci->server_cast_time > *g_time && sci->server_cast_time != inst.
                        last_server_cast_time ) {
                        inst.last_target_index     = sci->get_target_index( );
                        inst.last_server_cast_time = sci->server_cast_time;
                    }

                    if ( inst.attackrange_circle.empty( ) ) {
                        inst.attackrange_circle = g_render->get_3d_circle_points(
                            inst.position,
                            inst.type == EPlantType::thornspitter ? 575.f : 400.f,
                            60
                        );
                    }
                }
                break;
                default:
                    break;
                }

                object.update( );

                if ( object->is_dead( ) ) {
                    if ( inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            inst.network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                            6,
                            1,
                            true
                        );

                        inst.is_glowing = false;
                    }

                    if ( inst.is_glowing ) continue;
                    remove_plant( inst.network_id );
                    continue;
                }

                if ( inst.type == EPlantType::seed ) {
                    if ( !inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                        const auto rainbow    = Color::white( );
                        const auto glow_color = D3DCOLOR_ARGB( 255, rainbow.r, rainbow.g, rainbow.b );
                        g_function_caller->enable_glow( inst.network_id, glow_color, 3, 0, false );

                        inst.is_glowing            = true;
                        inst.last_glow_update_time = *g_time;
                    }
                } else if ( !inst.is_glowing && g_function_caller->is_glow_queueable( ) ) {
                    const auto plant_color = Color( 0, 0, 0 );

                    const auto glow_color = D3DCOLOR_ARGB( 255, plant_color.r, plant_color.g, plant_color.b );
                    g_function_caller->enable_glow( inst.network_id, glow_color, 4, 0, false );

                    inst.is_glowing            = true;
                    inst.last_glow_update_time = *g_time;
                }
            }
        }

        auto draw_plants( ) const -> void{
            for ( auto inst : m_spawning_seeds ) {
                Vec2 sp{ };

                if ( !world_to_screen( inst.position, sp ) ) continue;

                auto data = std::to_string( inst.end_time - *g_time );
                data.resize( 4 );
                std::string text = " 30.0 ";

                auto text_size = g_render->get_text_size( text, g_fonts->get_bold_16px( ), 16 );

                Vec2 background_start = { sp.x - text_size.x / 2.f - 1.f, sp.y - text_size.y / 2.f - 1.f };
                Vec2 background_size  = { text_size.x + text_size.y + 1.f, text_size.y };

                background_start.x -= background_size.y / 2.f;

                g_render->filled_box( background_start, background_size, Color( 20, 20, 20, 225 ) );

                static auto Zyra_w =                         path::join(
                                            {
                                                directory_manager::get_resources_path( ),
                                                "champions",
                                                "Zyra",
                                                "spells",
                                                "Zyra_w.png"
                                            }
                                        );
                auto texture =
                    g_render->load_texture_from_file(
                        Zyra_w.has_value(  ) ? *Zyra_w : ""
                    );


                Vec2 texture_size{ background_size.y, background_size.y };

                if ( texture ) {
                    g_render->image(
                        { background_start.x, background_start.y },
                        texture_size,
                        texture
                    );
                }

                /*
                Vec2 text_position = { background_start.x + texture_size.x, background_start.y };

                g_render->text_shadow(
                    text_position, color( 255, 255, 255 ), g_fonts->get_bold_16px( ), text.data( ), 16 );*/

                auto max_width = text_size.x + 1.f;

                auto modifier = 1.f - ( inst.end_time - *g_time ) / 1.f;
                if ( modifier > 1.f ) modifier = 1.f;

                Vec2 bar_start = { background_start.x + texture_size.x, background_start.y };
                Vec2 bar_size  = { max_width * modifier, background_size.y };

                g_render->filled_box(
                    bar_start,
                    bar_size,
                    Color( 255.f - 190.f * modifier, 255.f * modifier, 25.f, 255.f )
                );

                g_render->box( background_start, background_size, Color( 5, 5, 5, 255 ), -1 );
            }

            std::vector< sdk::math::Polygon > poly_list{ };

            for ( auto plant : m_plants ) {
                switch ( plant.type ) {
                case EPlantType::thornspitter:
                case EPlantType::vinelasher:
                {
                    poly_list.push_back( sdk::math::Polygon{ plant.attackrange_circle } );
                    break;
                }
                case EPlantType::seed:
                {
                    Vec2 sp{ };

                    if ( !world_to_screen( plant.position, sp ) ) continue;

                    auto data = std::to_string( plant.end_time - *g_time );
                    data.resize( 4 );
                    auto text = " " + data + " ";

                    auto text_size = g_render->get_text_size( text, g_fonts->get_bold_16px( ), 16 );

                    Vec2 background_start = { sp.x - text_size.x / 2.f - 1.f, sp.y - text_size.y / 2.f - 1.f };
                    Vec2 background_size  = { text_size.x + text_size.y + 1.f, text_size.y + 1.f };

                    background_start.x -= background_size.y / 2.f;

                    g_render->filled_box( background_start, background_size, Color( 10, 10, 10, 200 ) );


                    static auto Zyra_w =                             path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    "Zyra",
                                    "spells",
                                    "Zyra_w.png"
                                }
                            );
                    auto texture =
                        g_render->load_texture_from_file(
                            Zyra_w.has_value(  ) ? *Zyra_w : ""
                        );


                    Vec2 texture_size{ background_size.y, background_size.y };

                    if ( texture ) g_render->image( { background_start.x, background_start.y }, texture_size, texture );


                    Vec2 text_position = { background_start.x + texture_size.x, background_start.y };

                    auto border_color = Color( 75, 75, 75, 255 );

                    if ( plant.end_time - plant.start_time <= 40.f ) {
                        auto modifier = std::max( 1.f - ( *g_time - plant.start_time ) / 1.25f, 0.f );
                        if ( modifier > 1.f ) modifier = 1.f;

                        border_color = Color(
                            75.f + 180.f * modifier,
                            75.f + 180.f * modifier,
                            75.f + 180.f * modifier,
                            255.f
                        );
                    }

                    g_render->text_shadow(
                        text_position,
                        Color( 255, 255, 255 ),
                        g_fonts->get_bold_16px( ),
                        text.data( ),
                        16
                    );

                    g_render->box( background_start, background_size, border_color, -1 );

                    continue;

                    auto max_width = text_size.x + 1.f;

                    auto modifier = 1.f - ( plant.end_time - *g_time ) / ( plant.end_time - plant.start_time );
                    if ( modifier > 1.f ) modifier = 1.f;

                    Vec2 bar_start = { background_start.x + texture_size.x, background_start.y };
                    Vec2 bar_size  = { max_width * modifier, background_size.y };

                    g_render->filled_box(
                        bar_start,
                        bar_size,
                        Color( 255 - 200 * modifier, 255 * modifier, 75 * modifier, 125 + 130 * modifier )
                    );

                    break;
                }
                default:
                    continue;
                }

                auto object = g_entity_list.get_by_index( plant.index );
                if ( !object ) continue;

                object.update( );
            }

            auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );

            for ( const auto poly : draw_poly )
                g_render->polygon_3d(
                    poly,
                    Color( 255, 255, 255, 155 ),
                    Renderer::outline,
                    1.f
                );
        }

        static auto get_distance_to_nearest_minion( const Vec3& position ) -> float{
            auto lowest_distance{ FLT_MAX };

            for ( const auto enemy : g_entity_list.get_enemy_minions( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                    !enemy->is_lane_minion( ) && !enemy->is_jungle_monster( ) )
                    continue;

                const auto distance = enemy->position.dist_to( position );
                if ( distance > lowest_distance ) continue;

                lowest_distance = distance;
            }


            return lowest_distance;
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ get_slot_q( )->level ] + g_local->ability_power( ) * 0.6f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ get_slot_e( )->level ] + g_local->ability_power( ) * 0.5f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ get_slot_r( )->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::e:
            {
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1150.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                return 0.25f + g_local->position.dist_to( pred.value( ) ) / 1150.f;
            }
            case ESpellSlot::q:
                return 0.875f;
            case ESpellSlot::r:
                return 1.f;
            default:
                return 0.f;
            }

            return std::numeric_limits< float >::max( );
        }

        auto remove_spawning_seed( const Vec3& position ) -> void{
            for ( auto inst : m_spawning_seeds ) {
                if ( inst.position.dist_to( position ) <= 25.f ) {
                    remove_spawning_seed( inst.network_id );
                    return;
                }
            }
        }

        auto remove_plant( const unsigned network_id ) -> void{
            if ( m_plants.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_plants,
                    [ & ]( const PlantInstance& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_plants.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_plant_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_plants ) if ( inst.network_id == network_id ) return true;

            return false;
        }

        auto remove_spawning_seed( const unsigned network_id ) -> void{
            if ( m_spawning_seeds.empty( ) ) return;

            const auto to_remove =
                std::ranges::remove_if(
                    m_spawning_seeds,
                    [ & ]( const SpawningSeedInstance& instance ) -> bool{ return instance.network_id == network_id; }
                );

            if ( to_remove.empty( ) ) return;

            m_spawning_seeds.erase( to_remove.begin( ), to_remove.end( ) );
        }

        auto is_spawning_seed_tracked( const unsigned network_id ) const -> bool{
            for ( const auto inst : m_spawning_seeds ) if ( inst.network_id == network_id ) return true;

            return false;
        }

    private:
        float m_last_q_time{ };
        float m_last_w_time{ };
        float m_last_e_time{ };
        float m_last_r_time{ };

        float m_last_cast_time{ };

        Vec3 m_last_w_position{ };

        // cast track
        bool  m_cast_active{ };
        float m_server_cast_time{ };
        Vec3  m_start_position{ };
        Vec3  m_end_position{ };
        int   m_cast_slot{ };

        // seed spawn tracking
        std::vector< SpawningSeedInstance > m_spawning_seeds{ };

        // plant tracking
        std::vector< PlantInstance > m_plants{ };

        std::vector< Vec3 > possible_seed_points{ };
        std::vector< Vec3 > m_selected_seed_points{ };


        // seed area
        bool               m_is_active_seed_area{ };
        sdk::math::Polygon m_seed_area{ };
        float              m_seed_area_expire_time{ };
        int                m_seed_area_slot{ };

        std::array< float, 6 > m_q_damage{ 0.f, 50.f, 95.f, 130.f, 165.f, 200.f };
        std::array< float, 6 > m_e_damage{ 0.f, 60.f, 105.f, 150.f, 195.f, 240.f };
        std::array< float, 6 > m_r_damage{ 0.f, 180.f, 265.f, 350.f };

        float m_q_range{ 800.f };
        float m_w_range{ 850.f };
        float m_e_range{ 1100.f };
        float m_r_range{ 700.f };
    };
}
