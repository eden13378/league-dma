#include "pch.hpp"

#include "evade.hpp"
#include "orbwalker.hpp"
#include "buff_cache.hpp"
#include "entity_list.hpp"
#include "prediction.hpp"
#include "target_selector/target_selector.hpp"
#include "tracker.hpp"
#if enable_new_lua
#include "../lua-v2/state.hpp"
#endif
#include "../utils/c_function_caller.hpp"
#include "../sdk/game/pw_hud.hpp"
#include "../sdk/game/hud_manager.hpp"

#include "../sdk/game/ai_manager.hpp"
#include "../sdk/game/spell_cast_info.hpp"

namespace features {
    auto Evade::on_draw( ) -> void{
        if ( !g_local ) return;

        if ( g_config->evade.draw_spells_mode->get< int >( ) == 2 ) {
            for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
                if ( spell.type == SpellDetector::ESpellType::line ) {
                    auto start = spell.get_current_position( );
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f
                    );

                    auto draw_radius = spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius / 2.f : 0.f );

                    auto hitbox      = sdk::math::Rectangle( start, end, draw_radius ).to_polygon( );

                    auto full_duration = spell.server_cast_time - spell.start_time;
                    auto time_left     = spell.server_cast_time - *g_time;
                    auto time_to_end   = spell.end_time - *g_time;

                    auto early_spawn_modifier = 1.f - ( time_left - full_duration / 2.f ) / ( full_duration / 2.f );
                    auto late_spawn_modifier  = 1.f - time_left / ( full_duration / 2.f );

                    auto animation_modifier      = 1.f - time_left / full_duration;
                    auto fast_animation_modifier =
                        1.f - ( time_left - full_duration * 0.3f ) / ( full_duration * 0.7f );
                    auto vanish_modifier = 1.f - time_to_end / 0.075f;

                    if ( vanish_modifier > 1.f ) vanish_modifier = 1.f;
                    if ( fast_animation_modifier > 1.f ) fast_animation_modifier = 1.f;
                    if ( animation_modifier > 1.f ) animation_modifier = 1.f;
                    if ( early_spawn_modifier > 1.f ) early_spawn_modifier = 1.f;
                    if ( late_spawn_modifier > 1.f ) late_spawn_modifier = 1.f;

                    auto slow_animation_out = utils::ease::ease_out_circ( animation_modifier );
                    auto slow_animation_in  = utils::ease::ease_in_sine( animation_modifier );

                    animation_modifier      = utils::ease::ease_out_expo( animation_modifier );
                    fast_animation_modifier = utils::ease::ease_out_expo( fast_animation_modifier );


                    //g_render->polygon_3d(helper::get_missile_hitbox_polygon( start, end, draw_radius ), Color::white().alpha(30), Renderer::outline | Renderer::filled, 3.f);


                    //continue;

                    if ( spell.server_cast_time > *g_time ) {
                        const auto mod_radius =
                            draw_radius - draw_radius * ( ( spell.server_cast_time - *g_time ) / spell.windup_time );
                        const auto opacity = static_cast< int >(
                            70.f - 70.f * ( ( spell.server_cast_time - *g_time ) / spell.windup_time ) );


                        if ( spell.speed <= 0.f ) {
                            if ( time_to_end <= 0.075f ) { // end animation

                                auto opacity     = static_cast< i32 >( 255 - 255 * vanish_modifier );
                                auto half_radius = spell.radius / 2.f;

                                auto mod_poly =
                                    sdk::math::Rectangle( start, end, draw_radius )
                                    .to_polygon( static_cast< i32 >( -half_radius * vanish_modifier ) );

                                // std::cout << "T: " << *g_time - spell.start_time << " | modifier: " << modifier <<
                                // std::endl;

                                g_render->polygon_3d(
                                    mod_poly,
                                    Color( 255, 255, 255, opacity ),
                                    Renderer::outline,
                                    3.f
                                );

                                continue;
                            }

                            auto anim_radius = draw_radius * slow_animation_out;

                            auto center = start.extend( end, start.dist_to( end ) / 2.f );

                            auto anim_start = center.extend( start, center.dist_to( start ) * animation_modifier );
                            auto anim_end   = center.extend( end, center.dist_to( end ) * animation_modifier );

                            g_render->rectangle_3d(
                                start,
                                anim_end,
                                draw_radius,
                                Color( 255.f, 255.f, 255.f, 200.f - 200.f * slow_animation_in ),
                                Renderer::filled,
                                2.f
                            );

                            //g_render->rectangle_3d(
                            //    start, end, draw_radius, Color( 15, 15, 15, 255 - 255 * slow_animation_in ), Renderer::outline, 5.f );

                            auto anim_hitbox =
                                sdk::math::Rectangle( start, end, draw_radius )
                                .to_polygon( /* draw_radius * 0.5f - draw_radius * 0.5f * slow_animation_out*/ );

                            for ( auto i = 0; i < static_cast< i32 >( anim_hitbox.points.size( ) ); i++ ) {
                                auto line_start = anim_hitbox.points[ i ];
                                auto line_end   = i == anim_hitbox.points.size( ) - 1
                                                      ? anim_hitbox.points[ 0 ]
                                                      : anim_hitbox.points[ i + 1 ];

                                auto line_length = line_start.dist_to( line_end );

                                auto draw_start  = line_start.extend( line_end, line_length / 2.f );
                                auto color_level = static_cast< i32 >( 155 + 100 * fast_animation_modifier );
                                auto box_color   = Color( color_level, color_level, color_level, 255 );

                                g_render->line_3d(
                                    draw_start.extend( line_start, line_length / 2.f ),
                                    draw_start.extend( line_end, line_length / 2.f ),
                                    box_color,
                                    7.f - 3.f * slow_animation_out
                                );
                            }


                            continue;
                        }

                        auto anim_radius = draw_radius * slow_animation_out;

                        auto center = start.extend( end, start.dist_to( end ) / 2.f );

                        auto anim_start = center.extend( start, center.dist_to( start ) * animation_modifier );
                        auto anim_end   = center.extend( end, center.dist_to( end ) * animation_modifier );

                        g_render->rectangle_3d(
                            anim_start,
                            anim_end,
                            anim_radius,
                            Color( 255.f, 255.f, 255.f, 100.f - 100.f * slow_animation_in ),
                            Renderer::filled,
                            3.f
                        );


                        auto anim_hitbox = sdk::math::Rectangle( anim_start, anim_end, anim_radius ).to_polygon( );

                        for ( auto i = 0; i < static_cast< i32 >( anim_hitbox.points.size( ) ); i++ ) {
                            auto line_start = anim_hitbox.points[ i ];
                            auto line_end   = i == anim_hitbox.points.size( ) - 1
                                                  ? anim_hitbox.points[ 0 ]
                                                  : anim_hitbox.points[ i + 1 ];

                            auto line_length = line_start.dist_to( line_end );

                            auto draw_start  = line_start.extend( line_end, line_length / 2.f );
                            auto color_level = static_cast< i32 >( 155 + 100 * fast_animation_modifier );
                            auto box_color   = Color( color_level, color_level, color_level, 255 );

                            g_render->line_3d(
                                draw_start.extend( line_start, line_length / 2.f ),
                                draw_start.extend( line_end, line_length / 2.f ),
                                box_color,
                                7.f - 3.f * slow_animation_out
                            );
                        }

                        continue;
                    }

                    if ( time_to_end <= 0.075f ) { // end animation

                        auto opacity     = static_cast< i32 >( 255 - 255 * vanish_modifier );
                        auto half_radius = spell.radius / 2.f;

                        auto mod_poly =
                            sdk::math::Rectangle( start, end, draw_radius )
                            .to_polygon( static_cast< i32 >( -half_radius * vanish_modifier ) );

                        // std::cout << "T: " << *g_time - spell.start_time << " | modifier: " << modifier <<
                        // std::endl;

                        g_render->polygon_3d( mod_poly, Color( 255, 255, 255, opacity ), Renderer::outline, 3.f );
                        continue;
                    }

                    auto time_since_cast = *g_time - spell.server_cast_time;
                    auto fade_modifier   = std::min( time_since_cast / 0.25f, 1.f );
                    auto color_level     = 255; // 55 + 200 * fade_modifier;

                    auto base_position = start;
                    auto delta         = ( base_position - end ).normalize( );
                    auto rotated_left  =
                        base_position.extend( base_position + delta.rotated( 1.58f ), draw_radius * 1.33f );
                    auto rotated_right = base_position.extend( rotated_left, -draw_radius * 1.33f );

                    g_render->rectangle_3d(
                        start,
                        end,
                        draw_radius,
                        Color( 255, color_level, color_level ),
                        Renderer::outline,
                        4.f
                    );

                    g_render->line_3d( rotated_left, rotated_right, Color( 70, 70, 225 ), 8.f );
                } else if ( spell.type == SpellDetector::ESpellType::circle ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger
                                     || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 50 )
                                         : Color( 255, 255, 255, 50 );
                    else
                        fill_color = spell.danger < m_minimum_danger
                                         ? Color( 255, 140, 41, 50 )
                                         : Color( 255, 75, 75, 50 );

                    auto total_duration = spell.end_time - spell.start_time;
                    auto full_duration  = total_duration * 0.8f;
                    auto time_left      = spell.end_time - total_duration * 0.2f - *g_time;
                    auto time_to_end    = spell.end_time - *g_time;

                    auto delay_duration  = spell.end_time - spell.server_cast_time;
                    auto time_before_end = spell.end_time - *g_time - total_duration * 0.2f;

                    if ( time_to_end <= 0.f ) continue;
                    auto accelerated_spawn_modifier = 1.f - ( time_left - full_duration * 0.75f ) / ( full_duration *
                        0.75f );
                    auto early_spawn_modifier = 1.f - ( time_left - full_duration / 2.f ) / ( full_duration / 2.f );
                    auto late_spawn_modifier  = 1.f - time_left / ( full_duration / 2.f );

                    auto animation_modifier = 1.f - time_left / full_duration;
                    auto vanish_modifier    = std::max( 1.f - time_to_end / ( total_duration * 0.2f ), 0.f );

                    auto delay_modifier = 1.f - time_before_end / delay_duration;

                    if ( vanish_modifier > 1.f ) vanish_modifier = 1.f;
                    if ( delay_modifier > 1.f ) delay_modifier = 1.f;
                    if ( animation_modifier > 1.f ) animation_modifier = 1.f;
                    if ( early_spawn_modifier > 1.f ) early_spawn_modifier = 1.f;
                    if ( late_spawn_modifier > 1.f ) late_spawn_modifier = 1.f;
                    if ( accelerated_spawn_modifier > 1.f ) accelerated_spawn_modifier = 1.f;

                    vanish_modifier  = utils::ease::ease_in_quart( vanish_modifier );
                    auto start_color = 255;

                    auto fill_fade = Color(
                        static_cast< i32 >( start_color - start_color * early_spawn_modifier ),
                        static_cast< i32 >( start_color - start_color * early_spawn_modifier ),
                        static_cast< i32 >( start_color - start_color * early_spawn_modifier ),
                        60
                    );


                    auto outline_color = Color( 10, 10, 10, 155 );
                    auto loading_color = Color( 255, 255, 255, 255 );

                    if ( *g_time - spell.start_time <= total_duration * 0.8f ) {
                        if ( accelerated_spawn_modifier != 1.f ) {
                            g_render->circle_3d(
                                spell.end_pos,
                                Color(
                                    static_cast< int32_t >( 255 * early_spawn_modifier ),
                                    static_cast< int32_t >( 255 * early_spawn_modifier ),
                                    static_cast< int32_t >( 255 * early_spawn_modifier ),
                                    100
                                ),
                                spell.radius + ( 70.f - 70.f * accelerated_spawn_modifier ),
                                Renderer::filled,
                                -1,
                                2.f
                            );

                            g_render->circle_3d(
                                spell.end_pos,
                                outline_color,
                                spell.radius + ( 70.f - 70.f * accelerated_spawn_modifier ),
                                Renderer::outline,
                                -1,
                                3.f
                            );

                            g_render->circle_3d(
                                spell.end_pos,
                                loading_color.alpha( static_cast< i32 >( 55 + 200 * accelerated_spawn_modifier ) ),
                                spell.radius + ( 70.f - 70.f * accelerated_spawn_modifier ),
                                Renderer::outline,
                                -1,
                                4.f,
                                360.f * animation_modifier
                            );
                        }

                        if ( accelerated_spawn_modifier != 1.f ) continue;

                        g_render->circle_3d(
                            spell.end_pos,
                            fill_fade,
                            spell.radius,
                            Renderer::filled,
                            -1,
                            1.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            outline_color,
                            spell.radius,
                            Renderer::outline,
                            -1,
                            3.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            loading_color,
                            spell.radius,
                            Renderer::outline,
                            -1,
                            4.f,
                            360.f * animation_modifier
                        );
                        continue;
                    } else {
                        if ( time_to_end <= total_duration * 0.2f ) {
                            g_render->circle_3d(
                                spell.end_pos,
                                fill_fade.alpha( static_cast< i32 >( 75 - 75 * vanish_modifier ) ),
                                spell.radius + 30.f * vanish_modifier,
                                Renderer::filled,
                                -1,
                                3.f
                            );

                            g_render->circle_3d(
                                spell.end_pos,
                                Color( 255, 255, 255, static_cast< i32 >( 255 - 255 * vanish_modifier ) ),
                                spell.radius + 30.f * vanish_modifier,
                                Renderer::outline,
                                -1,
                                4.f
                            );

                            continue;
                        }


                        g_render->circle_3d(
                            spell.end_pos,
                            fill_color,
                            spell.radius - 5.f,
                            Renderer::outline | Renderer::filled,
                            -1,
                            3.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            Color( 255, 255, 255, 255 ),
                            spell.radius,
                            Renderer::outline,
                            -1,
                            4.f
                        );
                    }
                } else if ( spell.type == SpellDetector::ESpellType::cone ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger
                                     || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color = spell.danger < m_minimum_danger
                                         ? Color( 255, 140, 41, 70 )
                                         : Color( 255, 75, 75, 70 );

                    if ( spell.server_cast_time > *g_time ) {
                        const auto mod_angle = spell.angle - spell.angle * ( ( spell.server_cast_time - *g_time ) /
                            spell.windup_time );

                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, mod_angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    } else {
                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    }

                    auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                    g_render->polygon_3d( sect.to_polygon_new( ), Color( 255, 255, 255 ), Renderer::outline, 2.f );

                    //auto polygon = get_cone_segment_points(spell.start_pos, spell.end_pos, spell.range, spell.angle);
                    //for (auto point : polygon.points) g_render->circle_3d(point, color(255, 255, 255), 25.f, 2, 20, 2.f);
                }
            }

            for ( auto& missile : g_features->spell_detector->get_active_missiles( ) ) {
                if ( missile.type == SpellDetector::ESpellType::line ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = missile.danger <
                                     m_minimum_danger || !missile.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color = missile.danger < m_minimum_danger
                                         ? Color( 255, 140, 41, 70 )
                                         : Color( 255, 75, 75, 70 );

                    auto start = missile.position;
                    auto end   = missile.get_dynamic_line_endpos( );

                    auto draw_radius = missile.radius + m_raw_bounding_radius / 2.f;
                    auto hitbox      = sdk::math::Rectangle(
                        missile.position,
                        missile.get_dynamic_line_endpos( ),
                        draw_radius
                    ).to_polygon( );

                    auto full_duration = missile.end_time - missile.start_time;
                    auto time_left     = missile.end_time - 0.075f - *g_time;
                    auto time_to_end   = missile.end_time - *g_time;

                    auto spawn_duration  = full_duration * 0.2f > 0.25f ? 0.25f : full_duration * 0.2f;
                    auto spawn_time_left = missile.start_time + spawn_duration - *g_time;

                    auto early_spawn_modifier = 1.f - ( spawn_time_left - spawn_duration / 2.f ) / ( spawn_duration /
                        2.f );
                    auto late_spawn_modifier = 1.f - spawn_time_left / ( spawn_duration / 2.f );
                    auto animation_modifier  = 1.f - spawn_time_left / spawn_duration;

                    auto vanish_modifier = 1.f - time_to_end / 0.075f;

                    if ( vanish_modifier > 1.f ) vanish_modifier = 1.f;
                    if ( animation_modifier > 1.f ) animation_modifier = 1.f;
                    if ( early_spawn_modifier > 1.f ) early_spawn_modifier = 1.f;
                    if ( late_spawn_modifier > 1.f ) late_spawn_modifier = 1.f;

                    if ( spawn_time_left > 0.f ) {
                        if ( missile.speed == 0.f ) {
                            g_render->rectangle_3d(
                                start,
                                end,
                                draw_radius,
                                Color::white( ).alpha( 200 ), // color( 25, 25, 25, 100 ),
                                Renderer::outline,
                                3.f
                            );

                            for ( auto i = 0; i < static_cast< i32 >( hitbox.points.size( ) ); i++ ) {
                                auto begin_point = hitbox.points[ i ];
                                auto end_point   =
                                    i == hitbox.points.size( ) - 1 ? hitbox.points[ 0 ] : hitbox.points[ i + 1 ];

                                auto line_length = begin_point.dist_to( end_point );

                                auto draw_start          = begin_point.extend( end_point, line_length / 2.f );
                                auto primary_color_level = static_cast< i32 >( 255 * early_spawn_modifier );
                                auto base_color_level    = static_cast< i32 >( 55 * early_spawn_modifier );


                                auto box_color = Color(
                                    primary_color_level,
                                    base_color_level,
                                    base_color_level,
                                    static_cast< i32 >( 255 * early_spawn_modifier )
                                );

                                g_render->line_3d(
                                    draw_start.extend( begin_point, line_length / 2.f * early_spawn_modifier ),
                                    draw_start.extend( end_point, line_length / 2.f * early_spawn_modifier ),
                                    box_color,
                                    4.f
                                );
                            }


                            auto custom_modifier = std::min( animation_modifier + 0.3f, 1.f );
                            auto animated_hitbox = sdk::math::Rectangle( start, end, draw_radius )
                                .to_polygon( static_cast< i32 >( 20.f * std::min( custom_modifier, 1.f ) ) );

                            for ( auto i = 0; i < static_cast< i32 >( animated_hitbox.points.size( ) ); i++ ) {
                                auto start_point = animated_hitbox.points[ i ];
                                auto end_point   = i == animated_hitbox.points.size( ) - 1
                                                       ? animated_hitbox.points[ 0 ]
                                                       : animated_hitbox.points[ i + 1 ];

                                auto line_length = start_point.dist_to( end_point );

                                auto draw_start       = start_point.extend( end_point, line_length / 2.f );
                                auto base_color_level = 55;

                                auto box_color =
                                    Color(
                                        255,
                                        base_color_level,
                                        base_color_level,
                                        static_cast< i32 >( 255 - 255 * custom_modifier )
                                    );

                                g_render->line_3d(
                                    draw_start.extend( start_point, line_length / 2.f * custom_modifier ),
                                    draw_start.extend( end_point, line_length / 2.f * custom_modifier ),
                                    box_color,
                                    6.f
                                );
                            }

                            if ( early_spawn_modifier == 1.f ) {
                                animated_hitbox = sdk::math::Rectangle( start, end, draw_radius )
                                    .to_polygon(
                                        static_cast< i32 >( 50 - 45 * std::min( late_spawn_modifier, 1.f ) )
                                    );

                                for ( auto i = 0; i < static_cast< i32 >( animated_hitbox.points.size( ) ); i++ ) {
                                    auto begin_point = animated_hitbox.points[ i ];
                                    auto end_point   = i == animated_hitbox.points.size( ) - 1
                                                           ? animated_hitbox.points[ 0 ]
                                                           : animated_hitbox.points[ i + 1 ];

                                    auto line_length = begin_point.dist_to( end_point );

                                    auto box_color = Color(
                                        255,
                                        255,
                                        255,
                                        static_cast< i32 >( 255 * late_spawn_modifier )
                                    );
                                    line_length = 150.f;

                                    g_render->line_3d(
                                        begin_point,
                                        begin_point.extend( end_point, line_length / 2.f * late_spawn_modifier ),
                                        box_color,
                                        4.f
                                    );

                                    g_render->line_3d(
                                        end_point,
                                        end_point.extend( begin_point, line_length / 2.f * late_spawn_modifier ),
                                        box_color,
                                        4.f
                                    );
                                }
                            }
                        } else {
                            auto animated_hitbox = sdk::math::Rectangle( start, end, draw_radius )
                                .to_polygon( static_cast< i32 >( 20 * std::min( animation_modifier, 1.f ) ) );

                            for ( auto i = 0; i < static_cast< i32 >( animated_hitbox.points.size( ) ); i++ ) {
                                auto start_point = animated_hitbox.points[ i ];
                                auto end_point   = i == animated_hitbox.points.size( ) - 1
                                                       ? animated_hitbox.points[ 0 ]
                                                       : animated_hitbox.points[ i + 1 ];

                                auto line_length = start_point.dist_to( end_point );

                                auto draw_start       = start_point.extend( end_point, line_length / 2.f );
                                auto base_color_level = 55;

                                auto box_color =
                                    Color(
                                        255,
                                        base_color_level,
                                        base_color_level,
                                        static_cast< i32 >( 255 - 255 * animation_modifier )
                                    );

                                g_render->line_3d(
                                    draw_start.extend( start_point, line_length / 2.f * animation_modifier ),
                                    draw_start.extend( end_point, line_length / 2.f * animation_modifier ),
                                    box_color,
                                    6.f
                                );
                            }

                            g_render->rectangle_3d(
                                start,
                                end,
                                draw_radius,
                                Color( 20, 20, 20, 60 ),
                                Renderer::outline,
                                3.f
                            );

                            for ( auto i = 0; i < static_cast< i32 >( hitbox.points.size( ) ); i++ ) {
                                auto start_point = hitbox.points[ i ];
                                auto end_point   =
                                    i == hitbox.points.size( ) - 1 ? hitbox.points[ 0 ] : hitbox.points[ i + 1 ];

                                auto line_length = start_point.dist_to( end_point );

                                auto draw_start          = start_point.extend( end_point, line_length / 2.f );
                                auto primary_color_level = static_cast< i32 >( 255 * early_spawn_modifier );
                                auto base_color_level    = static_cast< i32 >( 55 * early_spawn_modifier );
                                auto box_color           = Color(
                                    primary_color_level,
                                    base_color_level,
                                    base_color_level,
                                    static_cast< i32 >( 25 + 225 * early_spawn_modifier )
                                );

                                g_render->line_3d(
                                    draw_start.extend( start_point, line_length / 2.f * early_spawn_modifier ),
                                    draw_start.extend( end_point, line_length / 2.f * early_spawn_modifier ),
                                    box_color,
                                    4.f
                                );
                            }

                            auto base_position = start;
                            auto delta         = ( base_position - end ).normalize( );
                            auto rotated_left  =
                                base_position.extend( base_position + delta.rotated( 1.58f ), draw_radius * 1.35f );
                            auto rotated_right = base_position.extend( rotated_left, -draw_radius * 1.35f );

                            g_render->line_3d( rotated_left, rotated_right, Color( 70, 70, 225 ), 5.f );
                        }
                    } else {
                        if ( time_to_end <= 0.075f ) { // end animation

                            auto opacity     = static_cast< i32 >( 255 - 255 * vanish_modifier );
                            auto half_radius = missile.radius / 2.f;

                            auto mod_poly = sdk::math::Rectangle( start, end, draw_radius )
                                .to_polygon( static_cast< i32 >( -half_radius * vanish_modifier ) );

                            // std::cout << "T: " << *g_time - spell.start_time << " | modifier: " << modifier <<
                            // std::endl;

                            g_render->polygon_3d( mod_poly, Color( 255, 255, 255, opacity ), Renderer::outline, 3.f );
                            continue;
                        }

                        auto time_since_cast = *g_time - ( missile.start_time + spawn_duration );
                        auto fade_modifier   = std::min(
                            time_since_cast / ( time_left < 0.25f ? time_left : 0.25f ),
                            1.f
                        );

                        auto color_level = static_cast< i32 >( 55 + 200 * fade_modifier );

                        auto base_position = start;
                        auto delta         = ( base_position - end ).normalize( );
                        auto rotated_left  =
                            base_position.extend( base_position + delta.rotated( 1.58f ), draw_radius * 1.33f );
                        auto rotated_right = base_position.extend( rotated_left, -draw_radius * 1.33f );

                        g_render->rectangle_3d(
                            start,
                            end,
                            draw_radius,
                            Color( 255, color_level, color_level ),
                            Renderer::outline,
                            3.f
                        );

                        g_render->line_3d( rotated_left, rotated_right, Color( 70, 70, 225 ), 5.f );
                    }
                } else if ( missile.type == SpellDetector::ESpellType::circle ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = missile.danger <
                                     m_minimum_danger || !missile.should_dodge( )
                                         ? Color( 100, 100, 100, 75 )
                                         : Color( 20, 20, 20, 75 );
                    else
                        fill_color = missile.danger < m_minimum_danger
                                         ? Color( 255, 140, 41, 70 )
                                         : Color( 255, 75, 75, 70 );

                    auto start = missile.position;
                    auto end   = missile.end_position;

                    auto total_duration = missile.end_time - missile.start_time;
                    auto full_duration  = total_duration * 0.8f;
                    auto time_left      = missile.end_time - total_duration * 0.2f - *g_time;
                    auto time_to_end    = missile.end_time - *g_time;

                    if ( time_to_end <= 0.f ) continue;
                    auto early_spawn_modifier = 1.f - ( time_left - full_duration / 2.f ) / ( full_duration / 2.f );
                    auto late_spawn_modifier  = 1.f - time_left / ( full_duration / 2.f );

                    auto animation_modifier = 1.f - time_left / full_duration;
                    auto vanish_modifier    = std::max( 1.f - time_to_end / ( total_duration * 0.2f ), 0.f );

                    if ( vanish_modifier > 1.f ) vanish_modifier = 1.f;
                    if ( animation_modifier > 1.f ) animation_modifier = 1.f;
                    if ( early_spawn_modifier > 1.f ) early_spawn_modifier = 1.f;
                    if ( late_spawn_modifier > 1.f ) late_spawn_modifier = 1.f;

                    auto start_color = 255;

                    auto fill_fade = Color(
                        static_cast< i32 >( start_color - start_color * animation_modifier ),
                        static_cast< i32 >( start_color - start_color * animation_modifier ),
                        static_cast< i32 >( start_color - start_color * animation_modifier ),
                        60
                    );

                    auto loading_fade = Color( 255, 255, 255, 255 );

                    if ( *g_time - missile.start_time <= full_duration ) {
                        if ( early_spawn_modifier != 1.f ) {
                            g_render->circle_3d(
                                end,
                                fill_color.alpha( static_cast< i32 >( 25 + 55 * early_spawn_modifier ) ),
                                missile.radius + ( 50.f - 50.f * early_spawn_modifier ),
                                Renderer::filled,
                                -1,
                                2.f
                            );

                            g_render->circle_3d(
                                end,
                                fill_color.alpha( static_cast< i32 >( 55 + 200 * early_spawn_modifier ) ),
                                missile.radius + ( 50.f - 50.f * early_spawn_modifier ),
                                Renderer::outline,
                                -1,
                                2.f
                            );

                            g_render->circle_3d(
                                end,
                                Color( 255, 255, 255, static_cast< i32 >( 55 + 200 * early_spawn_modifier ) ),
                                missile.radius + ( 50.f - 50.f * early_spawn_modifier ),
                                Renderer::outline,
                                -1,
                                4.f,
                                360.f * animation_modifier
                            );
                        }

                        if ( early_spawn_modifier != 1.f ) continue;

                        g_render->circle_3d( end, fill_color, missile.radius, Renderer::filled, -1, 1.f );

                        g_render->circle_3d(
                            end,
                            fill_color.alpha( 255 ),
                            missile.radius,
                            Renderer::outline,
                            -1,
                            2.f
                        );

                        g_render->circle_3d(
                            end,
                            Color( 255, 255, 255, 255 ),
                            missile.radius,
                            Renderer::outline,
                            -1,
                            4.f,
                            360.f * animation_modifier
                        );
                    } else {
                        if ( time_to_end <= total_duration * 0.2f ) {
                            g_render->circle_3d(
                                end,
                                fill_fade.alpha( static_cast< i32 >( 90 - 50 * vanish_modifier ) ),
                                missile.radius + 25.f * vanish_modifier,
                                Renderer::filled,
                                -1,
                                2.f
                            );

                            g_render->circle_3d(
                                end,
                                Color( 255, 255, 255, static_cast< i32 >( 255 - 255 * vanish_modifier ) ),
                                missile.radius + 25.f * vanish_modifier,
                                Renderer::outline,
                                -1,
                                4.f
                            );

                            continue;
                        }
                    }
                }
            }

            for ( auto& object : g_features->spell_detector->get_active_objects( ) ) {
                if ( object.type == SpellDetector::ESpellType::circle ) {
                    g_render->circle_3d(
                        object.position,
                        g_config->evade.draw_spells_color_circular->get< Color >( ),
                        object.radius,
                        Tracker::get_draw_style( g_config->evade.draw_spells_style->get< int32_t >( ) ),
                        50,
                        2.f
                    );
                }
            }
        }
        else if ( g_config->evade.draw_spells_mode->get< int >( ) == 1 ) {
            for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
                if ( spell.type == SpellDetector::ESpellType::line ) {
                    auto start = spell.get_current_position( );
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f,
                        spell.end_time - spell.start_time
                    );

                    // auto collision_point = get_collision_point(spell);
                    // if (collision_point)  end = *collision_point;

                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            spell.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    auto draw_radius = spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius / 2.f : 0.f );

                    auto range_modifier = std::clamp(
                        ( *g_time - spell.start_time ) / ( ( spell.end_time - spell.start_time ) * 0.5f ),
                        0.f,
                        1.f
                    );
                    range_modifier = utils::ease::ease_out_quart( range_modifier );

                    auto adjusted_end = start.extend( end, start.dist_to( end ) * range_modifier );

                    if ( spell.server_cast_time > *g_time ) {
                        const auto mod_radius = draw_radius - draw_radius * ( spell.server_cast_time - *g_time ) /
                            spell.windup_time;
                        const auto opacity = static_cast< int >(
                            70.f - 70.f * ( ( spell.server_cast_time - *g_time ) / spell.windup_time ) );

                        g_render->rectangle_3d(
                            start,
                            adjusted_end,
                            mod_radius,
                            fill_color.alpha( opacity ),
                            Renderer::filled
                        );

                        g_render->rectangle_3d(
                            end,
                            adjusted_end,
                            mod_radius,
                            fill_color.alpha( opacity ),
                            Renderer::outline
                        );
                    } else {
                        g_render->rectangle_3d(
                            start,
                            adjusted_end,
                            draw_radius,
                            fill_color,
                            Renderer::filled | Renderer::outline
                        );
                    }

                    const auto extra_size = spell.server_cast_time > *g_time
                                                ? draw_radius / 2.f * utils::ease::ease_in_expo(
                                                    ( spell.server_cast_time - *g_time ) / spell.
                                                    windup_time
                                                )
                                                : 0.f;
                    const auto extra_time   = spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f;
                    const auto line_opacity = spell.server_cast_time > *g_time
                                                  ? static_cast< int >( 255.f -
                                                      255.f * ( ( spell.server_cast_time - *g_time ) / spell.
                                                          windup_time ) )
                                                  : 255.f;

                    g_render->rectangle_3d(
                        start,
                        adjusted_end,
                        draw_radius + extra_size,
                        Color( 255, 255, 255, static_cast< int32_t >( line_opacity ) ),
                        Renderer::outline,
                        2.f
                    );


                    // ah

                    /*float time_to_hit = spell.time_till_impact(g_local->position);

                    std::string text = std::to_string(time_to_hit);
                    text.resize(4);

                    g_render->text_3d(spell.get_current_position(), color(25, 255, 25), g_fonts->get_zabel(),
                    text.c_str(), 16, true);*/
                }
                else if ( spell.type == SpellDetector::ESpellType::circle ) {
                    auto duration  = spell.end_time - spell.start_time;
                    auto time_left = spell.end_time - *g_time;
                    if ( time_left <= 0.f ) continue;

                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            spell.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    auto animation_duration = 0.2f;
                    auto animation_start    = duration * animation_duration;

                    /*if (spell.is_lollipop) {
                        vec3 direction = (spell.end_pos - spell.start_pos).normalize().rotated(-1.61f);

                        float draw_radius{ spell.radius + m_bounding_radius / 2.f };
                        float opacity{ 70.f };

                        const float extra_size = spell.server_cast_time > *g_time ? draw_radius / 2.f *
                    ((spell.server_cast_time - *g_time) / spell.windup_time) : 0.f; const float line_opacity =
                    spell.server_cast_time > *g_time ? static_cast<int>(255.f - 255.f * ((spell.server_cast_time -
                    *g_time) / spell.windup_time)) : 255.f;

                        if(spell.server_cast_time > *g_time) {
                            opacity = static_cast<int>(70.f - 70.f * ((spell.server_cast_time - *g_time) /
                    spell.windup_time));
                        }

                        g_render->circle_3d(
                            spell.end_pos,
                            fill_color.alpha(opacity),
                            draw_radius - extra_size,
                            c_renderer::filled,
                            60,
                            0.f,
                            179.f,
                            direction);

                        g_render->circle_3d(
                            spell.end_pos,
                            color(255, 255, 255, line_opacity),
                            draw_radius + extra_size,
                            2,
                            60,
                            2.f,
                            179.f,
                            direction);

                        continue;
                    }*/

                    if ( time_left > animation_start ) {
                        auto opacity = static_cast< int32_t >(
                            std::floor( 75.f - 75.f * ( ( spell.server_cast_time - *g_time ) / spell.windup_time ) ) );
                        if ( *g_time > spell.server_cast_time || opacity > 75 ) opacity = 75;

                        g_render->circle_3d(
                            spell.end_pos,
                            fill_color.alpha( opacity ),
                            spell.radius,
                            Renderer::outline | Renderer::filled,
                            50,
                            2.f
                        );

                        // can remove this after c_renderer::filled flag working again
                        /* g_render->circle_3d(
                         spell.end_pos,
                         fill_color.alpha(opacity),
                         spell.radius,
                         c_renderer::filled,
                         50,
                         2.f
                     );*/


                        auto angle = 360.f -
                            360.f * ( ( time_left - animation_start ) / ( duration * ( 1.f - animation_duration ) ) );

                        g_render->circle_3d( spell.end_pos, Color( 255, 255, 255 ), spell.radius, 2, 50, 2.f, angle );
                    } else {
                        auto opacity = static_cast< int32_t >( std::floor( 255.f * ( time_left / animation_start ) ) );
                        auto fill_opacity =
                            static_cast< int32_t >( std::floor( 75.f * ( time_left / animation_start ) ) );

                        auto bonus_radius = 35.f - 35.f * ( time_left / animation_start );

                        g_render->circle_3d(
                            spell.end_pos,
                            Color( 255, 255, 255, opacity ),
                            spell.radius + bonus_radius,
                            2,
                            50,
                            2.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            fill_color.alpha( opacity ),
                            spell.radius,
                            Renderer::outline,
                            50,
                            2.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            fill_color.alpha( fill_opacity ),
                            spell.radius,
                            Renderer::filled,
                            50,
                            2.f
                        );
                    }
                } else if ( spell.type == SpellDetector::ESpellType::cone ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            spell.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    if ( spell.server_cast_time > *g_time ) {
                        const auto mod_angle =
                            spell.angle - spell.angle * ( ( spell.server_cast_time - *g_time ) / spell.windup_time );

                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, mod_angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    } else {
                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    }

                    auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                    g_render->polygon_3d( sect.to_polygon_new( ), Color( 255, 255, 255 ), Renderer::outline, 2.f );

                    // auto polygon = get_cone_segment_points(spell.start_pos, spell.end_pos, spell.range, spell.angle);
                    // for (auto point : polygon.points) g_render->circle_3d(point, color(255, 255, 255), 25.f, 2,
                    // 20, 2.f);
                }
            }

            for ( auto& missile : g_features->spell_detector->get_active_missiles( ) ) {
                if ( missile.type == SpellDetector::ESpellType::line ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = missile.danger < m_minimum_danger || !missile.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            missile.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    auto draw_radius = missile.radius + (
                        missile.has_edge_radius ? m_raw_bounding_radius / 2.f : 0.f );

                    g_render->rectangle_3d(
                        missile.position,
                        missile.get_dynamic_line_endpos( ),
                        draw_radius,
                        fill_color,
                        Renderer::filled
                    );

                    g_render->rectangle_3d(
                        missile.position,
                        missile.get_dynamic_line_endpos( ),
                        draw_radius + 3,
                        Color( 255, 255, 255 ),
                        Renderer::outline,
                        2.f
                    );
                } else if ( missile.type == SpellDetector::ESpellType::circle ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = missile.danger < m_minimum_danger || !missile.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            missile.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    auto duration  = missile.end_time - missile.start_time;
                    auto time_left = duration - ( *g_time - missile.start_time );

                    if ( time_left > 0.f ) {
                        auto animation_duration = 0.2f;
                        auto animation_start    = duration * animation_duration;

                        if ( time_left > animation_start ) {
                            g_render->circle_3d(
                                missile.end_position,
                                fill_color,
                                missile.radius,
                                Renderer::outline | Renderer::filled,
                                50,
                                2.f
                            );

                            g_render->circle_3d(
                                missile.end_position,
                                fill_color,
                                missile.radius,
                                Renderer::filled,
                                50,
                                2.f
                            );

                            auto angle = 360.f -
                                360.f *
                                ( ( time_left - animation_start ) / ( duration * ( 1.f - animation_duration ) ) );

                            g_render->circle_3d(
                                missile.end_position,
                                Color( 255, 255, 255 ),
                                missile.radius,
                                2,
                                50,
                                2.f,
                                angle
                            );
                        } else {
                            auto opacity =
                                static_cast< int32_t >( std::floor( 255.f * ( time_left / animation_start ) ) );
                            auto fill_opacity =
                                static_cast< int32_t >( std::floor( 75.f * ( time_left / animation_start ) ) );

                            auto bonus_radius = 40.f - 40.f * ( time_left / animation_start );

                            g_render->circle_3d(
                                missile.end_position,
                                Color( 255, 255, 255, opacity ),
                                missile.radius + bonus_radius,
                                2,
                                50,
                                2.f
                            );

                            g_render->circle_3d(
                                missile.end_position,
                                fill_color.alpha( opacity ),
                                missile.radius,
                                Renderer::outline,
                                50,
                                2.f
                            );

                            g_render->circle_3d(
                                missile.end_position,
                                fill_color.alpha( fill_opacity ),
                                missile.radius,
                                Renderer::filled,
                                50,
                                2.f
                            );
                        }
                    }
                }
            }

            for ( auto& object : g_features->spell_detector->get_active_objects( ) ) {
                if ( object.type == SpellDetector::ESpellType::circle ) {
                    g_render->circle_3d(
                        object.position,
                        g_config->evade.draw_spells_color_circular->get< Color >( ),
                        object.radius,
                        Tracker::get_draw_style( g_config->evade.draw_spells_style->get< int32_t >( ) ),
                        50,
                        2.f
                    );
                }
            }
        }
        else if ( g_config->evade.draw_spells_mode->get< int >( ) == 3 ) {
            //std::vector< sdk::math::Polygon > poly_list{ };

            for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
                if ( spell.type == SpellDetector::ESpellType::line ) {
                    auto start = spell.get_current_position( );
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f
                    );

                    auto draw_radius = spell.radius + m_raw_bounding_radius / 2.f;

                    auto end_modifier = 1.f - std::clamp( ( spell.end_time - *g_time ) / 0.25f, 0.f, 1.f );

                    auto animation_modifier =
                        std::clamp(
                            1.f - ( spell.server_cast_time - *g_time ) / ( spell.server_cast_time - spell.start_time ),
                            0.f,
                            1.f
                        );

                    auto early_modifier         = utils::ease::ease_out_expo( animation_modifier );
                    auto semi_early_modifier    = utils::ease::ease_out_cubic( animation_modifier );
                    auto slow_early_modifier    = utils::ease::ease_out_circ( animation_modifier );
                    auto slowest_early_modifier = utils::ease::ease_out_sine( animation_modifier );

                    auto position_offset = 40.f;

                    auto max_offset = 0.f;
                    int  opacity    = 100 + 155 * early_modifier;

                    if ( semi_early_modifier < 1.f ) {
                        start = start.extend( end, -( position_offset - position_offset * semi_early_modifier ) );
                        end   = end.extend( start, position_offset - position_offset * semi_early_modifier );
                    }

                    auto offset_direction = ( end - start ).normalize( ).rotated_raw( 90.f );
                    auto adjusted_start   = start.extend( start + offset_direction, 0.f );

                    auto original_direction = ( end - start );
                    auto adjusted_end       =
                        adjusted_start.extend( adjusted_start + original_direction, start.dist_to( end ) );

                    auto height_offset = 60.f;

                    auto additional_height = height_offset - height_offset * semi_early_modifier;
                    adjusted_start.y -= additional_height;
                    adjusted_end.y -= additional_height;

                    auto hitbox = sdk::math::Rectangle( adjusted_start, adjusted_end, draw_radius )
                        .to_polygon( );


                    additional_height = height_offset - height_offset * slow_early_modifier;

                    auto modified_start = start.extend(
                        start + offset_direction,
                        max_offset - max_offset * animation_modifier
                    );
                    auto modified_end = modified_start.extend(
                        modified_start + original_direction,
                        start.dist_to( end )
                    );
                    modified_start.y = adjusted_start.y - 20.f - additional_height * 0.25f;
                    modified_end.y   = adjusted_end.y - 20.f - additional_height * 0.25f;

                    auto shadow_hitbox =
                        sdk::math::Rectangle( modified_start, modified_end, draw_radius ).to_polygon( );

                    modified_start.y -= additional_height * 0.3f;
                    modified_end.y -= additional_height * 0.3f;

                    auto secondary_shadow_hitbox =
                        sdk::math::Rectangle( modified_start, modified_end, draw_radius ).to_polygon( );


                    //poly_list.push_back( hitbox );

                    g_render->polygon_3d(
                        shadow_hitbox,
                        Color( 255.f - 255.f * slowest_early_modifier, 0.f, 0.f, 25.f * semi_early_modifier ),
                        Renderer::outline | Renderer::filled,
                        3.f
                    );

                    g_render->polygon_3d(
                        secondary_shadow_hitbox,
                        Color( 255.f - 255.f * slowest_early_modifier, 0.f, 0.f, 25.f * semi_early_modifier ),
                        Renderer::outline | Renderer::filled,
                        4.f
                    );

                    g_render->polygon_3d(
                        hitbox,
                        Color(
                            255.f,
                            255.f * slowest_early_modifier,
                            255.f * slowest_early_modifier,
                            static_cast< float >( opacity )
                        ),
                        Renderer::outline,
                        3.f
                    );
                    /* g_render->rectangle_3d( adjusted_start,
                                            adjusted_end,
                                            draw_radius * 0.5f + draw_radius * 0.5f * animation_modifier,
                                            Color( 255, 255, 255, opacity ),
                                            Renderer::outline,
                                            3.f );*/
                } else if ( spell.type == SpellDetector::ESpellType::circle ) {
                    auto start = spell.get_current_position( );
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f
                    );

                    auto draw_radius = spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius / 2.f : 0.f );

                    auto full_duration = spell.server_cast_time - spell.start_time;
                    auto time_left     = spell.server_cast_time - *g_time;
                    auto time_to_end   = spell.end_time - *g_time;

                    auto early_spawn_modifier = 1.f - ( time_left - full_duration / 2.f ) / ( full_duration / 2.f );
                    auto late_spawn_modifier  = 1.f - time_left / ( full_duration / 2.f );

                    auto animation_modifier      = 1.f - time_left / full_duration;
                    auto fast_animation_modifier =
                        1.f - ( time_left - full_duration * 0.3f ) / ( full_duration * 0.7f );
                    auto vanish_modifier = 1.f - time_to_end / 0.075f;

                    if ( vanish_modifier > 1.f ) vanish_modifier = 1.f;
                    if ( fast_animation_modifier > 1.f ) fast_animation_modifier = 1.f;
                    if ( animation_modifier > 1.f ) animation_modifier = 1.f;
                    if ( early_spawn_modifier > 1.f ) early_spawn_modifier = 1.f;
                    if ( late_spawn_modifier > 1.f ) late_spawn_modifier = 1.f;

                    auto slow_animation_out = utils::ease::ease_out_circ( early_spawn_modifier );
                    auto slow_animation_in  = utils::ease::ease_in_sine( early_spawn_modifier );

                    animation_modifier      = utils::ease::ease_out_expo( animation_modifier );
                    fast_animation_modifier = utils::ease::ease_out_expo( fast_animation_modifier );

                    if ( spell.server_cast_time > *g_time ) {
                        const auto opacity = 255.f - 255.f * slow_animation_in;

                        auto anim_radius = draw_radius - 20.f + 20.f * slow_animation_out;

                        auto filled_color = Color( opacity, opacity, opacity, 255 - 255 * slow_animation_out );

                        g_render->circle_3d(
                            spell.end_pos,
                            filled_color,
                            anim_radius,
                            Renderer::filled,
                            50,
                            3.f
                        );

                        g_render->circle_3d(
                            spell.end_pos,
                            Color( 255, 255, 255 ),
                            anim_radius,
                            Renderer::outline,
                            50,
                            10.f - 6.f * slow_animation_in
                        );

                        continue;
                    }

                    if ( time_to_end <= 0.075f ) { // end animation

                        auto opacity     = static_cast< i32 >( 255 - 255 * vanish_modifier );
                        auto half_radius = spell.radius / 2.f;

                        g_render->circle_3d(
                            spell.end_pos,
                            Color( 255, 255, 255, opacity ),
                            draw_radius - half_radius * vanish_modifier,
                            Renderer::outline,
                            50,
                            4.f
                        );
                        continue;
                    }

                    auto time_since_cast = *g_time - spell.server_cast_time;
                    auto fade_modifier   = std::min( time_since_cast / 0.25f, 1.f );
                    auto color_level     = 255;

                    /* g_render->circle_3d(
                        spell.end_pos, Color( 0, 0, 0, 80 ),
                                         draw_radius,
                                         Renderer::filled,
                                         50,
                                         3.f );*/

                    g_render->circle_3d(
                        spell.end_pos,
                        Color( 255, 255, 255 ),
                        draw_radius,
                        Renderer::outline,
                        50,
                        4.f
                    );
                } else if ( spell.type == SpellDetector::ESpellType::cone ) {
                    Color fill_color{ };
                    if ( g_config->evade.draw_spells_blue->get< bool >( ) )
                        fill_color = spell.danger < m_minimum_danger || !spell.should_dodge( )
                                         ? Color( 61, 192, 252, 75 )
                                         : Color( 41, 102, 255, 75 );
                    else
                        fill_color =
                            spell.danger < m_minimum_danger ? Color( 255, 140, 41, 70 ) : Color( 255, 75, 75, 70 );

                    if ( spell.server_cast_time > *g_time ) {
                        const auto mod_angle =
                            spell.angle - spell.angle * ( ( spell.server_cast_time - *g_time ) / spell.windup_time );

                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, mod_angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    } else {
                        auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                        g_render->polygon_3d(
                            sect.to_polygon_new( ),
                            fill_color,
                            Renderer::outline | Renderer::filled,
                            2.f
                        );
                    }

                    auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );

                    g_render->polygon_3d( sect.to_polygon_new( ), Color( 255, 255, 255 ), Renderer::outline, 2.f );

                    // auto polygon = get_cone_segment_points(spell.start_pos, spell.end_pos, spell.range, spell.angle);
                    // for (auto point : polygon.points) g_render->circle_3d(point, color(255, 255, 255), 25.f, 2,
                    // 20, 2.f);
                }
            }

            /* auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );

            for ( const auto poly : draw_poly ) {
                g_render->polygon_3d( poly, Color( 255, 255, 255, 255 ), Renderer::outline, 4.f );
            }*/
        }
        else if (g_config->evade.draw_spells_mode->get<int>() == 4)
        {
            // std::vector< sdk::math::Polygon > poly_list{ };

            for (auto &spell : g_features->spell_detector->get_active_spells())
            {
                if (spell.type == SpellDetector::ESpellType::line)
                {
                    auto start = spell.get_current_position();
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f);

                    auto draw_radius = spell.radius + m_raw_bounding_radius / 2.f;

                    auto end_modifier = 1.f - std::clamp((spell.end_time - *g_time) / 0.25f, 0.f, 1.f);

                    auto animation_modifier = std::clamp(
                        1.f - (spell.server_cast_time - *g_time) / (spell.server_cast_time - spell.start_time), 0.f,
                        1.f);

                    auto early_modifier         = utils::ease::ease_out_expo(animation_modifier);
                    auto semi_early_modifier    = utils::ease::ease_out_cubic(animation_modifier);
                    auto slow_early_modifier    = utils::ease::ease_out_circ(animation_modifier);
                    auto slowest_early_modifier = utils::ease::ease_out_sine(animation_modifier);

                    auto position_offset = 40.f;

                    auto max_offset = 0.f;
                    int  opacity    = 100 + 155 * early_modifier;

                    if (semi_early_modifier < 1.f)
                    {
                        start = start.extend(end, -(position_offset - position_offset * semi_early_modifier));
                        end   = end.extend(start, position_offset - position_offset * semi_early_modifier);
                    }

                    auto offset_direction = (end - start).normalize().rotated_raw(90.f);
                    auto adjusted_start   = start.extend(start + offset_direction, 0.f);

                    auto original_direction = (end - start);
                    auto adjusted_end = adjusted_start.extend(adjusted_start + original_direction, start.dist_to(end));

                    auto height_offset = 5.f;

                    auto additional_height = height_offset - height_offset * semi_early_modifier;
                    adjusted_start.y -= additional_height;
                    adjusted_end.y -= additional_height;

                    auto hitbox = sdk::math::Rectangle(adjusted_start, adjusted_end, draw_radius).to_polygon();


                    additional_height = height_offset - height_offset * slow_early_modifier;

                    auto modified_start =
                        start.extend(start + offset_direction, max_offset - max_offset * animation_modifier);
                    auto modified_end = modified_start.extend(modified_start + original_direction, start.dist_to(end));
                    modified_start.y  = adjusted_start.y - 20.f - additional_height * 0.25f;
                    modified_end.y    = adjusted_end.y - 20.f - additional_height * 0.25f;

                    auto shadow_hitbox = sdk::math::Rectangle(modified_start, modified_end, draw_radius).to_polygon();

                    modified_start.y -= additional_height * 0.3f;
                    modified_end.y -= additional_height * 0.3f;

                    auto secondary_shadow_hitbox =
                        sdk::math::Rectangle(modified_start, modified_end, draw_radius).to_polygon();


                    // poly_list.push_back( hitbox );

                    g_render->polygon_3d(
                        shadow_hitbox,
                        Color(255.f, 255.f, 255.f - 255.f * slowest_early_modifier,  50.f- 25.f * semi_early_modifier),
                         Renderer::filled, 3.f);
               
                    g_render->polygon_3d(
                        secondary_shadow_hitbox,
                        Color(255.f, 255.f - 55.f * slowest_early_modifier, 255.f - 255.f * slowest_early_modifier, 255.f),
                                         Renderer::outline, 16.f - 12.f * early_modifier);

                    //g_render->polygon_3d(hitbox,
                    //Color(255.f, 255.f * slowest_early_modifier, 255.f * slowest_early_modifier,
                     //                          static_cast<float>(opacity)),
                     //                    Renderer::outline, 3.f);
                    /* g_render->rectangle_3d( adjusted_start,
                                            adjusted_end,
                                            draw_radius * 0.5f + draw_radius * 0.5f * animation_modifier,
                                            Color( 255, 255, 255, opacity ),
                                            Renderer::outline,
                                            3.f );*/
                }
                else if (spell.type == SpellDetector::ESpellType::circle)
                {
                    auto start = spell.get_current_position();
                    auto end   = spell.get_dynamic_line_endpos(
                        spell.server_cast_time > *g_time ? spell.server_cast_time - *g_time : 0.f);

                    auto draw_radius = spell.radius + (spell.has_edge_radius ? m_raw_bounding_radius / 2.f : 0.f);

                    auto full_duration = spell.server_cast_time - spell.start_time;
                    auto time_left     = spell.server_cast_time - *g_time;
                    auto time_to_end   = spell.end_time - *g_time;

                    auto early_spawn_modifier = 1.f - (time_left - full_duration / 2.f) / (full_duration / 2.f);
                    auto late_spawn_modifier  = 1.f - time_left / (full_duration / 2.f);

                    auto animation_modifier      = 1.f - time_left / full_duration;
                    auto fast_animation_modifier = 1.f - (time_left - full_duration * 0.3f) / (full_duration * 0.7f);
                    auto vanish_modifier         = 1.f - time_to_end / 0.075f;

                    if (vanish_modifier > 1.f) vanish_modifier = 1.f;
                    if (fast_animation_modifier > 1.f) fast_animation_modifier = 1.f;
                    if (animation_modifier > 1.f) animation_modifier = 1.f;
                    if (early_spawn_modifier > 1.f) early_spawn_modifier = 1.f;
                    if (late_spawn_modifier > 1.f) late_spawn_modifier = 1.f;

                    auto slow_animation_out = utils::ease::ease_out_circ(early_spawn_modifier);
                    auto slow_animation_in  = utils::ease::ease_in_sine(early_spawn_modifier);

                    animation_modifier      = utils::ease::ease_out_expo(animation_modifier);
                    fast_animation_modifier = utils::ease::ease_out_expo(fast_animation_modifier);

                    if (spell.server_cast_time > *g_time)
                    {
                        const auto opacity = 255.f - 255.f * slow_animation_in;

                        auto anim_radius = draw_radius - 20.f + 20.f * slow_animation_out;

                        auto filled_color = Color(opacity, opacity, opacity, 255 - 255 * slow_animation_out);

                        g_render->circle_3d(spell.end_pos, filled_color, anim_radius, Renderer::filled, 50, 3.f);

                        g_render->circle_3d(spell.end_pos, Color(255, 255, 255), anim_radius, Renderer::outline, 50,
                                            10.f - 6.f * slow_animation_in);

                        continue;
                    }

                    if (time_to_end <= 0.075f)
                    { // end animation

                        auto opacity     = static_cast<i32>(255 - 255 * vanish_modifier);
                        auto half_radius = spell.radius / 2.f;

                        g_render->circle_3d(spell.end_pos, Color(255, 255, 255, opacity),
                                            draw_radius - half_radius * vanish_modifier, Renderer::outline, 50, 4.f);
                        continue;
                    }

                    auto time_since_cast = *g_time - spell.server_cast_time;
                    auto fade_modifier   = std::min(time_since_cast / 0.25f, 1.f);
                    auto color_level     = 255;

                    /* g_render->circle_3d(
                        spell.end_pos, Color( 0, 0, 0, 80 ),
                                         draw_radius,
                                         Renderer::filled,
                                         50,
                                         3.f );*/

                    g_render->circle_3d(spell.end_pos, Color(255, 255, 255), draw_radius, Renderer::outline, 50, 4.f);
                }
                else if (spell.type == SpellDetector::ESpellType::cone)
                {
                    Color fill_color{};
                    if (g_config->evade.draw_spells_blue->get<bool>())
                        fill_color = spell.danger < m_minimum_danger || !spell.should_dodge() ? Color(61, 192, 252, 75)
                                                                                              : Color(41, 102, 255, 75);
                    else
                        fill_color = spell.danger < m_minimum_danger ? Color(255, 140, 41, 70) : Color(255, 75, 75, 70);

                    if (spell.server_cast_time > *g_time)
                    {
                        const auto mod_angle =
                            spell.angle - spell.angle * ((spell.server_cast_time - *g_time) / spell.windup_time);

                        auto sect = sdk::math::Sector(spell.start_pos, spell.end_pos, mod_angle, spell.range);

                        g_render->polygon_3d(sect.to_polygon_new(), fill_color, Renderer::outline | Renderer::filled,
                                             2.f);
                    }
                    else
                    {
                        auto sect = sdk::math::Sector(spell.start_pos, spell.end_pos, spell.angle, spell.range);

                        g_render->polygon_3d(sect.to_polygon_new(), fill_color, Renderer::outline | Renderer::filled,
                                             2.f);
                    }

                    auto sect = sdk::math::Sector(spell.start_pos, spell.end_pos, spell.angle, spell.range);

                    g_render->polygon_3d(sect.to_polygon_new(), Color(255, 255, 255), Renderer::outline, 2.f);

                    // auto polygon = get_cone_segment_points(spell.start_pos, spell.end_pos, spell.range, spell.angle);
                    // for (auto point : polygon.points) g_render->circle_3d(point, color(255, 255, 255), 25.f, 2,
                    // 20, 2.f);
                }
            }

            /* auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );

            for ( const auto poly : draw_poly ) {
                g_render->polygon_3d( poly, Color( 255, 255, 255, 255 ), Renderer::outline, 4.f );
            }*/
        }


        /* else {

             for ( auto spell : g_features->spell_detector->get_active_spells( ) ) {

                 auto draw_color = spell.hitbox_area.is_inside( g_local->position ) &&
                        spell.time_till_impact( g_local->position ) < 0.05f ?
                    Color( 255, 50, 50 ) :
                    Color( 255, 255, 255 );


                 g_render->polygon_3d( spell.hitbox_area, draw_color, 2, 3.f );
             }
        }*/

        /* auto cage = g_features->spell_detector->get_veigar_cage( );
        if ( cage ) {
            g_render->circle_3d( cage->position, Color( 255, 25, 25, 255 ), 400.f, 2, 72, 3.f );
            g_render->circle_3d( cage->position, Color( 255, 255, 25, 255 ), 250.f, 2, 72, 3.f );
        }*/

        //for ( auto &point : m_last_evade_points )
        //     g_render->circle_3d(
        //         point, Color::white( ).alpha( 75 ), 15.f, Renderer::outline | Renderer::filled, -1, 1.f );

        //for ( auto &point : m_unsafe_positions )
        //g_render->circle_3d(
        //        point, color::red( ).alpha( 50 ), 15.f, c_renderer::outline | c_renderer::filled, -1, 1.f );

        //for (auto point : m_render_tether_points) g_render->circle_3d(point, Color::white(), 20, 2, 10);
        //for ( const auto point : m_valid_tether_points ) g_render->circle_3d( point, Color::green( ), 15.f, Renderer::outline, 16, 2.f );
        //g_render->circle_3d(g_features->orbwalker->get_extrapolated_cursor_position(), Color::blue().alpha(50), 20.f,
        //                    Renderer::filled | Renderer::outline, 20, 1.f);
        //g_render->circle_3d(m_alt_tether_point, color::white().alpha(40), 10.f, c_renderer::e_3d_circle_flags::filled | c_renderer::e_3d_circle_flags::outline, 20, 1.f);

        /* if ( *g_time - m_last_spell_calculation_time <= 1.f ) {

            //for ( auto &point : m_adjusted_spell_points )
            //    g_render->circle_3d( point, color::white( ).alpha( 50 ), 15.f, c_renderer::outline | c_renderer::filled, -1, 1.f );

              //for ( auto &point : m_adjusted_spell_points )
              //  g_render->circle_3d(
              //      point, color::green( ).alpha( 50 ), 20.f, c_renderer::outline | c_renderer::filled, -1, 1.f );
        }*/


        /*auto aimgr = g_local->get_ai_manager();
        if (aimgr) {

             g_render->circle_3d(g_local->position, Color::green(), 10.f, Renderer::outline, -1, 2.f);
            g_render->circle_3d(aimgr->get_server_position(), Color::red(), 10.f, Renderer::outline, -1, 2.f);


        }*/

        if ( g_config->evade.show_position->get< bool >( ) && is_active( ) ) {
            /* g_render->circle_3d( m_goal_position,
                                 color::green( ).alpha( 50 ),
                                 15.f,
                                 c_renderer::e_3d_circle_flags::filled | c_renderer::e_3d_circle_flags::outline,
                                 20,
                                 1.f );

            if ( m_adjusted ) {
                g_render->circle_3d( m_adjusted_goal,
                                     color::blue( ).alpha( 50 ),
                                     10.f,
                                     c_renderer::e_3d_circle_flags::filled | c_renderer::e_3d_circle_flags::outline,
                                     20,
                                     1.f );
            }*/

            auto arrow_direction{ m_goal_position };

            if ( is_active( ) ) {
                if ( m_spell_evading ) {
                    /* g_render->line_3d( g_local->position, m_spell_end_position, Color( 50, 255, 50 ), 3.f );

                    Vec2 sp{ };
                    if ( world_to_screen( m_spell_end_position, sp ) ) {
                        g_render->filled_circle( sp, Color( 50, 255, 50 ), 3.f, 16 );
                    }*/

                    arrow_direction = m_spell_end_position;
                } else if ( m_goal_position.length( ) > 0.f ) {
                    arrow_direction = m_goal_position;
                    /* g_render->line_3d( g_local->position,
                                       m_goal_position,
                                       m_adjusted ? Color( 60, 80, 250 ) : Color( 255, 255, 25 ),
                                       3.f );

                    Vec2 sp{ };
                    if ( world_to_screen( m_goal_position, sp ) ) {
                        g_render->filled_circle(
                            sp, m_adjusted ? Color( 60, 80, 250 ) : Color( 255, 255, 25 ), 3.f, 16 );
                    }*/

                    /* auto pred = g_features->prediction->predict_default(
                        g_local->index, g_features->orbwalker->get_ping( ), false );

                    auto path_danger = get_path_danger( pred.has_value(  ) ? *pred : g_local->position, m_goal_position,
                    g_local->movement_speed ); auto text_color  = path_danger > 0 ? Color( 255, 25, 50 ) : Color( 25,
                    255, 50 );

                    g_render->text_3d(
                        m_goal_position,
                        text_color,
                        g_fonts->get_nexa( ),
                                       std::to_string( path_danger )
                            .c_str( ),
                        32,
                        true );*/
                }
            }

            auto arrow_point = m_goal_position;
            auto point_color = m_adjusted ? Color( 3, 102, 252 ) : Color( 255, 255, 0 );

            g_render->line_3d( g_local->position, arrow_point, point_color, 3.f );

            auto arrow_angle = 42.5f;

            auto temp = ( g_local->position - arrow_point ).rotated_raw( arrow_angle );

            auto arrow_side_left = arrow_point.extend( arrow_point + temp, 30.f );

            temp                  = ( g_local->position - arrow_point ).rotated_raw( -arrow_angle );
            auto arrow_side_right = arrow_point.extend( arrow_point + temp, 30.f );

            auto arrow_color = point_color;
            auto bg_color    = Color( 0, 0, 0, 225 );

            auto shadow_depth = 6.f;

            Vec3 shadow_point = { arrow_point };
            shadow_point.y -= shadow_depth;

            Vec3 shadow_left = { arrow_side_left };
            shadow_left.y -= shadow_depth;

            auto shadow_right{ arrow_side_right };
            shadow_right.y -= shadow_depth;

            //g_render->line_3d( shadow_point, shadow_left, bg_color, 4.f );
            //g_render->line_3d( shadow_point, shadow_right, bg_color, 4.f );
            //g_render->line_3d( g_local->position.extend( shadow_point, shadow_point.dist_to( g_local->position ) - 2.f ), shadow_point.extend( g_local->position, -2.f ), bg_color, 3.f );

            g_render->line_3d( arrow_point, arrow_side_left, arrow_color, 4.f );
            g_render->line_3d( arrow_point, arrow_side_right, arrow_color, 4.f );
            g_render->line_3d(
                g_local->position.extend( arrow_point, arrow_point.dist_to( g_local->position ) - 1.f ),
                arrow_point.extend( g_local->position, -1.f ),
                arrow_color,
                4.f
            );

            //auto calculated_path = calculate_path_danger( g_features->prediction->get_server_position( g_local->index ), m_goal_position, g_local->movement_speed );

            /*g_render->text_3d( arrow_point,
                               calculated_path.danger > 0 ? Color( 255, 50, 50 ) : Color( 50, 255, 50 ),
                               g_fonts->get_zabel( ),
                               std::to_string( calculated_path.danger ).data( ),
                               20,
                               true );

            g_render->circle_3d( calculated_path.projected_position,
                                 Color( 255, 255, 0, 75 ),
                                 15.f,
                                 Renderer::outline | Renderer::filled,
                                 32,
                                 2.f );*/
        }

        if ( g_config->evade.visualize_maximum_angle->get< bool >( ) ) {
            float max_angle{ };
            switch ( g_config->evade.humanizer_maximum_angle->get< int >( ) ) {
            case 1:
                max_angle = 150.f;
                break;
            case 2:
                max_angle = 130.f;
                break;
            case 3:
                max_angle = 110.f;
                break;
            case 4:
                max_angle = 90.f;
                break;
            case 5:
                max_angle = 70.f;
                break;
            case 6:
                max_angle = 50.f;
                break;
            case 7:
                max_angle = 37.5f;
                break;
            default:
                break;
            }

            float max_dangerous_angle{ };
            switch ( g_config->evade.humanizer_dangerous_maximum_angle->get< int >( ) ) {
            case 1:
                max_dangerous_angle = 150.f;
                break;
            case 2:
                max_dangerous_angle = 130.f;
                break;
            case 3:
                max_dangerous_angle = 110.f;
                break;
            case 4:
                max_dangerous_angle = 90.f;
                break;
            case 5:
                max_dangerous_angle = 70.f;
                break;
            case 6:
                max_dangerous_angle = 50.f;
                break;
            case 7:
                max_dangerous_angle = 37.5f;
                break;
            default:
                break;
            }

            if ( max_dangerous_angle > 0.f ) {
                auto dangerous_sector = sdk::math::Sector(
                    g_local->position,
                    g_local->position + g_local->get_direction( ),
                    max_dangerous_angle * 2.f,
                    300.f
                );

                g_render->polygon_3d(
                    dangerous_sector.to_polygon_new( ),
                    g_features->orbwalker->get_pulsing_color( ),
                    Renderer::outline,
                    2.f
                );
            }

            if ( max_angle > 0.f ) {
                auto sector = sdk::math::Sector(
                    g_local->position,
                    g_local->position + g_local->get_direction( ),
                    max_angle * 2.f,
                    275.f
                );

                g_render->polygon_3d( sector.to_polygon_new( ), Color( 255, 255, 255 ), Renderer::outline, 2.f );
            }
        }

        if ( !g_config->evade.show_text->get< bool >( ) ) return;

        Vec2 sp{ };

        g_local.update( );
        if ( !sdk::math::world_to_screen( g_local->position, sp ) ) return;

        const auto text_size     = g_render->get_text_size( "evade", g_fonts->get_zabel_16px( ), 16 );
        Vec2       text_position = { sp.x - text_size.x / 2.f, sp.y - text_size.y / 2.f };

        text_position.y += 8;

        if ( !g_config->evade.toggle->get< bool >( ) )
            g_render->text_shadow(
                text_position,
                Color( 255, 25, 25 ).alpha( 220 ),
                g_fonts->get_zabel_16px( ),
                "evade",
                16
            );
        else if ( !is_active( ) )
            g_render->text_shadow(
                text_position,
                Color( 25, 255, 25 ).alpha( 220 ),
                g_fonts->get_zabel_16px( ),
                "evade",
                16
            );
        else g_render->text_shadow( text_position, Color( 255, 255, 0, 220 ), g_fonts->get_zabel_16px( ), "evade", 16 );

        if (false && g_config->evade.preset_mode->get<int>() == 3) {

            auto size = g_render->get_text_size("evade", g_fonts->get_zabel_16px(), 16);

            text_position.y += 12;
            text_position.x += size.x / 2.f;

            auto preset_size = g_render->get_text_size("rage", g_fonts->get_block(), 8);
            text_position.x -= preset_size.x / 2.f;

            g_render->text_shadow(text_position, Color::white(), g_fonts->get_block(), "rage", 8);

        }
    }

    auto Evade::run( ) -> void{
#if enable_lua
        if ( g_lua2 ) {
            g_lua2->execute_locked( []( ) -> void{ g_lua2->run_callback( ct_hash( "features.evade" ) ); } );
        }
#endif

        // // TODO: REMOVE THIS, THIS IS THE AWARENESS ONLY CHANGE ONLY
        // return;

        if ( m_disable_this_tick_lua ) {
            m_disable_this_tick_lua = false;
            return;
        }

        update_preset_settings();

        m_minimum_danger = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo
                               ? g_config->evade.combo_min_danger_level->get< int >( )
                               : g_config->evade.min_danger_level->get< int >( );

        if ( m_raw_bounding_radius == 0.f ) {
            m_raw_bounding_radius = g_features->prediction->get_champion_radius(
                rt_hash( g_local->champion_name.text )
            );

            switch ( g_config->evade.hitbox_modifier->get< int >( ) ) {
            case 1:
                m_bounding_radius = m_raw_bounding_radius * 1.1f;
                break;
            case 2:
                m_bounding_radius = m_raw_bounding_radius * 1.2f;
                break;
            case 3:
                m_bounding_radius = m_raw_bounding_radius * 1.3f;
                break;
            case 4:
                m_bounding_radius = m_raw_bounding_radius * 1.4f;
                break;
            case 5:
                m_bounding_radius = m_raw_bounding_radius * 1.5f;
                break;
            default:
                m_bounding_radius = m_raw_bounding_radius;
                break;
            }
        }

        if ( g_config->evade.hitbox_modifier->get< int >( ) != m_last_radius_modifier ) {
            switch ( g_config->evade.hitbox_modifier->get< int >( ) ) {
            case 1:
                m_bounding_radius = m_raw_bounding_radius * 1.1f;
                break;
            case 2:
                m_bounding_radius = m_raw_bounding_radius * 1.2f;
                break;
            case 3:
                m_bounding_radius = m_raw_bounding_radius * 1.3f;
                break;
            case 4:
                m_bounding_radius = m_raw_bounding_radius * 1.4f;
                break;
            case 5:
                m_bounding_radius = m_raw_bounding_radius * 1.5f;
                break;
            default:
                m_bounding_radius = m_raw_bounding_radius;
                break;
            }

            m_last_radius_modifier = g_config->evade.hitbox_modifier->get< int >( );
        }

        m_should_fix_height = false;

        if ( !g_config->evade.toggle->get< bool >( ) ) m_is_evading = false;

        if ( !g_config->evade.toggle->get< bool >( )
            || g_local->is_dead( )
            || helper::get_current_hero( ) == EHeroes::olaf && g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "OlafRagnarok" )
            )
            || helper::get_current_hero( ) == EHeroes::kog_maw && g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "KogMawIcathianSurprise" )
            )
            || helper::get_current_hero( ) == EHeroes::xerath && g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "XerathLocusOfPower2" )
            ) ||
            helper::get_current_hero() == EHeroes::karthus &&
                g_features->buff_cache->get_buff(g_local->index, ct_hash("KarthusDeathDefiedBuff"))
            || g_features->orbwalker->is_hard_crowdcontrolled( )
            || g_features->buff_cache->has_buff_of_type( g_local->index, EBuffType::snare ) ) {
            if ( m_blocked_input || g_config->evade.block_input->get< bool >( ) && m_is_evading ) {
                g_function_caller->set_issue_order_blocked( false );
                m_is_input_blocked = false;
            }

            m_is_evading = false;
            return;
        }

        m_spell_evading = *g_time < m_spellcast_end_time && *g_time - m_last_spell_time <= 0.5f;

        m_blocked_input = g_function_caller->is_movement_blocked( );
        if ( m_blocked_input != m_is_input_blocked ) g_function_caller->set_issue_order_blocked( m_is_input_blocked );

        //if ( m_dynamic_safe_distance ) m_safe_distance = g_local->movement_speed * 0.3f;

        if ( m_is_evading ) move_to_safe_position( );
        else {
            auto       local_position  = g_local->position;
            const auto server_position = g_features->prediction->get_server_position( g_local->index );
            if ( server_position.length( ) > 0.f ) local_position = server_position;

            if ( !is_position_safe( local_position ) ) calculate_safe_position( );
        }
    }

    auto Evade::calculate_safe_position( ) -> void {
        const auto evade_point =
            get_smart_position(Vec3(), g_config->evade.autoattack_sync->get<bool>() ? g_features->orbwalker->in_attack() : false);

        if ( evade_point.length( ) <= 0.f ) return;

        m_goal_position    = evade_point;
        m_evade_start_time = *g_time;
        m_last_calc_time   = *g_time;
        m_start_position   = g_local->position;
        m_free_time_left   = m_last_free_duration;
        m_was_forced_fastest = g_config->evade.autoattack_sync->get<bool>()  && g_features->orbwalker->in_attack();

        auto sci       = g_local->spell_book.get_spell_cast_info();
        if (g_config->evade.autoattack_sync->get<bool>() && sci && sci->is_autoattack)
        {

            auto collision = get_colliding_skillshot(g_local->index);
            if (collision) {

                m_cancel_autoattack =
                    sci->server_cast_time + g_local->position.dist_to(evade_point) / g_local->movement_speed >=
                    *g_time + collision->time_until_collision;

                if (m_cancel_autoattack) std::cout << "[ $YNC ] Force canceling aa due to not enough time to finish windup\n";
            }
        }

        m_evade_direction = ( evade_point - g_local->position ).normalize( );

        if ( g_config->evade.block_input->get< bool >( ) ) {
            g_function_caller->set_issue_order_blocked( true );
            m_is_input_blocked = true;
        }

        move_to_safe_position( );
    }

    auto Evade::move_to_safe_position( ) -> void{
        g_local.update( );

        auto ai_aimgr = g_local->get_ai_manager( );
        Vec3 future_position{ g_features->prediction->get_server_position( g_local->index ) };

        if (is_position_safe(  g_local->position))
        { //||
            //!m_adjusted && g_local->position.dist_to( m_goal_position ) <= 10.f ||
            //m_adjusted && g_local->position.dist_to( m_adjusted_goal ) <= 10.f ) {
            /* if ( m_is_evading && m_adjusted ) {

                if ( g_config->evade.humanizer_randomize_pattern->get< bool >( ) ) {
                    m_post_evade_point = g_local->position.extend(
                        g_local->position + g_local->get_direction( ),
                        200.f
                    );

                    int   random_percent = rand( ) % 80 + 20;
                    float move_time = g_local->position.dist_to( m_post_evade_point ) / g_local->movement_speed * 0.6f;
                    m_post_evade_expire_time = *g_time + move_time / 100.f * random_percent;

                    //std::cout << "randomized pattern " << random_percent << "%\n";
                } else {
                    m_post_evade_expire_time = *g_time + 0.075f;
                    m_post_evade_point       = g_local->position.extend(
                        g_local->position + g_local->get_direction( ),
                        200.f
                    );
                }
            }*/

            auto       local_position  = g_local->position;
            const auto server_position = g_features->prediction->get_server_position( g_local->index );
            if ( server_position.length( ) > 0.f ) local_position = server_position;

            constexpr auto angle        = 125.f;
            constexpr auto rotate_angle = angle / 2.f * ( 3.14159265359f / 180.f );

            auto tether_direction = m_evade_direction;

            m_post_evade_tether_points = g_render->get_3d_circle_points( local_position, 175.f, 32 );

            m_goal_position    = Vec3( );
            m_adjusted_goal    = Vec3( );
            m_evade_start_time = 0.f;
            m_last_calc_time   = 0.f;
            m_is_evading       = false;
            m_adjusted         = false;
            m_last_evade_time  = *g_time;
            m_was_forced_fastest = false;
            m_cancel_autoattack  = false;

            g_features->orbwalker->allow_fast_move( );

            if ( g_config->evade.block_input->get< bool >( ) ) {
                g_function_caller->set_issue_order_blocked( false );
                m_is_input_blocked = false;
            }

            return;
        }

        m_tether_point = Vec3( );

        // recalculate path
        if ( !m_spell_evading && *g_time - m_last_calc_time > 0.1f ) {
            const auto possible_position = get_smart_position( m_goal_position );

            if ( possible_position != Vec3( ) ) {
                auto local_position = g_features->prediction->get_server_position(g_local->index);

                bool skip{ };

                /*auto current_path_danger = calculate_path_danger(
                    local_position,
                    m_goal_position,
                    g_local->movement_speed
                ).danger;
                auto possible_danger = calculate_path_danger(
                    local_position,
                    possible_position,
                    g_local->movement_speed
                ).danger;*/

                auto force_path = !is_position_safe(m_goal_position) && is_position_safe( possible_position );

                if ( !skip ) {
                    const auto     current_dist  = m_goal_position.dist_to( local_position );
                    const auto     possible_dist = possible_position.dist_to( local_position );
                    constexpr auto threshold{ 100.f };
                    auto           angle_threshold{ 25.f };

                    // if ( current_dist > 275.f ) angle_threshold = 10.f;
                    // else if ( current_dist > 200.f ) angle_threshold = 20.f;
                    // else if (current_dist > 150.f) angle_threshold = 40.f;
                    // else if (current_dist > 100.f) angle_threshold = 60.f;
                    // else angle_threshold = 25.f;

                    auto in_combo       = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo;
                    auto dodge_priority = in_combo
                                              ? g_config->evade.combo_dodge_priority->get< int >( )
                                              : g_config->evade.default_dodge_priority->get< int >( );

                    const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
                    auto       should_swap_paths{ force_path };

                    // prioritize cursor position
                    if ( !should_swap_paths ) {
                        switch ( dodge_priority ) {
                        case 0:
                            should_swap_paths = m_goal_position.dist_to( local_position ) >
                                possible_position.dist_to( local_position ) + 10.f;
                            break;
                        case 1:
                        {
                            auto       v1            = g_local->get_direction( );
                            auto       v2            = cursor - g_local->position;
                            auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                            const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                            v1                   = possible_position - g_local->position;
                            v2                   = cursor - g_local->position;
                            dot                  = v1.normalize( ).dot_product( v2.normalize( ) );
                            const auto new_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                            auto angle_diff = current_angle > new_angle ? current_angle - new_angle : 0.f;

                            should_swap_paths = new_angle <= 30.f && angle_diff <= 50.f;
                            break;
                        }
                        case 2: {
                            auto possible_weight = get_position_weight( possible_position );
                            auto current_weight  = get_position_weight( m_goal_position );

                            should_swap_paths = possible_weight > current_weight;
                            break;
                        }
                        case 3:
                        {
                            auto       v1            = m_goal_position - local_position;
                            auto       v2            = cursor - local_position;
                            auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                            const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                            v1                   = possible_position - local_position;
                            v2                   = cursor - local_position;
                            dot                  = v1.normalize( ).dot_product( v2.normalize( ) );
                            const auto new_angle = acos( dot ) * 180.f / 3.14159265358979323846f;


                            should_swap_paths = new_angle + 10.f < current_angle &&
                                m_goal_position.dist_to( local_position ) + 15.f >
                                possible_position.dist_to( local_position );
                            break;
                        }
                        default:
                            break;
                        }
                    }

                    // if recalculated path is better than current path
                    if ( should_swap_paths ) {
                        m_goal_position  = possible_position;
                        m_last_calc_time = *g_time;
                        m_adjusted       = false;

                        m_evade_direction = ( possible_position - g_local->position ).normalize( );

                        std::cout << "[ MagicEvade ] recalculated path | " << *g_time << std::endl;
                    }
                }
                //else std::cout << "[ EVADE ]: Skipped recalculation due to loma syy\n";
            }
            else if( !is_position_safe( m_goal_position )) {

                m_goal_position    = Vec3();
                m_adjusted_goal    = Vec3();
                m_evade_start_time = 0.f;
                m_last_calc_time   = 0.f;
                m_is_evading       = false;
                m_adjusted         = false;
                m_last_evade_time  = *g_time;

                g_features->orbwalker->allow_fast_move();

                if (g_config->evade.block_input->get<bool>())
                {
                    g_function_caller->set_issue_order_blocked(false);
                    m_is_input_blocked = false;
                }

                return;



            }
        }

        if ( m_spell_evading && m_reset_path && *g_time - m_last_move_time > 0.05f &&
            g_features->orbwalker->send_move_input( m_spell_end_position, true ) ) {
            m_last_move_time = *g_time;
            m_reset_path     = false;
        }

        if ( m_spell_evading ) return;

        if ( *g_time - m_last_move_time > 0.05f || m_goal_position != m_last_goal_pos ) {

            if (g_features->orbwalker->in_attack() && g_config->evade.autoattack_sync->get<bool>() &&
                 !m_cancel_autoattack) {

                if (!m_was_forced_fastest) {
                    m_goal_position    = Vec3();
                    m_adjusted_goal    = Vec3();
                    m_evade_start_time = 0.f;
                    m_last_calc_time   = 0.f;
                    m_is_evading       = false;
                    m_adjusted         = false;
                    m_last_evade_time  = *g_time;
                    m_is_evading       = false;

                    std::cout << "[ $YNC ] Force evade to evaluate new path\n";
                }
                else
                    std::cout << "[ $YNC ] waiting for AA to finish\n";

                return;
            }

            auto aimgr = g_local->get_ai_manager( );
            if ( !aimgr ) return;

            auto       local_position  = g_local->position;
            const auto server_position = g_features->prediction->get_server_position( g_local->index );
            if ( server_position.length( ) > 0.f ) local_position = server_position;

            auto near_goal = local_position.dist_to( m_goal_position ) <= 200.f;
            auto goal_point{ m_goal_position };

            if (near_goal && g_features->orbwalker->get_mode() != Orbwalker::EOrbwalkerMode::none) {

                auto extend_value   = 200.f;
                auto possible_point = g_local->position.extend( goal_point, extend_value );

                if ( is_position_safe( possible_point ) && !g_navgrid->is_wall( possible_point ) ) {
                    goal_point      = possible_point;
                    m_goal_position = possible_point;
                    m_adjusted      = true;

                    std::cout << "[ cougarvade ] Adjusted goal | " << *g_time << std::endl;
                }
            }

            auto path     = aimgr->get_path( );
            auto path_end = path[ path.size( ) - 1 ];

            if ( goal_point.dist_to( path_end ) > 30.f ) {
                if ( g_features->orbwalker->send_move_input( goal_point, true ) ) {
                    m_last_move_time = *g_time;
                    m_last_goal_pos  = m_goal_position;
                    m_cancel_autoattack = false;
                }
            }
        }

        m_is_evading = true;
    }

    auto Evade::calculate_minimum_dodge_time(Vec3 goal_position) const  -> float {

        std::vector<Vec3> available_positions{};

        auto lowest_impact_time = std::numeric_limits<float>::max();

        auto local_position{ g_local->position };

        const auto cursor_position = g_pw_hud->get_hud_manager()->cursor_position_unclipped;
        auto       standing_still  = false;

        auto aimgr = g_local->get_ai_manager();
        if (aimgr)
        {
            auto path      = aimgr->get_path();
            standing_still = !aimgr->is_moving || path.size() <= 1;
            local_position = aimgr->get_server_position();
        }

        auto ping{ 0.f };
        switch (g_config->evade.include_ping_mode->get<int>())
        {
        case 0:
            ping = g_features->orbwalker->get_ping() + 0.033f;
        default:
            ping = g_features->orbwalker->get_ping() / 2.f + 0.033f;
            break;
        }

        std::vector<Vec3> unsafe_points{};

        auto evadable_spells = get_dangerous_spells(local_position);

        if (goal_position.length() > 0.f)
        {

            auto secondary_spells = get_dangerous_spells(goal_position);
            evadable_spells.insert(evadable_spells.end(), secondary_spells.begin(), secondary_spells.end());
        }

        const auto evadable_missiles = get_dangerous_missiles(local_position);

        for (auto &spell : evadable_spells) // get all possible evade positions
        {
            // humanizer
            if (!spell.is_dangerous() && g_config->evade.humanizer_delay->get<int>() > 0 &&
                    spell.start_time + 0.025f * g_config->evade.humanizer_delay->get<int>() > *g_time ||
                spell.is_dangerous() && g_config->evade.humanizer_dangerous_delay->get<int>() > 0 &&
                    spell.start_time + 0.025f * g_config->evade.humanizer_dangerous_delay->get<int>() > *g_time)
                continue;

            auto dodge_points = spell.dodge_points;

            auto time_until_impact = spell.time_till_impact(local_position);
            if (time_until_impact < lowest_impact_time) lowest_impact_time = time_until_impact;

            float max_cursor_angle{};
            float max_angle{};
            if (spell.is_dangerous())
            {
                switch (g_config->evade.humanizer_dangerous_maximum_angle->get<int>())
                {
                case 1:
                    max_angle        = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle        = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle        = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle        = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle        = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle        = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle        = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }
            else
            {
                switch (g_config->evade.humanizer_maximum_angle->get<int>())
                {
                case 1:
                    max_angle        = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle        = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle        = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle        = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle        = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle        = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle        = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }

            if (max_angle < 360.f) max_angle /= 2.f;

            bool found_point{};

            for (auto &point : dodge_points)
            {
                if (g_navgrid->is_wall(point) || point.dist_to(local_position) <= 70.f) continue;


                auto time_to_point = point.dist_to(local_position) / g_local->movement_speed;
                if (time_to_point + ping >= time_until_impact) continue;

                if (!standing_still && max_angle < 360.f)
                {
                    auto       v1            = g_local->get_direction();
                    auto       v2            = point - g_local->position;
                    auto       dot           = v1.normalize().dot_product(v2.normalize());
                    const auto current_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    v1                      = cursor_position - g_local->position;
                    const auto cursor_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    if (current_angle > max_angle || cursor_angle > max_cursor_angle)
                    {
                        if (!found_point && is_position_safe(point)) found_point = true;

                        continue;
                    }
                }

                if (is_position_safe(point, spell.danger))
                {
                    available_positions.push_back(point);
                    found_point = true;
                }
                else
                    unsafe_points.push_back(point);
            }
        }

        std::vector<Vec3> possible_dodges{};

        for (auto &missile : evadable_missiles)
        {
            // humanizer
            if (!missile.is_dangerous() && g_config->evade.humanizer_delay->get<int>() > 0 &&
                    missile.start_time + 0.025f * g_config->evade.humanizer_delay->get<int>() > *g_time ||
                missile.is_dangerous() && g_config->evade.humanizer_dangerous_delay->get<int>() > 0 &&
                    missile.start_time + 0.025f * g_config->evade.humanizer_dangerous_delay->get<int>() > *g_time)
                continue;

            sdk::math::Polygon polygon{};
            if (missile.type == SpellDetector::ESpellType::line)
                polygon = get_line_segment_points(missile.position, missile.end_position, missile.radius);
            else
                polygon = get_circle_segment_points(missile.end_position, missile.radius,
                                                    g_config->evade.calculation_accuracy->get<int>());


            auto time_until_impact = missile.time_till_impact(local_position);
            if (time_until_impact < lowest_impact_time) lowest_impact_time = time_until_impact;

            float max_cursor_angle{};
            float max_angle{};
            if (missile.is_dangerous())
            {
                switch (g_config->evade.humanizer_dangerous_maximum_angle->get<int>())
                {
                case 1:
                    max_angle        = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle        = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle        = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle        = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle        = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle        = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle        = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }
            else
            {
                switch (g_config->evade.humanizer_maximum_angle->get<int>())
                {
                case 1:
                    max_angle        = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle        = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle        = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle        = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle        = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle        = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle        = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }

            if (max_angle < 360.f) max_angle /= 2.f;

            for (auto &point : polygon.points)
            {
                auto time_to_point = local_position.dist_to(point) / g_local->movement_speed + ping;
                if (g_navgrid->is_wall(point) || time_to_point >= time_until_impact) continue;

                if (missile.type == SpellDetector::ESpellType::line && missile.speed > 0)
                {
                    auto line_point = get_closest_line_point(missile.position, missile.end_position, point);

                    if (time_to_point >= missile.time_till_impact(line_point)) continue;
                }

                if (!standing_still && max_angle < 360.f)
                {
                    auto       v1            = g_local->get_direction();
                    auto       v2            = point - local_position;
                    auto       dot           = v1.normalize().dot_product(v2.normalize());
                    const auto current_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    v1                      = cursor_position - local_position;
                    const auto cursor_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    if (current_angle > max_angle || cursor_angle > max_cursor_angle) continue;
                }

                if (is_position_safe(point)) available_positions.push_back(point);
            }

            // if (!found_point && (try_spell_evade(spell) || try_summoner_evade(spell))) return vec3(0, 0, 0);
        }

        auto movable_distance = lowest_impact_time * g_local->movement_speed;

        if (false && !m_is_evading)
        {
            auto nearby_points =
                get_circle_segment_points(local_position, std::clamp(movable_distance, 75.f, 300.f), 16);

            for (auto point : nearby_points.points)
            {
                if (g_navgrid->is_wall(point)) continue;

                auto time_to_point = point.dist_to(local_position) / g_local->movement_speed;
                if (time_to_point + ping >= lowest_impact_time) continue;

                if (is_position_safe(point)) available_positions.push_back(point);
            }
        }

        Vec3 best_position{};
        auto lowest_distance = std::numeric_limits<float>::max();
        auto lowest_path_danger{ 5 };

        std::vector<Vec3> free_positions{ available_positions };


        /*for ( auto& position : available_positions ) {
            if ( position.dist_to( local_position ) <= 75.f || position.dist_to(local_position) > movable_distance )
                continue;

            auto path_instance = calculate_path_danger( local_position, position, g_local->movement_speed );
            if ( path_instance.danger > lowest_path_danger ) continue;

            /* for ( int i = 0; i <= 2; i++ ) {

                Vec3 path_point = g_local->position.extend( position, g_local->position.dist_to( position ) / 3.f * i );

                for ( auto spell : get_dangerous_spells( path_point ) ) {

                    auto  collision_delay  = spell.time_till_impact( path_point );
                    if ( collision_delay <= 0.f) {
                        is_dangerous_path = true;
                        break;
                    }

                    float movable_distance = g_local->movement_speed * collision_delay;
                    movable_distance -= local_position.dist_to( path_point );

                    if ( movable_distance > g_local->position.dist_to( position ) ) continue;

                    auto predicted_hitbox = spell.get_future_hitbox(
                        false, local_position.dist_to( path_point ) / g_local->movement_speed );

                    Vec3 simulated_position = path_point.extend( position, movable_distance );

                    if ( predicted_hitbox.is_inside( simulated_position ) || movable_distance <= 0.f ) {
                        is_dangerous_path = true;
                        break;
                    }
                }

                if ( is_dangerous_path ) break;
            }

            free_positions.push_back( position );

            if ( position.dist_to( local_position ) < lowest_distance || path_instance.danger < lowest_path_danger ) {
                best_position   = position;
                lowest_distance = local_position.dist_to( position );

                if ( path_instance.danger < lowest_path_danger ) {
                    free_positions.clear( );
                    free_positions.push_back( position );
                    lowest_path_danger = path_instance.danger;
                }
            }
        }
        */

        if (free_positions.empty()) return 999.f;

        auto in_combo = g_features->orbwalker->get_mode() == Orbwalker::EOrbwalkerMode::combo;
        Vec3 fastest_dodge_position{};

        float free_action_duration{ 0.f };

        float lowest_evade_distance = std::numeric_limits<float>::max();
        for (auto &position : free_positions) {

            if (position.dist_to(local_position) < lowest_evade_distance)
            {
                fastest_dodge_position = position;
                lowest_evade_distance  = position.dist_to(local_position);
            }
        }

        return local_position.dist_to(fastest_dodge_position) / g_local->movement_speed;
    }


    auto Evade::get_smart_position(Vec3 goal_position, bool force_fastest) -> Vec3 {
        // Function is used in LUA, message @tore if you change args

        std::vector< Vec3 > available_positions{ };

        auto lowest_impact_time = std::numeric_limits< float >::max( );

        auto local_position{ g_local->position };

        const auto cursor_position = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        auto       standing_still  = false;

        auto aimgr = g_local->get_ai_manager( );
        if ( aimgr ) {
            auto path      = aimgr->get_path( );
            standing_still = !aimgr->is_moving || path.size( ) <= 1;
            local_position = aimgr->get_server_position();
        }

        auto ping{ 0.f };
        switch ( g_config->evade.include_ping_mode->get< int >( ) ) {
        case 0:
            ping = g_features->orbwalker->get_ping( ) + 0.033f;
        default:
            ping = g_features->orbwalker->get_ping( ) / 2.f + 0.033f;
            break;
        }

        std::vector< Vec3 > unsafe_points{ };

        auto evadable_spells   = get_dangerous_spells(local_position);

        if (goal_position.length() > 0.f) {

            auto secondary_spells = get_dangerous_spells(goal_position);
            evadable_spells.insert(evadable_spells.end(), secondary_spells.begin(), secondary_spells.end());
        }

        const auto evadable_missiles = get_dangerous_missiles(local_position);

        for ( auto& spell : evadable_spells ) // get all possible evade positions
        {
            // humanizer
            if ( !spell.is_dangerous( ) && g_config->evade.humanizer_delay->get< int >( ) > 0 && spell.start_time +
                0.025f * g_config->evade.humanizer_delay->get< int >( ) > *g_time ||
                spell.is_dangerous( ) && g_config->evade.humanizer_dangerous_delay->get< int >( ) > 0 && spell.
                start_time + 0.025f * g_config->evade.humanizer_dangerous_delay->get< int >( ) > *g_time )
                continue;

            auto dodge_points = spell.dodge_points;

            auto time_until_impact = spell.time_till_impact( local_position );
            if ( time_until_impact < lowest_impact_time ) lowest_impact_time = time_until_impact;

            float max_cursor_angle{ };
            float max_angle{ };
            if ( spell.is_dangerous( ) ) {
                switch ( g_config->evade.humanizer_dangerous_maximum_angle->get< int >( ) ) {
                case 1:
                    max_angle = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }
            else {
                switch ( g_config->evade.humanizer_maximum_angle->get< int >( ) ) {
                case 1:
                    max_angle = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }

            if ( max_angle < 360.f ) max_angle /= 2.f;

            bool found_point{ };

            for ( auto& point : dodge_points ) {
                if (g_navgrid->is_wall(point) || point.dist_to( local_position ) <= 70.f ) continue;


                auto time_to_point = point.dist_to(local_position) / g_local->movement_speed;
                if (time_to_point + ping >= time_until_impact) continue;

                if ( !standing_still && max_angle < 360.f ) {
                    auto       v1            = g_local->get_direction( );
                    auto       v2            = point - g_local->position;
                    auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    v1                      = cursor_position - g_local->position;
                    const auto cursor_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    if ( current_angle > max_angle || cursor_angle > max_cursor_angle ) {
                        if ( !found_point && is_position_safe( point ) ) found_point = true;

                        continue;
                    }
                }

                if ( is_position_safe( point, spell.danger ) ) {
                    available_positions.push_back( point );
                    found_point = true;
                } else unsafe_points.push_back( point );
            }

            if ( !found_point && !m_is_evading && ( try_spell_evade( spell ) || try_summoner_evade( spell ) ) )
                return
                    Vec3( 0, 0, 0 );
        }

        if ( !unsafe_points.empty( ) ) m_unsafe_positions = unsafe_points;

        std::vector< Vec3 > possible_dodges{ };

        for ( auto& missile : evadable_missiles ) {
            // humanizer
            if ( !missile.is_dangerous( ) && g_config->evade.humanizer_delay->get< int >( ) > 0 &&
                missile.start_time + 0.025f * g_config->evade.humanizer_delay->get< int >( ) > *g_time ||
                missile.is_dangerous( ) && g_config->evade.humanizer_dangerous_delay->get< int >( ) > 0 &&
                missile.start_time + 0.025f * g_config->evade.humanizer_dangerous_delay->get< int >( ) > *g_time )
                continue;

            sdk::math::Polygon polygon{ };
            if ( missile.type == SpellDetector::ESpellType::line )
                polygon = get_line_segment_points(
                    missile.position,
                    missile.end_position,
                    missile.radius
                );
            else
                polygon = get_circle_segment_points(
                    missile.end_position,
                    missile.radius,
                    g_config->evade.calculation_accuracy->get< int >( )
                );


            auto time_until_impact = missile.time_till_impact( local_position );
            if ( time_until_impact < lowest_impact_time ) lowest_impact_time = time_until_impact;

            float max_cursor_angle{ };
            float max_angle{ };
            if ( missile.is_dangerous( ) ) {
                switch ( g_config->evade.humanizer_dangerous_maximum_angle->get< int >( ) ) {
                case 1:
                    max_angle = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            } else {
                switch ( g_config->evade.humanizer_maximum_angle->get< int >( ) ) {
                case 1:
                    max_angle = 150.f;
                    max_cursor_angle = 150.f;
                    break;
                case 2:
                    max_angle = 130.f;
                    max_cursor_angle = 130.f;
                    break;
                case 3:
                    max_angle = 110.f;
                    max_cursor_angle = 110.f;
                    break;
                case 4:
                    max_angle = 90.f;
                    max_cursor_angle = 90.f;
                    break;
                case 5:
                    max_angle = 70.f;
                    max_cursor_angle = 70.f;
                    break;
                case 6:
                    max_angle = 50.f;
                    max_cursor_angle = 50.f;
                    break;
                case 7:
                    max_angle = 37.5f;
                    max_cursor_angle = 37.5f;
                    break;
                default:
                    max_angle = 360.f;
                    break;
                }
            }

            if ( max_angle < 360.f ) max_angle /= 2.f;

            for ( auto& point : polygon.points ) {
                auto time_to_point = local_position.dist_to( point ) / g_local->movement_speed + ping;
                if ( g_navgrid->is_wall( point ) || time_to_point >= time_until_impact ) continue;

                if ( missile.type == SpellDetector::ESpellType::line && missile.speed > 0 ) {
                    auto line_point = get_closest_line_point( missile.position, missile.end_position, point );

                    if ( time_to_point >= missile.time_till_impact( line_point ) ) continue;
                }

                if ( !standing_still && max_angle < 360.f ) {
                    auto       v1            = g_local->get_direction( );
                    auto       v2            = point - local_position;
                    auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    v1                      = cursor_position - local_position;
                    const auto cursor_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    if ( current_angle > max_angle || cursor_angle > max_cursor_angle ) continue;
                }

                if ( is_position_safe( point ) ) available_positions.push_back( point );
            }

            //if (!found_point && (try_spell_evade(spell) || try_summoner_evade(spell))) return vec3(0, 0, 0);
        }

        auto movable_distance = lowest_impact_time * g_local->movement_speed;

        if ( false && !m_is_evading ) {
            auto nearby_points = get_circle_segment_points( local_position, std::clamp(movable_distance, 75.f, 300.f), 16 );

            for ( auto point : nearby_points.points ) {
                if ( g_navgrid->is_wall( point ) ) continue;

                 auto time_to_point = point.dist_to(local_position) / g_local->movement_speed;
                if (time_to_point + ping >= lowest_impact_time) continue;

                if (is_position_safe(point)) available_positions.push_back(point);
            }
        }

        Vec3 best_position{ };
        auto lowest_distance = std::numeric_limits< float >::max( );
        auto lowest_path_danger{ 5 };

        std::vector<Vec3> free_positions{ available_positions };


        /*for ( auto& position : available_positions ) {
            if ( position.dist_to( local_position ) <= 75.f || position.dist_to(local_position) > movable_distance )
                continue;

            auto path_instance = calculate_path_danger( local_position, position, g_local->movement_speed );
            if ( path_instance.danger > lowest_path_danger ) continue;

            /* for ( int i = 0; i <= 2; i++ ) {

                Vec3 path_point = g_local->position.extend( position, g_local->position.dist_to( position ) / 3.f * i );

                for ( auto spell : get_dangerous_spells( path_point ) ) {

                    auto  collision_delay  = spell.time_till_impact( path_point );
                    if ( collision_delay <= 0.f) {
                        is_dangerous_path = true;
                        break;
                    }

                    float movable_distance = g_local->movement_speed * collision_delay;
                    movable_distance -= local_position.dist_to( path_point );

                    if ( movable_distance > g_local->position.dist_to( position ) ) continue;

                    auto predicted_hitbox = spell.get_future_hitbox(
                        false, local_position.dist_to( path_point ) / g_local->movement_speed );

                    Vec3 simulated_position = path_point.extend( position, movable_distance );

                    if ( predicted_hitbox.is_inside( simulated_position ) || movable_distance <= 0.f ) {
                        is_dangerous_path = true;
                        break;
                    }
                }

                if ( is_dangerous_path ) break;
            }

            free_positions.push_back( position );

            if ( position.dist_to( local_position ) < lowest_distance || path_instance.danger < lowest_path_danger ) {
                best_position   = position;
                lowest_distance = local_position.dist_to( position );

                if ( path_instance.danger < lowest_path_danger ) {
                    free_positions.clear( );
                    free_positions.push_back( position );
                    lowest_path_danger = path_instance.danger;
                }
            }
        }
        */

        if ( free_positions.empty( ) ) return Vec3( );

        auto in_combo = g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo;
        Vec3 fastest_dodge_position{};

         float free_action_duration{ 0.f };

        // prioritize cursor position
        if ( true || !in_combo && g_config->evade.default_dodge_priority->get< int >( ) > 0
            || in_combo && g_config->evade.combo_dodge_priority->get< int >( ) > 0 ) {
            auto lowest_angle         = std::numeric_limits< float >::max( );
            auto lowest_safe_distance = std::numeric_limits< float >::max( );
            float lowest_evade_distance      = std::numeric_limits<float>::max( );

            auto best_weight{ 0.f };

            auto safe_distance_threshold{ 100.f };

            auto priority_mode = in_combo
                                     ? g_config->evade.combo_dodge_priority->get< int >( )
                                     : g_config->evade.default_dodge_priority->get< int >( );

            auto highest_distance{ 0.f };

            

            for ( auto& position : free_positions ) {

                if (position.dist_to(local_position) < lowest_evade_distance) {

                    fastest_dodge_position = position;
                    lowest_evade_distance  = position.dist_to(local_position);
                    if (force_fastest) best_position = position;
                }

                if (force_fastest) continue;

                if ( g_config->evade.force_longest_path->get< bool >( ) ) {
                    auto distance = get_units_to_safety( g_local->position, position );
                    if ( distance < highest_distance ) continue;

                    best_position    = position;
                    highest_distance = distance;
                    continue;
                }

                if ( priority_mode == 1 ) { // nearest to cursor mode

                    auto       v1            = position - g_local->position;
                    auto       v2            = cursor_position - g_local->position;
                    auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;
                    //auto        distance_to_safe = get_units_to_safety( g_local->position, local_position );

                    if ( current_angle < lowest_angle ) {
                        best_position = position;
                        lowest_angle  = current_angle;
                        //lowest_safe_distance = distance_to_safe;
                    }

                    continue;
                }

                switch ( priority_mode ) {
                case 0:
                {

                    if ( local_position.dist_to( position ) < lowest_safe_distance ) {

                        best_position = position;
                        lowest_safe_distance = local_position.dist_to(position);
                    }

                    break;
                }
                case 1:
                {
                    auto       v1               = position - local_position;
                    auto       v2               = cursor_position - local_position;
                    auto       dot              = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle    = acos( dot ) * 180.f / 3.14159265358979323846f;
                    auto       distance_to_safe = get_units_to_safety( local_position, position );

                    if ( current_angle < lowest_angle &&
                        distance_to_safe <= lowest_safe_distance + safe_distance_threshold ) {
                        best_position        = position;
                        lowest_angle         = current_angle;
                        lowest_safe_distance = distance_to_safe;
                    }

                    break;
                }
                case 2:
                {
                    auto weight = get_position_weight( position );
                    if ( weight < best_weight ) continue;

                    best_position = position;
                    best_weight   = weight;

                    break;
                }
                case 3:
                {
                    auto       v1            = position - local_position;
                    auto       v2            = cursor_position - local_position;
                    auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                    const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                    if ( current_angle < lowest_angle ) {
                        best_position = position;
                        lowest_angle  = current_angle;
                    }

                    break;
                }
                default:
                    break;
                }
            }

            if ( priority_mode == 3 && lowest_angle > 40.f || priority_mode == 0 ) best_position = fastest_dodge_position;
        }

        m_last_evade_points = free_positions;
        m_last_free_duration = free_action_duration;

        return best_position;
    }

    auto Evade::is_position_safe(
        const Vec3& position,
        const int   lowest_danger_level,
        const float radius_mod,
        const bool  is_blink
    ) const -> bool{
        // Function is used in LUA, message @tore if you change args
        sdk::math::Polygon poly{ };

        const auto cage = g_features->spell_detector->get_veigar_cage( );
        if ( cage.has_value( ) ) {
            if ( is_blink ) {
                if ( position.dist_to( cage->position ) <= 410.f && position.dist_to( cage->position ) >= 245.f )
                    return
                        false;
            } else {
                const auto is_inside = g_local->position.dist_to( cage->position ) <= 300.f;

                if ( is_inside && position.dist_to( cage->position ) >= 250.f ||
                    !is_inside && position.dist_to( cage->position ) <= 410.f )
                    return false;
            }
        }

        auto bounding_radius = m_raw_bounding_radius * radius_mod;

        for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && spell.danger < g_config->
                evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && spell.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !spell.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && spell.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && spell.type == SpellDetector::ESpellType::circle
            )
                continue;

            if ( spell.danger < lowest_danger_level ) continue;

            if ( spell.hitbox_area.is_inside( position ) ) return false;
        }

        for ( auto& missile : g_features->spell_detector->get_active_missiles( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                missile.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo &&
                missile.danger < g_config->evade.min_danger_level->get< int >( ) ||
                !missile.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) &&
                missile.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) &&
                missile.type == SpellDetector::ESpellType::circle
            )
                continue;

            if ( missile.danger < lowest_danger_level ) continue;

            if ( missile.type == SpellDetector::ESpellType::line ) {
                auto rect = sdk::math::Rectangle(
                    missile.position,
                    missile.get_dynamic_line_endpos( ),
                    missile.radius
                );
                poly = rect.to_polygon( missile.has_edge_radius ? static_cast< int >( bounding_radius ) : 0 );

                if ( !poly.is_outside( position ) ) return false;
            } else {
                auto c = Circle( missile.end_position, missile.radius );
                poly   = c.to_polygon( missile.has_edge_radius ? static_cast< int >( bounding_radius ) : 0 );

                if ( !poly.is_outside( position ) ) return false;
            }
        }

        bounding_radius = m_raw_bounding_radius * radius_mod;

        for ( auto& object : g_features->spell_detector->get_active_objects( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && object.danger < g_config
                ->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && object.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !g_config->evade.dodge_traps->get< bool >( ) || object.position.dist_to( g_local->position ) > 1000.f )
                continue;

            if ( object.danger < lowest_danger_level ) continue;

            auto c = Circle( object.position, object.radius );
            poly   = c.to_polygon( radius_mod > 1.f ? bounding_radius : 0 );

            if ( !poly.is_outside( position ) ) return false;
        }

        return true;
    }

    auto Evade::is_position_tetherable( const Vec3& position, bool use_tethering_hitbox ) const -> bool{
        auto cage = g_features->spell_detector->get_veigar_cage( );
        if ( cage.has_value( ) ) {
            auto is_inside = g_local->position.dist_to( cage->position ) < 400.f;

            if ( is_inside && position.dist_to( cage->position ) >= 245.f ||
                !is_inside && position.dist_to( cage->position ) <= 410.f )
                return true;
        }


        sdk::math::Polygon poly{ };

        auto bounding_radius = static_cast< int >( m_raw_bounding_radius );
        auto tether_radius{ g_config->evade.tether_distance->get< int >( ) };

        for ( auto spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                spell.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo &&
                spell.danger < g_config->evade.min_danger_level->get< int >( ) ||
                !spell.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && spell.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && spell.type == SpellDetector::ESpellType::circle )
                continue;

            switch ( spell.type ) {
            case SpellDetector::ESpellType::line:
                if ( !use_tethering_hitbox && spell.hitbox_area.is_inside( position ) ||
                    use_tethering_hitbox && spell.tether_area.is_inside(position) && !spell.hitbox_area.is_inside(position))
                    return true;
                break;
            case SpellDetector::ESpellType::circle:
                if ( spell.hitbox_area.is_inside( position ) ) return true;
                break;
            case SpellDetector::ESpellType::cone:
            {
                auto sect = sdk::math::Sector( spell.start_pos, spell.end_pos, spell.angle, spell.range );
                auto poly = sect.to_polygon_new( );

                if ( !poly.is_outside( position ) ) return true;

                break;
            }
            default:
                break;
            }
        }

        for ( auto missile : g_features->spell_detector->get_active_missiles( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                missile.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo &&
                missile.danger < g_config->evade.min_danger_level->get< int >( ) ||
                !missile.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && missile.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && missile.type == SpellDetector::ESpellType::circle )
                continue;


            auto additional_hitbox = missile.has_edge_radius ? bounding_radius : 0;

            if ( missile.type == SpellDetector::ESpellType::line ) {
                auto rect =
                    sdk::math::Rectangle( missile.position, missile.get_dynamic_line_endpos( ), missile.radius );

                poly = rect.to_polygon( additional_hitbox + tether_radius );

                if ( !poly.is_outside( position ) ) return true;
            } else {
                auto c = Circle( missile.end_position, missile.radius );
                poly   = c.to_polygon( additional_hitbox + tether_radius );

                if ( !poly.is_outside( position ) ) return true;
            }
        }

        for ( auto object : g_features->spell_detector->get_active_objects( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                object.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo &&
                object.danger < g_config->evade.min_danger_level->get< int >( ) ||
                !g_config->evade.dodge_traps->get< bool >( ) || object.position.dist_to( g_local->position ) > 1000.f )
                continue;

            auto c = Circle( object.position, object.radius + 50.f );
            poly   = c.to_polygon( );

            if ( !poly.is_outside( position ) ) return true;
        }

        return false;
    }


#if enable_lua
    auto Evade::get_safe_position_lua( ) -> sol::object{
        auto pos = get_smart_position( );

        if ( pos == Vec3( ) ) return sol::nil;

        return sol::make_object( g_lua_state2, pos );
    }

    auto Evade::disable_this_tick_lua( ) -> void{
        // Function is used in LUA, message @tore if you change args
        m_disable_this_tick_lua = true;
    }

#endif

    auto Evade::get_dangerous_spells(
        const Vec3& position,
        const bool  allow_evade_logic,
        const float unit_bounding_radius
    ) const
        -> std::vector< SpellDetector::SpellInstance >{
        // Function is used in LUA, message @tore if you change args

        std::vector< SpellDetector::SpellInstance > spells{ };

        const auto bounding_radius = unit_bounding_radius > 0.f ? unit_bounding_radius : m_raw_bounding_radius;

        for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( allow_evade_logic && ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                spell.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && spell.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !spell.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && spell.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && spell.type == SpellDetector::ESpellType::circle ) )
                continue;

            if ( spell.hitbox_area.is_inside( position ) ) spells.push_back( spell );
        }

        return spells;
    }
#if enable_new_lua
    auto Evade::get_dangerous_spells_table(
        const Vec3& position,
        const bool  allow_evade_logic,
        const float unit_bounding_radius
    ) const -> sol::as_table_t< std::vector< SpellDetector::SpellInstance > >{
        return sol::as_table( get_dangerous_spells( position, allow_evade_logic, unit_bounding_radius ) );
    }
#endif

    auto Evade::get_dangerous_missiles(
        const Vec3& position,
        const bool  allow_evade_logic,
        const float unit_bounding_radius
    ) const -> std::vector< SpellDetector::MissileInstance >{
        std::vector< SpellDetector::MissileInstance > missiles{ };

        const auto bounding_radius = unit_bounding_radius > 0.f ? unit_bounding_radius : m_raw_bounding_radius;

        for ( auto& missile : g_features->spell_detector->get_active_missiles( ) ) {
            if ( allow_evade_logic && ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo &&
                missile.danger < g_config->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && missile.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !missile.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && missile.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && missile.type == SpellDetector::ESpellType::circle ) )
                continue;

            if ( missile.type == SpellDetector::ESpellType::line ) {
                auto rect = sdk::math::Rectangle(
                    missile.position,
                    missile.get_dynamic_line_endpos( ),
                    missile.radius
                );
                auto polygon = rect.to_polygon( missile.has_edge_radius ? static_cast< int >( bounding_radius ) : 0 );

                if ( !polygon.is_outside( position ) ) missiles.push_back( missile );
            } else {
                auto c    = Circle( missile.end_position, missile.radius );
                auto poly = c.to_polygon( missile.has_edge_radius ? static_cast< int >( bounding_radius ) : 0 );

                if ( !poly.is_outside( position ) ) missiles.push_back( missile );
            }
        }

        return missiles;
    }

    auto Evade::get_dangerous_objects( const Vec3& position ) const -> std::vector< SpellDetector::ObjectInstance >{
        std::vector< SpellDetector::ObjectInstance > objects{ };

        for ( auto& object : g_features->spell_detector->get_active_objects( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && object.danger < g_config
                ->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && object.danger < g_config->
                evade.min_danger_level->get< int >( ) )
                continue;


            auto c    = Circle( object.position, object.radius );
            auto poly = c.to_polygon(
                static_cast< int32_t >( m_bounding_radius / 2.f + g_config->evade.extra_distance->get< int >( ) / 2.f )
            );

            if ( !poly.is_outside( position ) ) objects.push_back( object );
        }

        return objects;
    }


    auto Evade::update_tether_points( ) -> void{
        m_tether_points.clear( );
        const auto tether_radius = static_cast< float >( g_config->evade.tether_distance->get< int >( ) );

        std::vector< Vec3 > candidates{ };

        //if ( is_tethering_after_evade( ) ) m_tether_points = m_post_evade_tether_points;

        const auto cage = g_features->spell_detector->get_veigar_cage( );
        if ( cage.has_value( ) && cage->position.dist_to( g_local->position ) <= 700.f ) {
            const auto is_inside = g_local->position.dist_to( cage->position ) < 400.f;

            if ( is_inside ) {
                const auto points = g_render->get_3d_circle_points( cage->position, 225.f, 32 );

                for ( size_t i = 0u; i < points.size( ); i++ ) {
                    if ( !g_navgrid->is_wall( points[ i ] ) && is_position_safe( points[ i ], 0, 1.1f ) ) {
                        candidates.push_back( points[ i ] );
                    }
                }
            } else {
                const auto points = g_render->get_3d_circle_points( cage->position, 490.f, 32 );

                for ( size_t i = 0u; i < points.size( ); i++ ) {
                    if ( !g_navgrid->is_wall( points[ i ] ) && is_position_safe( points[ i ], 0, 1.1f ) ) {
                        candidates.push_back( points[ i ] );
                    }
                }
            }
        }

        for ( auto& spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && spell.danger < g_config->
                evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && spell.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !spell.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && spell.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && spell.type == SpellDetector::ESpellType::circle
            )
                continue;


            std::vector< Vec3 > points{ };

            switch ( spell.type ) {
            case SpellDetector::ESpellType::line:
                points = spell.tether_points; // get_linear_tether_points( spell.tether_area, g_local->position );
                break;
            case SpellDetector::ESpellType::circle:
                points = spell.dodge_points;
                break;
            case SpellDetector::ESpellType::cone:
                points = get_cone_segment_points( spell.start_pos, spell.end_pos, spell.range, spell.angle ).points;
                break;
            default:
                continue;
            }

             candidates.insert(candidates.end(), points.begin(), points.end());
        }

        for ( auto& missile : g_features->spell_detector->get_active_missiles( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && missile.danger < g_config
                ->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && missile.danger < g_config
                ->evade.min_danger_level->get< int >( ) ||
                !missile.should_dodge( ) ||
                !g_config->evade.dodge_linear->get< bool >( ) && missile.type == SpellDetector::ESpellType::line ||
                !g_config->evade.dodge_circle->get< bool >( ) && missile.type ==
                SpellDetector::ESpellType::circle )
                continue;

            sdk::math::Polygon polygon{ };
            if ( missile.type == SpellDetector::ESpellType::line )
                polygon = get_line_segment_points(
                    missile.position,
                    missile.get_dynamic_line_endpos( ),
                    missile.radius + tether_radius
                );
            else
                polygon = get_circle_segment_points(
                    missile.end_position,
                    missile.radius + tether_radius,
                    60
                );

            candidates.insert(candidates.end(), polygon.points.begin(), polygon.points.end());
        }

        for ( auto& object : g_features->spell_detector->get_active_objects( ) ) {
            if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::combo && object.danger < g_config
                ->evade.combo_min_danger_level->get< int >( ) ||
                g_features->orbwalker->get_mode( ) != Orbwalker::EOrbwalkerMode::combo && object.danger < g_config->
                evade.min_danger_level->get< int >( ) ||
                !g_config->evade.dodge_traps->get< bool >( ) || object.position.dist_to( g_local->position ) > 500.f )
                continue;

            auto polygon{
                get_circle_segment_points(
                    object.position,
                    object.radius + 25.f,
                    g_config->evade.calculation_accuracy->get< int >( )
                )
            };

            candidates.insert(candidates.end(), polygon.points.begin(), polygon.points.end());
        }

        m_tether_points        = candidates;
        m_render_tether_points = candidates;

        if ( *g_time - m_last_evade_time < 0.3f ) {
            for ( const auto point : m_post_evade_tether_points ) {
                if ( is_position_tetherable( point, true ) ) continue;


                m_tether_points.push_back( point );
                m_render_tether_points.push_back( point );
            }
        }
    }

    auto Evade::update_preset_settings() -> void {

        if (g_config->evade.preset_mode->get<int>() == 0) return;

        switch (g_config->evade.preset_mode->get<int>())
        {
        case 3: {
            g_config->evade.extra_distance->get<int>() = 10;
            g_config->evade.tether_distance->get<int>() = 65;
            g_config->evade.default_dodge_priority->get<int>() = 3;
            g_config->evade.combo_dodge_priority->get<int>()   = 0;
            g_config->evade.combo_min_danger_level->get<int>() = 2;
            g_config->evade.min_danger_level->get<int>()       = 1;
            g_config->evade.include_ping_mode->get<int>()      = 1;

            g_config->evade.block_input->get<bool>()           = true;
            g_config->evade.champion_collision->get<bool>()    = false;
            g_config->evade.minion_collision->get<bool>()      = true;

            // humanizer
            g_config->evade.humanizer_maximum_angle->get<int>() = 0;
            g_config->evade.humanizer_delay->get<int>()         = 0;
            g_config->evade.humanizer_dangerous_delay->get<int>() = 0;
            g_config->evade.humanizer_dangerous_maximum_angle->get<int>() = 0;
            g_config->evade.humanizer_crowdcontrol_dangerous->get<bool>() = true;
            g_config->evade.humanizer_randomize_pattern->get<bool>()      = true;

            g_config->evade.force_longest_path->get<bool>() = false;
            g_config->evade.dodge_fow->get<bool>()          = true;
            break;
        }
        case 2: {


            g_config->evade.extra_distance->get<int>()  = 10;
            g_config->evade.tether_distance->get<int>() = 65;

            g_config->evade.default_dodge_priority->get<int>() = 1;
            g_config->evade.combo_dodge_priority->get<int>()   = 3;

            g_config->evade.combo_min_danger_level->get<int>() = 2;
            g_config->evade.min_danger_level->get<int>()       = 3;
            g_config->evade.include_ping_mode->get<int>()      = 0;

            g_config->evade.block_input->get<bool>()        = true;
            g_config->evade.champion_collision->get<bool>() = false;
            g_config->evade.minion_collision->get<bool>()   = true;

            // humanizer
            g_config->evade.humanizer_maximum_angle->get<int>()           = 0;
            g_config->evade.humanizer_delay->get<int>()                   = 0;
            g_config->evade.humanizer_dangerous_delay->get<int>()         = 0;
            g_config->evade.humanizer_dangerous_maximum_angle->get<int>() = 0;
            g_config->evade.humanizer_crowdcontrol_dangerous->get<bool>() = true;
            g_config->evade.humanizer_randomize_pattern->get<bool>()      = true;

            g_config->evade.force_longest_path->get<bool>() = false;
            g_config->evade.dodge_fow->get<bool>()          = true;
            break;

        }
        case 1:
        {

            g_config->evade.extra_distance->get<int>()  = 15;
            g_config->evade.tether_distance->get<int>() = 80;

            g_config->evade.default_dodge_priority->get<int>() = 1;
            g_config->evade.combo_dodge_priority->get<int>()   = 1;

            g_config->evade.combo_min_danger_level->get<int>() = 2;
            g_config->evade.min_danger_level->get<int>()       = 3;
            g_config->evade.include_ping_mode->get<int>()      = 0;

            g_config->evade.block_input->get<bool>()        = true;
            g_config->evade.champion_collision->get<bool>() = false;
            g_config->evade.minion_collision->get<bool>()   = true;

            // humanizer
            g_config->evade.humanizer_maximum_angle->get<int>()           = 2;
            g_config->evade.humanizer_delay->get<int>()                   = 3;
            g_config->evade.humanizer_dangerous_delay->get<int>()         = 1;
            g_config->evade.humanizer_dangerous_maximum_angle->get<int>() = 0;
            g_config->evade.humanizer_crowdcontrol_dangerous->get<bool>() = true;
            g_config->evade.humanizer_randomize_pattern->get<bool>()      = true;

            g_config->evade.force_longest_path->get<bool>() = false;
            g_config->evade.dodge_fow->get<bool>()          = true;
            break;
        }
        default:
            return;
        }

    }

    auto Evade::initialize_menu( ) -> void{
        const auto navigation = g_window->push( _( "evade" ), menu_order::evade );
        const auto general    = navigation->add_section( _( "general" ) );
        const auto settings   = navigation->add_section( _( "settings" ) );
        const auto humanizer  = navigation->add_section( _( "humanizer" ) );
        const auto spells     = navigation->add_section( _( "spell evade" ) );
        const auto visuals    = navigation->add_section( _( "visuals" ) );
        const auto summoners  = navigation->add_section( _( "summoner evade" ) );
        const auto whitelist  = navigation->add_section( _( "dodge whitelist" ) );
        const auto developer  = navigation->add_section( _( "developer" ) );

        general->checkbox( _( "enable evade" ), g_config->evade.toggle );
        visuals->select(
            _( "draw spells" ),
            g_config->evade.draw_spells_mode,
                        { _("Off"), _("Default"), _("New"), _("Experimental"), _("Dynamic") }
        );
        visuals->checkbox( _( "^ alternate color (default only)" ), g_config->evade.draw_spells_blue );
        visuals->checkbox( _( "show evade point" ), g_config->evade.show_position );
        visuals->checkbox( _( "show evade text" ), g_config->evade.show_text );

        general->slider_int( _( "default min danger level" ), g_config->evade.min_danger_level, 1, 5 );
        general->slider_int( _( "combo min danger level" ), g_config->evade.combo_min_danger_level, 1, 5 );
        general->select(
            _( "default priority" ),
            g_config->evade.default_dodge_priority,
            { _( "Lowest distance" ), _( "Near cursor" ), _( "Smart" ), _( "Hybrid" ) }
        );
        general->select(
            _( "combo priority" ),
            g_config->evade.combo_dodge_priority,
            { _( "Lowest distance" ), _( "Near cursor" ), _( "Smart" ), _( "Hybrid" ) }
        );

        general->slider_int( _( "tether range" ), g_config->evade.tether_distance, 25, 150, 1 );

        general->select(_("presets"), g_config->evade.preset_mode,
                        { _("No preset"), _("Legit"), _("Balanced"), _("Rage") });

        humanizer->checkbox( _( "randomize dodge pattern" ), g_config->evade.humanizer_randomize_pattern );
        humanizer->select(
            _( "normal delay" ),
            g_config->evade.humanizer_delay,
            {
                _( "No delay" ),
                _( "25ms" ),
                _( "50ms" ),
                _( "75ms" ),
                _( "100ms" ),
                _( "125ms" ),
                _( "150ms" ),
                _( "175ms" ),
                _( "200ms" )
            }
        );

        humanizer->select(
            _( "normal max angle" ),
            g_config->evade.humanizer_maximum_angle,
            {
                _( "No limit" ),
                _( "300*" ),
                _( "260*" ),
                _( "220*" ),
                _( "180*" ),
                _( "140*" ),
                _( "100*" ),
                _( "75*" )
            }
        );

        humanizer->select(
            _( "dangerous delay" ),
            g_config->evade.humanizer_dangerous_delay,
            {
                _( "No delay" ),
                _( "25ms" ),
                _( "50ms" ),
                _( "75ms" ),
                _( "100ms" ),
                _( "125ms" ),
                _( "150ms" ),
                _( "175ms" ),
                _( "200ms" )
            }
        );

        humanizer->select(
            _( "dangerous max angle" ),
            g_config->evade.humanizer_dangerous_maximum_angle,
            {
                _( "No limit" ),
                _( "300*" ),
                _( "260*" ),
                _( "220*" ),
                _( "180*" ),
                _( "140*" ),
                _( "100*" ),
                _( "75*" )
            }
        );

        humanizer->checkbox( _( "visualize max angle (?)" ), g_config->evade.visualize_maximum_angle )->set_tooltip(
            _( "White = normal dodge angle, Red = dangerous dodge angle" )
        );

        humanizer->checkbox( _( "crowdcontrol = dangerous" ), g_config->evade.humanizer_crowdcontrol_dangerous );
        humanizer->slider_int(
            _( "is dangerous if spell danger >= X" ),
            g_config->evade.humanizer_dangerous_threshold,
            1,
            5
        );

        // advanced->slider_int( _( "pathing accuracy" ), g_config->evade.calculation_accuracy, 25, 80 );
        // advanced->checkbox(_("smooth pathing"), g_config->evade.smooth_pathing);
        // settings->checkbox( _( "use new tether logic" ), g_config->evade.new_tether_logic );


        settings->checkbox( _( "block manual input (?)" ), g_config->evade.block_input )
                ->set_tooltip( _( "Will block manual input during evade" ) );

        settings->checkbox( _( "dodge spells from fog of war (?)" ), g_config->evade.dodge_fow )
                ->set_tooltip(
                    _(
                        "Disabling this option will not dodge some spells that are only detected from "
                        "missile (Thresh Q, Lillia E, Xerath R)"
                    )
                );

        settings->checkbox( _( "use server position (?)" ), g_config->evade.use_server_position )
                ->set_tooltip( _( "Enable this if your ping is < 15ms" ) );
        settings->select(
            _( "include ping mode" ),
            g_config->evade.include_ping_mode,
            { _( "Full" ), _( "Half" ), _( "None" ) }
        );
        settings->select(
            _( "hitbox modifier" ),
            g_config->evade.hitbox_modifier,
            { _( "None" ), _( "+10%" ), _( "+20%" ), _( "+30%" ), _( "+40%" ), _( "+50%" ) }
        );

        settings->slider_int( _( "extra distance" ), g_config->evade.extra_distance, 0, 100, 1 );

        settings->multi_select(
            _( "spell collision on " ),
            {
                g_config->evade.minion_collision,
                g_config->evade.champion_collision
            },
            { _( "Minions" ), _( "Champions" ) }
        );

        settings->checkbox(_("alternative tether logic"), g_config->evade.alternative_tether_logic);
        settings->checkbox(_("autoattack sync (?)"), g_config->evade.autoattack_sync)->set_tooltip(_("Allows orbwalker to attack during evade for more DPS"));

        spells->checkbox( _( "use spells to evade" ), g_config->evade.toggle_spells );
        spells->slider_int( _( "min danger level" ), g_config->evade.spell_min_danger_level, 0, 5 );
        spells->select(
            _( "priority logic" ),
            g_config->evade.spell_dodge_priority,
            { _( "Near cursor" ), _( "Smart" ) }
        );

        summoners->checkbox( _( "use summoners to evade" ), g_config->evade.toggle_summoner );
        summoners->slider_int( _( "min danger level" ), g_config->evade.summoner_min_danger_level, 0, 5 );

        whitelist->checkbox( _( "evade line spells" ), g_config->evade.dodge_linear );
        whitelist->checkbox( _( "evade circle spells" ), g_config->evade.dodge_circle );
        whitelist->checkbox( _( "evade traps" ), g_config->evade.dodge_traps );

        const auto simulation = navigation->add_section( _( "spell simulation" ) );
        simulation->checkbox( _( "simulate spells" ), g_config->evade.simulate_spells );
        simulation->slider_int( _( "sim delay" ), g_config->evade.simulation_delay, 1, 15 );

        developer->checkbox( _( "force longest path" ), g_config->evade.force_longest_path );
    }

    auto Evade::should_tether( Vec3 position ) const -> bool{
        if ( g_features->orbwalker->get_mode( ) == Orbwalker::EOrbwalkerMode::none || g_local->is_dead( ) || !g_config
            ->evade.toggle->get<
                bool >( ) )
            return false;

        if ( position.length( ) <= 0.f ) position = g_local->position;

        auto aimgr = g_local->get_ai_manager( );
        if ( aimgr && aimgr->is_moving ) position += aimgr->velocity;

        const auto cursor =
            g_pw_hud->get_hud_manager()
                ->cursor_position_unclipped; // g_features->orbwalker->get_extrapolated_cursor_position( );

        for ( auto i = 1; i <= 6; i++ ) {

            const auto testpos = position.extend( cursor, 50.f * i );

            if ( is_position_tetherable( testpos, true ) || !is_position_safe( testpos ) ) return true;
        }

        return false;

        auto pred = g_features->prediction->predict_movement( g_local->index, 0.175f );
        if ( !pred ) return false;

        if ( is_position_tetherable( *pred ) ) return true;

        return false;
    }

    auto Evade::get_tether_point( bool pre_tether ) -> Vec3{
        update_tether_points( );
        std::vector< Vec3 > valid_points{ };


        auto local_position = g_local->position ;

        auto aimgr = g_local->get_ai_manager();
        if (aimgr) local_position = aimgr->get_server_position();

        Vec3 temporary{ };

        for ( auto& point : m_tether_points ) {

            if ( local_position.dist_to( point ) > 250.f || g_navgrid->is_wall(point))
                continue;

            bool in_danger{ };

            for ( auto i = 1; i <= 4; i++ ) {
                temporary = local_position.extend( point, local_position.dist_to( point ) / 4.f * i );

                if ( !is_position_safe( temporary, 0, 1.1f ) ) {
                    in_danger = true;
                    break;
                }
            }

            if ( in_danger ) continue;

            valid_points.push_back( point );
        }

        m_valid_tether_points = valid_points;

        if ( valid_points.empty( ) ) return Vec3( );

        auto lowest_distance_to_local = std::numeric_limits< float >::max( );

        for ( auto point : valid_points ) {
            const auto dist = g_local->position.dist_to( point );

            if ( dist < lowest_distance_to_local ) lowest_distance_to_local = dist;
        }

        const auto cursor                = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
        auto       lowest_point_distance = std::numeric_limits< float >::max( );
        Vec3       best_point{ };
        Vec3       next_best_point{ };
        auto       lowest_angle{ 180.f };
        auto       lowest_evade_angle{ 180.f };

        const auto path              = aimgr->get_path( );
        auto       path_end          = path.empty( ) ? Vec3( ) : path[ path.size( ) - 1 ];
        const auto allow_angle_check = m_tether_point.length( ) > 0.f && *g_time - m_last_tether_direction_update <=
            0.75f;

        const auto is_after_evade = is_tethering_after_evade( );

        if (g_config->evade.alternative_tether_logic->get<bool>())
        {

            for (const auto point : valid_points)
            {
                const auto dist = cursor.dist_to(point);
                // if ( dist > 350.f ) continue;

               /* if (point.dist_to(local_position) <= 75.f && best_point.length() > 0.f) continue;
                // if ( g_local->position.dist_to( point ) > 200.f ) continue;

                auto       v1           = cursor - local_position;
                auto       v2           = point - local_position;
                auto       dot          = v1.normalize().dot_product(v2.normalize());
                const auto tether_angle = acos(dot) * 180.f / 3.14159265358979323846f;
                if (tether_angle > 150.f && best_point.length() > 0.f) continue;

                if (allow_angle_check)
                {
                    v1                         = m_tether_point - local_position;
                    v2                         = point - local_position;
                    dot                        = v1.normalize().dot_product(v2.normalize());
                    const auto direction_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    if (direction_angle > 60.f && best_point.length() > 0.f) continue;
                }

                float evade_angle{};

                if (is_after_evade)
                {
                    v1          = m_evade_direction;
                    dot         = v1.normalize().dot_product(v2.normalize());
                    evade_angle = acos(dot) * 180.f / 3.14159265358979323846f;
                }*/

                if (best_point.length() > 0.f && dist > best_point.dist_to(cursor) ||
                    helper::is_wall_in_line(local_position, point))
                    continue;

                if (best_point.length() > 0.f) next_best_point = best_point;
                best_point         = point;
                //lowest_angle       = tether_angle;
                //
                //lowest_evade_angle = evade_angle;
            }


        }
        else {

            for (const auto point : valid_points)
            {
                const auto dist = local_position.dist_to(point);
                // if ( dist > 350.f ) continue;

                if (point.dist_to(local_position) <= 75.f && best_point.length() > 0.f) continue;
                // if ( g_local->position.dist_to( point ) > 200.f ) continue;

                auto       v1           = cursor - local_position;
                auto       v2           = point - local_position;
                auto       dot          = v1.normalize().dot_product(v2.normalize());
                const auto tether_angle = acos(dot) * 180.f / 3.14159265358979323846f;
                if (tether_angle > 150.f && best_point.length() > 0.f) continue;

                if (allow_angle_check)
                {
                    v1                         = m_tether_point - local_position;
                    v2                         = point - local_position;
                    dot                        = v1.normalize().dot_product(v2.normalize());
                    const auto direction_angle = acos(dot) * 180.f / 3.14159265358979323846f;

                    if (direction_angle > 60.f && best_point.length() > 0.f) continue;
                }

                float evade_angle{};

                if (is_after_evade)
                {
                    v1          = m_evade_direction;
                    dot         = v1.normalize().dot_product(v2.normalize());
                    evade_angle = acos(dot) * 180.f / 3.14159265358979323846f;
                }

                if (!is_after_evade && tether_angle > lowest_angle ||
                    is_after_evade && evade_angle > lowest_evade_angle ||
                    helper::is_wall_in_line(local_position, point))
                    continue;

                if (best_point.length() > 0.f) next_best_point = best_point;
                best_point         = point;
                lowest_angle       = tether_angle;
                lowest_evade_angle = evade_angle;
            }


        }
        if ( best_point.length( ) <= 0.f )
            return *g_time - m_last_tether_time <= 0.25f && m_tether_point.length( ) >
                   0.f
                       ? m_tether_point
                       : Vec3( );

        /* if ( aimgr->is_moving && best_point.dist_to( g_local->position ) < 80.f &&
                  *g_time - m_last_tether_time <= 0.2f ) {
            //m_tether_point = path_end;
            //best_point     = path_end;

            return m_tether_point;
        }*/

        //std::cout << "tether distance to cursor: " << best_point.dist_to( cursor ) << std::endl;

        /* if ( *g_time - m_last_tether_time <= 0.2f ) {

            const auto        v2            = best_point - g_local->position;
            const auto        dot           = v1.normalize( ).dot_product( v2.normalize( ) );
            const float current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

            if ( current_angle > 45.f) {

                m_tether_direction             = ( best_point - g_local->position ).normalize( );
                m_last_tether_direction_update = *g_time;
            }

        } else {
            m_last_tether_direction_update = *g_time;
            m_tether_direction             = ( best_point - g_local->position ).normalize( );
        }*/

        if ( lowest_angle > 90.f || m_tether_point.length( ) <= 0.f ) m_last_tether_direction_update = *g_time;

        m_tether_point     = best_point;
        m_alt_tether_point = next_best_point;
        m_last_tether_time = *g_time;

        return best_point;
    }

    auto Evade::is_tethering_after_evade( ) const -> bool{ return *g_time - m_last_evade_time < 0.1f || m_is_evading; }

    auto Evade::get_circle_segment_points(
        Vec3       center,
        float      radius,
        const int  segments,
        const bool no_modification
    ) const -> sdk::math::Polygon{
        const auto         angle_step = D3DX_PI * 2.0f / static_cast< float >( segments );
        sdk::math::Polygon poly{ };

        if ( m_should_fix_height ) center.y = g_navgrid->get_height( center );

        // default
        if ( !no_modification )
            radius += m_bounding_radius + static_cast< float >( g_config->evade.extra_distance->get<
                int >( ) ) / 2.f;

        const auto local_position = g_local->position;
        const auto points         = g_render->get_3d_circle_points( center, radius + 25.f, segments );

        for ( const auto& point : points ) {
            if ( local_position.dist_to( point ) > 1500.f ) continue;

            poly.points.push_back( point );
        }

        return poly;
    }

    auto Evade::get_line_segment_points(
        const Vec3& start_pos,
        const Vec3& end_pos,
        const float radius,
        const bool  no_modification
    ) const -> sdk::math::Polygon{
        sdk::math::Polygon poly{ };

        const Vec3 real_start = { start_pos.x, g_navgrid->get_height( g_local->position ), start_pos.z };
        const Vec3 real_end   = { end_pos.x, g_navgrid->get_height( g_local->position ), end_pos.z };

        const auto rect = sdk::math::Rectangle(
            m_should_fix_height ? real_start : start_pos,
            m_should_fix_height ? real_end : end_pos,
            radius + 10.f
        );

        const auto temp_poly = rect.to_polygon(
            no_modification
                ? 0
                : static_cast< int >( m_bounding_radius ) + g_config->evade.extra_distance->get< int >( )
        );

        Vec3 start{ };
        Vec3 end{ };

        for ( size_t i = 0u; i < temp_poly.points.size( ); i++ ) {
            start = temp_poly.points[ i ];

            if ( i == temp_poly.points.size( ) - 1 ) end = temp_poly.points[ 0 ];
            else end                                     = temp_poly.points[ i + 1 ];

            constexpr auto point_distance = 30.f;
            const auto     segment_count  = start.dist_to( end ) / point_distance;

            for ( auto a = 1; a <= segment_count; a++ ) {
                const auto extend_distance = point_distance * a;

                if ( extend_distance > start.dist_to( end ) ) {
                    auto position = start.extend( end, start.dist_to( end ) );
                    if ( position.dist_to( g_local->position ) > 1000.f ) continue; // optimization

                    poly.points.push_back( position );
                    break;
                }

                auto position = start.extend( end, point_distance * a );
                if ( position.dist_to( g_local->position ) > 1500.f ) continue; // optimization

                poly.points.push_back( position );
            }
        }

        return poly;
    }

    auto Evade::get_polygon_points( const sdk::math::Polygon& hitbox ) -> std::vector< Vec3 >{
        Vec3 start{ };
        Vec3 end{ };

        std::vector< Vec3 > points{ };

        for ( size_t i = 0u; i < hitbox.points.size( ); i++ ) {
            start = hitbox.points[ i ];

            if ( i == hitbox.points.size( ) - 1 ) end = hitbox.points[ 0 ];
            else end                                  = hitbox.points[ i + 1 ];

            constexpr auto point_distance = 50.f;
            const auto     segment_count  = start.dist_to( end ) / point_distance;

            for ( auto a = 1; a <= segment_count; a++ ) {
                const auto extend_distance = point_distance * a;

                if ( extend_distance > start.dist_to( end ) ) {
                    auto position = start.extend( end, start.dist_to( end ) );
                    if ( position.dist_to( g_local->position ) > 600.f ) continue; // optimization

                    points.push_back( position );
                    break;
                }

                auto position = start.extend( end, point_distance * a );
                if ( position.dist_to( g_local->position ) > 600.f ) continue; // optimization

                points.push_back( position );
            }
        }

        return points;
    }

    auto Evade::get_linear_tether_points(
        const sdk::math::Polygon& hitbox,
        const Vec3&               source_position
    ) -> std::vector< Vec3 >{
        auto lowest_distance = 9999.f;
        Vec3 start{ };
        Vec3 end{ };

        for ( auto i = 1; i <= hitbox.points.size( ); i++ ) {
            Vec3 line_start{ }, line_end{ };

            if ( i == hitbox.points.size( ) ) {
                line_start = hitbox.points[ hitbox.points.size( ) - 1 ];
                line_end   = hitbox.points[ 0 ];
            } else {
                line_start = hitbox.points[ i - 1 ];
                line_end   = hitbox.points[ i ];
            }

            auto closest = get_closest_line_point( line_start, line_end, source_position );
            if ( closest.dist_to( source_position ) >= lowest_distance ) continue;

            start           = line_start;
            end             = line_end;
            lowest_distance = closest.dist_to( source_position );
        }

        std::vector< Vec3 > points{ };

        constexpr auto point_distance = 30.f;
        const auto     segment_count  = start.dist_to( end ) / point_distance;

        for ( auto a = 0; a <= segment_count; a++ ) {
            const auto extend_distance = point_distance * a;

            if ( extend_distance > start.dist_to( end ) ) {
                auto position = end;
                if ( position.dist_to( source_position ) > 800.f ) continue; // optimization

                points.push_back( position );
                break;
            }

            auto position = start.extend( end, point_distance * a );
            if ( position.dist_to( source_position ) > 800.f ) continue; // optimization

            points.push_back( position );
        }

        return points;
    }


    auto Evade::get_cone_segment_points(
        Vec3        start_pos,
        Vec3        end_pos,
        const float range,
        const float angle
    ) const -> sdk::math::Polygon{
        sdk::math::Polygon poly{ };


        if ( m_should_fix_height ) {
            start_pos.y = g_navgrid->get_height( start_pos );
            end_pos.y   = g_navgrid->get_height( end_pos );
        }

        const auto adjusted_start = start_pos.extend( end_pos, -140.f );

        const auto cone      = sdk::math::Sector( adjusted_start, end_pos, angle, range );
        const auto temp_poly = cone.to_polygon_new( 200 );

        Vec3 start{ };
        Vec3 end{ };

        for ( size_t i = 0u; i < temp_poly.points.size( ); i++ ) {
            start = temp_poly.points[ i ];

            if ( i == temp_poly.points.size( ) - 1 ) end = temp_poly.points[ 0 ];
            else end                                     = temp_poly.points[ i + 1 ];

            constexpr auto point_distance = 40.f;
            const auto     segment_count  = start.dist_to( end ) / point_distance;

            for ( auto a = 1; a <= segment_count; a++ ) {
                const auto extend_distance = point_distance * a;
                if ( extend_distance > start.dist_to( end ) ) {
                    auto position = start.extend( end, start.dist_to( end ) );
                    if ( position.dist_to( g_local->position ) > 1000.f ) continue; // optimization

                    poly.points.push_back( position );
                    break;
                }

                auto position = start.extend( end, point_distance * a );
                if ( position.dist_to( g_local->position ) > 1000.f ) continue; // optimization

                poly.points.push_back( position );
            }
        }

        return poly;
    }

    auto Evade::get_time_until_collision( const SpellDetector::SpellInstance& spell ) -> float{
        if ( spell.type == SpellDetector::ESpellType::line )
            return ( spell.start_time + spell.windup_time +
                g_local->position.dist_to( spell.get_current_position( ) ) / spell.speed ) + 0.1f;

        return spell.end_time;
    }

    auto Evade::try_summoner_evade( const SpellDetector::SpellInstance& spell ) -> bool{
        if ( !g_config->evade.toggle_summoner->get< bool >( ) || spell.danger < g_config->evade.
            summoner_min_danger_level->get< int >( )
            || *g_time - m_last_spell_time <= 0.5f )
            return false;


        auto slot       = ESpellSlot::d;
        auto spell_slot = g_local->spell_book.get_spell_slot( ESpellSlot::d );
        if ( !spell_slot || spell_slot->get_name( ).find( _( "SummonerFlash" ) ) == std::string::npos ) {
            spell_slot = g_local->spell_book.get_spell_slot( ESpellSlot::f );
            slot       = ESpellSlot::f;

            if ( !spell_slot || spell_slot->get_name( ).find( _( "SummonerFlash" ) ) == std::string::npos )
                return
                    false;
        }

        if ( !spell_slot->is_ready( ) ) return false;

        std::vector< Vec3 > available_positions{ };
        sdk::math::Polygon  polygon{ };
        if ( spell.type == SpellDetector::ESpellType::line )
            polygon = get_line_segment_points(
                spell.current_pos,
                spell.end_pos,
                spell.radius
            );
        else if ( spell.type == SpellDetector::ESpellType::circle )
            polygon = get_circle_segment_points(
                spell.end_pos,
                spell.radius,
                g_config->evade.calculation_accuracy->get< int >( )
            );
        else if ( spell.type == SpellDetector::ESpellType::cone )
            polygon = get_cone_segment_points(
                spell.start_pos,
                spell.end_pos,
                spell.range,
                spell.angle
            );

        for ( auto& point : polygon.points ) {
            if ( g_local->position.dist_to( point ) > 400.f ) continue;

            auto end_point = get_blink_position( point, 400.f );

            if ( is_position_safe( end_point ) ) {
                available_positions.push_back( end_point );
                break;
            }
        }

        if ( available_positions.empty( ) ) return false;

        Vec3  best_position{ };
        float best_weight{ };

        for ( auto& point : available_positions ) {
            if ( g_navgrid->is_wall( point ) ) continue;

            if ( point.dist_to( g_local->position ) < 150.f ) {
                auto new_point = extend_evade_point( spell, point, 300.f - g_local->position.dist_to( point ) );
                auto blink_end = get_blink_position( new_point, 400.f );

                if ( !g_navgrid->is_wall( blink_end ) && blink_end.dist_to( g_local->position ) > 150.f
                    && is_position_safe( blink_end ) )
                    point = blink_end;
            }

            const auto weight{ get_position_weight( point ) };

            if ( weight < best_weight ) continue;

            best_position = point;
            best_weight   = weight;
        }

        if ( best_position.length( ) <= 0.f ) return false;

        if ( g_input->cast_spell( slot, best_position ) ) {
            m_last_spell_time = *g_time;
            return true;
        }

        return false;
    }


    auto Evade::try_spell_evade( const SpellDetector::SpellInstance& spell ) -> bool{
        if ( !g_config->evade.toggle_spells->get< bool >( ) ||
            spell.danger < g_config->evade.spell_min_danger_level->get< int >( ) ||
            *g_time - m_last_spell_time <= 0.5f
        )
            return false;

        const auto evading_spell = get_evade_spell( );
        if ( !evading_spell ) return false;

        auto evade_spell = evading_spell.value( );
        auto spell_slot  = g_local->spell_book.get_spell_slot( evade_spell.slot );
        if ( !spell_slot || !spell_slot->is_ready( true ) ) return false;

        std::vector< Vec3 > available_positions{ };
        sdk::math::Polygon  polygon{ };
        if ( spell.type == SpellDetector::ESpellType::line )
            polygon = get_line_segment_points(
                spell.current_pos,
                spell.end_pos,
                spell.radius
            );
        else if ( spell.type == SpellDetector::ESpellType::circle )
            polygon = get_circle_segment_points(
                spell.end_pos,
                spell.radius,
                g_config->evade.calculation_accuracy->get< int >( )
            );
        else if ( spell.type == SpellDetector::ESpellType::cone )
            polygon = get_cone_segment_points(
                spell.start_pos,
                spell.end_pos,
                spell.range,
                spell.angle
            );

        for ( const auto point : polygon.points ) {
            auto safe_point{ point };

            if ( g_navgrid->is_wall( safe_point ) && evade_spell.extends_range_on_wall ) {
                auto end_point = get_blink_position( safe_point, evade_spell.range );
                safe_point     = end_point;
            } else if ( g_local->position.dist_to( safe_point ) > evade_spell.range )
                safe_point = g_local->position.
                                      extend( safe_point, evade_spell.range );

            auto time_to_point = evade_spell.get_time_to_position( safe_point ) + g_features->orbwalker->get_ping( ) *
                1.5f;

            if ( !evade_spell.penetrates_wall && helper::is_wall_in_line( g_local->position, safe_point ) ) continue;

            auto time_until_hit = spell.time_till_impact( g_local->position );

            if ( spell.type == SpellDetector::ESpellType::line && spell.speed > 0.f && evade_spell.speed > 0.f ) {
                auto impact_time = spell.time_till_impact(
                    get_closest_line_point( spell.current_pos, spell.end_pos, safe_point )
                );
                if ( impact_time <= time_to_point ) continue;
            } else if ( time_to_point >= time_until_hit ) continue;

            if ( is_position_safe( safe_point ) ) { available_positions.push_back( safe_point ); }
        }

        for ( const auto point : polygon.points ) {
            auto safe_point{ g_local->position.extend( point, evade_spell.range ) };

            auto safe_distance = get_units_to_safety( g_local->position, safe_point );
            auto temp          = g_local->position.extend( safe_point, safe_distance );

            if ( g_navgrid->is_wall( safe_point ) && evade_spell.extends_range_on_wall ) {
                auto end_point = get_blink_position( safe_point, evade_spell.range );
                safe_point     = end_point;
            } else if ( g_local->position.dist_to( safe_point ) > evade_spell.range )
                safe_point = g_local->position.
                                      extend( safe_point, evade_spell.range );

            auto time_to_point = evade_spell.get_time_to_position( temp ) + g_features->orbwalker->get_ping( ) * 1.5f;

            if ( !evade_spell.penetrates_wall && helper::is_wall_in_line( g_local->position, safe_point ) ) continue;

            auto time_until_hit = spell.time_till_impact( g_local->position );

            if ( spell.type == SpellDetector::ESpellType::line && spell.speed > 0.f && evade_spell.speed > 0.f ) {
                auto impact_time =
                    spell.time_till_impact( get_closest_line_point( spell.current_pos, spell.end_pos, temp ) );
                if ( impact_time <= time_to_point ) continue;
            } else if ( time_to_point >= time_until_hit ) continue;

            if ( is_position_safe( safe_point ) ) available_positions.push_back( safe_point );
        }

        if ( available_positions.empty( ) ) return false;

        auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        Vec3 best_position{ };
        auto best_weight{ 0.f };

        m_spell_points                = available_positions;
        m_last_spell_calculation_time = *g_time;

        std::vector< Vec3 > modified_points{ };

        for ( auto point : available_positions ) {
            if ( g_navgrid->is_wall( point ) && !evade_spell.ignores_wall ) continue;

            if ( evade_spell.has_minimum_range && point.dist_to( g_local->position ) < evade_spell.min_range ) {
                auto new_point = extend_evade_point( spell, point, evade_spell.min_range * 1.1f );

                if ( !g_navgrid->is_wall( new_point ) && is_position_safe( new_point ) ) point = new_point;
            }

            modified_points.push_back( point );

            auto weight{ 0.f };

            switch ( g_config->evade.spell_dodge_priority->get< int >( ) ) {
            case 0:
            {
                auto       v1            = point - g_local->position;
                auto       v2            = cursor - g_local->position;
                auto       dot           = v1.normalize( ).dot_product( v2.normalize( ) );
                const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

                weight += 1.f - current_angle / 180.f;
                break;
            }
            default:
                weight = get_position_weight( point );
                break;
            }

            if ( weight < best_weight ) continue;

            best_position = point;
            best_weight   = weight;
        }

        m_adjusted_spell_points = modified_points;

        if ( best_position.length( ) <= 0.f ) return false;

        if ( evade_spell.invert )
            best_position = g_local->position.extend(
                best_position,
                -g_local->position.dist_to( best_position )
            );

        if ( g_input->cast_spell( evade_spell.slot, best_position ) ) {
            m_last_spell_time = *g_time;
            //set_disable_time( evade_spell.get_time_to_position( best_position ) );
            g_features->orbwalker->on_cast( );

            m_is_evading    = true;
            m_spell_evading = true;
            m_reset_path    = true;

            m_goal_position      = best_position;
            m_spell_end_position = best_position;
            m_spellcast_end_time = *g_time + evade_spell.get_time_to_position( best_position ) + 0.15f;

            return true;
        }

        return false;
    }

    auto Evade::extend_evade_point(
        const SpellDetector::SpellInstance& spell,
        const Vec3&                         point,
        const float                         extend_distance
    ) const -> Vec3{
        sdk::math::Polygon polygon{ };
        if ( spell.type == SpellDetector::ESpellType::line )
            polygon = get_line_segment_points(
                spell.current_pos,
                spell.end_pos,
                spell.radius + extend_distance
            );
        else if ( spell.type == SpellDetector::ESpellType::circle )
            polygon = get_circle_segment_points(
                spell.end_pos,
                spell.radius + extend_distance,
                g_config->evade.calculation_accuracy->get< int >( )
            );
        else if ( spell.type == SpellDetector::ESpellType::cone )
            polygon = get_cone_segment_points(
                spell.start_pos,
                spell.end_pos,
                spell.range,
                spell.angle
            );

        Vec3 best_point{ };
        auto lowest_distance{ 9999999.f };

        for ( const auto position : polygon.points ) {
            if ( point.dist_to( position ) < lowest_distance ) {
                best_point      = position;
                lowest_distance = point.dist_to( position );
            }
        }

        return best_point;
    }

    auto Evade::get_closest_line_point(
        const Vec3& line_start,
        const Vec3& line_end,
        const Vec3& position
    ) -> Vec3{
        /* if ( m_should_fix_height ) {
            line_start.y = g_navgrid->get_height( line_start );
            line_end.y   = g_navgrid->get_height( line_end );
            position.y   = g_navgrid->get_height( position );   
        }*/

        const auto start_delta = line_start.dist_to( position );
        const auto end_delta   = line_end.dist_to( position );
        const auto distance    = line_start.dist_to( line_end );

        const auto distance_to_line = start_delta + end_delta - distance;

        const auto closer_to_start = start_delta < end_delta;
        if ( closer_to_start ) {
            const auto distance_multiplier = start_delta / end_delta * 0.5f;
            return line_end.extend( line_start, end_delta - distance_to_line * distance_multiplier );
        }

        const auto distance_multiplier = end_delta / start_delta * 0.5f;
        return line_start.extend( line_end, start_delta - distance_to_line * distance_multiplier );
    }

    auto Evade::get_colliding_skillshot(int16_t index) const -> std::optional<DangerousSkillshot>
    {
        auto hero = g_entity_list.get_by_index(index);
        if (!hero || hero->is_dead() || hero->is_invisible()) return std::nullopt;

        hero.update();

        DangerousSkillshot instance{};
        bool               found_collision{};

        const auto bounding_radius  = hero->get_bounding_radius();
        const auto dangerous_spells = g_features->evade->get_dangerous_spells(hero->position, false, bounding_radius);

        for (auto spell : dangerous_spells)
        {
            auto time_till_impact = spell.time_till_impact(hero->position);
            if (time_till_impact <= 0.f) continue;

            auto pred = g_features->prediction->predict_default(hero->index, time_till_impact, true);
            if (!pred) continue;

            sdk::math::Polygon hitbox{};

            switch (spell.type)
            {
            case SpellDetector::ESpellType::line:
            case SpellDetector::ESpellType::circle:
                hitbox = spell.hitbox_area;
                break;
            case SpellDetector::ESpellType::cone:
                {
                auto adjusted_start = spell.start_pos.extend(spell.end_pos, -140.f);

                auto cone = sdk::math::Sector(adjusted_start, spell.end_pos, spell.angle, spell.range);
                hitbox    = cone.to_polygon_new(200);
                break;
                }
            default:
                continue;
            }

            if (hitbox.is_outside(pred.value()) ||
                found_collision &&
                    (spell.danger < instance.danger ||
                     spell.danger == instance.danger && instance.time_until_collision < time_till_impact))
                continue;

            instance.danger               = spell.danger;
            instance.time_until_collision = time_till_impact;
            instance.name                 = spell.spell_name;
            found_collision               = true;
        }

        if (found_collision) return std::make_optional(instance);

        const auto dangerous_missiles =
            g_features->evade->get_dangerous_missiles(hero->position, false, bounding_radius);

        for (auto missile : dangerous_missiles)
        {
            auto time_till_impact = missile.time_till_impact(hero->position);
            if (time_till_impact <= 0.f) continue;

            auto pred = g_features->prediction->predict_default(hero->index, time_till_impact, false);
            if (!pred) continue;

            sdk::math::Polygon hitbox{};

            switch (missile.type)
            {
            case SpellDetector::ESpellType::line:
                hitbox = sdk::math::Rectangle(missile.position, missile.get_dynamic_line_endpos(), missile.radius)
                             .to_polygon(static_cast<int>(bounding_radius));
                break;
            case SpellDetector::ESpellType::circle:
                hitbox = sdk::math::Circle(missile.end_position, missile.radius)
                             .to_polygon(static_cast<int>(bounding_radius));
                break;
            default:
                continue;
            }

            if (hitbox.is_outside(pred.value()) || missile.danger < instance.danger ||
                missile.danger == instance.danger && instance.time_until_collision < time_till_impact)
                continue;

            instance.danger               = missile.danger;
            instance.time_until_collision = time_till_impact;
            instance.name                 = missile.name;
            found_collision               = true;
        }

        return found_collision ? std::make_optional(instance) : std::nullopt;
    }

    auto Evade::get_collision_point( const SpellDetector::SpellInstance& spell ) -> std::optional< Vec3 >{
        if ( !spell.collision || spell.type != SpellDetector::ESpellType::line || spell.speed == 0.f )
            return
                std::nullopt;

        const auto current_position = spell.get_current_position( );
        const auto rect             = sdk::math::Rectangle( current_position, spell.end_pos, spell.radius );
        auto       poly             = rect.to_polygon( );


        auto lowest_distance{ spell.range };
        Vec3 end_point{ };
        bool found_collision{ };

        for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
            if ( !minion ||
                minion->is_dead( ) ||
                minion->is_invisible( ) ||
                minion->position.dist_to( current_position ) > spell.range ||
                !minion->is_lane_minion( ) &&
                !minion->is_jungle_monster( )
            )
                continue;

            auto pred = g_features->prediction->predict_default( minion->index, g_features->orbwalker->get_ping( ) );
            if ( !pred ) continue;

            auto time_to_impact = spell.time_till_impact( *pred );
            if ( time_to_impact >= FLT_MAX ) continue;

            poly = rect.to_polygon( static_cast< int32_t >( g_entity_list.get_bounding_radius( minion->index ) ) );
            if ( poly.is_outside( *pred ) ) continue;

            const auto time_reduction = fmax( spell.server_cast_time - *g_time, 0.f );

            time_to_impact += g_features->orbwalker->get_ping( );

            auto end_candidate = current_position.extend(
                spell.end_pos,
                ( time_to_impact - time_reduction ) * spell.speed
            );
            if ( end_candidate.dist_to( current_position ) > lowest_distance ) continue;

            lowest_distance = end_candidate.dist_to( current_position );
            end_point       = end_candidate;
            found_collision = true;
        }


        return found_collision ? std::make_optional( end_point ) : std::nullopt;
    }


    auto Evade::get_evade_spell( ) -> std::optional< EvadeSpell >{
        EvadeSpell spell{ };
        switch ( helper::get_current_hero( ) ) {
        case EHeroes::ezreal:
            spell.delay = 0.25f;
            spell.range                 = 475.f;
            spell.speed                 = 0.f;
            spell.slot                  = ESpellSlot::e;
            spell.is_fixed_range        = false;
            spell.extends_range_on_wall = true;
            spell.min_range             = 200.f;
            spell.has_minimum_range     = true;
            break;
        case EHeroes::graves:
            spell.delay = 0.25f;
            spell.range                 = 375.f;
            spell.speed                 = 1150.f;
            spell.slot                  = ESpellSlot::e;
            spell.extends_range_on_wall = false;
            spell.min_range             = 275.f;
            spell.has_minimum_range     = true;
            break;
        case EHeroes::zeri:
            spell.delay = 0.f;
            spell.range                 = 300.f;
            spell.speed                 = 600.f + g_local->movement_speed;
            spell.ignores_wall          = true;
            spell.slot                  = ESpellSlot::e;
            spell.is_fixed_range        = true;
            spell.extends_range_on_wall = true;
            break;
        case EHeroes::vayne:
            if ( g_features->buff_cache->get_buff( g_local->index, ct_hash( "vaynetumblebonus" ) ) )
                return
                    std::nullopt;

            spell.delay           = 0.f;
            spell.range           = 300.f;
            spell.speed           = 910.f;
            spell.slot            = ESpellSlot::q;
            spell.is_fixed_range  = true;
            spell.penetrates_wall = false;
            break;
        case EHeroes::tristana:
            spell.delay = 0.25f;
            spell.range             = 900.f;
            spell.speed             = 1100.f;
            spell.slot              = ESpellSlot::w;
            spell.is_fixed_range    = false;
            spell.has_minimum_range = true;
            spell.min_range         = 350.f;
            break;
        case EHeroes::riven:
        {
            auto spell_e = g_local->spell_book.get_spell_slot( ESpellSlot::e );
            if ( spell_e && spell_e->is_ready( ) ) {
                spell.delay           = 0.f;
                spell.range           = 250.f;
                spell.speed           = 2000.f;
                spell.slot            = ESpellSlot::e;
                spell.is_fixed_range  = true;
                spell.penetrates_wall = false;
            } else {
                spell.delay           = 0.25f;
                spell.range           = 275.f;
                spell.speed           = 560.f;
                spell.slot            = ESpellSlot::q;
                spell.is_fixed_range  = true;
                spell.penetrates_wall = false;
            }

            break;
        }
        case EHeroes::tryndamere:
            spell.delay = 0.f;
            spell.range          = 650.f;
            spell.speed          = 900.f;
            spell.slot           = ESpellSlot::e;
            spell.is_fixed_range = false;
            break;
        case EHeroes::lucian:
            spell.delay = 0.f;
            spell.range          = 425.f;
            spell.speed          = 1350.f;
            spell.slot           = ESpellSlot::e;
            spell.is_fixed_range = false;
            break;
        case EHeroes::caitlyn:
            spell.delay = 0.15f;
            spell.range          = 390.f;
            spell.speed          = 500.f;
            spell.invert         = true;
            spell.slot           = ESpellSlot::e;
            spell.is_fixed_range = true;
            break;
        case EHeroes::kindred:
            spell.delay = 0.f;
            spell.range          = 300.f;
            spell.speed          = 500.f + g_local->movement_speed;
            spell.slot           = ESpellSlot::q;
            spell.is_fixed_range = false;
            break;
        case EHeroes::shaco:
            spell.delay = 0.125f;
            spell.range                 = 400.f;
            spell.speed                 = 0.f;
            spell.slot                  = ESpellSlot::q;
            spell.is_fixed_range        = false;
            spell.extends_range_on_wall = true;
            break;
        case EHeroes::corki:
        {
            auto slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
            if ( !slot || rt_hash( slot->get_name( ).data( ) ) == ct_hash( "CarpetBombMega" ) ) return std::nullopt;

            spell.delay             = 0.f;
            spell.range             = 600.f;
            spell.speed             = 650.f + g_local->movement_speed;
            spell.slot              = ESpellSlot::w;
            spell.is_fixed_range    = false;
            spell.has_minimum_range = true;
            spell.min_range         = 250.f;
            break;
        }
        default:
            return std::nullopt;
        }

        return std::make_optional( spell );
    }

    auto Evade::get_units_to_safety( const Vec3& start, const Vec3& path_end ) const -> float{
        for ( auto i = 1; i <= 5; i++ ) {
            auto temp = start.extend( path_end, start.dist_to( path_end ) / 5.f * i );
            if ( is_position_safe( temp ) ) return start.dist_to( temp );
        }

        return 9999.f;
    }

    auto Evade::calculate_path_danger(
        const Vec3& path_start,
        const Vec3& path_end,
        const float movement_speed
    ) -> PathInstance{
        int  total_danger{ };
        auto projected_position{ path_end };

        auto ping{ 0.f };
        switch ( g_config->evade.include_ping_mode->get< int >( ) ) {
        case 0:
            ping = g_features->orbwalker->get_ping( ) + 0.033f;
            break;
        case 1:
            ping = g_features->orbwalker->get_ping( ) / 2.f + 0.033f;
            break;
        default:
            break;
        }

        for ( auto spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( spell.dodge_points.empty( ) ) continue;

            switch ( spell.type ) {
            case SpellDetector::ESpellType::line:
            {
                if ( spell.hitbox_area.is_outside( path_start ) && spell.hitbox_area.is_outside( path_end ) ) continue;

                const auto collision_point   = get_closest_line_point( spell.current_pos, spell.end_pos, path_start );
                const auto time_until_impact = spell.time_till_impact( collision_point ) - ping;

                auto position_on_impact = path_start.extend(
                    path_end,
                    std::min( time_until_impact * movement_speed, path_start.dist_to( path_end ) )
                );

                auto mid_point = path_start.extend(path_end, path_start.dist_to(path_end) / 2.f);

                if (spell.hitbox_area.is_outside(position_on_impact) && spell.hitbox_area.is_outside(mid_point))
                {
                    projected_position = position_on_impact;
                    continue;
                }

                total_danger += spell.danger;
                projected_position = position_on_impact;
                break;
            }
            case SpellDetector::ESpellType::circle:
            {
                if ( spell.hitbox_area.is_outside( path_start ) && spell.hitbox_area.is_outside( path_end ) ) continue;

                const auto time_until_impact  = spell.end_time - *g_time - ping;
                auto       position_on_impact = path_start.extend(
                    path_end,
                    std::min( time_until_impact * movement_speed, path_start.dist_to( path_end ) )
                );

                if ( spell.hitbox_area.is_outside( position_on_impact ) ) {
                    projected_position = position_on_impact;
                    continue;
                }

                total_danger += spell.danger;
                projected_position = position_on_impact;

                break;
            }
            default:
                break;
            }
        }

        return { projected_position, total_danger };
    }


    auto Evade::get_path_danger( const Vec3& start, const Vec3& end, const float move_speed ) const -> int{
        int        danger{ };
        const auto path_duration = start.dist_to( end ) / move_speed;

        for ( auto spell : g_features->spell_detector->get_active_spells( ) ) {
            if ( spell.dodge_points.empty( ) ) continue;

            //float total_radius = spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius : 0.f );

            switch ( spell.type ) {
            case SpellDetector::ESpellType::line:
            {
                if ( spell.hitbox_area.is_outside( start ) && spell.hitbox_area.is_outside( end ) ) continue;

                const auto collision_point   = get_closest_line_point( spell.current_pos, spell.end_pos, start );
                const auto time_until_impact = spell.time_till_impact( collision_point );

                if ( time_until_impact > path_duration ) {
                    if ( spell.hitbox_area.is_outside( end ) ) continue;

                    danger += spell.danger;
                    continue;
                }

                if ( time_until_impact <= 0.f ) {
                    danger += spell.danger;
                    continue;
                }

                const auto projected_safe_point = get_closest_line_point( spell.current_pos, spell.end_pos, end );
                const auto nearest_safe_point   = projected_safe_point.extend(
                    end,
                    spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius : 0.f )
                );

                const auto safe_distance = start.dist_to( nearest_safe_point );

                const auto extend_amount = std::min( start.dist_to( end ), time_until_impact * move_speed );
                if ( extend_amount > safe_distance ) continue;

                auto path_position = start.extend( end, extend_amount );
                //auto future_hitbox = spell.get_future_hitbox( time_until_collision );

                if ( spell.hitbox_area.is_outside( path_position ) ) continue;

                danger += spell.danger;
                continue;
            }
            case SpellDetector::ESpellType::circle:
            {
                auto projection = get_closest_line_point( start, end, spell.end_pos );
                if ( spell.hitbox_area.is_outside( projection ) ) continue;

                const auto time_to_point = start.dist_to( projection ) / move_speed;
                if ( spell.end_time < *g_time + time_to_point ) continue;

                const auto nearest_safe_point = spell.end_pos.extend(
                    end,
                    spell.radius + ( spell.has_edge_radius ? m_raw_bounding_radius : 0.f )
                );
                const auto safe_distance = start.dist_to( nearest_safe_point );

                const auto collision_delay = spell.time_till_impact( projection );
                auto       movable_units   = collision_delay * move_speed;
                //if ( movable_units > safe_distance ) continue;

                auto path_position = start.extend(
                    end,
                    std::min( start.dist_to( end ), collision_delay * move_speed )
                );
                if ( spell.hitbox_area.is_outside( path_position ) ) continue;

                danger += spell.danger;
                break;
            }
            default:
                break;
            }
        }

        return danger;
    }


    auto Evade::get_blink_position( const Vec3& cast_position, const float range ) -> Vec3{
        if ( !g_navgrid->is_wall( cast_position ) ) return cast_position;

        for ( auto i = 1; i <= 20; i++ ) {
            auto points = g_render->get_3d_circle_points( cast_position, 25.f * i, 16 );

            for ( auto point : points ) {
                if ( point.dist_to( g_local->position ) > range * 1.75f ) continue;

                if ( !g_navgrid->is_wall( point ) ) return point;
            }
        }

        return cast_position;
    }

    auto Evade::get_position_weight( const Vec3& point ) -> float{
        float      weight{ };
        const auto cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;

        const auto v1            = point - g_local->position;
        const auto v2            = cursor - g_local->position;
        const auto dot           = v1.normalize( ).dot_product( v2.normalize( ) );
        const auto current_angle = acos( dot ) * 180.f / 3.14159265358979323846f;

        // angle weight
        weight += 1.f - current_angle / 180.f;

        // turret weight
        if ( !helper::is_position_under_turret( point ) ) weight += 1.f;
        if ( helper::is_position_under_turret( point, true ) ) weight += 0.5f;

        // distance weight
        //weight += 1.f - g_local->position.dist_to( point ) / 700.f;

        // ally weight
        const auto nearby_enemies = helper::get_nearby_champions_count( point, false );
        const auto nearby_allies  = helper::get_nearby_champions_count( point, true ) + 1;

        const auto enemy_weight{ nearby_enemies / 5.f };
        const auto ally_weight{ nearby_allies / 5.f };

        if ( ally_weight > enemy_weight ) weight += ally_weight - enemy_weight;
        else if ( enemy_weight > ally_weight ) weight -= enemy_weight - ally_weight;

        return weight;
    }

    auto Evade::get_post_evade_point( ) -> std::optional< Vec3 >{
        if ( *g_time > m_post_evade_expire_time || m_is_evading || g_local->is_dead( ) ) return std::nullopt;

        return std::make_optional( m_post_evade_point );
    }

    auto Evade::can_fit_autoattack() const -> bool {

        if (!g_config->evade.autoattack_sync->get<bool>()) return false;

        auto skillshot = get_colliding_skillshot(g_local->index);
        if (!skillshot) return true;

        auto dodge_time = calculate_minimum_dodge_time();

        return dodge_time < 5.f && skillshot->time_until_collision - dodge_time > 0.033f &&
            skillshot->time_until_collision - dodge_time >
            g_features->orbwalker->get_attack_cast_delay() + g_features->orbwalker->get_ping( ) / 2.f + 0.033f;
    }


}
