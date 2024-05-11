#pragma once
#include "../evade.hpp"
#include "../feature.hpp"
#include "../orbwalker.hpp"
#include "../prediction.hpp"
#include "../target_selector/target_selector.hpp"
#include "module.hpp"

namespace features::champion_modules {
    class malzahar_module final : public IModule {
    public:
        virtual ~malzahar_module( ) = default;

        auto get_name( ) -> hash_t override{ return ct_hash( "malzahar_module" ); }

        auto get_champion_name( ) -> hash_t override{ return ct_hash( "Malzahar" ); }

        auto initialize( ) -> void override{ m_priority_list = { q_spell, w_spell }; }

        auto initialize_menu( ) -> void override{
            const auto navigation = g_window->push( _( "malzahar" ), menu_order::champion_module );
            const auto q_settings = navigation->add_section( _( "q settings" ) );
            const auto drawings   = navigation->add_section( _( "drawings" ) );
            const auto w_settings = navigation->add_section( _( "w settings" ) );
            const auto e_settings = navigation->add_section( _( "e settings" ) );
            const auto r_settings = navigation->add_section( _( "r settings" ) );

            q_settings->checkbox( _( "enable" ), g_config->malzahar.q_enabled );
            q_settings->checkbox( _( "harass q" ), g_config->malzahar.q_harass );
            q_settings->checkbox( _( "autointerrupt q" ), g_config->malzahar.q_autointerrupt )->set_tooltip(
                _( "Automatically interrupts spells like Katarina R, Fiddlesticks R, etc" )
            );
            q_settings->select(
                _( "hitchance" ),
                g_config->malzahar.q_hitchance,
                { _( "Fast" ), _( "Medium" ), _( "High" ), _( "Very high" ), _( "Immobile" ) }
            );
            q_settings->multi_select(
                _( "cast logic " ),
                { g_config->malzahar.q_direct, g_config->malzahar.q_normal },
                { _( "Direct" ), _( "Delayed" ) }
            );

            w_settings->checkbox( _( "enable" ), g_config->malzahar.w_enabled );
            w_settings->slider_int( _( "min pets count" ), g_config->malzahar.w_min_spawn_count, 1, 3, 1 );

            e_settings->checkbox( _( "enable" ), g_config->malzahar.e_enabled );

            r_settings->checkbox( _( "semi manual r [ mouse5 ]" ), g_config->malzahar.r_enabled );

            drawings->checkbox( _( "draw q range" ), g_config->malzahar.q_draw_range );
            drawings->checkbox( _( "draw e range" ), g_config->malzahar.e_draw_range );
            drawings->checkbox( _( "draw r range" ), g_config->malzahar.r_draw_range );
            drawings->checkbox( _( "only draw off cooldown" ), g_config->malzahar.dont_draw_on_cooldown );
        }

        auto on_draw( ) -> void override{
            if ( g_local->is_dead( ) ) return;

            g_local.update( );
            utils::MemoryHolder< SpellSlot > slot{ };

            if ( g_config->malzahar.q_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::q );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->malzahar.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 31, 88, 255, 255 ),
                        m_q_range,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }
            }

            if ( g_config->malzahar.e_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::e );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->malzahar.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 255, 213, 0, 200 ),
                        m_e_range,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }
            }

            if ( g_config->malzahar.r_draw_range->get< bool >( ) ) {
                slot = g_local->spell_book.get_spell_slot( ESpellSlot::r );
                if ( slot && slot->level > 0 &&
                    ( slot->is_ready( true ) || !g_config->malzahar.dont_draw_on_cooldown->get< bool >( ) ) ) {
                    g_render->circle_3d(
                        g_local->position,
                        Color( 173, 47, 68, 255 ),
                        m_r_range,
                        Renderer::outline,
                        72,
                        2.f
                    );
                }
            }

            g_render->rectangle_3d( m_hitbox_start, m_hitbox_end, 100.f, Color::white( ), 2, 3.f );

            Vec2 sp{ };
            if ( world_to_screen( g_local->position, sp ) ) {
                auto font_size = 12;
                auto font      = g_fonts->get_zabel_12px( );

                sp.x += 35.f;
                sp.y -= 30.f;

                std::string text1{ "PETS | " };
                auto        text_mode{ std::to_string( m_spawn_count ) };


                Vec2 texture_size{ 25.f, 25.f };
                static auto Malzahar_w =                     path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            "Malzahar",
                            "spells",
                            "Malzahar_w.png"
                        }
                    );
                auto texture = g_render->load_texture_from_file(
                    Malzahar_w.has_value(  ) ? *Malzahar_w : ""
                );

                float height{ };

                slot = g_local->spell_book.get_spell_slot( ESpellSlot::w );
                if ( slot && slot->level > 0 && m_slot_w->is_ready( true ) ) {
                    if ( texture ) {
                        g_render->image( sp, texture_size, texture );

                        g_render->filled_box( sp, texture_size, Color( 0, 0, 0, 80 ), -1 );

                        bool small_font{ };

                        Color spawn_color{ 255, 255, 255 };
                        switch ( m_spawn_count ) {
                        case 2:
                            spawn_color = Color( 0, 255, 155 );
                            break;
                        case 3:
                            text_mode = "MAX";
                            spawn_color = g_features->orbwalker
                                                    ->get_pulsing_color( );

                            small_font = true;

                            break;
                        default:
                            break;
                        }

                        g_render->box( sp, texture_size, spawn_color, -1, 2.f );

                        auto size = g_render->get_text_size(
                            text_mode,
                            small_font ? g_fonts->get_zabel_12px( ) : g_fonts->get_zabel_16px( ),
                            small_font ? 12 : 16
                        );

                        g_render->text_shadow(
                            { sp.x + texture_size.x / 2.f - size.x / 2.f, sp.y + texture_size.y / 2.f - size.y / 2.f },
                            spawn_color,
                            small_font ? g_fonts->get_zabel_12px( ) : g_fonts->get_zabel_16px( ),
                            text_mode.c_str( ),
                            small_font ? 12 : 16
                        );

                        height = texture_size.y;
                    } else {
                        auto size = g_render->get_text_size( text1, font, font_size );
                        g_render->text_shadow( sp, Color( 255, 255, 255 ), font, text1.c_str( ), font_size );

                        Color spawn_color{ 255, 255, 255 };
                        switch ( m_spawn_count ) {
                        case 2:
                            spawn_color = Color( 0, 255, 155 );
                            break;
                        case 3:
                            text_mode = "MAX";
                            spawn_color = Color( 177, 15, 46 );
                            break;
                        default:
                            break;
                        }

                        g_render->text_shadow(
                            { sp.x + size.x, sp.y },
                            spawn_color,
                            font,
                            text_mode.c_str( ),
                            font_size
                        );

                        height = size.y;
                    }
                }

                if ( *g_time < m_passive_cooldown_expire ) {
                    std::string shield_text{ "SHIELD | " };
                    auto        duration_text{ std::to_string( m_passive_cooldown_expire - *g_time ) };
                    if ( m_passive_cooldown_expire - *g_time >= 10.f ) duration_text.resize( 4 );
                    else duration_text.resize( 3 );

                    Vec2 shield_text_position{ sp.x, sp.y + height };

                    g_render->text_shadow(
                        shield_text_position,
                        Color( 255, 255, 255 ),
                        font,
                        shield_text.c_str( ),
                        font_size
                    );

                    auto size = g_render->get_text_size( shield_text, font, font_size );

                    auto duration_color{
                        m_passive_cooldown_expire - *g_time <= 3.f ? Color( 120, 255, 0 ) : Color( 255, 218, 34 )
                    };


                    g_render->text_shadow(
                        { shield_text_position.x + size.x, shield_text_position.y },
                        duration_color,
                        font,
                        duration_text.c_str( ),
                        font_size
                    );
                }

                // auto buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "2065passivemovespeed" ) );
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

            update_data( );
            track_cast( );

            auto sci = g_local->spell_book.get_spell_cast_info( );
            if ( sci && sci->slot == 3 ) return;

            if ( g_features->orbwalker->in_action( ) || g_features->evade->is_active( ) ) return;

            semi_manual_r( );

            autointerrupt_q( );
            killsteal_q( );

            if ( g_features->orbwalker->in_attack( ) ) return;

            switch ( g_features->orbwalker->get_mode( ) ) {
            case Orbwalker::EOrbwalkerMode::combo:

                spell_e( );
                spell_q( );
                spell_w( );

                break;

                spell_r( );
                break;
            case Orbwalker::EOrbwalkerMode::harass:
            case Orbwalker::EOrbwalkerMode::laneclear:
                if ( g_config->malzahar.q_harass->get< bool >( ) ) spell_q( );
                break;
            default:
                break;
            }
        }

    private:
        auto spell_q( ) -> bool override{
            if ( !g_config->malzahar.q_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.075f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return false;

            for ( auto i = 0; i < 2; i++ ) {
                if ( g_features->target_selector->is_forced( ) && i > 0 ) break;

                const auto target = g_features->target_selector->get_default_target( i > 0 );
                if ( !target ) break;

                if ( direct_q( target ) || combo_q( target ) ) return true;
            }

            return false;
        }

        auto combo_q( Object* target ) -> bool{
            if ( !g_config->malzahar.q_normal->get< bool >( ) || !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                85.f,
                0.865f,
                { },
                true,
                Prediction::include_ping
            );
            if ( !pred.valid ) return false;

            if ( pred.hitchance <
                static_cast< Prediction::EHitchance >( g_config->malzahar.q_hitchance->get< int >( ) ) ) {
                const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "MalzaharE" ) );
                if ( !buff || buff->buff_data->end_time - *g_time > 1.f ) return false;
            }

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar: Q ] Normal target " << target->champion_name.text << " | " << *g_time
                    << std::endl;
                return true;
            }

            return false;
        }

        auto direct_q( Object* target ) -> bool{
            if ( !g_config->malzahar.q_direct->get< bool >( ) || !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                0.f,
                0.65f,
                { },
                false,
                Prediction::include_ping
            );
            if ( !pred.valid ) return false;


            const auto points = g_render->get_3d_circle_points(
                pred.position,
                200.f,
                16
            );

            m_simulation_points = points;


            const auto missile_width = 85.f;

            Vec3 best_point{ };
            bool found_cast{ };
            auto point_distance = FLT_MAX;

            //float target_radius = target->get_bounding_radius( );

            for ( auto point : points ) {
                if ( point.dist_to( g_local->position ) > m_q_range ||
                    target->position.dist_to( point ) > point_distance )
                    continue;

                auto direction =
                    ( point - g_local->position ).normalize( ).rotated( 90.f * ( 3.141592653589793f / 180.f ) );

                auto hitbox1 = point.extend( point + direction, 350.f );
                auto hitbox2 = hitbox1.extend( point, 700.f ).extend( hitbox1, missile_width );

                hitbox1 = hitbox1.extend( point, missile_width );

                if ( hitbox1.dist_to( pred.position ) >= missile_width &&
                    hitbox2.dist_to( pred.position ) >= missile_width )
                    continue;

                best_point     = point;
                found_cast     = true;
                point_distance = target->position.dist_to( point );
            }

            if ( !found_cast || !g_features->orbwalker->is_attackable( target->index ) && point_distance > 120.f &&
                pred.hitchance < static_cast< Prediction::EHitchance >( g_config->malzahar.q_hitchance->get<
                    int >( ) ) )
                return false;

            const auto direction =
                ( best_point - g_local->position ).normalize( ).rotated( 90.f * ( 3.141592653589793f / 180.f ) );

            const auto hitbox1 = best_point.extend( best_point + direction, 350.f );
            auto       hitbox2 = hitbox1.extend( best_point, 700.f );

            //m_hitbox_start = hitbox1;
            //m_hitbox_end   = hitbox2;

            if ( cast_spell( ESpellSlot::q, best_point ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar: Q ] Direct target " << target->champion_name.text << " | point distance " <<
                    point_distance << std::endl;
                return true;
            }

            return false;
        }

        auto killsteal_q( ) -> bool{
            if ( !g_config->malzahar.q_killsteal->get< bool >( ) || *g_time - m_last_q_time <= 0.4f ||
                *g_time - m_last_cast_time <= 0.05f || !m_slot_q->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_killsteal_target(
                m_q_range,
                [ this ]( Object* unit ) -> float{ return get_spell_travel_time( ESpellSlot::q, unit ); },
                [ this ]( Object* unit ) -> float{ return get_spell_damage( ESpellSlot::q, unit ); },
                2
            );
            if ( !target ) return false;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                85.f,
                0.865f,
                { },
                true,
                Prediction::include_ping
            );

            if ( !pred.valid ) return false;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Malzahar: Q ] Killsteal " << target->champion_name.text << " | " << *g_time <<
                    std::endl;
                return true;
            }

            return false;
        }

        auto spell_w( ) -> bool override{
            if ( !g_config->malzahar.w_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_w_time <= 0.4f ||
                !m_slot_w->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 750.f ) return false;

            if ( target->dist_to_local( ) > 500.f &&
                m_spawn_count < g_config->malzahar.w_min_spawn_count->get< int >( ) )
                return false;

            const auto pred = g_features->prediction->predict( target->index, 700.f, 0.f, 0.f, 1.f );
            if ( !pred.valid ) return false;

            const auto buff = g_features->buff_cache->get_buff( target->index, ct_hash( "MalzaharE" ) );
            if ( !buff && g_local->position.dist_to( pred.position ) > 600.f ) return false;

            const auto has_buff = !!buff;

            if ( cast_spell( ESpellSlot::w, pred.position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar: W ] Combo at " << target->champion_name.text << " | Has E buff: " << has_buff
                    << std::endl;
                return true;
            }

            return false;
        }

        auto w_exploit( ) -> bool{
            if ( !GetAsyncKeyState( 0x30 ) ) return false;

            if ( cast_spell( ESpellSlot::w, Vec3( -25000, 0, -250000 ) ) ) {
                std::cout << "[ Malzahar: W ] Cast: " << " | T: " << *g_time
                    << " addy: " << std::hex << m_slot_w.get_address( ) << std::endl;

                m_last_cast_time = *g_time;
                m_w_active       = !m_w_active;

                return true;
            }


            return false;
        }

        auto instant_w( const Vec3& position ) -> bool{
            if ( position.length( ) <= 0.f || *g_time - m_last_w_time <= 0.1f || *g_time - m_last_cast_time <=
                0.05f )
                return false;

            if ( cast_spell( ESpellSlot::w, position ) ) {
                m_last_w_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar: W ] Pre-ult cast | " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto instant_e( const unsigned network_id ) -> bool{
            if ( network_id == 0 || *g_time - m_last_e_time <= 0.3f || *g_time - m_last_cast_time <= 0.05f || !m_slot_e
                ->is_ready( true ) )
                return false;

            if ( cast_spell( ESpellSlot::e, network_id ) ) {
                g_features->orbwalker->on_cast( );
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                m_was_instant_e = true;

                std::cout << "[ Malzahar: E ] Pre-ult cast | " << *g_time << std::endl;

                return true;
            }

            return false;
        }

        auto spell_e( ) -> bool override{
            if ( !g_config->malzahar.e_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_e_time <= 0.4f || !m_slot_e->is_ready( true ) )
                return false;

            const auto target = g_features->target_selector->get_default_target( );
            if ( !target || target->dist_to_local( ) > 650.f ) return false;

            if ( cast_spell( ESpellSlot::e, target->network_id ) ) {
                m_last_e_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar ] E Cast at " << target->champion_name.text << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto spell_r( ) -> bool override{
            if ( !g_config->nami.r_enabled->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_r_time <= 0.4f || !m_slot_r->is_ready( true ) )
                return false;

            const auto multihit = get_multihit_position( m_r_range, 850.f, 260.f, 0.5f, true );
            if ( multihit.hit_count < g_config->nami.r_minimum_hitcount->get< int >( ) ) return false;

            if ( cast_spell( ESpellSlot::r, multihit.position ) ) {
                m_last_r_time    = *g_time;
                m_last_cast_time = *g_time;
                g_features->orbwalker->on_cast( );

                std::cout << "[ Nami ] R Multihit, count: " << multihit.hit_count << " | " << *g_time << std::endl;
                return true;
            }

            return false;
        }

        auto semi_manual_r( const bool force = false ) -> bool{
            if ( !force && !GetAsyncKeyState( 0x06 ) || *g_time - m_last_r_time <= 0.4f || !m_slot_r->
                is_ready( true ) )
                return false;

            const auto    cursor = g_pw_hud->get_hud_manager( )->cursor_position_unclipped;
            const Object* target{ };
            auto          lowest_distance = 500.f;

            for ( const auto enemy : g_entity_list.get_enemies( ) ) {
                if ( !enemy || enemy->dist_to_local( ) > m_r_range - 5.f ||
                    g_features->target_selector->is_bad_target( enemy->index ) )
                    continue;

                const auto distance = enemy->position.dist_to( cursor );
                if ( distance > lowest_distance ) continue;

                target          = enemy;
                lowest_distance = distance;
            }

            if ( !target ) return false;

            if ( m_slot_e->is_ready( true ) && *g_time - m_last_e_time > 0.5f && target->dist_to_local( ) < m_e_range &&
                instant_e( target->network_id ) ||
                m_slot_w->is_ready( true ) && *g_time - m_last_w_time > 0.25f &&
                instant_w( target->position ) )
                return false;

            if ( cast_spell( ESpellSlot::r, target->network_id ) ) {
                std::cout << "[ Malzahar: R ] Semi manual cast | " << *g_time << std::endl;

                g_features->orbwalker->on_cast( );
                m_last_cast_time = *g_time;
                m_last_r_time    = *g_time;

                return true;
            }

            return false;
        }

        auto autointerrupt_q( ) -> void{
            if ( !g_config->malzahar.q_autointerrupt->get< bool >( ) || *g_time - m_last_cast_time <= 0.05f ||
                *g_time - m_last_q_time <= 0.4f || !m_slot_q->is_ready( true ) )
                return;

            const auto target = get_interruptable_target( m_q_range );
            if ( !target ) return;

            const auto pred = g_features->prediction->predict(
                target->index,
                m_q_range,
                0.f,
                85.f,
                0.865f,
                { },
                true,
                Prediction::include_ping
            );
            if ( !pred.valid || ( int )pred.hitchance < 3 ) return;

            if ( cast_spell( ESpellSlot::q, pred.position ) ) {
                g_features->orbwalker->on_cast( );
                m_last_q_time    = *g_time;
                m_last_cast_time = *g_time;

                std::cout << "[ Malzahar: Q ] Auto-interrupting " << target->champion_name.text << " | " << *g_time
                    << std::endl;
            }
        }

        auto update_data( ) -> void{
            auto buff = g_features->buff_cache->get_buff(
                g_local->index,
                ct_hash( "malzaharpassiveshieldcooldownindicator" )
            );
            if ( buff ) m_passive_cooldown_expire = buff->buff_data->end_time;
            else m_passive_cooldown_expire        = 0.f;

            buff = g_features->buff_cache->get_buff( g_local->index, ct_hash( "MalzaharW" ) );
            if ( buff ) m_spawn_count = 1 + buff->stacks( );
            else m_spawn_count        = 1;
        }

        auto track_cast( ) -> void{
            if ( !m_e_active ) {
                auto sci = g_local->spell_book.get_spell_cast_info( );
                if ( !sci || sci->slot != 2 || sci->server_cast_time < *g_time - 0.1f ) {
                    if ( m_was_instant_e && *g_time - m_last_e_time >= 0.2f ) m_was_instant_e = false;
                    return;
                }

                m_e_active           = true;
                m_e_server_cast_time = sci->server_cast_time;
            }

            if ( m_was_instant_e && m_e_server_cast_time - *g_time <= 0.05f ) semi_manual_r( true );

            if ( m_e_server_cast_time <= *g_time ) {
                m_was_instant_e = false;
                m_e_active      = false;
            }
        }

    protected:
        auto get_spell_damage( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return helper::calculate_damage(
                    m_q_damage[ m_slot_q->level ] + g_local->ability_power( ) * 0.8f,
                    target->index,
                    false
                );
            case ESpellSlot::r:
                return helper::calculate_damage(
                    m_r_damage[ m_slot_r->level ] + g_local->ability_power( ) * 0.7f,
                    target->index,
                    false
                );
            default:
                return 0.f;
            }
        }

        auto get_spell_travel_time( const ESpellSlot slot, Object* target ) -> float override{
            switch ( slot ) {
            case ESpellSlot::q:
                return 0.65f;
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

        // cast track
        bool  m_e_active{ };
        float m_e_server_cast_time{ };

        bool m_was_instant_e{ };

        // Q Simulation
        Vec3 m_hitbox_start{ };
        Vec3 m_hitbox_end{ };

        std::vector< Vec3 > m_simulation_points{ };

        int m_spellslot{ 63 };

        // w
        bool m_w_active{ };

        // DATA
        int   m_spawn_count{ };
        float m_passive_cooldown_expire{ };

        std::vector< float > m_q_damage{ 0.f, 70.f, 105.f, 140.f, 175.f, 210.f };
        std::vector< float > m_e_damage{ 0.f, 80.f, 115.f, 150.f, 185.f, 220.f };
        std::vector< float > m_r_damage{ 0.f, 125.f, 200.f, 275.f };

        float m_q_range{ 900.f };
        float m_w_range{ 200.f };
        float m_e_range{ 650.f };
        float m_r_range{ 700.f };
    };
}
