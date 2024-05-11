#pragma once
#include "module.hpp"
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"

namespace features::champion_modules {
    class lulu_module final : public IModule {
    public:

        struct w_blacklist_instance {

            int16_t index{};
            unsigned network_id{};

            float blacklist_time{};
        };

        virtual ~lulu_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "lulu_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Lulu" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation    = g_window->push( _( "lulu" ), menu_order::champion_module );
            const auto q_settings    = navigation->add_section( _( "q settings" ) );
            const auto drawings      = navigation->add_section( _( "drawings" ) );
            const auto w_settings    = navigation->add_section( _( "w settings" ) );
            const auto e_settings    = navigation->add_section( _( "e settings" ) );
            const auto r_settings    = navigation->add_section( _( "r settings" ) );
            const auto ally_priority = navigation->add_section( _( "priority ally" ) );
            const auto hotkey        = navigation->add_section( _( "semi manual" ) );

            // Q settings
            q_settings->checkbox( _( "enable" ), g_config->lulu.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->lulu.q_harass );
            q_settings->checkbox( _( "flee q" ), g_config->lulu.q_flee );
            q_settings->checkbox( _( "killsteal q" ), g_config->lulu.q_killsteal );
            q_settings->checkbox( _( "antigapclose q" ), g_config->lulu.q_antigapclose );
            q_settings->checkbox( _( "multihit q" ), g_config->lulu.q_multihit );

            q_settings->select(
                _( "hitchance" ),
                g_config->lulu.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );

            q_settings->slider_int( _( "max range %" ), g_config->lulu.q_max_range, 75, 100, 1 );

            // W settings
            w_settings->checkbox( _( "enable" ), g_config->lulu.w_enabled );
            w_settings->checkbox( _( "autointerrupt w" ), g_config->lulu.w_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            w_settings->checkbox( _( "always w enemy in range" ), g_config->lulu.w_enemies );
            w_settings->checkbox( _( "w flee" ), g_config->lulu.w_flee );

            // E settings
            e_settings->checkbox( _( "enable" ), g_config->lulu.e_enabled );
            e_settings->checkbox( _( "killsteal e" ), g_config->lulu.e_killsteal );
            e_settings->multi_select(
                _( "e block damage from " ),
                {
                    g_config->lulu.e_block_damage_from_autoattacks,
                    g_config->lulu.e_block_damage_from_spells,
                    g_config->lulu.e_block_damage_from_turret_shots,
                    g_config->lulu.e_block_damage_from_ignite,
                    g_config->lulu.e_block_damage_from_poison,
                    g_config->lulu.e_block_damage_from_item_burn
                },
                {
                    _( "Autoattacks" ),
                    _( "Skillshots" ),
                    _( "Turret shots" ),
                    _( "Ignite" ),
                    _( "Poison" ),
                    _( "Liandrys burn" )
                }
            );

            e_settings->slider_int(
                _( "min skillshot danger to shield" ),
                g_config->lulu.e_skillshot_minimum_danger,
                1,
                5,
                1
            );
            e_settings->checkbox( _( "buff ally autoattack" ), g_config->lulu.e_increase_damage );
            e_settings->checkbox( _( "allow e self" ), g_config->lulu.e_self );

            // R settings
            r_settings->checkbox( _( "enable" ), g_config->lulu.r_enabled );
            r_settings->select(
                _( "target selection" ),
                g_config->lulu.r_logic,
                { _( "Everyone" ), _( "Priority ally only" ) }
            );
            r_settings->slider_int( _( "min multihit count" ), g_config->lulu.r_multihit_count, 1, 5, 1 );
            r_settings->checkbox( _( "autointerrupt r" ), g_config->lulu.r_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Malzahar R, etc" )
            );
            r_settings->checkbox( _( "antigapclose r" ), g_config->lulu.r_antigapclose );
            r_settings->checkbox( _( "antimelee r" ), g_config->lulu.r_antimelee );
            r_settings->slider_int( _( "min hp%" ), g_config->lulu.r_health_threshold, 10, 100, 1 );

            // Ally priority settings
            ally_priority->checkbox( _( "enable" ), g_config->lulu.ally_priority_enabled )->set_tooltip(
                _( "Press MB5 to select a priority ally to buff with E/W" )
            );
            ally_priority->multi_select(
                _( "w ally if " ),
                {
                    g_config->lulu.ally_buff_on_chase,
                    g_config->lulu.ally_buff_on_flee,
                    g_config->lulu.ally_buff_in_combat
                },
                { _( "Chasing" ), _( "Fleeing" ), _( "In combat" ) }
            );

            ally_priority->checkbox( _( "only buff selected ally (?)" ), g_config->lulu.only_buff_priority_ally )->
                           set_tooltip( _( "When priority ally is nearby, will not cast E/W on other allies" ) );

            hotkey->checkbox( _( "enable [ hotkey: T ]" ), g_config->lulu.semi_manual_we );
            hotkey->multi_select(
                _( "semi manual toggle " ),
                {
                    g_config->lulu.semi_manual_e,
                    g_config->lulu.semi_manual_w
                },
                { _( "E" ), _( "W" ) }
            );
            //only_buff_priority_ally

            drawings->checkbox( _( "draw q range" ), g_config->lulu.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->lulu.e_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->lulu.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->lulu.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lulu.dont_draw_on_cooldown->get<
                    bool >( ) ) ) {
                    if ( false && m_pixie_found ) {
                        std::vector< sdk::math::Polygon > poly_list{
                            {
                                g_render->get_3d_circle_points(
                                    g_local->position,
                                    m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                                    60
                                )
                            },
                            {
                                g_render->get_3d_circle_points(
                                    get_pixie_position( ),
                                    m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                                    60
                                )
                            }
                        };

                        const auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );

                        for ( const auto poly : draw_poly )
                            g_render->polygon_3d(
                                poly,
                                Color( 31, 88, 255, 255 ),
                                Renderer::outline,
                                2.f
                            );
                    } else {
                        g_render->circle_3d(
                            g_local->position,
                            Color( 31, 88, 255, 255 ),
                            m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                            Renderer::outline,
                            60,
                            2.f
                        );
                    }
                }
            }

            if ( g_config->lulu.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 && ( slot->is_ready( true ) || !g_config->lulu.dont_draw_on_cooldown->get<
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

            for ( auto inst : m_blacklist ) {

                auto object = g_entity_list.get_by_index(inst.index);
                if (!object || object->is_dead() || object->is_invisible()) continue;

                Vec2 sp{};
                if (!world_to_screen(object->position, sp)) continue;

                std::string text = "W BLACKLIST";
                auto size = g_render->get_text_size(text, g_fonts->get_zabel_16px(), 16);

                Vec2 position = { sp.x - size.x / 2.f, sp.y - size.y / 2.f };
                g_render->text_shadow(position, Color(255, 255, 255), g_fonts->get_zabel_16px(), text.data(), 16);
            }

            if ( m_ally_selected ) {
                auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally ) {
                    ally.update( );

                    auto draw_color = Color( 255, 255, 255 );

                    if ( *g_time - m_ally_selection_time <= 1.7f && *g_time - m_ally_selection_time >= 0.75f ) {
                        auto mod = std::clamp( ( *g_time - m_ally_selection_time - 0.75f ) / 1.f, 0.f, 1.f );
                        mod      = utils::ease::ease_out_quint( mod );

                        auto circle_color = draw_color;
                        circle_color.alpha( 255 - 255 * mod );

                        g_render->circle_3d(
                            ally->position,
                            circle_color,
                            80.f + 150.f * mod,
                            Renderer::outline,
                            50,
                            2.f,
                            360.f,
                            ( g_local->position - ally->position ).normalize( )
                        );
                    }

                    if ( *g_time - m_ally_selection_time > 0.75f ) {
                        auto start_extend = g_local->position.extend( ally->position, 80.f );
                        auto extended     = g_local->position.extend( ally->position, ally->dist_to_local( ) - 80.f );


                        auto max_thickness = 6.f;
                        auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 2500.f );

                        /* g_render->circle_3d( ally->position,
                                             draw_color,
                                             80.f,
                                             c_renderer::outline,
                                             32,
                                             3.f,
                                             360.f,
                                             ( g_local->position - ally->position ) );*/

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color,
                                thickness
                            );
                        }

                        Vec2 screen_position{ };
                        if ( world_to_screen(
                            g_local->position.extend( ally->position, ally->dist_to_local( ) / 2.f ),
                            screen_position
                        ) ) {
                            std::string text{ };

                            if ( ally->dist_to_local( ) > m_e_range ) {
                                auto distance = ally->dist_to_local( ) - m_e_range;

                                text = std::to_string( static_cast< int >( distance ) ) + "m";
                            } else {
                                auto distance = m_e_range - ally->dist_to_local( );

                                text = std::to_string( static_cast< int >( distance ) ) + "m";
                            }

                            auto size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                            Vec2 text_position{ screen_position.x - size.x / 2.f, screen_position.y - size.y / 2.f };

                            Vec2 background_position = {
                                screen_position.x - size.x / 2.f,
                                screen_position.y - size.y / 2.f
                            };
                            auto background_size = size;

                            g_render->filled_box( background_position, background_size, Color( 5, 5, 5, 200 ), 0 );

                            g_render->text_shadow(
                                text_position,
                                ally->dist_to_local( ) <= m_e_range ? Color( 60, 255, 20 ) : Color( 255, 255, 25 ),
                                g_fonts->get_zabel_16px( ),
                                text.data( ),
                                16
                            );
                        }
                    } else {
                        auto modifier       = std::clamp( ( *g_time - m_ally_selection_time ) / 1.f, 0.f, 1.f );
                        auto eased_modifier = utils::ease::ease_out_quart( modifier );


                        auto extended = g_local->position.extend(
                            ally->position,
                            ( ally->dist_to_local( ) - 80.f ) * eased_modifier
                        );
                        auto start_extend = g_local->position.extend( ally->position, 80.f );

                        auto max_thickness = 6.f;
                        auto thickness     = max_thickness - max_thickness * ( ally->dist_to_local( ) / 2500.f );

                        if ( ally->dist_to_local( ) > 160.f ) {
                            g_render->line_3d(
                                start_extend,
                                extended,
                                draw_color.alpha( 255 * eased_modifier ),
                                thickness
                            );
                        }

                        g_render->circle_3d(
                            ally->position,
                            draw_color,
                            80.f,
                            Renderer::outline,
                            32,
                            3.f,
                            360.f * eased_modifier,
                            ( g_local->position - ally->position )
                        );
                    }
                }
            }

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                sp.x += 30.f;
                sp.y -= 20.f;

                if ( false && m_ally_selected ) {
                    Vec2 texture_size{ 32.f, 32.f };

                    auto ally = g_entity_list.get_by_index( m_ally_index );
                    if ( ally ) {
                        std::string name    = ally->champion_name.text;
                        auto name_square_texture =                             path::join(
                                { directory_manager::get_resources_path( ), "champions", name, name + "_square.png" }
                            );
                        auto        texture = g_render->load_texture_from_file(
                            name_square_texture.has_value(  ) ? *name_square_texture : ""
                        );
                        if ( texture ) {
                            g_render->image(
                                { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f },
                                texture_size,
                                texture
                            );
                        }

                        std::string text = "ALLY SELECTED";
                        auto        size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                        Vec2 text_position{ sp.x + texture_size.x / 2.f, sp.y - size.y / 2.f };

                        g_render->text_shadow(
                            text_position,
                            Color( 60, 255, 20 ),
                            g_fonts->get_zabel_16px( ),
                            text.data( ),
                            16
                        );
                    }
                }

                //sp.y += 16.f;

                std::string text1{ "[ U ] PERMASHIELD: " };
                std::string text_mode{ m_permashield_toggle ? "ON" : "OFF" };

                auto size = g_render->get_text_size( text1, g_fonts->get_zabel_12px( ), 12 );

                g_render->text_shadow( sp, Color( 255, 255, 255 ), g_fonts->get_zabel_12px( ), text1.c_str( ), 12 );

                g_render->text_shadow(
                    { sp.x + size.x, sp.y },
                    m_permashield_toggle ? Color( 50, 255, 50 ) : Color( 255, 75, 50 ),
                    g_fonts->get_zabel_12px( ),
                    text_mode.c_str( ),
                    12
                );

                Vec2 second_position = { sp.x, sp.y + size.y };
                text1 =  "[ J ] BLACKLIST W";

                g_render->text_shadow(second_position, Color(255, 255, 255), g_fonts->get_zabel_12px(), text1.c_str(),
                                      12);

                //auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "2065passivemovespeed" ) );
                /*if ( buff ) {

                    std::string texte{ "DURATION: " };
                    texte += std::to_string( buff->buff_data->end_time - *g_time );


                    size = g_render->get_text_size( texte, g_fonts->get_zabel_12px( ), 12 );
                    g_render->text_shadow( { sp.x, sp.y + size.y },
                                           Color( 255, 200, 255 ),
                                           g_fonts->get_zabel_12px( ),
                                           texte.c_str( ),
                                           12 );
                }*/
            }
        }

        auto run( ) -> void override{
            initialize_spell_slots( );

            update_pixie( );
            select_priority_ally( );

            update_blacklist( );

            if ( !m_key_down && GetAsyncKeyState( 0x55 ) ) {
                m_permashield_toggle = !m_permashield_toggle;
                m_key_down           = true;
            } else if ( m_key_down && !GetAsyncKeyState( 0x55 ) ) m_key_down = false;

            if ( g_features->orbwalker->in_action( ) ) return;

            permashield_ally( );

            spell_e( );
            spell_r( );
            killsteal_e( );
            buff_allies_w( );

            semi_manual_we( );

            //lulufaerieattackaid
            //2065passivemovespeed - shurelya


            if ( g_features->evade->is_active( ) ) return;

            autointerrupt_w( );
            killsteal_q( );
            pixie_killsteal_q( );

            antigapclose_q( );
            pixie_antigapclose_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_w( );
                spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
                if ( g_config->lulu.q_harass->get< bool >( ) ) spell_q( );
                break;
            case Orbwalker::EOrbwalkerMode::flee:
                if ( g_config->lulu.q_flee->get< bool >( ) ) spell_q( );
                flee_w( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->lulu.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            if ( multihit_q( ) ) return true;

            const auto pixie_position = get_pixie_position( );

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( target->position.dist_to( pixie_position ) < target->dist_to_local( ) ) {
                    if ( pixie_q( target ) || direct_q( target ) ) return true;

                    continue;
                }

                if ( direct_q( target ) || pixie_q( target ) ) return true;
            }

            return false;
        }

        auto direct_q( Object* target ) -> bool{
            if ( !target ||
                target->dist_to_local( ) > m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ) )
                return false;

            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                    1450.f,
                    60.f,
                    0.25f,
                    { },
                    true
                );
            if ( !pred.valid ||
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lulu.q_hitchance->get< int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: Q ] Direct cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto pixie_q( Object* target ) -> bool{
            if ( !target || !m_pixie_found ) return false;

            const auto pixie = get_pixie_position( );
            if ( target->position.dist_to( get_pixie_position( ) ) >
                m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ) )
                return false;


            const auto pred =
                g_features->prediction->predict(
                    target->index,
                    m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                    1450.f,
                    60.f,
                    0.25f,
                    pixie,
                    true
                );
            if ( !pred.valid || pred.hitchance < static_cast< Prediction::EHitchance >( g_config->lulu.q_hitchance->get
                < int >( ) ) )
                return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: Q ] Pixie cast at " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto multihit_q( ) -> bool{
            if ( !g_config->lulu.q_multihit->get< bool >( ) || *g_time - m_last_cast_time <= 0.4f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            auto target_one = g_features->target_selector->get_default_target( );
            auto target_two = g_features->target_selector->get_default_target( true );
            if ( !target_one || !target_two || target_one->network_id == target_two->network_id ) return false;

            auto pixie_position = get_pixie_position( );

            Vec3 pred_one{ };
            Vec3 pred_two{ };

            Vec3 pred_one_source{ };
            Vec3 pred_two_source{ };

            for ( auto i = 0; i < 2; i++ ) {
                auto pred = g_features->prediction->predict(
                    i == 0 ? target_one->index : target_two->index,
                    m_q_range * ( g_config->lulu.q_max_range->get< int >( ) / 100.f ),
                    1450.f,
                    60.f,
                    0.25f,
                    i == 0 ? Vec3( ) : pixie_position,
                    true
                );
                if ( !pred.valid ) return false;

                if ( i == 0 ) {
                    pred_one        = pred.position;
                    pred_one_source = g_local->position;
                    continue;
                }

                pred_two        = pred.position;
                pred_two_source = pixie_position;
            }

            auto line_start = pred_one_source;
            auto line_end   = pred_one;

            Vec3 intersection_point{ };
            auto lowest_distance{ 9999.f };
            for ( auto i = 1; i <= 30; i++ ) {
                auto simulated = line_start.extend( line_end, line_start.dist_to( line_end ) / 30.f * i );

                auto simulated_end = pred_two_source.extend( simulated, m_q_range );
                auto closest = g_features->evade->get_closest_line_point( pred_two_source, simulated_end, pred_two );

                if ( closest.dist_to( pred_two ) > lowest_distance ) break;

                intersection_point = simulated;
                lowest_distance    = closest.dist_to( pred_two );
            }

            auto closest_one = g_features->evade->get_closest_line_point(
                pred_one_source,
                pred_one_source.extend( intersection_point, m_q_range ),
                pred_one
            );
            auto closest_two = g_features->evade->get_closest_line_point(
                pred_two_source,
                pred_two_source.extend( intersection_point, m_q_range ),
                pred_two
            );

            auto threshold = 10.f;

            if ( closest_one.dist_to( pred_one ) > threshold || closest_two.dist_to( pred_two ) > threshold ) {
                intersection_point = Vec3( );
                lowest_distance    = 99999.f;

                for ( auto i = 1; i <= 30; i++ ) {
                    auto simulated = pred_two_source.extend( pred_two, pred_two_source.dist_to( pred_two ) / 30.f * i );

                    auto simulated_end = pred_one_source.extend( simulated, m_q_range );
                    auto closest       = g_features->evade->get_closest_line_point(
                        pred_one_source,
                        simulated_end,
                        pred_one
                    );

                    if ( closest.dist_to( pred_one ) > lowest_distance ) break;

                    intersection_point = simulated;
                    lowest_distance    = closest.dist_to( pred_one );
                }

                closest_one = g_features->evade->get_closest_line_point(
                    pred_one_source,
                    pred_one_source.extend( intersection_point, m_q_range ),
                    pred_one
                );
                closest_two = g_features->evade->get_closest_line_point(
                    pred_two_source,
                    pred_two_source.extend( intersection_point, m_q_range ),
                    pred_two
                );

                if ( closest_one.dist_to( pred_one ) > threshold || closest_two.dist_to( pred_two ) > threshold ) {
                    return
                        false;
                }
            }

            if ( intersection_point.dist_to( g_local->position ) < 25.f ||
                intersection_point.dist_to( pixie_position ) < 25.f )
                return false;

            auto cast_position = intersection_point;

            if ( cast_spell( ESpellSlot::q, cast_position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                m_q_position = cast_position;

                std::cout << "[ Lulu: Q ] Multi-target Q | dist " << lowest_distance << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->lulu.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto predicted =
                g_features->prediction->predict( target->index, m_q_range, 1450.f, 60.f, 0.25f, { }, true );
            if ( !predicted.valid ) return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Lulu: Q ] Killsteal, target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto pixie_killsteal_q( ) -> bool{
            if ( !g_config->lulu.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto pixie_position = get_pixie_position( );

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2,
                pixie_position
            );

            if ( !target ) return false;

            const auto predicted = g_features->prediction->predict(
                target->index,
                m_q_range,
                1450.f,
                60.f,
                0.25f,
                pixie_position,
                true
            );
            if ( !predicted.valid ) return false;

            if ( cast_spell( ESpellSlot::q, predicted.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Lulu: Q ] Pixie killsteal, target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_q( ) -> void{
            if ( !g_config->lulu.q_antigapclose->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_advanced_antigapclose_target( m_q_range, 1450.f, 60.f, 0.25f, true );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1450.f,
                60.f,
                0.25f,
                { },
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ Lulu: Q ] Antigapclose on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto pixie_antigapclose_q( ) -> void{
            if ( !g_config->lulu.q_antigapclose->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto pixie_position = get_pixie_position( );

            const auto target = get_advanced_antigapclose_target(
                m_q_range,
                1450.f,
                60.f,
                0.25f,
                true,
                pixie_position
            );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                1450.f,
                60.f,
                0.25f,
                pixie_position,
                true
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                debug_log(
                    "[ Lulu: Q ] Pixie antigapclose on {} | hitchance: {}",
                    target->champion_name.text,
                    ( int )pred.hitchance
                );
            }
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->lulu.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            if ( polymorph_w( ) || buff_allies_w( ) ) return true;

            return false;
        }

        auto polymorph_w( ) -> bool{
            if ( !g_config->lulu.w_enemies->get< bool >( ) || *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            unsigned target_nid{ };
            auto     target_priority{ 0 };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_w_range || enemy->is_dead( ) || enemy->is_invisible( ) ||
                    enemy->get_selectable_flag( ) != 1 || is_polymorph_blacklisted( enemy->index ) )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority( enemy->champion_name.text );
                if ( priority < target_priority ) continue;


                target_nid      = enemy->network_id;
                target_priority = priority;
            }

            if ( target_nid == 0 ) return false;

            if ( cast_spell( ESpellSlot::w, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Combo\n";
                return true;
            }


            return false;
        }

        auto buff_allies_w( ) -> bool{
            if (!g_config->lulu.w_enabled->get<bool>() ||
                !g_config->lulu.ally_buff_in_combat->get<bool>() && !g_config->lulu.ally_buff_on_chase->get<bool>() &&
                    !g_config->lulu.ally_buff_on_flee->get<bool>() ||
                *g_time - m_last_cast_time <= 0.025f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            if ( buff_priority_ally( ) || m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                m_ally_distance <= m_nearby_threshold )
                return true;

            const Object* target{ };
            auto          target_priority{ -1 };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->network_id == g_local->network_id || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 ||
                    ally->dist_to_local( ) > m_w_range )
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                    sci->server_cast_time < *g_time + g_features->orbwalker->get_ping( ) / 2.f )
                    continue;

                const auto attack_target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !attack_target || !attack_target->is_hero( ) ) continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target          = ally;
            }

            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Buff ally attackspeed, target: " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto permashield_ally( ) -> bool{
            if ( !m_permashield_toggle || !m_ally_selected || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.025f || !m_slot_e->is_ready( true ) )
                return false;


            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ) return false;

            ally.update( );
            if ( ally->dist_to_local( ) > m_e_range ) return false;

            const auto ally_pred  = g_features->prediction->predict_default(ally->index, 0.25f);
            const auto local_pred = g_features->prediction->predict_default(g_local->index, 0.2f);
            if (!ally_pred || !local_pred) return false;

            const auto distance = ally_pred.value( ).dist_to( local_pred.value( ) );
            const auto should_cast{ distance > m_e_range };

            if ( !should_cast ) return false;

            if ( cast_spell( ESpellSlot::e, ally->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] Permashielding priority ally | distance: " << ally->dist_to_local( ) <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto buff_priority_ally( ) -> bool{
            if ( !m_ally_selected || *g_time - m_last_w_time <= 0.4f || *g_time - m_last_cast_time <= 0.05f ||
                !g_config->lulu.ally_buff_in_combat->get<bool>() && !g_config->lulu.ally_buff_on_chase->get<bool>() &&
                    !g_config->lulu.ally_buff_on_flee->get<bool>())
                return false;

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) return false;

            ally.update( );

             auto local_position = g_features->prediction->get_server_position(g_local->index);

            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 || ally->position.dist_to(local_position) > m_e_range)
                return false;

            int cast_reason{ };

            bool allow_cast{ };

            if ( !allow_cast && g_config->lulu.ally_buff_in_combat->get< bool >( ) ) {
                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( sci && sci->server_cast_time > *g_time ) {
                    const auto target_index = sci->get_target_index( );
                    const auto ally_target  = g_entity_list.get_by_index( target_index );

                    if ( ally_target && ally_target->is_hero( ) &&
                        ally_target->position.dist_to( ally->position ) <= 850.f )
                        allow_cast = true;
                }
            }

            if ( allow_cast ) cast_reason = 1;

            if ( !allow_cast ) {
                const auto pred = g_features->prediction->predict_default( ally->index, 0.5f, false );
                if ( !pred ) return false;

                const auto ally_direction{ *pred };
                const auto chase_threshold{ 50.f };

                for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                    if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                        enemy->position.dist_to( ally->position ) > 1000.f )
                        continue;

                    auto enemy_pred = g_features->prediction->predict_default( enemy->index, 0.5f, false );
                    if ( !enemy_pred ) return false;

                    auto enemy_direction = *enemy_pred;

                    auto       v1                   = ally_direction - ally->position;
                    auto       v2                   = enemy->position - ally->position;
                    auto       dot                  = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto ally_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    v1                               = enemy_direction - enemy->position;
                    v2                               = ally->position - enemy->position;
                    dot                              = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto enemy_direction_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    const auto is_chasing_ally{
                        ally_direction.dist_to( enemy->position ) >
                        ally->position.dist_to( enemy->position ) + chase_threshold &&
                        enemy_direction.dist_to( ally->position ) + chase_threshold <
                        enemy->position.dist_to( ally->position ) &&
                        enemy_direction_angle <= 35.f
                    };

                    const auto is_ally_chasing{
                        ally_direction.dist_to( enemy->position ) + chase_threshold <
                        ally->position.dist_to( enemy->position ) &&
                        enemy_direction.dist_to( ally->position ) >
                        enemy->position.dist_to( ally->position ) + chase_threshold &&
                        ally_direction_angle <= 35.f
                    };

                    if ( is_ally_chasing && g_config->lulu.ally_buff_on_chase->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 2;
                        break;
                    }

                    if ( is_chasing_ally && g_config->lulu.ally_buff_on_flee->get< bool >( ) ) {
                        allow_cast  = true;
                        cast_reason = 3;
                        break;
                    }
                }
            }

            if ( !allow_cast ) return false;

            std::string reason_text{ };

            switch ( cast_reason ) {
            case 1:
                reason_text = "In combat";
                break;
            case 2:
                reason_text = "Chasing enemy";
                break;
            case 3:
                reason_text = "Fleeing enemy";
                break;
            default:
                std::cout << "unknown ally shield reason: " << cast_reason << std::endl;
                return false;
            }

            if ( cast_spell( ESpellSlot::w, ally->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Buffed priority ally due to " << reason_text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->lulu.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            if ( e_block_autoattacks( ) || e_block_skillshots( ) || e_block_turret_shots( ) ||
                e_block_ignite_and_poison( ) || e_increase_ally_damage( ) || e_cast_event( ) )
                return true;

            return false;
        }

        auto e_cast_event( )-> bool {

                if (!g_config->lulu.e_enabled->get<bool>() || !g_config->lulu.e_increase_damage->get<bool>() ||
                *g_time - m_last_cast_time <= 0.01f || *g_time - m_last_e_time <= 0.25f || !m_slot_e->is_ready(true))
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{};
            auto        target_priority{ -1 };
            bool        cast_allowed{};
            std::string reason{};

            for (const auto ally : g_entity_list.get_allies())
            {
                if (!ally || ally->is_dead() || ally->position.dist_to(local_position) > m_e_range ||
                    ally->network_id == g_local->network_id || ally->is_invisible() ||
                    ally->get_selectable_flag() != 1 ||
                    m_ally_selected && g_config->lulu.only_buff_priority_ally->get<bool>() &&
                        m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid)
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info();
                if (!sci || sci->get_spell_name().find("TristanaW") == std::string::npos ||
                    sci->server_cast_time < *g_time || sci->server_cast_time - *g_time > 0.1f )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority(ally->champion_name.text);
                if (priority < target_priority) continue;

                target_priority = priority;
                target_nid      = ally->network_id;
                cast_allowed    = true;

                reason = _("Buff ") + ally->get_name() + " special cast";
            }

            if (!cast_allowed) return false;

            if (cast_spell(ESpellSlot::e, target_nid)) {

                g_features->orbwalker->on_cast();
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_autoattacks( ) -> bool{
            if ( !g_config->lulu.e_enabled->get< bool >( ) ||
                !g_config->lulu.e_block_damage_from_autoattacks->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 3000.f )
                    continue;

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->server_cast_time < *
                    g_time )
                    continue;

                const auto victim = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !victim || !victim->is_hero( ) || victim->position.dist_to( local_position ) > m_e_range ||
                    victim->team != g_local->team || victim->is_invisible( ) || victim->get_selectable_flag( ) != 1 ||
                    victim->network_id == g_local->network_id && !g_config->lulu.e_self->get< bool >( ) ||
                    m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != m_ally_nid )
                    continue;

                const auto priority = g_features->target_selector->get_target_priority( victim->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target_nid = victim->network_id;
                cast_allowed = true;
                reason = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from " ) + enemy->get_name( ) +
                    "s autoattack";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_skillshots( ) -> bool{
            if ( !g_config->lulu.e_enabled->get< bool >( ) || !g_config->lulu.e_block_damage_from_spells->get< bool >( )
                ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto       local_position = g_features->prediction->get_server_position( g_local->index );

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                auto colliding_spell = get_colliding_skillshot( ally->index );
                if ( !colliding_spell ||
                    colliding_spell->danger < g_config->lulu.e_skillshot_minimum_danger->get< int >( ) ||
                    colliding_spell->time_until_collision > 0.1f + g_features->orbwalker->get_ping( ) * 2.f )
                    continue;

                target_nid      = ally->network_id;
                target_priority = priority;
                cast_allowed    = true;

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from " ) + colliding_spell->name +
                    " spell";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_turret_shots( ) -> bool{
            if ( !g_config->lulu.e_enabled->get< bool >( ) ||
                !g_config->lulu.e_block_damage_from_turret_shots->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f
                ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto missile : g_entity_list.get_enemy_missiles( ) ) {
                if ( !missile || missile->dist_to_local( ) > 2000.f ) continue;

                auto info = missile->missile_spell_info( );
                if ( !info ) continue;

                auto data = info->get_spell_data( );
                if ( !data ) continue;

                const auto name_hash = rt_hash( data->get_name( ).data( ) );
                if ( name_hash != ct_hash( "TurretBasicAttack" ) ) continue;

                const auto target_index = missile->get_missile_target_index( );
                if ( target_index == 0 ) continue;

                const auto victim = g_entity_list.get_by_index( target_index );
                if ( !victim || !victim->is_hero( ) || victim->dist_to_local( ) > m_e_range
                    || m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && victim->network_id != m_ally_nid )
                    break;

                auto missile_object = g_entity_list.get_by_index( missile->index );
                if ( !missile_object ) break;

                missile_object.update( );

                const auto time_until_impact = missile_object->position.dist_to( victim->position ) / 1200.f;
                if ( time_until_impact > 0.05f + g_features->orbwalker->get_ping( ) * 2.f ) break;

                target_nid   = victim->network_id;
                cast_allowed = true;
                reason       = _( "Shielding " ) + victim->get_name( ) + _( " to block damage from turret shot" );
                break;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_block_ignite_and_poison( ) -> bool{
            if ( !g_config->lulu.e_enabled->get< bool >( ) ||
                !g_config->lulu.e_block_damage_from_ignite->get< bool >( ) &&
                !g_config->lulu.e_block_damage_from_poison->get< bool >( ) &&
                !g_config->lulu.e_block_damage_from_item_burn->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

          auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )

                    continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                auto dot_data = get_damage_overtime_data(
                    ally->index,
                    g_config->lulu.e_block_damage_from_poison->get< bool >( ),
                    g_config->lulu.e_block_damage_from_ignite->get< bool >( ),
                    g_config->lulu.e_block_damage_from_item_burn->get< bool >( ),
                    false
                );

                if ( !dot_data ) continue;

                std::string names{ };
                float       longest_duration{ };

                for ( auto inst : dot_data.value( ) ) {
                    const auto duration = inst.end_time - *g_time;
                    if ( duration <= 1.f ) continue;

                    if ( duration > longest_duration ) longest_duration = duration;
                    names += inst.name + ", ";
                }

                target_nid      = ally->network_id;
                target_priority = priority;
                cast_allowed    = true;

                reason = _( "Shielding " ) + ally->get_name( ) + _( " to block damage from " ) + names;
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto e_increase_ally_damage( ) -> bool{
            if ( !g_config->lulu.e_enabled->get< bool >( ) ||
                !g_config->lulu.e_increase_damage->get< bool >( ) ||
                *g_time - m_last_cast_time <= 0.05f || *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

           auto local_position = g_features->prediction->get_server_position(g_local->index);

            unsigned    target_nid{ };
            auto        target_priority{ -1 };
            bool        cast_allowed{ };
            std::string reason{ };

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->is_dead( ) || ally->position.dist_to( local_position ) > m_e_range ||
                    ally->network_id == g_local->network_id || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 ||
                    m_ally_selected && g_config->lulu.only_buff_priority_ally->get< bool >( ) &&
                    m_ally_distance <= m_nearby_threshold && ally->network_id != m_ally_nid )
                    continue;

                auto sci = ally->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack ||
                    sci->server_cast_time < *g_time + g_features->orbwalker->get_ping( ) / 2.f )
                    continue;

                const auto target = g_entity_list.get_by_index( sci->get_target_index( ) );
                if ( !target || !target->is_hero( ) ) continue;

                const auto priority = g_features->target_selector->get_target_priority( ally->champion_name.text );
                if ( priority < target_priority ) continue;

                target_priority = priority;
                target_nid      = ally->network_id;
                cast_allowed    = true;

                reason = _( "Buff " ) + ally->get_name( ) + " autoattack damage";
            }

            if ( !cast_allowed ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] " << reason << std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_we( ) -> bool{
            if ( !g_config->lulu.semi_manual_we->get< bool >( ) || !g_input->is_key_pressed( utils::EKey::T ) ) {
                return
                    false;
            }

            if ( semi_manual_w( ) || semi_manual_e( ) ) return true;

            return false;
        }

        auto semi_manual_e( ) -> bool{
            if ( !g_config->lulu.semi_manual_e->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time < 0.05f && *g_time - m_last_w_time > 0.1f || !m_slot_e->is_ready( true ) ||
                m_slot_w->is_ready( true ) && *g_time - m_last_w_time > 1.f )
                return false;

            bool        allow_cast{ };
            unsigned    target_nid{ };
            std::string ally_name{ };

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            if ( m_ally_selected && m_ally_distance <= m_nearby_threshold ) {
                const auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally && ally->is_alive( ) && !ally->is_invisible( ) && ally->get_selectable_flag( ) == 1 
                    && ally->position.dist_to(local_position) <= m_e_range ) {

                    allow_cast = true;
                    target_nid = ally->network_id;
                    ally_name  = ally->champion_name.text;
                }
            }


            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::e, target_nid ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] Semi manual cast, target ally " << ally_name << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_w( ) -> bool{
            if ( !g_config->lulu.semi_manual_w->get< bool >( ) || *g_time - m_last_w_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_w->is_ready( true ) )
                return false;

            bool        allow_cast{ };
            unsigned    target_nid{ };
            std::string ally_name{ };

            auto local_position = g_features->prediction->get_server_position(g_local->index);

            if ( m_ally_selected && m_ally_distance <= m_nearby_threshold ) {
                const auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( ally && ally->is_alive( ) && !ally->is_invisible( ) && ally->get_selectable_flag( ) == 1 &&
                    ally->position.dist_to(local_position) <= m_w_range ) {
                    allow_cast = true;
                    target_nid = ally->network_id;
                    ally_name  = ally->champion_name.text;
                }
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::w, target_nid ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Semi manual cast, target ally " << ally_name << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_e( ) -> bool{
            if ( !g_config->lulu.e_killsteal->get< bool >( ) || *g_time - m_last_e_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_e_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::e, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::e, unit ); },
                2
            );

            if ( !target || !g_features->orbwalker->is_attackable( target->index, m_e_range, false ) ) return false;


            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: E ] Killsteal, target " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->lulu.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

             auto local_position = g_features->prediction->get_server_position(g_local->index);

            for ( const auto ally : g_entity_list.get_allies( ) ) {
                if ( !ally || ally->position.dist_to( local_position ) > m_r_range || ally->is_dead( ) || ally->is_invisible( ) ||
                    ally->get_selectable_flag( ) != 1 )
                    continue;

                autointerrupt_r( ally );
            }

            if ( g_config->lulu.r_logic->get< int >( ) == 1 && m_ally_selected ) {
                const auto ally = g_entity_list.get_by_index( m_ally_index );
                if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 || ally->
                    dist_to_local( ) > m_r_range )
                    return false;

                const auto hero = ally.get( );

                if ( combo_r( hero ) || antimelee_r( hero ) || antigapclose_r( hero ) ) return true;
            } else {
                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 ||
                        ally->dist_to_local( ) > m_r_range )
                        continue;


                    if ( combo_r( ally ) || antimelee_r( ally ) || antigapclose_r( ally ) ) return true;
                }
            }

            return false;
        }

        auto combo_r( Object* ally ) -> bool{
            if ( !ally ) return false;

            int count{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                    enemy->position.dist_to( ally->position ) > 450.f )
                    continue;

                auto pred =
                    g_features->prediction->predict_default(
                        enemy->index,
                        g_features->orbwalker->get_ping( ) * 2.f,
                        false
                    );
                if ( !pred || ally->position.dist_to( pred.value( ) ) > 390.f ) continue;

                ++count;
            }

            if ( count < g_config->lulu.r_multihit_count->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, ally->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                std::cout << "[ Lulu: R ] Multihit count: " << count << std::endl;
                return true;
            }

            return false;
        }

        auto antimelee_r( Object* ally ) -> bool{
            if ( !ally ) return false;

            bool allow_cast{ };

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ||
                    enemy->position.dist_to( ally->position ) > 390.f || enemy->attack_range > 300.f )
                    continue;

                auto sci = enemy->spell_book.get_spell_cast_info( );
                if ( !sci || !sci->is_autoattack && !sci->is_special_attack || sci->get_target_index( ) != ally->
                    index )
                    continue;

                allow_cast = true;
                break;
            }

            if ( !allow_cast ) return false;

            if ( cast_spell( ESpellSlot::r, ally->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: R ] Antimelee " << std::endl;
                return true;
            }

            return false;
        }

        auto antigapclose_r( Object* ally ) -> bool{
            if ( !ally || ally->health / ally->max_health > g_config->lulu.r_health_threshold->get< int >( ) / 100.f *
                ally->max_health
                && !g_features->buff_cache->is_immobile( ally->index ) )
                return false;

            const auto target = get_advanced_antigapclose_target(
                380.f,
                0.f,
                0.f,
                g_features->orbwalker->get_ping( ) * 1.5f,
                false,
                ally->position
            );
            if ( !target ) return false;

            if ( cast_spell( ESpellSlot::r, ally->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: R ] Antigapclose against " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto autointerrupt_r( Object* ally ) -> bool{
            if ( !ally ) return false;

            const auto pred = g_features->prediction->predict_default(
                ally->index,
                g_features->orbwalker->get_ping( ) * 1.f
            );
            if ( !pred ) return false;

            const auto target = get_interruptable_target( 400.f, pred.value( ) );
            if ( !target || target->position.dist_to( ally->position ) > 400.f ) return false;

            if ( cast_spell( ESpellSlot::r, ally->network_id ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: R ] Autointerrupt against " << target->champion_name.text << std::endl;
                return true;
            }

            return false;
        }

        auto autointerrupt_w( ) -> void{
            if ( !g_config->lulu.w_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_w_range );
            if ( !target ) return;

            if ( cast_spell( ESpellSlot::w, target->network_id ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Lulu: W ] Autointerrupt against " << target->champion_name.text << std::endl;
            }
        }

        auto flee_w( ) -> bool{
            if ( !g_config->lulu.w_flee->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f || !m_slot_w->is_ready( true ) )
                return false;

            auto closest_enemy{ 9999.f };
            bool was_moving_towards{ };
            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) >
                    1500.f )
                    continue;

                if ( !was_moving_towards ) {
                    auto pred = g_features->prediction->predict_default( enemy->index, 0.25f, false );
                    if ( pred && pred.value( ).dist_to( g_local->position ) < enemy->
                        dist_to_local( ) )
                        was_moving_towards = true;
                }

                if ( enemy->dist_to_local( ) < closest_enemy ) closest_enemy = enemy->dist_to_local( );
            }

            if ( closest_enemy > 950.f || !was_moving_towards ) return false;

            if ( cast_spell( ESpellSlot::w, g_local->network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Lulu: W ] Flee speedboost\n";

                return true;
            }

            return false;
        }

        auto update_pixie( ) -> void{
            if ( !m_pixie_found ) {
                for ( const auto minion : g_entity_list.get_ally_minions( ) ) {
                    if ( !minion || minion->dist_to_local( ) > 500.f || minion->is_dead( ) ||
                        rt_hash( minion->name.text ) != ct_hash( "RobotBuddy" ) )
                        continue;

                    m_pixie_index = minion->index;
                    m_pixie_found = true;
                    break;
                }

                if ( m_pixie_found ) std::cout << "[ Lulu ] found pixie object | " << *g_time << std::endl;

                return;
            }

            const auto pixie = g_entity_list.get_by_index( m_pixie_index );
            if ( !pixie || pixie->is_dead( ) ) {
                m_pixie_index   = 0;
                m_pixie_found   = false;
                m_pixie_glowing = false;
                std::cout << "[ Lulu ] pixie object deleted | " << *g_time << std::endl;
                return;
            }

            if ( *g_time - m_last_pixie_glow_time > 0.05f && g_function_caller->is_glow_queueable( ) ) {
                const auto glow_color = g_features->orbwalker->animate_color(
                    g_features->tracker->get_rainbow_color( ),
                    EAnimationType::pulse,
                    5
                );

                g_function_caller->enable_glow(
                    pixie->network_id,
                    D3DCOLOR_ARGB( 255, glow_color.r, glow_color.g, glow_color.b ),
                    0,
                    3,
                    0
                );
                m_last_pixie_glow_time = *g_time;
            }
        }

        void update_blacklist() {

            if (!m_blacklist_pressed && GetAsyncKeyState(0x4A ) ) {

                m_blacklist_pressed = true;

                auto target_idx = g_pw_hud->get_hud_manager()->hovered_object_handle;
                for (auto enemy : g_entity_list.get_enemies())
                {
                    if (!enemy || enemy->is_dead() || enemy->is_invisible() || enemy->index != target_idx)
                        continue;


                    if ( is_polymorph_blacklisted( enemy->index ) ) {

                        remove_blacklist(enemy->index);
                        break;
                    }

                    m_blacklist.push_back({ enemy->index, enemy->network_id, *g_time });
                    break;
                }

                return;
            }

            if ( m_blacklist_pressed && !GetAsyncKeyState(0x4A ) ) m_blacklist_pressed = false;
        }

        auto get_pixie_position( ) const -> Vec3{
            auto pixie = g_entity_list.get_by_index( m_pixie_index );
            if ( !pixie ) return Vec3( );

            pixie.update( );

            return pixie->position;
        }

        auto select_priority_ally( ) -> bool{
            if ( !g_config->lulu.ally_priority_enabled->get< bool >( ) ) {
                if ( m_ally_selected ) unselect_ally( );

                return false;
            }

            if ( !m_ally_selected ) {
                if ( !GetAsyncKeyState( 0x06 ) ) return false;

                const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

                int16_t  ally_index{ };
                unsigned ally_nid{ };
                auto     ally_distance_to_cursor{ 200.f };

                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->dist_to_local( ) > 2250.f || ally->network_id == g_local->network_id ||
                        ally->is_dead( ) || ally->is_invisible( ) || ally->get_selectable_flag( ) != 1 )
                        continue;

                    const auto distance = ally->position.dist_to( cursor );
                    if ( distance > ally_distance_to_cursor ) continue;

                    ally_index              = ally->index;
                    ally_nid                = ally->network_id;
                    ally_distance_to_cursor = distance;
                }

                if ( ally_index == 0 || ally_nid == 0 ) return false;

                m_ally_selected = true;
                m_ally_index    = ally_index;
                m_ally_nid      = ally_nid;

                m_holding_key = true;

                m_ally_selection_time = *g_time;

                return false;
            }

            auto ally = g_entity_list.get_by_index( m_ally_index );
            if ( !ally ) {
                unselect_ally( );
                return false;
            }

            ally.update( );
            if ( ally->is_dead( ) || ally->is_invisible( ) || ally->dist_to_local( ) > 2500.f ) {
                unselect_ally( );
                return false;
            }

            m_ally_distance = ally->dist_to_local( );

            if ( m_unselecting ) {
                unselect_ally( );
                return false;
            }

            if ( ( !m_ally_glowing || !m_ally_second_glowing ) && g_function_caller->is_glow_queueable( ) ) {
                Color layer_color{ };

                if ( !m_ally_glowing ) {
                    layer_color = Color( 255, 200, 0 );


                    g_function_caller->enable_glow(
                        m_ally_nid,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        0,
                        5,
                        10
                    );
                    m_ally_glowing = true;
                } else if ( !m_ally_second_glowing ) {
                    layer_color = Color( 255, 255, 0 );


                    g_function_caller->enable_glow(
                        m_ally_nid,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        1,
                        3,
                        0
                    );

                    m_ally_second_glowing = true;
                }
            }

            if ( !GetAsyncKeyState( 0x06 ) ) m_holding_key = false;

            if ( m_holding_key ) return false;

            if ( GetAsyncKeyState( 0x06 ) ) {
                unselect_ally( );
                return false;
            }

            return false;
        }

        auto unselect_ally( ) -> void{
            if ( m_ally_glowing || m_ally_second_glowing ) {
                if ( g_function_caller->is_glow_queueable( ) ) {
                    if ( m_ally_glowing ) {
                        g_function_caller->enable_glow( m_ally_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 0, 3, 3, true );
                        m_ally_glowing = false;
                        m_unselecting  = true;
                    } else if ( m_ally_second_glowing ) {
                        g_function_caller->enable_glow( m_ally_nid, D3DCOLOR_ARGB( 255, 255, 255, 25 ), 1, 3, 3, true );
                        m_ally_second_glowing = false;
                        m_unselecting         = true;
                    }
                }

                if ( m_ally_second_glowing || m_ally_glowing ) return;
            }

            m_ally_selected       = false;
            m_ally_last_glow_time = 0.f;
            m_ally_glowing        = false;
            m_ally_second_glowing = false;
            m_unselecting         = false;
        }

    protected:
        bool is_polymorph_blacklisted( int16_t index )
        {
            return std::ranges::find_if(
                       m_blacklist, [&](const w_blacklist_instance &inst) -> bool { return inst.index == index; }) != m_blacklist.end();
        }

        void remove_blacklist( int16_t index )
        {
            const auto to_remove = std::ranges::remove_if(m_blacklist,
                                                          [&](const w_blacklist_instance &inst) -> bool
                                                          { return inst.index == index; });

            if (to_remove.empty()) return;

            m_blacklist.erase(to_remove.begin(), to_remove.end());
        }

        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.4f,
                    target->index,
                    false
                );
            case ESpellSlot::e:
                return helper::calculate_damage(
                    m_e_damage[ m_slot_e->level ] + g_local->ability_power( ) * 0.4f,
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
                const auto tt   = 0.25f + g_local->position.dist_to( target->position ) / 1450.f;
                const auto pred = g_features->prediction->predict_default( target->index, tt );
                if ( !pred ) return 0.f;

                const auto base_travel_time = 0.25f + g_local->position.dist_to( pred.value( ) ) / 1450.f;

                const auto pixie_position = get_pixie_position( );
                const auto traveltime     = 0.25f + pixie_position.dist_to( target->position ) / 1450.f;
                const auto pixie_pred     = g_features->prediction->predict_default( target->index, tt );
                if ( !pixie_pred ) return 0.f;

                const auto pixie_travel_time = 0.25f + pixie_position.dist_to( pixie_pred.value( ) ) / 1450.f;

                return pixie_travel_time < base_travel_time ? pixie_travel_time : base_travel_time;
            }
            case ESpellSlot::e:
                return g_features->orbwalker->get_ping( );
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
        float m_last_cast_time{ };

        // priority ally logic
        bool     m_ally_selected{ };
        int16_t  m_ally_index{ };
        unsigned m_ally_nid{ };

        bool m_holding_key{ };
        bool m_unselecting{ };

        bool  m_ally_glowing{ };
        bool  m_ally_second_glowing{ };
        float m_ally_last_glow_time{ };

        float m_ally_selection_time{ };
        float m_ally_distance{ };

        float m_nearby_threshold{ 750.f };


        // blacklisting
        std::vector<w_blacklist_instance> m_blacklist{};
        bool                              m_blacklist_pressed{};

        // permashielding
        bool m_permashield_toggle{ };
        bool m_key_down{ };

        // pixie tracking
        int16_t m_pixie_index{ };
        bool    m_pixie_found{ };
        bool    m_pixie_glowing{ };
        float   m_last_pixie_glow_time{ };

        Vec3 m_q_position{ };

        std::vector< float > m_q_damage = { 0.f, 70.f, 105.f, 140.f, 175.f, 210.f };
        std::vector< float > m_e_damage = { 0.f, 80.f, 120.f, 160.f, 200.f, 240.f };

        float m_q_range{ 950.f };
        float m_w_range{ 650.f };
        float m_e_range{ 650.f };
        float m_r_range{ 900.f };
    };
}
