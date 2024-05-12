#include "pch.hpp"

#include "tracker.hpp"

#include "activator/activator.hpp"
#include "entity_list.hpp"
#include "../renderer/c_fonts.hpp"
#include "../include/fmt/core.h"

#include "evade.hpp"
// #include "target_selector/target_selector.hpp"
#include "prediction.hpp"
#include "../sdk/audio/audio.hpp"

// #include "../utils/utils.hpp"
#include "../utils/c_function_caller.hpp"
#include "../sdk/game/ai_manager.hpp"
// #include "../sdk/game/buff.hpp"
// #include "../sdk/game/hud_manager.hpp"
#include "../sdk/game/render_manager.hpp"
// #include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/spell_data.hpp"
// #include "../sdk/game/spell_details.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../utils/directory_manager.hpp"
#include "../utils/path.hpp"
#include "buff_cache.hpp"
#include "../sdk/game/hud_manager.hpp"
#include "../sdk/game/spell_cast_info.hpp"
#include "target_selector/ITargetSelector.hpp"
#pragma comment(lib, "winmm.lib")

namespace features {
    auto Tracker::run( ) -> void{
        update_enemies( );
        update_recalls( );
        update_wards( );
        update_clones( );
        update_turrets( );
        update_spell_tracker( );
        update_hud( );
        update_jungle_tracker( );
        update_last_seen_data( );
        update_dashes( );
        update_items( );
        update_experience_tracker( );
        initialize_jungler_tracking( );

        update_summoner_cooldowns( );

        update_skin();


        //update_incoming_wave( );

        //update_predictions( );
        //update_objectives( );

        // TODO: uncomment this to re-enable zoomhack
        static auto tick = 0;
        if ( tick % 120 == 0 && g_function_caller->is_zoom_bypassed( ) ) {
            if ( g_config->awareness.zoomhack_toggle->get< bool >( ) &&
                m_last_zoom_value != g_config->awareness.zoomhack_modifier->get< int >( )
            ) {
                g_camera_config->set_zoom(
                    2250.f + static_cast< float >( g_config->awareness.zoomhack_modifier->get< int >( ) )
                );
                m_last_zoom_value = g_config->awareness.zoomhack_modifier->get< int >( );
            }

#if __DEBUG // Should be safe but we will wait a bit
            if ( g_config->awareness.fov_toggle->get< bool >( ) &&
                m_last_fov_value != g_config->awareness.fov_modifier->get< int >( )
            ) {
                g_pw_hud->set_fov(
                    40.f + static_cast< float >( g_config->awareness.fov_modifier->get< int >( ) )
                );
                m_last_fov_value = g_config->awareness.fov_modifier->get< int >( );
            }
#endif
        }
        tick++;

        if ( g_config->awareness.show_enemy_turret_range->get< bool >( ) ) {
            g_function_caller->set_turret_range_indicator(
                true,
                false
            );
        }
        if ( g_config->awareness.show_ally_turret_range->get< bool >( ) )
            g_function_caller->set_turret_range_indicator(
                true,
                true
            );

        if ( *g_time == m_last_tick ) return;

        int r, g, b;

        switch ( m_color_stage ) {
        case 0:
            r = 255;
            g = m_color_value;
            b = 0;
            break;
        case 1:
            r = 255 - m_color_value;
            g = 255;
            b = 0;
            break;
        case 2:
            r = 0;
            g = 255;
            b = m_color_value;
            break;
        case 3:
            r = 0;
            g = 255 - m_color_value;
            b = 255;
            break;
        case 4:
            r = m_color_value;
            g = 0;
            b = 255;
            break;
        case 5:
            r = 255;
            g = 0;
            b = 255 - m_color_value;
            break;
        default:
            return;
        }

        if ( m_color_value == 255 ) {
            m_color_stage++;
            if ( m_color_stage > 5 ) m_color_stage = 0;
            m_color_value = 0;
        }

        m_color_value += 5;

        m_rainbow_color = { r, g, b };

        if ( m_glow_working && ( g_config->orbwalker.glow_mode->get< int >( ) > 0 || m_local_glow ) &&
            g_function_caller->is_glow_queueable( ) ) {
            if ( GetAsyncKeyState( 0x30 ) ) {
                std::cout << "\ncustom glow color ----- \n";
                std::cout << "Layer 1: { " << std::dec << g_config->orbwalker.glow_color->get< Color >( ).r << ", "
                    << g_config->orbwalker.glow_color->get< Color >( ).g << ", "
                    << g_config->orbwalker.glow_color->get< Color >( ).b << " } \n";

                std::cout << "Layer 2: { " << std::dec << g_config->orbwalker.second_glow_color->get< Color >( ).r <<
                    ", "
                    << g_config->orbwalker.second_glow_color->get< Color >( ).g << ", "
                    << g_config->orbwalker.second_glow_color->get< Color >( ).b << " } \n";

                std::cout << "Layer 3: { " << std::dec << g_config->orbwalker.third_glow_color->get< Color >( ).r <<
                    ", "
                    << g_config->orbwalker.third_glow_color->get< Color >( ).g << ", "
                    << g_config->orbwalker.third_glow_color->get< Color >( ).b << " } \n";
            }

            switch ( g_config->orbwalker.glow_preset->get< int >( ) ) {
            case 1:
            {
                /*
                Layer 1: { 213, 170, 0 }
Layer 2: { 255, 255, 255 }
Layer 3: { 255, 224, 0 }
                 */

                switch ( m_last_glow_id ) {
                case 0:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 213, 170, 0 ), 0, 5, 16 );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 255, 255, 255 ), 1, 3, 1 );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 255, 225, 0 ), 2, 2, 1 );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                break;
            }
            case 2:
            {
                Color layer_color{ };

                switch ( m_last_glow_id ) {
                case 0:
                    layer_color = g_features->orbwalker->animate_color(
                        Color( 86, 31, 175 ),
                        EAnimationType::pulse,
                        1,
                        200
                    );


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        0,
                        5,
                        12
                    );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    layer_color = Color( 255, 255, 255 );


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        1,
                        3,
                        2
                    );

                    m_last_glow_id = !m_adjusted_layers ? 2 : 0;
                    break;
                default:

                    if ( !m_adjusted_layers ) {
                        g_function_caller->enable_glow(
                            g_local->network_id,
                            D3DCOLOR_ARGB( 255, 0, 220, 255 ),
                            2,
                            3,
                            0,
                            true
                        );

                        m_adjusted_layers = true;
                    }

                    m_last_glow_id = 0;

                    break;
                }


                break;
            }
            case 3:
            {
                Color layer_color{ };

                switch ( m_last_glow_id ) {
                case 0:
                    layer_color =
                        m_rainbow_color;


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        0,
                        3,
                        10
                    );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    layer_color = m_rainbow_color;


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, layer_color.r, layer_color.g, layer_color.b ),
                        1,
                        3,
                        0
                    );

                    m_last_glow_id = !m_adjusted_layers ? 2 : 0;
                    break;
                default:

                    if ( !m_adjusted_layers ) {
                        g_function_caller->enable_glow(
                            g_local->network_id,
                            D3DCOLOR_ARGB( 255, 0, 220, 255 ),
                            2,
                            3,
                            0,
                            true
                        );

                        m_adjusted_layers = true;
                    }

                    m_last_glow_id = 0;

                    break;
                }


                break;
            }
            case 4:
            {
                switch ( m_last_glow_id ) {
                case 0:
                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, 0, 0, 0 ),
                        0,
                        5,
                        5
                    );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    //layer_color = g_features->orbwalker->animate_color( Color( 0, 0, 0 ), e_animation_type::pulse, 2 );


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, 80, 0, 0 ),
                        1,
                        4,
                        3
                    );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    //layer_color = g_features->orbwalker->animate_color( Color( 0, 0, 0 ), e_animation_type::pulse, 2 );


                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 255, 0, 0 ), 2, 2, 0 );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                break;
            }
            case 5:
            {
                switch ( m_last_glow_id ) {
                case 0:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 0, 82, 255 ), 0, 6, 30 );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 0, 255, 255 ), 1, 3, 4 );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 255, 255, 255 ), 2, 3, 0 );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                break;
            }
            case 6:
            {
                switch ( m_last_glow_id ) {
                case 0:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 0, 0, 0 ), 0, 3, 8 );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, 0, 232, 10 ),
                        1,
                        3,
                        4
                    );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( 255, 215, 255, 120 ),
                        2,
                        3,
                        0
                    );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                break;
            }
            case 7:
            {
                //Layer 1: { 75, 255, 162 }
                //Layer 2: { 70, 255, 181 }
                //Layer 3: { 53, 255, 150 }
                switch ( m_last_glow_id ) {
                case 0:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 75, 255, 162 ), 0, 3, 30 );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 70, 255, 181 ), 1, 3, 1 );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    g_function_caller->enable_glow( g_local->network_id, D3DCOLOR_ARGB( 255, 53, 255, 150 ), 2, 3, 0 );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                break;
            }
            default:
                break;
            }

            if ( g_config->orbwalker.glow_preset->get< int >( ) > 0 ) {
                const auto layers =
                    g_config->orbwalker.glow_preset->get< int >( ) > 3 || g_config->orbwalker.glow_preset->get< int >( )
                    == 1
                        ? 3
                        : 2;

                if ( layers != m_glow_layer_count ) {
                    m_last_glow_layer_count = m_glow_layer_count;
                    m_adjusted_layers       = m_last_glow_layer_count != 3;
                }

                m_glow_layer_count =
                    g_config->orbwalker.glow_preset->get< int >( ) > 3 || g_config->orbwalker.glow_preset->get< int >( )
                    == 1
                        ? 3
                        : 2;

                m_last_tick  = *g_time;
                m_local_glow = true;
                return;
            }

            // gold base
            //255, 234, 148

            if ( false && g_config->orbwalker.glow_preset->get< int >( ) == 0 ) {
                Color glow_color{ 0, 0, 0 };

                switch ( m_last_glow_id ) {
                case 0:

                    glow_color = g_config->orbwalker.glow_style->get< int >( ) == 2
                                     ? m_rainbow_color
                                     : g_config->orbwalker.glow_color->get< Color >( );
                    if ( g_config->orbwalker.glow_style->get< int >( ) == 1 ) {
                        glow_color = g_features->orbwalker->
                                                 animate_color( glow_color, EAnimationType::pulse, 3 );
                    }


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        0,
                        g_config->orbwalker.glow_size->get< int >( ),
                        g_config->orbwalker.glow_diffusion->get< int >( )
                    );

                    m_last_glow_id = 1;
                    break;
                case 1:
                    glow_color = g_config->orbwalker.second_glow_style->get< int >( ) == 2
                                     ? m_rainbow_color
                                     : g_config->orbwalker.second_glow_color->get< Color >( );
                    if ( g_config->orbwalker.second_glow_style->get< int >( ) == 1 ) {
                        glow_color = g_features->orbwalker
                                               ->animate_color( glow_color, EAnimationType::pulse, 3 );
                    }


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        1,
                        g_config->orbwalker.second_glow_size->get< int >( ),
                        g_config->orbwalker.second_glow_diffusion->get< int >( )
                    );

                    m_last_glow_id = 2;
                    break;
                case 2:
                    glow_color = g_config->orbwalker.third_glow_style->get< int >( ) == 2
                                     ? m_rainbow_color
                                     : g_config->orbwalker.third_glow_color->get< Color >( );
                    if ( g_config->orbwalker.third_glow_style->get< int >( ) == 1 ) {
                        glow_color = g_features->orbwalker->
                                                 animate_color( glow_color, EAnimationType::pulse, 3 );
                    }


                    g_function_caller->enable_glow(
                        g_local->network_id,
                        D3DCOLOR_ARGB( glow_color.a, glow_color.r, glow_color.g, glow_color.b ),
                        2,
                        g_config->orbwalker.third_glow_size->get< int >( ),
                        g_config->orbwalker.third_glow_diffusion->get< int >( )
                    );

                    m_last_glow_id = 0;
                    break;
                default:
                    break;
                }

                m_local_glow       = true;
                m_glow_layer_count = 3;
            } else if ( false && m_local_glow ) {
                g_function_caller->enable_glow(
                    g_local->network_id,
                    D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                    3,
                    0,
                    true
                );
                m_local_glow = false;
            }
        }

        m_last_tick = *g_time;
    }

    auto Tracker::update_skin() -> void {

        if (!m_skin_key_pressed_up && GetAsyncKeyState(VK_RIGHT)){

            m_skin_value       = m_skin_value + 1;
            m_skin_key_pressed_up = true;

            g_function_caller->set_player_skin_id(m_skin_value);

            std::cout << "[ Skinchanger: UPDATE ] Set skinID to " << std::dec << m_skin_value << std::endl;
        }
        else if (m_skin_key_pressed_up && !GetAsyncKeyState(VK_RIGHT))
            m_skin_key_pressed_up = false;

        if (!m_skin_key_pressed_down && GetAsyncKeyState(VK_LEFT))
        {

            m_skin_value       = m_skin_value == 0 ? 0 : m_skin_value - 1;
            m_skin_key_pressed_down = true;

            g_function_caller->set_player_skin_id(m_skin_value);

            std::cout << "[ Skinchanger: UPDATE ] Set skinID to " << std::dec << m_skin_value << std::endl;
        }
        else if (m_skin_key_pressed_down && !GetAsyncKeyState(VK_LEFT))
            m_skin_key_pressed_down = false;
    }

    auto Tracker::draw_hud( ) -> void{
        if ( !g_config->hud.toggle->get< bool >( ) ) return;
        auto scale = static_cast< float >( g_config->hud.scale->get< int >( ) ) / 100.f;

        const Vec2 champ_img_size{ 100.f * scale, 100.f * scale };
        const Vec2 spell_img_size{ 40.f * scale, 40.f * scale };

        const Vec2 bar_size{ 100.f * scale, 20.f * scale };

        const Vec2 spell_inset{ 2.f, 2.f };
        const Vec2 bar_inset{ 2.f, 2.f };

        const Color health_color = { 5, 170, 5, 255 };
        const Color mana_color   = { 55, 144, 222, 255 };

        // get the max size possible, champion img size x + spell img size + spell inset, champion img size + both bars and their insets
        const Vec2 entry_size{
            champ_img_size.x + spell_img_size.x + spell_inset.x,
            champ_img_size.y + bar_size.y * 2 + bar_inset.y * 2
        };

        Vec2 config_position = {
            static_cast< float >( g_config->hud.base_x->get< int >( ) ),
            static_cast< float >( g_config->hud.base_y->get< int >( ) )
        };


        /*
        int size = static_cast< int >( m_hud_infos.size( ) );
        if ( g_config->hud.vertical->get< bool >( ) ) {
            //g_render->filled_box( config_position, { entry_size.x, entry_size.y * static_cast< float >( size ) }, color( 0, 0, 0, 80 ) );
        } else {
            //g_render->filled_box( config_position, { static_cast< float >( size ) * entry_size.x, entry_size.y }, color( 0, 0, 0, 80 ) );
        }
        */

        float counter    = 0;
        auto  hund_infos = m_hud_infos;
        for ( const auto& hud_info : hund_infos ) {
            Vec2 base_position{ };
            if ( g_config->hud.vertical->get< bool >( ) ) {
                // std::cout << "WE ARE VERTICAL" << std::endl;
                base_position = { config_position.x, config_position.y + entry_size.y * counter };
            } else {
                // std::cout << "WE ARE HORIZONTAL" << std::endl;
                base_position = { config_position.x + entry_size.x * counter, config_position.y };
            }

            auto& ent_upd = g_entity_list.get_by_index( hud_info.index );
            if ( !ent_upd ) continue;
            ent_upd.update( );

            std::string champ_name = ent_upd->champion_name.text;
            auto        texture    = g_render->load_texture_from_file(
                path::join(
                    { directory_manager::get_resources_path( ), "champions", champ_name, champ_name + "_square.png" }
                ).value( )
            );
            if ( texture ) g_render->image( base_position, champ_img_size, texture );

            auto dead = ent_upd->is_dead( );
            if ( dead || !ent_upd->is_visible( ) ) {
                g_render->filled_box( base_position, champ_img_size, Color( 0, 0, 0, 150 ) );
                std::string text = dead ? "DEAD" : "MIA";

                auto size = g_render->get_text_size( text, g_fonts->get_bold( ), 16 );
                g_render->text(
                    {
                        base_position.x + champ_img_size.x / 2.f - size.x / 2.f,
                        base_position.y + champ_img_size.y / 2.f - size.y / 2.f
                    },
                    Color::white( ),
                    g_fonts->get_bold( ),
                    text.c_str( ),
                    16
                );
            }

            Vec2 hp_position = { base_position.x, base_position.y + champ_img_size.y + bar_inset.y };

            g_render->filled_box(
                hp_position,
                bar_size,
                Color( 0, 0, 0, 140 )
            ); // draw health bar background [2 -> 98, 102 -> 118]
            g_render->filled_box(
                hp_position,
                { ( bar_size.x * hud_info.health_normalized ), bar_size.y },
                health_color
            );
            g_render->box( hp_position, bar_size, Color::black( ) ); // draw health bar outline [2 -> 98, 102 -> 118]

            g_render->box(
                { hp_position.x + 1.f, hp_position.y + 1.f },
                { bar_size.x - 2.f, bar_size.y - 2.f },
                Color( 0, 0, 0, 80 ),
                0,
                2.f
            );


            Vec2 mana_position = { hp_position.x, hp_position.y + bar_inset.y + bar_size.y };

            g_render->filled_box(
                mana_position,
                bar_size,
                Color( 0, 0, 0, 140 )
            ); // draw mana bar background [2 -> 98, 120 -> 136]
            g_render->filled_box(
                mana_position,
                { ( bar_size.x * hud_info.mana_normalized ), bar_size.y },
                mana_color
            );
            g_render->box( mana_position, bar_size, Color::black( ) ); // draw health bar outline [2 -> 98, 102 -> 118]

            const auto spell_info = get_hero_spells( hud_info.index );
            if ( !spell_info ) continue;

            Vec2 spell_pos = { base_position.x + champ_img_size.x + spell_inset.x, base_position.y };
            for ( auto i = 0; i < 3; i++ ) {
                renderer::Renderer::Texture*     img{ };
                utils::MemoryHolder< SpellSlot > slot;
                ESpellSlot                       slot_index;
                SpellSlotInfo                    slot_info;

                switch ( i ) {
                default:
                    break;
                case 0:
                    img = hud_info.summoner_1_img;
                    slot_index = ESpellSlot::d;
                    slot_info  = spell_info.value( ).summoner_d;
                    break;
                case 1:
                    img = hud_info.summoner_2_img;
                    slot_index = ESpellSlot::f;
                    slot_info  = spell_info.value( ).summoner_f;

                    break;
                case 2:
                    img = hud_info.ult_img;
                    slot_index = ESpellSlot::r;
                    slot_info  = spell_info.value( ).spell_r;
                    break;
                }

                if ( !img ) continue;

                g_render->image( spell_pos, spell_img_size, img );

                float cooldown_expire = slot_info.cooldown_expire;

                auto spell_slot_raw = ent_upd->spell_book.get_spell_slot(slot_index);
                if (spell_slot_raw && rt_hash(spell_slot_raw->get_name().data()) == ct_hash("SummonerFlash")) {
                    cooldown_expire = get_summoner_cooldown(ent_upd->index);
                }

                if (cooldown_expire > *g_time)
                {
                    g_render->filled_box( spell_pos, spell_img_size, Color( 0, 0, 0, 150 ) );
                    if ( slot_info.level == 0 ) continue;

                    send_cooldown_in_chat(
                        spell_pos,
                        spell_img_size,
                        ent_upd->champion_name,
                        ent_upd->index,
                        slot_index,
                        ent_upd->network_id
                    );

                    auto cooldown_value = static_cast<int>(cooldown_expire - *g_time);
                    if ( cooldown_value < 0 ) cooldown_value = 0;
                    auto text = fmt::format( "{}", cooldown_value );
                    auto size = g_render->get_text_size( text, g_fonts->get_bold( ), 16 );
                    g_render->text(
                        {
                            spell_pos.x + spell_img_size.x / 2.f - size.x / 2.f,
                            spell_pos.y + spell_img_size.y / 2.f - size.y / 2.f
                        },
                        Color::white( ),
                        g_fonts->get_bold( ),
                        text.c_str( ),
                        16
                    );
                }

                spell_pos = { spell_pos.x, spell_pos.y + spell_img_size.y + spell_inset.y };
            }

            counter++;
        }
    }


    auto Tracker::is_hud_cached( int16_t index ) -> bool{
        return std::ranges::find_if(
            m_hud_infos,
            [index]( const HudInfo& hud_info ) -> bool{ return hud_info.index == index; }
        ) != m_hud_infos.end( );
    }

    auto Tracker::update_hud( ) -> void{
        if ( !g_config->hud.toggle->get< bool >( ) ) return;

        for ( const auto obj : g_entity_list.get_enemies( ) ) {
            if ( !obj ) continue;

            if ( !is_hud_cached( obj->index ) ) {
                if ( !is_hero_spells_tracked( obj->index ) ) continue;
                const auto spell_info = get_hero_spells( obj->index );
                if ( !spell_info ) continue;

                auto name = obj->get_name( );

                HudInfo hud_info{ };
                hud_info.index          = obj->index;
                hud_info.summoner_1_img = spell_info->summoner_d.spell_texture;
                hud_info.summoner_2_img = spell_info->summoner_f.spell_texture;
                hud_info.ult_img        = spell_info->spell_r.spell_texture;
                hud_info.champ_img      = g_render->load_texture_from_file(
                    path::join(
                        { directory_manager::get_resources_path( ), "champions", name, name + "_square.png" }
                    ).value( )
                ).get( );

                m_hud_infos.push_back( hud_info );
            }

            auto hud_info = get_hud_info( obj->index );
            //debug_log( "Updating hud info for index {}, normalized health to {}", hud_info->index, (obj->health / obj->max_health) );
            hud_info->health_normalized = obj->health / obj->max_health;

            if ( obj->max_mana > 0.f ) hud_info->mana_normalized = obj->mana / obj->max_mana;

            hud_info = get_hud_info( obj->index );
            //debug_log( "After update, get health normalized is {}", hud_info->health_normalized );
        }
    }

    auto Tracker::draw_recall_tracker( ) const -> void{
        if ( !g_config->awareness.recall_tracker->get< bool >( ) ) return;

        constexpr auto bar_height        = 20.f;
        constexpr auto bar_width         = 250.f;
        constexpr auto bar_gap           = 4.f;
        constexpr auto bar_bottom_offset = 200.f;

        const auto screen_size = g_render->get_screensize( );

        const auto                                      draw_recall_bar = [&](
            const int32_t                               recall,
            const Color&                                inner_color,
            const Color&                                outer_color,
            const std::string&                          hero_name,
            const float                                 progress = -1.f,
            const std::shared_ptr< Renderer::Texture >& texture  = { }
        ) -> void{
            const auto box_position = Vec2(
                ( screen_size.x / 2.f ) - ( bar_width / 2.f ),
                screen_size.y - bar_bottom_offset - static_cast< float >( recall ) * ( bar_height + bar_gap ) - bar_gap
                / 2.f
            );

            g_render->filled_box( box_position + Vec2( 1.f, 1.f ), Vec2( 248.f, bar_height - 2.f ), inner_color );
            if ( progress != -1.f ) {
                g_render->filled_box(
                    box_position + Vec2( 1.f, 1.f ),
                    Vec2( 248.f * progress, bar_height - 2.f ),
                    Color::white( )
                );
            }
            g_render->box( box_position, Vec2( 250.f, bar_height ), outer_color, 0, 2.f );

            if ( texture ) {
                const Vec2 texture_size{ 26.f, 26.f };
                const Vec2 texture_position{ box_position.x + 254.f, box_position.y - 3.f };

                g_render->image( texture_position, texture_size, texture );
            }

            const auto size = g_render->get_text_size( hero_name, g_fonts->get_bold( ), 16 );
            g_render->text_shadow(
                Vec2( box_position.x - size.x + 245.f, box_position.y + 0.f ),
                Color::white( ),
                g_fonts->get_bold( ),
                hero_name.data( ),
                16
            );
        };

        auto       current_recall = 0;
        const auto recalls        = m_recalls;
        for ( const auto& recall : recalls ) {
            const auto total_recall_time = recall.type == ERecallType::normal ? 7.9f : 3.9f;

            const auto& object = g_entity_list.get_by_index( recall.index );
            if ( !object ) continue;

            std::string champ_name = object->champion_name.text;
            auto        texture    = g_render->load_texture_from_file(
                path::join(
                    { directory_manager::get_resources_path( ), "champions", champ_name, champ_name + "_square.png" }
                ).value( )
            );

            if ( recall.canceled ) {
                draw_recall_bar(
                    current_recall,
                    Color( 255, 50, 50, 200 ),
                    Color( 113, 2, 2, 255 ),
                    recall.object_name,
                    { },
                    texture
                );
            } else if ( recall.finish_time > *g_time ) {
                draw_recall_bar(
                    current_recall,
                    Color( 20, 20, 20, 240 ),
                    Color( 25, 25, 25, 255 ),
                    recall.object_name,
                    std::clamp( 1.f - ( ( recall.finish_time - *g_time ) / total_recall_time ), 0.f, 1.f ),
                    texture
                );
            } else {
                draw_recall_bar(
                    current_recall,
                    Color( 50, 255, 50, 200 ),
                    Color( 25, 110, 0, 255 ),
                    recall.object_name,
                    { },
                    texture
                );
            }

            current_recall++;
        }
    }

    auto Tracker::draw_orbwalker_indicator( ) const -> void{
        if ( !g_config->awareness.show_orbwalking_flag->get< bool >( ) ) return;

        for ( auto& inst : m_enemies ) {
            const auto& enemy = g_entity_list.get_by_index( inst );

            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

            Vec2 sp{ };
            if ( !world_to_screen( enemy->position, sp ) ) continue;


            const auto avg = g_features->prediction->get_average_path_time( enemy->network_id );
            if ( avg >= 0.2f || avg <= 0.05f ) continue;

            sp.y -= 20.f;
            sp.x += 40.f;

            std::string text   = avg < 0.16f ? _( "ORBWALKING | " ) : _( "SWEATING | " );
            auto        number = std::to_string( avg );
            number.resize( 4 );

            text += number;

            auto text_color = avg < 0.16f ? g_features->orbwalker->get_pulsing_color( ) : Color( 255, 255, 30 );

            g_render->text_shadow( sp, text_color, g_fonts->get_block( ), text.c_str( ), 8 );
        }
    }

    auto Tracker::update_last_seen_data( ) -> void{
        if ( !g_config->awareness.last_seen_position->get< bool >( ) ) return;

        for ( auto& inst : m_last_seen_data ) {
            auto& object = g_entity_list.get_by_index( inst.index );
            if ( !object ) continue;

            object.update( );
            if ( !object ) continue;


            if ( object->is_invisible( ) && object->is_alive( ) ) {

                if ( object->position != inst.last_raw_position ) {

                    bool was_actual_update = object->position.dist_to(inst.last_raw_position) > 1.f;

                    
                    if ( was_actual_update)
                    {

                        std::cout << object->get_name()
                                  << ": FOW UPDATE pos_delta: " << object->position.dist_to(inst.last_position)
                                  << " | time delta: " << *g_time - inst.last_seen_time << std::endl;
                    }

                    if (*g_time - inst.last_death_time > 0.25f && was_actual_update)
                        inst.last_position = object->position;

                    inst.last_raw_position = object->position;

                    if (was_actual_update) inst.last_seen_time = *g_time;

                } else if ( false && g_config->awareness.last_seen_look_for_particles->get< bool >( ) ) {
                    auto fow_position = get_fow_particle_position( rt_hash( object->champion_name.text ) );
                    if ( fow_position && fow_position->dist_to( inst.last_position ) > 100.f ) {
                        inst.last_position  = *fow_position;
                        inst.last_seen_time = *g_time;

                        std::cout << "[ tracking ] Updated FOW position for " << object->champion_name.text
                            << " due to fow particle | " << *g_time << std::endl;
                    }
                }

                if ( inst.is_updated_path &&
                    inst.last_position.dist_to( inst.last_path_position ) >= 75.f )
                    inst.is_updated_path = false;

                auto aimgr = object->get_ai_manager( );
                if ( aimgr ) {
                    aimgr.update( );

                    auto path = aimgr->get_path( );
                    if ( static_cast< int >( path.size( ) ) > 1 && aimgr->is_moving ) {
                        auto path_end = aimgr->get_path_end_position( );

                        if ( path_end != inst.path_end && object->position != inst.last_path_position ) {
                            inst.path_end           = aimgr->get_path_end_position( );
                            inst.path               = path;
                            inst.next_path_node     = aimgr->next_path_node;
                            inst.last_path_position = object->position;

#if DEBUG
                            std::cout << "[ " << object->champion_name.text << " ]: " << "Path updated in FOW! " << " | is_moving: " << aimgr->is_moving << std::endl;
#endif

                            inst.last_path_update_time = *g_time;
                            inst.is_updated_path       = true;
                        }
                    }
                }
            }

            if ( object->is_dead( ) || object->is_visible( ) ) {
                inst.total_recall_time = 0.f;
                inst.time_in_recall    = 0.f;
                inst.recall_start      = 0.f;
                inst.is_recalling      = false;

                if ( object->is_dead( ) ) {
                    if ( object->team == 100 ) inst.last_position = Vec3( 394.f, 182.f, 462.f );
                    else inst.last_position                       = Vec3( 14310.f, 171.f, 14386.f );

                    inst.last_death_time = *g_time;

                } else {

                    if ( g_config->awareness.show_gank_alert->get< bool >( ) && *g_time - inst.last_seen_time > 10.f &&
                        object->dist_to_local( ) <= 3000.f ) {
                        std::cout << "[ Gank alert ] " << object->champion_name.text << " | distance:  " << object->
                            dist_to_local( ) << std::endl;
                        add_gank( inst.index );
                    }

                    if ( *g_time - inst.last_seen_time > 1.f ) {
                        inst.last_fow_duration = *g_time - inst.last_seen_time;
                        inst.vision_gain_time  = *g_time;
                    }

                    // update path
                    auto aimgr = object->get_ai_manager( );
                    if ( aimgr ) {
                        aimgr.update( );

                        auto path = aimgr->get_path( );
                        if ( path.size( ) > 1 && aimgr->is_moving ) {
                            auto path_end = aimgr->get_path_end_position( );
                            if ( path_end != inst.path_end ) {
                                inst.path_end           = aimgr->get_path_end_position( );
                                inst.path               = path;
                                inst.next_path_node     = aimgr->next_path_node;
                                inst.last_path_position = object->position;
                            } else {
                                inst.next_path_node     = aimgr->next_path_node;
                                inst.last_path_position = object->position;
                            }

                            inst.last_path_update_time = *g_time;
                            inst.is_updated_path       = true;
                        } else inst.is_updated_path = false;
                    } else inst.is_updated_path = false;

                    inst.last_visible_time     = *g_time;
                    inst.last_visible_position = object->position;
                }

                inst.last_seen_time = *g_time;

                continue;
            }


            auto       recall    = get_recall( inst.index );
            const auto in_recall = recall.has_value( ) && !recall.value( ).canceled && !recall.value( ).finished;

            if ( in_recall ) {
                if ( !inst.is_recalling || inst.recall_start <= 0.f ) {
                    inst.recall_start = *g_time;
                    inst.is_recalling = true;
                } else inst.time_in_recall = *g_time - inst.recall_start;
            } else if ( inst.is_recalling ) {
                inst.total_recall_time += inst.time_in_recall;
                inst.time_in_recall = 0.f;
                inst.recall_start   = 0.f;
                inst.is_recalling   = false;
            }
        }
    }

    auto Tracker::draw_last_seen_position( ) -> void{
        if ( !g_config->awareness.last_seen_position->get< bool >( ) ) return;

        for ( auto& inst : m_last_seen_data ) {
            auto& object = g_entity_list.get_by_index( inst.index );
            if ( !object || !object.is_valid( ) ) continue;

            object.update( );
            Vec2 sp;

            if ( object->is_invisible( ) && object->is_alive( ) ) {
                if ( object->position != inst.last_raw_position ) {
                    inst.last_position     = object->position;
                    inst.last_raw_position = object->position;
                    inst.last_seen_time    = *g_time;
                }

                if ( inst.is_updated_path && inst.last_position.dist_to( inst.last_path_position ) >= 75.f )
                    inst.
                        is_updated_path = false;


                /* if (object->position != inst.last_raw_position) {
                     inst.last_position = object->position;
                     inst.last_raw_position = object->position;
                     inst.last_seen_time = *g_time;
                 }
 
                 if ( inst.is_updated_path && inst.last_position.dist_to( inst.last_path_position ) >= 25.f )
                     inst.is_updated_path = false;
 
                 auto aimgr = object->get_ai_manager( );
                 if ( aimgr ) {
                     auto path = aimgr->get_path( );
                     if ( static_cast<int>(path.size( )) > 1 && aimgr->is_moving ) {
                         auto path_end = path[ path.size( ) - 1 ];
                         if ( path_end != inst.path_end && object->position != inst.last_path_position ) {
                             inst.path_end = path[ path.size( ) - 1 ];
                             inst.path = path;
                             inst.next_path_node = aimgr->next_path_node;
                             inst.last_path_position = object->position;
 
 #if DEBUG
                             std::cout << "[ " << object->champion_name.text << " ]: " << "Path updated in FOW! " << " | is_moving: " << aimgr->is_moving << std::endl;
 #endif
 
                             inst.last_path_update_time = *g_time;
                             inst.is_updated_path = true;
                         }
                     }
                 }*/


                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.
                    glow_enabled ) {
                    if ( g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow( object->network_id, D3DCOLOR_ARGB( 255, 255, 0, 0 ), 0, 3, 2 );
                        inst.glow_enabled = true;
                    }
                }
            }

            if ( object->is_dead( ) || object->is_visible( ) ) {
                /*inst.total_recall_time = 0.f;
                inst.time_in_recall = 0.f;
                inst.recall_start = 0.f;
                inst.is_recalling = false;

                if ( object->is_dead( ) ) {
                    if ( object->team == 100 ) inst.last_position = vec3( 394.f, 182.f, 462.f );
                    else inst.last_position = vec3( 14310.f, 171.f, 14386.f );
                } else {
                    // update path
                    auto aimgr = object->get_ai_manager( );
                    if ( aimgr ) {
                        auto path = aimgr->get_path( );
                        if ( path.size( ) > 1 && aimgr->is_moving ) {
                            auto path_end = path[ path.size( ) - 1 ];
                            if ( path_end != inst.path_end ) {
                                inst.path_end = path[ path.size( ) - 1 ];
                                inst.path = path;
                                inst.next_path_node = aimgr->next_path_node;
                                inst.last_path_position = object->position;
                            } else {
                                inst.next_path_node = aimgr->next_path_node;
                                inst.last_path_position = object->position;
                            }

                            inst.last_path_update_time = *g_time;
                            inst.is_updated_path = true;
                        } else inst.is_updated_path = false;
                    } else inst.is_updated_path = false;

                    inst.last_visible_time = *g_time;
                    inst.last_visible_position = object->position;
                }

                inst.last_seen_time = *g_time;*/

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && inst.glow_enabled ) {
                    if ( g_function_caller->is_glow_queueable( ) ) {
                        g_function_caller->enable_glow(
                            object->network_id,
                            D3DCOLOR_ARGB( 255, 255, 0, 0 ),
                            0,
                            3,
                            2,
                            true
                        );
                        inst.glow_enabled = false;
                    }
                }


                continue;
            }


            auto world_scale = g_config->awareness.last_seen_scale->get< int >( ) / 100.f;
            auto map_scale   = g_config->awareness.last_seen_minimap_scale->get< int >( ) / 100.f;

            const auto world_text_size{ 26.f * world_scale };
            const auto minimap_text_size{ 16.f * map_scale };

            const Vec2 indicator_size{ 27.f * map_scale, 27.f * map_scale };
            const Vec2 world_indicator_size{ 50.f * world_scale, 50.f * world_scale };
            const auto recall_color = Color( 32, 136, 247, 255 );
            const auto health_color = Color( 10, 200, 10, 255 );

            const Vec2 world_texture_outline_size{ 52.f * world_scale, 52.f * world_scale };

            auto       recall    = get_recall( inst.index );
            const auto in_recall = recall.has_value( ) && !recall.value( ).canceled && !recall.value( ).finished;

            if ( in_recall ) {
                if ( !inst.is_recalling || inst.recall_start <= 0.f ) {
                    inst.recall_start = recall->start_time;
                    inst.is_recalling = true;
                } else inst.time_in_recall = *g_time - inst.recall_start;
            } else if ( inst.is_recalling ) {
                inst.total_recall_time += inst.time_in_recall;
                inst.time_in_recall = 0.f;
                inst.recall_start   = 0.f;
                inst.is_recalling   = false;
            }

            std::string champ_name        = object->champion_name.text;
            const auto  time_reduction    = inst.total_recall_time + inst.time_in_recall;
            const auto  time_missing      = *g_time - inst.last_seen_time - time_reduction;
            auto        missing_threshold = 0.5f;

            const auto size = g_render->get_text_size(
                fmt::format( "{}", static_cast< int >( time_missing ) ),
                g_fonts->get_zabel( ),
                minimap_text_size
            );
            const auto w_size = g_render->get_text_size(
                fmt::format( "{}", static_cast< int >( time_missing ) ),
                g_fonts->get_zabel( ),
                world_text_size
            );

            auto texture = g_render->load_texture_from_file(
                path::join(
                    { directory_manager::get_resources_path( ), "champions", champ_name, champ_name + "_square.png" }
                ).value( )
            );

            auto screen_position = inst.last_position;
            if ( g_config->awareness.last_seen_simulate_position->get< bool >( ) && inst.is_updated_path && object->
                is_invisible( ) &&
                !object->is_dead( ) ) {
                screen_position = get_path_position(
                    inst.last_position,
                    inst.path,
                    inst.next_path_node,
                    object->movement_speed * time_missing
                );
            }

            const auto on_screen = sdk::math::world_to_screen( screen_position, sp );
            const auto minimap   = sdk::math::world_to_minimap( inst.last_position );

            if ( texture ) {
                // draw minimap texture
                g_render->image(
                    { minimap.x - indicator_size.x / 2, minimap.y - indicator_size.y / 2 },
                    indicator_size,
                    texture
                );

                // draw world texture
                //if (on_screen) g_render->image({ sp.x - world_indicator_size.x / 2, sp.y - world_indicator_size.y / 2 }, world_indicator_size, texture);
            }

            if ( on_screen ) {
                // draw red box / time missing on world
                if ( time_missing >= missing_threshold ) {
                    if ( texture )
                        g_render->image(
                            { sp.x - world_indicator_size.x / 2, sp.y - world_indicator_size.y / 2 },
                            world_indicator_size,
                            texture
                        );

                    g_render->filled_box(
                        { sp.x - world_indicator_size.x / 2, sp.y - world_indicator_size.y / 2 },
                        world_indicator_size,
                        Color( 0, 0, 0, 90 )
                    );
                } else {
                    bool rendered{ };

                    if ( in_recall ) {
                        g_render->box(
                            { sp.x - world_texture_outline_size.x / 2, sp.y - world_texture_outline_size.y / 2 },
                            world_texture_outline_size,
                            recall_color,
                            -1.f,
                            3.f
                        );

                        if ( !recall->finished && !recall->canceled ) {
                            auto modifier = 1.f - ( recall->finish_time - *g_time ) / recall->channel_duration;
                            if ( modifier >= 1 ) modifier = 1;

                            auto thick_outline_width = world_texture_outline_size.x + 1.f;

                            auto outline_base = Vec2(
                                sp.x - thick_outline_width / 2,
                                sp.y + world_texture_outline_size.y / 2
                            );
                            auto outline_size = Vec2(
                                thick_outline_width + 1.f,
                                -( world_texture_outline_size.y * modifier )
                            );

                            g_render->box( outline_base, outline_size, Color::white( ), -1.f, 3.f );
                        }
                    } else {
                        if ( texture )
                            g_render->image(
                                { sp.x - world_indicator_size.x / 2, sp.y - world_indicator_size.y / 2 },
                                world_indicator_size,
                                texture
                            );
                        rendered = true;

                        g_render->box(
                            { sp.x - world_texture_outline_size.x / 2, sp.y - world_texture_outline_size.y / 2 },
                            world_texture_outline_size,
                            Color( 255, 20, 20, 255 ),
                            -1.f,
                            3.f
                        );
                    }

                    if ( !rendered && texture )
                        g_render->image(
                            { sp.x - world_indicator_size.x / 2, sp.y - world_indicator_size.y / 2 },
                            world_indicator_size,
                            texture
                        );
                }

                // draw hp on worldd
                auto bar_width = 8.f;
                auto spacing   = 1.f;
                Vec2 hp_start  = { sp.x - world_indicator_size.x / 2.f, sp.y + world_indicator_size.x / 2.f };

                hp_start.y -= bar_width + spacing;
                hp_start.x += spacing;

                auto modifier   = object->health / object->max_health;
                auto bar_length = world_indicator_size.x * modifier - 2.f;
                if ( bar_length < 3.f ) bar_length = 3.f;

                Vec2 bar_size = { bar_length, bar_width };

                //245, 78, 66
                g_render->filled_box(
                    hp_start,
                    { world_indicator_size.x * modifier - 2.f, bar_width },
                    Color( 10, 10, 10, 90 )
                );
                g_render->filled_box( hp_start, bar_size, health_color );

                /// emboss effect
                g_render->line(
                    { hp_start.x - 1.f, hp_start.y + bar_width - 1.f },
                    { hp_start.x + bar_length + 1.f, hp_start.y + bar_width - 1.f },
                    Color( 0, 0, 0, 110 ),
                    3.f
                );
            }

            // draw red box / time missing on map
            if ( time_missing >= missing_threshold )
                g_render->filled_box(
                    { minimap.x - indicator_size.x / 2, minimap.y - indicator_size.y / 2 },
                    indicator_size,
                    Color( 0, 0, 0, 70 )
                );
            else
                g_render->box(
                    { minimap.x - indicator_size.x / 2, minimap.y - indicator_size.y / 2 },
                    indicator_size,
                    in_recall ? Color( 32, 136, 247, 255 ) : Color( 255, 20, 20, 255 ),
                    -1.f,
                    in_recall ? 2.f : 1.f
                );

            auto modifier   = object->health / object->max_health;
            auto bar_width  = 6.f * map_scale;
            auto bar_length = indicator_size.x * modifier - 2.f;
            if ( bar_length < 3.f ) bar_length = 3.f;

            Vec2 hp_start = { minimap.x - indicator_size.x / 2.f, minimap.y + indicator_size.y / 2.f };
            hp_start.y -= bar_width + 1.f;
            hp_start.x += 1.f;

            Vec2 bar_size = { bar_length, bar_width };

            g_render->filled_box( hp_start, bar_size, Color( 20, 20, 20, 90 ) );
            g_render->filled_box( hp_start, bar_size, health_color );

            // emboss effect
            g_render->line(
                { hp_start.x, hp_start.y + bar_width - 1.f },
                { hp_start.x + bar_size.x, hp_start.y + bar_width - 1.f },
                Color( 0, 0, 0, 100 ),
                bar_width / 3.f
            );

            if ( inst.is_updated_path && !inst.path.empty( ) && *g_time - inst.last_path_update_time <= 40.f ) {
                Vec3 start{ };
                Vec3 end{ };

                auto possible_position = get_path_position(
                    inst.last_position,
                    inst.path,
                    inst.next_path_node,
                    object->movement_speed * time_missing
                );

                auto path_start = inst.path[ 0 ];
                auto path_end   = inst.path[ inst.path.size( ) - 1 ];

                auto draw_world_path = possible_position.dist_to( path_end ) > 10.f;
                auto fade_modifier   = std::clamp(
                    possible_position.dist_to( path_end ) / path_start.dist_to( path_end ),
                    0.f,
                    1.f
                );
                fade_modifier = utils::ease::ease_in_quint( fade_modifier );

                auto time_left = possible_position.dist_to( path_end ) / object->movement_speed;


                Vec2 start_sp{ };
                Vec2 end_sp{ };

                if ( draw_world_path && ( world_to_screen( path_start, start_sp ) ||
                    world_to_screen( path_end, end_sp ) ) ) {
                    if ( inst.next_path_node < 0 ) return;

                    for ( auto i = inst.next_path_node; i < static_cast< int >( inst.path.size( ) ); i++ ) {
                        start = i == inst.next_path_node ? inst.last_position : inst.path[ i - 1 ];
                        end   = inst.path[ i ];

                        if ( !world_to_screen( start, start_sp ) && !world_to_screen( end, end_sp ) ) continue;
                        auto thickness = time_left > 1.5f ? 2.f : 7.f - 5.f * ( time_left / 1.5f );
                        int  opacity   = time_left > 1.5f ? 255 : 255 * ( time_left / 1.5f );

                        g_render->line_3d( start, end, Color( 255, 255, 255, opacity ), thickness );
                    }

                    if ( draw_world_path && world_to_screen( path_end, start_sp ) ) {
                        int opacity = time_left > 1.5f ? 255 : 255 * ( time_left / 1.5f );

                        g_render->filled_circle( start_sp, Color( 255, 255, 255, opacity ), 3.f, 15 );

                        std::string text      = object->champion_name.text;
                        const auto  text_size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                        std::string caution_text = _( "MISSING" );
                        const auto  caution_size = g_render->get_text_size(
                            caution_text,
                            g_fonts->get_zabel_12px( ),
                            12
                        );

                        Vec2 base_position{ start_sp.x - text_size.x / 2.f, start_sp.y };
                        g_render->text_shadow(
                            base_position,
                            Color( 255, 255, 255, opacity ),
                            g_fonts->get_zabel_16px( ),
                            text.c_str( ),
                            16
                        );
                        g_render->text_shadow(
                            { start_sp.x - caution_size.x / 2.f, start_sp.y + 16.f },
                            Color( 255, 25, 25, opacity ),
                            g_fonts->get_zabel_12px( ),
                            caution_text.c_str( ),
                            12
                        );
                    }
                }

                for ( auto i = inst.next_path_node; i < static_cast< int >( inst.path.size( ) ); i++ ) {
                    start = i == inst.next_path_node ? inst.last_position : inst.path[ i - 1 ];
                    end   = inst.path[ i ];

                    auto map_start = world_to_minimap( start );
                    auto map_end   = world_to_minimap( end );

                    g_render->line( map_start, map_end, Color( 255, 255, 255 ), 2.f );
                }
            }

            if ( time_missing >= missing_threshold ) {
                const auto units_moved   = time_missing * object->movement_speed;
                auto       circle_radius = units_moved;

                auto    draw_color = Color( 255, 48, 62, 255 );
                int32_t draw_flags = Renderer::outline;
                bool    in_animation{ };

                if ( time_missing <= missing_threshold * 4.f ) {
                    auto mod = ( time_missing - missing_threshold ) / ( missing_threshold * 3.f );
                    mod      = utils::ease::ease_out_expo( mod );

                    circle_radius = time_missing * object->movement_speed * mod;

                    draw_color.a = 225 - 225 * mod;
                    draw_flags   = Renderer::outline | Renderer::filled;
                    in_animation = true;
                }

                // draw possible movement circle on minimap
                if ( *g_time - inst.last_seen_time <= 40.f && g_config->awareness.last_seen_circle_mode->get<
                    int32_t >( ) != 2 )
                    g_render->circle_minimap(
                        inst.last_position,
                        in_recall ? Color( 255, 255, 0, 255 ) : Color::red( ),
                        circle_radius,
                        100,
                        1.f
                    );

                if ( g_config->awareness.last_seen_circle_mode->get< int32_t >( ) == 0 && circle_radius < 2500 ) {
                    // draw possible movement circle on world
                    const auto circle_alpha = static_cast< int32_t >( circle_radius > 1500
                                                                          ? 155 - 155 * ( ( circle_radius - 1500 ) /
                                                                              1000 )
                                                                          : 155 );
                    g_render->circle_3d(
                        inst.last_position,
                        in_animation ? draw_color : Color( 255, 48, 62, circle_alpha ),
                        circle_radius,
                        draw_flags,
                        75,
                        2.f
                    );
                }

                g_render->text_shadow(
                    { minimap.x - size.x / 2, minimap.y - size.y / 2 },
                    Color( 255, 255, 255, 255 ),
                    g_fonts->get_zabel( ),
                    fmt::format( "{}", static_cast< int >( time_missing ) ).c_str( ),
                    minimap_text_size
                );

                if ( on_screen ) {
                    g_render->text_shadow(
                        { sp.x - w_size.x / 2, sp.y - w_size.y / 2 },
                        Color( 255, 255, 255, 255 ),
                        g_fonts->get_zabel( ),
                        fmt::format( "{}", static_cast< int >( time_missing ) ).c_str( ),
                        world_text_size
                    );
                }
            }
        }
    }

    auto Tracker::update_dashes( ) -> void{
        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) ) continue;

            auto aimgr = enemy->get_ai_manager( );
            if ( !aimgr || !aimgr->is_moving || !aimgr->is_dashing ) continue;

            auto path = aimgr->get_path( );
            if ( path.size( ) != 2 ) continue;

            auto start = path[ 0 ];
            auto end   = path[ path.size( ) - 1 ];
            if ( is_dash_active( enemy->index, start ) ) continue;

            DashInstance inst{ };

            inst.type           = EDashType::shaco_q;
            inst.start_time     = *g_time;
            inst.end_time       = *g_time + start.dist_to( end ) / aimgr->dash_speed;
            inst.index          = enemy->index;
            inst.start_position = start;
            inst.end_position   = end;
            inst.champion_name  = enemy->champion_name.text;

            m_dashes.push_back( inst );
        }

        for ( auto& inst : m_dashes ) {
            if ( *g_time >= inst.end_time + 0.75f ) {
                remove_dash( inst.index, inst.type, inst.start_time );
                continue;
            }

            if ( !inst.can_update_position || inst.end_time > *g_time ) continue;

            const auto& unit = g_entity_list.get_by_index( inst.index );
            if ( !unit || unit->is_dead( ) || unit->is_visible( ) ) continue;

            auto index = get_last_seen_index( inst.index );
            if ( !index ) continue;

            if ( m_last_seen_data[ *index ].last_position.dist_to( inst.end_position ) <= 10.f ) continue;

            m_last_seen_data[ *index ].last_position  = inst.end_position;
            m_last_seen_data[ *index ].last_seen_time = *g_time;
            inst.can_update_position                  = false;

            std::cout << "[ DASH TRACK ] Updated " << inst.champion_name << " position due to dash end\n";
        }
    }

    auto Tracker::draw_dashes( ) const -> void{
        constexpr auto fade_duration{ 0.75f };

        for ( auto& inst : m_dashes ) {
            const auto time_left = *g_time - inst.end_time;
            auto       opacity{ 70 };
            auto       opacity_modifier = std::clamp( time_left / fade_duration, 0.f, 1.f );
            opacity_modifier            = utils::ease::ease_out_quart( opacity_modifier );

            if ( inst.end_time > *g_time ) continue;


            //g_render->line_3d( inst.start_position, inst.end_position, color( 255, 255, 255, opacity ), 5.f );
            //g_render->line_3d( draw_start, inst.end_position, color( 255, 255, 255, opacity ), 3.f );

            if ( time_left <= fade_duration )
                g_render->circle_3d(
                    inst.end_position,
                    Color( 255.f, 255.f, 255.f, 125.f - 125.f * opacity_modifier ),
                    250.f * opacity_modifier,
                    Renderer::filled,
                    48,
                    2.f
                );
        }
    }

    auto Tracker::update_turrets( ) -> void{
        if ( !g_config->awareness.show_enemy_turret_range->get< bool >( ) ) return;

        for ( const auto obj : g_entity_list.get_enemy_turrets( ) ) {
            if ( !obj || obj->is_dead( ) || obj->team == 300 || obj->get_name( ).find( "Turret" ) == std::string::npos
                || is_enemy_turret_tracked( obj->index ) )
                continue;

            auto turret = g_entity_list.get_by_index( obj->index );
            if ( !turret ) continue;

            turret.update( );

            if ( turret->is_dead( ) || turret->team == g_local->team || turret->team == 300
                || turret->get_name( ).find( "Turret" ) == std::string::npos )
                continue;

            m_enemy_turrets.push_back( turret->index );
        }
    }

    auto Tracker::draw_turret_ranges( ) -> void{
        return;
        if ( !g_config->awareness.show_enemy_turret_range->get< bool >( ) ) return;

        for ( auto index : m_enemy_turrets ) {
            auto turret = g_entity_list.get_by_index( index );
            if ( !turret ) continue;
            turret.update( );

            if ( turret->is_dead( ) ||
                turret->get_name( ).find( _( "Turret" ) ) == std::string::npos ||
                turret->team == g_local->team ||
                turret->team == 300
            ) {
                remove_turret( index );
                continue;
            }

            if ( g_local->position.dist_to( turret->position ) > 2000.f ) continue;

            Color range_color = Color( 255, 255, 0, 175 );
            bool  attacking_self{ };
            auto  sci = turret->spell_book.get_spell_cast_info( );
            if ( sci ) {
                if ( sci->get_target_index( ) == g_local->index ) {
                    range_color    = Color( 255, 50, 50, 50 );
                    attacking_self = true;
                } else range_color = Color( 0, 255, 155, 175 );
            }

            if ( attacking_self ) {
                g_render->circle_3d(
                    turret->position,
                    range_color,
                    900.f,
                    Renderer::outline | Renderer::filled,
                    60,
                    5.f
                );
                continue;
            }

            g_render->circle_3d(
                turret->position,
                range_color,
                900.f,
                2,
                60,
                3.f
            );
        }
    }

    auto Tracker::draw_paths( ) const -> void{
        if ( !g_config->awareness.show_enemy_paths->get< bool >( ) ) return;

        for ( const auto index : m_enemies ) {
            const auto& enemy = g_entity_list.get_by_index( index );
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( )
            )
                continue;

            auto aimgr = enemy->get_ai_manager( );
            auto path  = aimgr->get_path( );

            if ( aimgr->is_moving && path.size( ) > 1 && aimgr->next_path_node != 0 && path.size( ) > aimgr->
                next_path_node ) {
                Vec3 start{ };
                Vec3 end{ };
                Vec2 sp{ };

                auto modifier = std::clamp(
                    g_features->prediction->get_time_on_current_path( enemy->index ) / 0.2f,
                    0.f,
                    1.f
                );
                modifier = utils::ease::ease_out_expo( modifier );

                const auto bounding = enemy->get_bounding_radius( );
                const int  opacity  = 100 + 155 * modifier;

                const auto thickness = 5.f - 2.f * modifier;
                float      length{ };

                for ( auto i = aimgr->next_path_node; i < static_cast< int >( path.size( ) ); i++ ) {
                    end   = path[ i ];
                    start = i == aimgr->next_path_node ? enemy->position.extend( end, bounding ) : path[ i - 1 ];

                    length += start.dist_to( end );

                    if ( i == aimgr->next_path_node && enemy->position.dist_to( end ) < bounding ||
                        !world_to_screen( start, sp ) && !world_to_screen( end, sp ) )
                        continue;


                    g_render->line_3d( start, end, Color( 255, 255, 255, opacity ), thickness );
                }

                if ( sdk::math::world_to_screen( path[ path.size( ) - 1 ], sp ) ) {
                    g_render->filled_circle( sp, Color( 255, 255, 255, opacity ), 5.f - 2.f * modifier, 32 );

                    const auto path_duration = length / enemy->movement_speed;
                    auto       duration_text = std::to_string( path_duration );
                    duration_text.resize( 4 );


                    const int   size      = 26 - 10 * modifier; // 87, 112, 201
                    std::string text      = enemy->champion_name.text;
                    const auto  text_size = g_render->get_text_size(
                        text,
                        size == 16 ? g_fonts->get_zabel_16px( ) : g_fonts->get_zabel( ),
                        size
                    );
                    const auto duration_text_size = g_render->get_text_size(
                        duration_text,
                        g_fonts->get_zabel_12px( ),
                        12
                    );

                    Vec2 background_start =
                        { sp.x - text_size.x / 2.f, sp.y + text_size.y }; //{ sp.x - text_size.x / 2.f, sp.y };
                    Vec2 background_size =
                    {
                        text_size.x,
                        duration_text_size.y + 2.f
                    }; //{ text_size.x, text_size.y + duration_text_size.y };

                    //g_render->filled_box( background_start, background_size, color( 0, 0, 0, 155 ), -1 );
                    //g_render->box( background_start, background_size, color( 15, 60, 252, 255 ), -1 );

                    const Vec2 base_position{ sp.x - text_size.x / 2.f, sp.y };
                    g_render->text_shadow(
                        base_position,
                        Color( 20.f, 140.f, 255.f, 200.f + 55.f * modifier ),
                        size == 16 ? g_fonts->get_zabel_16px( ) : g_fonts->get_zabel( ),
                        text.c_str( ),
                        size
                    );

                    g_render->text_shadow(
                        { sp.x - duration_text_size.x / 2.f, sp.y + text_size.y - 2.f },
                        Color( 255.f, 255.f, 255.f, 255.f ),
                        g_fonts->get_zabel_12px( ),
                        duration_text.c_str( ),
                        12
                    );
                }
            }
        }
    }

    auto Tracker::draw_attack_ranges( ) const -> void{
        if ( g_config->visuals.enemy_attack_range->get< int >( ) == 0 ) return;

        for ( const auto index : m_enemies ) {
            auto enemy = g_entity_list.get_by_index( index );
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || enemy->dist_to_local( ) > 1200.f
            )
                continue;

            enemy.update( );
            // const auto enemy = obj->create_updated_copy( );

            /* auto entity_info = enemy->get_entity_info( );
            if ( entity_info ) {

                g_render->circle_3d( entity_info->get_cast_position( ),
                                     Color( 255, 50, 50, 75 ),
                                     50.f,
                                     Renderer::outline | Renderer::filled,
                                     64,
                                     2.f );

                g_render->line_3d( enemy->position, entity_info->get_cast_position( ), Color( 255, 50, 50 ), 4.f );

                Vec3 extend = enemy->position + enemy->get_direction( );
                Vec3 adjusted = enemy->position.extend( extend, 300.f );

                g_render->line_3d( enemy->position, adjusted, Color::white( ), 3.f );

               // std::cout << "Drawing entity info position | addy" << std::hex << entity_info.get_address( )
               //           << std::endl;
            }*/

            const auto bounding       = enemy->get_bounding_radius( );
            const auto local_bounding = g_features->orbwalker->get_bounding_radius( );
            const auto in_enemy_range = g_local->position.dist_to( enemy->position ) <= enemy->attack_range + bounding +
                local_bounding ||
                g_config->visuals.enemy_attack_range->get< int >( ) == 2;

            if ( in_enemy_range ) {
                g_render->circle_3d(
                    enemy->position,
                    g_config->visuals.enemy_attack_range_color->get< Color >( ),
                    enemy->attack_range + enemy->get_bounding_radius( ) + local_bounding,
                    get_draw_style( g_config->visuals.enemy_attack_range_draw_style->get< int32_t >( ) ),
                    45,
                    3.f
                );
            } else {
                const auto min = enemy->attack_range + bounding + g_features->orbwalker->get_bounding_radius( );
                const auto max = min + 400.f;

                const auto distance = enemy->dist_to_local( );
                if ( distance >= max ) continue;

                const auto modifier = std::clamp( 1.f - ( distance - min ) / 400.f, 0.f, 1.f );

                const auto angle        = 90.f * modifier;
                const auto rotate_angle = angle / 2.f * ( 3.14159265359f / 180.f );

                auto draw_color = g_config->visuals.enemy_attack_range_color->get< Color >( );

                g_render->circle_3d(
                    enemy->position,
                    draw_color.alpha( 100 + 155 * modifier ),
                    enemy->attack_range + enemy->get_bounding_radius( ) + local_bounding,
                    get_draw_style( g_config->visuals.enemy_attack_range_draw_style->get< int32_t >( ) ),
                    45,
                    3.f,
                    angle,
                    ( g_local->position - enemy->position ).rotated( -rotate_angle )
                );
            }
        }
    }

    auto Tracker::draw_spell_tracker( ) const -> void{
        if ( !g_config->awareness.show_spell_cooldowns->get< bool >( ) ) return;

        for ( const auto inst : m_hero_spells ) { draw_spells( inst.index ); }
    }

    auto Tracker::draw_clones( ) const -> void{
        if ( !g_config->awareness.show_clones->get< bool >( ) ) return;

        Vec2       sp{ };
        const auto clones = m_clones;
        for ( const auto index : clones ) {
            auto& clone = g_entity_list.get_by_index( index );
            if ( !clone ) continue;

            clone.update( );

            if ( clone->is_invisible( ) || !sdk::math::world_to_screen( clone->position, sp ) ) continue;

            const auto texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "common",
                        "clone.png"
                    }
                ).value( )
            );
            if ( !texture ) {
                g_render->text_shadow(
                    sp,
                    g_config->awareness.show_clones_color->get< Color >( ),
                    g_fonts->get_bold( ),
                    "CLONE",
                    32
                );
                continue;
            }

            const Vec2 draw_size        = { 200.f * 0.8f, 75.f * 0.8f };
            const Vec2 texture_position = { sp.x - draw_size.x / 2.f, sp.y - draw_size.y };

            g_render->image( texture_position, draw_size, texture );

            if ( g_config->awareness.internal_glow_toggle->get< bool >( ) && g_function_caller->is_glow_queueable( ) )
                g_function_caller->enable_glow(
                    clone->network_id,
                    D3DCOLOR_ARGB( 255, 42, 255, 5 )
                );

            //g_render->text_shadow( sp, g_config->awareness.show_clones_color->get< color >( ), g_fonts->get_bold( ), "CLONE", 32 );
        }
    }

    auto Tracker::draw_spells( int16_t index ) const -> void{
        auto unit = g_entity_list.get_by_index( index );
        if ( !unit ) return;
        unit.update( );

        if ( !unit || unit->is_dead( ) || unit->is_invisible( ) ) return;

        auto book = get_hero_spells( unit->index );
        if ( !book || !book->valid ) return;

        Vec2 sp;
        if ( !world_to_screen( unit->position, sp ) ) return;

        auto base_position = unit->get_hpbar_position( );
        auto bar_length    = static_cast< float >( g_render_manager->get_width( ) ) * 0.0546875f;
        auto bar_height    = static_cast< float >( g_render_manager->get_height( ) ) * 0.022222222f;

        base_position.x -= bar_length * 0.425f;
        base_position.y -= static_cast< float >( g_render_manager->get_height( ) ) * 0.003f;

        auto small_bar_height = bar_height * 0.9f;
        Vec2 gold_base{ base_position.x + bar_length + 4.f, base_position.y - bar_height * 0.95f + small_bar_height };

        if ( false && unit->network_id != g_local->network_id &&
            ( g_config->awareness.show_gold_mode->get< int >( ) == 2 ||
                g_config->awareness.show_gold_mode->get< int >( ) == 1 && !unit->is_ally( ) ) ) {
            // gold drawing

            Vec2 gold_size{ 25.f, 25.f };

            auto gold_texture = g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "goldicon.png" } ).value( )
            );
            if ( gold_texture ) {
                g_render->image( gold_base, gold_size, gold_texture );

                auto gold_text_position = gold_base + Vec2( 25.f, 12.5f );

                auto       text      = std::to_string( static_cast< int >( unit->get_current_gold( ) ) );
                const auto text_size = g_render->get_text_size( text, g_fonts->get_zabel_16px( ), 16 );

                //gold_text_position.x -= text_size.x / 3.f;
                gold_text_position.y -= text_size.y / 2.f;

                g_render->text_shadow(
                    gold_text_position,
                    Color( 255, 200, 0 ),
                    g_fonts->get_zabel_16px( ),
                    text.data( ),
                    16
                );
            }
        }

        switch ( g_config->awareness.show_spells_mode->get< int >( ) ) {
        case 0:
            if ( !unit->is_enemy( ) ) return;
            break;
        case 1:
            if ( unit->network_id == g_local->network_id ) return;
            break;
        default:
            break;
        }

        auto pixels_per_spell = bar_length * 0.255f;
        auto spell_width      = g_render_manager->get_height( ) * 0.0065f;
        bool yuumi_draw_mode{ };
        bool ammo_draw_mode{ };

        const auto name_hash = rt_hash( unit->champion_name.text );

        switch ( name_hash ) {
        case ct_hash( "Annie" ):
        case ct_hash( "Aphelios" ):
        case ct_hash( "Corki" ):
        case ct_hash( "Graves" ):
        case ct_hash( "Jhin" ):
        case ct_hash( "Seraphine" ):
        case ct_hash( "Vex" ):
        case ct_hash( "Zeri" ):
            ammo_draw_mode = true;
            break;
        case ct_hash( "Yuumi" ):
        {
            auto spell = unit->spell_book.get_spell_slot( ESpellSlot::w );
            if ( spell && rt_hash( spell->get_name().c_str() ) != ct_hash( "YuumiW" ) ) yuumi_draw_mode = true;

            break;
        }
        default:
            break;
        }


        auto cooldown_color = Color( 255, 128, 43 );
        auto ready_color    = Color( 165, 214, 101 );
        auto mana_color     = Color( 89, 153, 255 );
        auto yuumi_color    = Color( 88, 46, 255 );

        auto fade_time{ 0.5f };

        SpellSlotInfo spell{ };
        Vec2          spell_position;

        for ( auto i = 0; i < 4; i++ ) {
            switch ( i ) {
            case 0:
                spell = book->spell_q;
                break;
            case 1:
                spell = book->spell_w;
                break;
            case 2:
                spell = book->spell_e;
                break;
            case 3:
                spell = book->spell_r;
                break;
            default:
                break;
            }

            if ( spell.level == 0 ) continue;

            spell_position = { base_position.x + pixels_per_spell * i, base_position.y };

            if ( ammo_draw_mode ) spell_position.y += spell_width * 1.5f;
            else if ( yuumi_draw_mode ) spell_position.y -= bar_height + spell_width;

            g_render->filled_box(
                { spell_position.x, spell_position.y },
                { pixels_per_spell, spell_width },
                Color( 0, 0, 0, 150 )
            );


            if ( spell.cooldown_expire <= *g_time && spell.level > 0 ) {
                auto draw_color       = ready_color;
                auto time_since_ready = *g_time - spell.cooldown_expire;

                if ( time_since_ready <= fade_time / 2.f ) {
                    auto color_modifier = 1.f - time_since_ready / ( fade_time / 2.f );

                    if ( false && unit->mana < spell.mana_cost ) {
                        draw_color = Color(
                            mana_color.r + ( 255 - mana_color.r ) * color_modifier,
                            mana_color.g + ( 255 - mana_color.g ) * color_modifier,
                            mana_color.b + ( 255 - mana_color.b ) * color_modifier
                        );
                    } else {
                        draw_color = Color(
                            ready_color.r + ( 255 - ready_color.r ) * color_modifier,
                            ready_color.g + ( 255 - ready_color.g ) * color_modifier,
                            ready_color.b + ( 255 - ready_color.b ) * color_modifier
                        );
                    }
                } //else if ( unit->mana < spell.mana_cost ) draw_color = mana_color;

                g_render->filled_box( spell_position, { pixels_per_spell, spell_width }, draw_color );

                if ( g_config->awareness.show_spell_level->get< bool >( ) ) {
                    auto margin       = 4.f;
                    auto level_length = pixels_per_spell - margin;
                    auto separator    = std::floor( level_length * 0.2f );

                    for ( auto i = 1; i <= spell.level; i++ ) {
                        Vec2 level_position = {
                            spell_position.x + separator * i,
                            spell_position.y + spell_width / 2.f
                        };
                        g_render->filled_box(
                            { level_position.x, level_position.y - 1.f },
                            { 2.f, 2.f },
                            Color( 0, 0, 0, 200 )
                        );
                    }
                }
            } else {
                auto cooldown_left   = spell.cooldown_expire - *g_time;
                auto modifier        = 1.f - ( spell.cooldown_expire - *g_time ) / spell.cooldown;
                auto progress_length = ( pixels_per_spell - 1.f ) * modifier;

                // animation
                auto draw_color  = cooldown_color;
                auto opacity_mod = 1.f;

                if ( cooldown_left <= fade_time ) {
                    auto color_modifier = 1.f - cooldown_left / fade_time;
                    if ( color_modifier >= 1.f ) color_modifier = 1.f;

                    draw_color = Color(
                        cooldown_color.r + ( 255 - cooldown_color.r ) * color_modifier,
                        cooldown_color.g + ( 255 - cooldown_color.g ) * color_modifier,
                        cooldown_color.b + ( 255 - cooldown_color.b ) * color_modifier
                    );

                    opacity_mod = cooldown_left / fade_time;
                    if ( opacity_mod <= 0.f ) opacity_mod = 0.f;
                }

                g_render->line(
                    Vec2( spell_position.x + progress_length, spell_position.y - 1.f ),
                    Vec2( spell_position.x + progress_length, spell_position.y + spell_width ),
                    Color( draw_color.r, draw_color.g, draw_color.b, 155 ),
                    2.f
                );
                g_render->filled_box( spell_position, { progress_length, spell_width }, draw_color );

                std::string timer_text;
                if ( cooldown_left >= 0.9f )
                    timer_text = std::to_string(
                        static_cast< int >( std::floor( cooldown_left + 0.1f ) )
                    );
                else {
                    timer_text = std::to_string( cooldown_left + 0.1f );
                    timer_text.resize( 3 );
                }

                const auto text_size = g_render->get_text_size( timer_text, g_fonts->get_zabel( ), 16 );
                Vec2 text_position = { spell_position.x + pixels_per_spell / 2.f, spell_position.y + spell_width };
                if ( yuumi_draw_mode ) text_position.y = spell_position.y - spell_width - text_size.y / 2.f;

                g_render->text_shadow(
                    { text_position.x - text_size.x / 2.f, text_position.y },
                    Color( 255.f, 255.f, 255.f, 255.f * opacity_mod ),
                    g_fonts->get_zabel( ),
                    timer_text.c_str( ),
                    16
                );
            }

            g_render->box(
                { spell_position.x, spell_position.y },
                { pixels_per_spell, spell_width },
                Color( 0, 0, 0, 125 ),
                0,
                1
            );
            g_render->box(
                { spell_position.x - 1.f, spell_position.y - 1.f },
                { pixels_per_spell + 2.f, spell_width + 2.f },
                yuumi_draw_mode ? yuumi_color : Color( 0, 0, 0, 255 ),
                0,
                1
            );
        }

        constexpr auto font_size          = 16;
        auto           smaller_bar_height = bar_height * 0.9f;

        Vec2 size{ };
        Vec2 summoner_size{ smaller_bar_height, smaller_bar_height };
        Vec2 summoner_base{ base_position.x + bar_length + 4.f, base_position.y -= bar_height * 0.95f };

        if ( yuumi_draw_mode ) summoner_base.y = base_position.y - bar_height;

        //g_render->filled_circle(summoner_base, color::white(), 1.f);
        for ( auto i = 0; i < 2; i++ ) {

            auto summoner_book = get_hero_spells(unit->index);
            if (!summoner_book || !summoner_book->valid) return;

            if (i == 0)spell = summoner_book->summoner_d;
            else  spell = summoner_book->summoner_f;

            auto spellslot = unit->spell_book.get_spell_slot(i == 0 ? ESpellSlot::d : ESpellSlot::f);
            if (!spellslot) continue;

            float cooldown_expire = spell.cooldown_expire;

            if (rt_hash(spellslot->get_name().data()) == ct_hash("SummonerFlash")) {
                cooldown_expire = get_summoner_cooldown(unit->index);
            }

            Vec2 draw_pos = { summoner_base.x + i * ( smaller_bar_height + 2.f ), summoner_base.y };
            if ( !spell.spell_texture ) continue;

            g_render->image( draw_pos, summoner_size, spell.spell_texture );
            g_render->box(
                { draw_pos.x - 1.f, draw_pos.y - 1.f },
                { summoner_size.x + 2.f, summoner_size.y + 2.f },
                yuumi_draw_mode ? yuumi_color : Color( 0, 0, 0, 160 ),
                0,
                2
            );

            if (cooldown_expire > *g_time)
            {
                auto cooldown_value = static_cast<int>(cooldown_expire - *g_time);
                if ( cooldown_value < 0 ) cooldown_value = 0;

                size = g_render->get_text_size( fmt::format( "{}", cooldown_value ), g_fonts->get_bold( ), font_size );

                g_render->filled_box( { draw_pos.x, draw_pos.y }, summoner_size, Color( 10, 10, 10, 150 ) );
                g_render->text_shadow(
                    {
                        draw_pos.x + summoner_size.x / 2.f - size.x / 2.f,
                        draw_pos.y + summoner_size.y / 2.f - size.y / 2.f
                    },
                    Color( 255, 255, 255, 255 ),
                    g_fonts->get_bold( ),
                    fmt::format( "{}", cooldown_value ).c_str( ),
                    font_size
                );
            }
        }
    }

    auto Tracker::on_draw( ) -> void{
        m_mutex.lock( );

        draw_enemy_spell_ranges( );
        draw_ward_tracker( );
        draw_recall_tracker( );
        draw_last_seen_position( );
        draw_clones( );
        draw_spell_tracker( );
        draw_orbwalker_indicator( );
        draw_paths( );
        draw_attack_ranges( );
        draw_local_attack_range( );
        draw_hud( );
        draw_jungle_tracker( );
        //draw_notifications( );
        draw_dashes( );
        draw_experience_tracker( );

        draw_jungle_alerts();
        draw_nearby_enemies();

        //draw_gank_alert( );
        //draw_items( );
        draw_jungler_tracker( );

        draw_turret_ranges( );

       m_mutex.unlock( );


        return;

           for (auto obj : g_entity_list.get_ally_missiles())
        {
            if (!obj || !obj->dist_to_local() > 1000.f) continue;
            auto unit = g_entity_list.get_by_index(obj->index);
            if (!unit) continue;

            unit.update();

            Vec2 sp{};
            if (!world_to_screen(unit->position, sp)) continue;

            auto name = unit->get_alternative_name();

            
            g_render->line_3d(unit->missile_start_position, unit->missile_end_position, Color::red(), 4.f);
            g_render->circle_3d(unit->missile_end_position, Color(255, 255, 0), 25.f);
            g_render->circle_3d(unit->missile_start_position, Color(50, 255, 50), 25.f);

            g_render->circle_3d(unit->position, Color(255, 255, 255), 20.f, 2, -1, 2.f);
            g_render->text_shadow({ sp.x, sp.y - 8.f }, Color(25, 255, 25), g_fonts->get_zabel_12px(), name.c_str(),
                                  12);


            std::cout << name << "[ nid: " << unit->network_id << " ]" 
                      << " | missile address: " << std::hex << unit.get_address()
                      << "  | owner index: " << std::dec << unit->missile_spell_info()->get_spell_data()->get_name()
                      << " | spawntime: " << *g_time - unit->missile_spawn_time() << " | max_range "
                      << unit->missile_start_position.dist_to(unit->missile_end_position) << std::endl;
        }

        return;
        auto enemy = g_features->target_selector->get_default_target();
        if (!enemy) return;

        auto        distance = g_features->evade->get_closest_line_point(g_local->position, g_local->position.extend(g_pw_hud->get_hud_manager()->cursor_position_clipped, 800.f ),
                                enemy->position)
                            .dist_to(enemy->position);
        std::string text     = "dist: " + std::to_string((int)distance);

        Vec2 sp{};
        if (!world_to_screen(g_features->evade->get_closest_line_point(
                                 g_local->position,
                                 g_local->position.extend(g_pw_hud->get_hud_manager()->cursor_position_clipped, 800.f),
                                 enemy->position),
                             sp))
            return;

        g_render->text_shadow(sp, Color::green(), g_fonts->get_bold(), text.data(), 20);

        for (auto obj : g_entity_list.get_ally_missiles())
        {
            if (!obj || !obj->dist_to_local() > 1000.f) continue;
            auto unit = g_entity_list.get_by_index(obj->index);
            if (!unit) continue;

            unit.update();

            Vec2 sp{};
            if (!world_to_screen(unit->position, sp)) continue;

            auto name = unit->get_alternative_name();


            g_render->line_3d(unit->missile_start_position, unit->missile_end_position, Color::red(), 4.f);

            g_render->circle_3d(unit->position, Color(255, 255, 255), 20.f, 2, -1, 2.f);
            g_render->text_shadow({ sp.x, sp.y - 8.f }, Color(25, 255, 25), g_fonts->get_zabel_12px(), name.c_str(),
                                  12);


          
                std::cout << name << " | missile address: " << std::hex << unit.get_address()
                          << "  | owner index: " << std::dec << unit->missile_spell_info()->get_spell_data()->get_name()
                          << " | spawntime: " << *g_time - unit->missile_spawn_time() << " | max_range "
                          << unit->missile_start_position.dist_to(unit->missile_end_position) << std::endl;
        }

        return;

   for (auto particle : g_entity_list.get_enemy_uncategorized())
        {
            if (!particle || particle->dist_to_local() > 1000.f || is_ignored_object(particle->network_id)) continue;

            if (GetAsyncKeyState(0x30))
            {
                m_ignored_objects.push_back(particle->network_id);
                continue;
            }

            auto unit = g_entity_list.get_by_index(particle->index);
            if (!unit) continue;

            unit.update();

            Vec2 sp{};
            if (!world_to_screen(unit->position, sp)) continue;

            auto name = unit->get_alternative_name();


            // g_render->line_3d(
            //     g_local->position, g_local->position.extend( unit->position, 500.f ), Color::red( ), 4.f );

            g_render->circle_3d(unit->position, Color(255, 255, 255), 20.f, 2, -1, 2.f);
            g_render->text_shadow({ sp.x, sp.y - 8.f }, Color(25, 255, 25), g_fonts->get_zabel_12px(), name.c_str(),
                                  12);


            if (GetAsyncKeyState(VK_CONTROL))
                std::cout << name << " | particle address: " << std::hex << unit.get_address()
                          << "  | owner index: " << std::dec << particle->get_particle_source_index()
                          << " | spawn: " << particle->get_particle_spawn_time() << " ( "
                          << *g_time - particle->get_particle_spawn_time()
                          << " ) spawn2: " << particle->get_particle_alt_spawn_time() << std::endl;
        }

    return;

        for (auto obj : g_entity_list.get_ally_missiles())
        {
            if (!obj || !obj->dist_to_local() > 1000.f) continue;
            auto unit = g_entity_list.get_by_index(obj->index);
            if (!unit) continue;

            unit.update();

            Vec2 sp{};
            if (!world_to_screen(unit->position, sp)) continue;

            auto name = unit->get_alternative_name();


             g_render->line_3d(
                 unit->missile_start_position, unit->missile_end_position, Color::red( ), 4.f );

            g_render->circle_3d(unit->position, Color(255, 255, 255), 20.f, 2, -1, 2.f);
            g_render->text_shadow({ sp.x, sp.y - 8.f }, Color(25, 255, 25), g_fonts->get_zabel_12px(), name.c_str(),
                                  12);


            if (GetAsyncKeyState(VK_CONTROL))
                std::cout << name << " | missile address: " << std::hex << unit.get_address()
                          << "  | owner index: " << std::dec << unit->get_owner_index() << " | spawntime: " << unit->missile_spawn_time() << std::endl;
        }

        return;


        for (auto obj : g_entity_list.get_ally_minions()) {
            if (!obj || !obj->is_ward()) continue;
            auto unit = g_entity_list.get_by_index(obj->index);
            if (!unit) continue;

            unit.update();

            Vec2 sp{};
            if (!world_to_screen(unit->position, sp)) continue;

            auto name = unit->get_alternative_name();


            // g_render->line_3d(
            //     g_local->position, g_local->position.extend( unit->position, 500.f ), Color::red( ), 4.f );

            g_render->circle_3d(unit->position, Color(255, 255, 255), 20.f, 2, -1, 2.f);
            g_render->text_shadow({ sp.x, sp.y - 8.f }, Color(25, 255, 25), g_fonts->get_zabel_12px(), name.c_str(),
                                  12);


            if (GetAsyncKeyState(VK_CONTROL))
                std::cout << name << " | ward address: " << std::hex << unit.get_address()
                          << "  | owner index: " << std::dec << unit->get_owner_index() << std::endl;
            
        }

        return;

        for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
            if ( !particle || particle->dist_to_local( ) > 1000.f || is_ignored_object( particle->network_id )
             ) continue;

            if ( GetAsyncKeyState( 0x30 ) ) {
                m_ignored_objects.push_back( particle->network_id );
                continue;
            }

            auto unit = g_entity_list.get_by_index( particle->index );
            if ( !unit ) continue;

            unit.update( );

            Vec2 sp{ };
            if ( !world_to_screen( unit->position, sp ) ) continue;

            auto name = unit->get_alternative_name( );


            // g_render->line_3d(
            //     g_local->position, g_local->position.extend( unit->position, 500.f ), Color::red( ), 4.f );

            g_render->circle_3d( unit->position, Color( 255, 255, 255 ), 20.f, 2, -1, 2.f );
            g_render->text_shadow(
                { sp.x, sp.y - 8.f },
                Color( 25, 255, 25 ),
                g_fonts->get_zabel_12px( ),
                name.c_str( ),
                12
            );


            if ( GetAsyncKeyState( VK_CONTROL ) )
                std::cout << name << " | particle address: " << std::hex << unit.get_address( )
                    << "  | owner index: " << std::dec << particle->get_particle_source_index( )
                    << " | spawn: " << particle->get_particle_spawn_time( ) << " ( "
                    << *g_time - particle->get_particle_spawn_time( ) << " ) spawn2: " << particle->
                    get_particle_alt_spawn_time( )
                    << std::endl;
        }
    }

    auto Tracker::update_summoner_cooldowns( ) -> void{
        for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
            if ( !particle || is_ignored_object( particle->network_id ) ) continue;

            auto name = particle->get_alternative_name( );
            if ( name.find( "global_ss_flash.troy" ) == std::string::npos ) {
                m_ignored_objects.push_back( particle->network_id );
                continue;
            }

            force_update_hero_spell(
                particle->get_particle_source_index( ),
                ct_hash( "SummonerFlash" ),
                particle->get_particle_spawn_time( ) + 300.f
            );

            auto object = g_entity_list.get_by_index( particle->get_particle_source_index( ) );
            if ( !object ) continue;

            m_ignored_objects.push_back( particle->network_id );

            std::cout << "[ Summoner ] Found cast particle " << name << " from " << object->champion_name.text
                << ", Cooldown updated to " << particle->get_particle_spawn_time() + 300.f << std::endl;

            auto spells = get_hero_spells(object->index);
            if (!spells) continue;

            std::cout << "Current Flash Cooldown: " << spells->summoner_d.cooldown_expire
                      << " | IsReady: " << bool(spells->summoner_d.cooldown_expire < *g_time) << std::endl;

            continue;
        }
    }

    auto Tracker::force_update_hero_spell( int16_t index, hash_t name, float cooldown_expire ) -> void{

        for (auto& inst : m_summoner_cooldown_info) {
            if (inst.index != index) continue;

            inst.cooldown_expire = cooldown_expire;
            return;
        }

        for ( auto& inst : m_hero_spells ) {
            if ( inst.index != index ) continue;

            auto object = g_entity_list.get_by_index( index );
            if ( !object ) continue;

            for ( int i = 0; i < 2; i++ ) {
                auto spell = object->spell_book.get_spell_slot( i == 0 ? ESpellSlot::d : ESpellSlot::f );
                if ( !spell || rt_hash( spell->get_name().data() ) != name ) continue;

                if ( i == 0 ) {

                    std::cout << "[ Updated flash timer to ]\n";
                    inst.summoner_d.cooldown_expire = cooldown_expire;
                    return;
                }


                std::cout << "[ SummoerFlash Found spell on F ]\n";
                inst.summoner_f.cooldown_expire = cooldown_expire;
                return;
            }
        }

        std::cout << "[ SummoerFlash count find owner ]\n";
    }

    auto Tracker::update_jungle_tracker( ) -> void{
        if ( !g_config->awareness.jungletrack_enable->get< bool >( ) ) return;

        if ( m_jungle_camps.size( ) < 17 ) {
            // setup jungle camp tracking
            for ( auto i = 0; i < 17; i++ ) {
                switch ( i ) {
                case 0:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Red" ), true ) ); // topside red
                    break;
                case 1:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Red" ), false ) ); // botside red
                    break;
                case 2:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Blue" ), true ) ); // topside blue
                    break;
                case 3:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Blue" ), false ) ); // botside blue
                    break;
                case 4:
                    m_jungle_camps.push_back( initialize_camp( _( "Sru_Crab" ), true ) ); // topside crab
                    break;
                case 5:
                    m_jungle_camps.push_back( initialize_camp( _( "Sru_Crab" ), false ) ); // botside crab
                    break;
                case 6:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Krug" ), true ) ); // topside krugs
                    break;
                case 7:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Krug" ), false ) ); // botside krugs
                    break;
                case 8:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Razorbeak" ), true ) ); // topside chicken
                    break;
                case 9:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Razorbeak" ), false ) ); // botside chicken
                    break;
                case 10:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Murkwolf" ), true ) ); // topside wolves
                    break;
                case 11:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Murkwolf" ), false ) ); // botside wolves
                    break;
                case 12:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Gromp" ), true ) ); // topside gromp
                    break;
                case 13:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Gromp" ), false ) ); // botside gromp
                    break;
                case 14:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Dragon" ), false ) ); // dragon
                    break;
                case 15:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_RiftHerald" ), true ) ); // herald
                    break;
                case 16:
                    m_jungle_camps.push_back( initialize_camp( _( "SRU_Baron" ), true ) ); // baron
                    break;
                default:
                    break;
                }
            }

            debug_log( "[ GODTRACKER ]: Setup completed. Size: {}", (int)m_jungle_camps.size( ) );
        }

        for ( auto& inst : m_jungle_camps ) {
            if ( inst.is_disabled ) continue;

            if ( inst.is_respawning ) {
                inst.is_ready = false;

                if ( inst.type == ECampType::herald && *g_time >= 1200.f ) inst.is_disabled = true;

                if ( *g_time >= inst.respawn_time ) {
                    inst.is_respawning           = false;
                    inst.should_notify           = true;
                    inst.did_attack_notification = false;
                    inst.did_death_notification  = false;
                } else if ( *g_time - inst.death_time < 10.f ) continue;
            }

            if ( !inst.is_ready ) {
                for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                    if ( !minion || minion->is_dead( ) || minion->team != 300 || minion->position.dist_to(
                            inst.position
                        ) >= 1000.f || !minion->is_jungle_monster( )
                        || inst.type == ECampType::dragon && minion->get_name( ).find( _( "SRU_Dragon" ) ) ==
                        std::string::npos ||
                        inst.type != ECampType::dragon && rt_hash( minion->get_name().c_str() ) != rt_hash(
                            inst.name.c_str()
                        ) && rt_hash( minion->get_name().c_str() ) != rt_hash( inst.alternative_name.c_str() ) ||
                        inst.is_object_saved( minion->index ) || is_monster_tracked( minion->index ) )
                        continue;

                    CampObject instance = {
                        minion->index,
                        minion->position,
                        *g_time,
                        false,
                        inst.type != ECampType::krugs || rt_hash( minion->get_name().c_str() ) != ct_hash(
                            "SRU_KrugMiniMini"
                        )
                    };

                    if ( instance.is_main_objective ) {
                        if ( !inst.is_active ) {
                            if ( inst.is_respawning ) {
                                if ( rt_hash( minion->get_name().c_str() ) != rt_hash( inst.name.c_str() ) ) {
                                    //std::cout << "[ GODTRACK ]: Invalid early spawn detected for " << inst.display_name << " | Skipping.\n";
                                    continue;
                                }


                                //std::cout << "[ GODTRACK ]: Early spawn detected for " << inst.display_name << " | Overriding timer\n";
                                inst.is_respawning           = false;
                                inst.should_notify           = true;
                                inst.did_attack_notification = false;
                                inst.did_death_notification  = false;
                            }
                            //else std::cout << "[ GODTRACK ]: Activated " << inst.display_name << " track after respawn\n";

                            inst.is_active = true;
                        }

                        ++inst.objective_count;

                        inst.objects.push_back( instance );
                    }
                }
            }

            if ( !inst.is_active ) continue;


            if ( !inst.is_ready ) {
                switch ( inst.type ) {
                case ECampType::krugs:
                    inst.is_ready = inst.objective_count == 2;
                    break;
                case ECampType::gromp:
                case ECampType::blue:
                case ECampType::red:
                case ECampType::scuttle_crab:
                case ECampType::dragon:
                case ECampType::herald:
                case ECampType::baron:
                    inst.is_ready = inst.objective_count == 1;
                    break;
                case ECampType::raptors:
                    inst.is_ready = inst.objective_count == 6;
                    break;
                case ECampType::wolves:
                    inst.is_ready = inst.objective_count == 3;
                    break;
                default:
                    break;
                }

                if ( inst.is_ready && inst.position.dist_to( g_local->position ) < 4000.f &&
                    ( inst.type == ECampType::baron || inst.type == ECampType::herald || inst.type ==
                        ECampType::dragon ) ) {
                    g_features->activator->add_ping_instance(
                        inst.position,
                        utils::ESendPingType::assist_me,
                        0.f,
                        true
                    );
                    g_features->activator->add_ping_instance(
                        inst.position,
                        utils::ESendPingType::assist_me,
                        1.5f,
                        true
                    );

                    debug_log( "[ PING ] 2x assist ping recently spawned {}", inst.display_name );
                }
            }


            for ( auto& data : inst.objects ) {
                auto& object = g_entity_list.get_by_index( data.index );
                if ( !object ) {
                    if ( data.is_main_objective ) inst.objective_count--;

                    inst.remove_object( data.index );
                    continue;
                }

                object.update( );

                if ( g_config->awareness.jungletrack_glow->get< bool >( ) && ( !data.is_glowing || !data.
                        is_glowing_extra ) &&
                    g_function_caller->is_glow_queueable( ) ) {
                    if ( !data.is_glowing ) {
                        g_function_caller->enable_glow(
                            object->network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                            1,
                            3,
                            0,
                            false
                        );
                        data.is_glowing = true;
                    } else if ( !data.is_glowing_extra ) {
                        g_function_caller->enable_glow(
                            object->network_id,
                            D3DCOLOR_ARGB( 255, 255, 255, 255 ),
                            0,
                            3,
                            10,
                            false
                        );

                        data.is_glowing_extra = true;
                    }
                }

                if ( object->team != 300 ) {
                    if ( data.is_main_objective ) --inst.objective_count;

                    inst.remove_object( data.index );
                    continue;
                }

                if ( !object->is_dead( ) && !data.is_dead ) {
                    if ( object->is_invisible( ) ) {

                        if ( false && data.position != object->position ) {
                            const auto difference = data.position.dist_to( object->position );
                            const auto floor_diff = std::floor( difference );
                            const auto is_invalid = difference - floor_diff == 0.f;

                            if ( *g_time - data.last_visible_time > 1.f ) {
                                if ( !is_invalid && g_config->awareness.jungletrack_update_aggro->get< bool >( ) ) {
                                    inst.is_under_attack  = true;
                                    inst.should_notify    = true;
                                    inst.last_action_time = *g_time;

                                    if ( inst.type != ECampType::baron && inst.type !=
                                        ECampType::herald )
                                        g_features->activator->set_last_camp( inst.position );

                                    std::cout << object->get_name( ) << " | AGGRESSION FOUND! | distance: "
                                        << data.position.dist_to( object->position ) << std::endl;
                                }
                                //else std::cout << object->get_name( ) << " | fake aggression detected | distance: "<< difference << std::endl;
                            }

                            //std::cout << object->get_name() << " | visible time: " << *g_time - data.last_visible_time << std::endl;
                            data.position = object->position;
                        }

                        if ( inst.type == ECampType::baron || inst.type == ECampType::herald ) {
                            std::string particle_name = inst.type == ECampType::baron
                                                            ? _( "SRU_Baron_Base_BA1_tar.troy" )
                                                            : _( "SRU_RiftHerald_EyeGlow.troy" );

                            for ( const auto obj : g_entity_list.get_enemy_uncategorized( ) ) {
                                if ( !obj ||
                                    obj->position.dist_to( inst.position ) > 1000.f ||
                                    obj->get_alternative_name( ).find( particle_name ) == std::string::npos
                                )
                                    continue;

                                inst.is_under_attack  = true;
                                inst.last_action_time = *g_time;
                                inst.should_notify    = true;
                                g_features->activator->set_last_camp( inst.position );

                                if ( g_config->misc.ping_objectives->get< bool >( ) && !inst.did_ping ) {
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::ping,
                                        0.f,
                                        true
                                    );
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::ping,
                                        0.91f,
                                        true
                                    );
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::need_vision,
                                        5.57f,
                                        true
                                    );

                                    inst.did_ping = true;
                                }

                                break;
                            }
                        } else if ( inst.type == ECampType::dragon ) {
                            std::string particle_name = _( "SRU_Dragon_Spawn_Praxis.troy" );

                            for ( const auto object : g_entity_list.get_enemy_uncategorized( ) ) {
                                if ( !object || object->position.dist_to( inst.position ) > 1000.f
                                    || object->get_alternative_name( ).find( particle_name ) == std::string::npos )
                                    continue;

                                inst.is_under_attack  = true;
                                inst.last_action_time = *g_time;
                                inst.should_notify    = true;
                                g_features->activator->set_last_camp( inst.position );

                                if ( g_config->misc.ping_objectives->get< bool >( ) && !inst.did_ping ) {
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::ping,
                                        0.f,
                                        true
                                    );
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::ping,
                                        1.13f,
                                        true
                                    );
                                    g_features->activator->add_ping_instance(
                                        inst.position,
                                        utils::ESendPingType::need_vision,
                                        6.18f,
                                        true
                                    );

                                    inst.did_ping = true;
                                }

                                break;
                            }
                        }
                    } else {
                        data.position          = object->position;
                        data.last_visible_time = *g_time;
                    }
                }

                if ( data.is_dead || !object->is_dead( ) ) continue;

                bool should_ignore{ };

                for ( const auto ally : g_entity_list.get_allies( ) ) {
                    if ( !ally || ally->is_dead( ) ) continue;

                    should_ignore = object->position.dist_to( ally->position ) < 700.f;

                    if ( should_ignore ) break;
                }

                inst.should_notify = !should_ignore; 
                inst.is_under_attack  = true;
                inst.last_action_time = *g_time;
                data.is_dead          = true;

                if ( inst.should_notify ) { g_features->activator->set_last_camp( inst.position ); }

                if ( data.is_main_objective ) {
                    --inst.objective_count;
                    data.is_main_objective = false;
                }
            }

            if ( inst.is_under_attack && *g_time - inst.last_action_time > 8.f ) {
                inst.is_under_attack         = false;
                inst.did_attack_notification = false;
                inst.did_ping                = false;
            }


            if ( inst.objective_count <= 0 ) {
                //std::cout << "[ GODTRACKER ]: Camp " << inst.display_name << " has died.\n";

                inst.is_respawning   = true;
                inst.is_active       = false;
                inst.is_ready        = false;
                inst.is_under_attack = false;
                inst.objective_count = 0;
                inst.death_time      = *g_time;

                inst.did_ping = false;

                inst.objects.clear( );

                switch ( inst.type ) {
                case ECampType::red:
                case ECampType::blue:
                case ECampType::dragon:
                    inst.respawn_time = *g_time + 300.f;
                    break;
                case ECampType::herald:
                case ECampType::baron:
                    inst.respawn_time = *g_time + 360.f;
                    break;
                case ECampType::raptors:
                case ECampType::gromp:
                case ECampType::wolves:
                case ECampType::krugs:
                    inst.respawn_time = *g_time + 135.f;
                    break;
                case ECampType::scuttle_crab:
                    inst.respawn_time = *g_time + 10.f;
                    break;
                default:
                    break;
                }
            }
        }
    }

    auto Tracker::draw_jungle_tracker( ) -> void{
        if ( !g_config->awareness.jungletrack_enable->get< bool >( ) || m_jungle_camps.size( ) <= 16 ) return;

        for ( auto& inst : m_jungle_camps ) {
            if ( inst.is_disabled ) continue;

            const auto minimap = world_to_minimap( inst.position );

            if ( inst.is_respawning ) {
                if ( *g_time - inst.death_time <= 5.f ) {
                    g_render->circle_minimap( inst.position, Color( 255, 25, 50 ), 600.f, -1, 2.f );

                    if ( *g_time - inst.death_time <= 3.5f && inst.should_notify ) {
                        auto modifier = ( *g_time - inst.death_time ) / 3.5f;

                        modifier = utils::ease::ease_out_quart( modifier );

                        const auto opacity_modifier = 1.f - modifier;

                        g_render->circle_3d(
                            inst.position,
                            Color( 255.f, 255.f, 255.f, 255.f * opacity_modifier ),
                            750.f + 2000.f * modifier,
                            Renderer::outline,
                            -1,
                            2.f + 2.f * opacity_modifier
                        );
                    }
                }

                if ( inst.type != ECampType::scuttle_crab ) {
                    const auto time_left = inst.respawn_time - *g_time;

                    const int minutes = std::floor( time_left / 60.f );
                    const int seconds = time_left - minutes * 60.f;

                    auto counter_text = std::to_string( minutes ) + ":";
                    if ( seconds > 9 ) counter_text += std::to_string( seconds );
                    else counter_text += "0" + std::to_string( seconds );

                    const auto text_size = g_render->get_text_size(
                        counter_text,
                        g_config->awareness.jungletrack_font_size->get< int >( ) <= 16
                            ? g_fonts->get_zabel_16px( )
                            : g_fonts->get_zabel( ),
                        g_config->awareness.jungletrack_font_size->get< int >( )
                    );
                    g_render->text_shadow(
                        Vec2( minimap.x - text_size.x / 2.f, minimap.y - text_size.y / 2.f ),
                        Color::white( ),
                        g_config->awareness.jungletrack_font_size->get< int >( ) <= 16
                            ? g_fonts->get_zabel_16px( )
                            : g_fonts->get_zabel( ),
                        counter_text.data( ),
                        g_config->awareness.jungletrack_font_size->get< int >( )
                    );
                }

                if ( inst.should_notify && !inst.did_death_notification ) {
                    const auto should_notify{
                        *g_time < 1000.f || inst.type == ECampType::baron ||
                        inst.type == ECampType::dragon || inst.type == ECampType::herald
                    };

                    if ( g_config->awareness.jungletrack_notification->get< bool >( ) && should_notify ) {
                        auto       text    = inst.display_name;
                        const auto decider = rand( ) % ( 80 - 1 + 1 ) + 1;

                        if ( decider == 22 ) text += _( " died from overdose!\n" );
                        else text += _( " was killed!\n" );

                        add_notification( text, 6, inst.texture );
                    }

                    if ( g_config->awareness.jungletrack_sound_alert->get< bool >( ) && should_notify )
                        sdk::audio::play(
                            path::join( { directory_manager::get_resources_path( ), "select1.wav" } ).value( ),
                            g_config->misc.audio_volume->get< float >( )
                        );

                    inst.did_death_notification = true;
                }
            }

            if ( !inst.is_active ) continue;

            //std::string state = "ALIVE";
            //const auto text_size = g_render->get_text_size(state, g_fonts->get_block(), 8);
            //g_render->text_shadow(vec2(minimap.x - text_size.x / 2.f, minimap.y ), color(25, 255, 25), g_fonts->get_block(), state.data(), 8);

            if ( g_config->awareness.jungletrack_draw_count->get< bool >( ) && inst.objective_count > 1 ) {
                auto       count_text = "POP: " + std::to_string( inst.objective_count );
                const auto count_size = g_render->get_text_size( count_text, g_fonts->get_block( ), 8 );
                g_render->text_shadow(
                    Vec2( minimap.x - count_size.x / 2.f, minimap.y ),
                    Color::white( ),
                    g_fonts->get_block( ),
                    count_text.data( ),
                    8
                );
            }

            if ( inst.is_under_attack ) {
                if ( inst.last_action_start_time == 0.f ) inst.last_action_start_time = *g_time;

                if ( false && m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) ) {
                    for ( auto& obj : inst.objects ) {
                        if ( obj.is_glowing || obj.is_dead ) continue;

                        auto& unit = g_entity_list.get_by_index( obj.index );
                        if ( !unit || unit->is_dead( ) ) continue;

                        if ( g_function_caller->is_glow_queueable( ) ) {
                            g_function_caller->enable_glow(
                                unit->network_id,
                                D3DCOLOR_ARGB( 255, 255, 25, 25 ),
                                0,
                                3,
                                3,
                                false
                            );

                            obj.is_glowing = true;
                        }
                    }
                }


                if ( inst.should_notify && !inst.did_attack_notification ) {
                    const auto should_notify{
                        *g_time < 1000.f || inst.type == ECampType::baron ||
                        inst.type == ECampType::dragon || inst.type == ECampType::herald
                    };

                    if ( g_config->awareness.jungletrack_notification->get< bool >( ) && should_notify ) {
                        auto text = inst.display_name + " gain aggro!\n";
                        add_notification( text, 6, inst.texture, notification_type::aggro );
                    }

                    if ( g_config->awareness.jungletrack_sound_alert->get< bool >( ) )
                        sdk::audio::play(
                            path::join( { directory_manager::get_resources_path( ), "select1.wav" } ).value( ),
                            g_config->misc.audio_volume->get< float >( )
                        );

                    inst.did_attack_notification = true;
                }

                if ( *g_time - inst.last_action_start_time <= 1.5f ) {
                    const auto time_since_start = *g_time - inst.last_action_start_time;
                    auto       modifier         = time_since_start / 1.5f;

                    modifier = utils::ease::ease_out_expo( modifier );

                    g_render->circle_3d(
                        inst.position,
                        Color( 255.f, 255.f, 0.f, 255.f * modifier ),
                        750.f * modifier,
                        Renderer::outline,
                        72,
                        4.f
                    );
                } else {
                    //g_render->circle_3d(inst.position, color(255, 255, 0), 600.f, c_renderer::outline, 50, 2.f);

                    g_render->circle_3d(
                        inst.position,
                        g_features->orbwalker->animate_color( Color( 255, 255, 0 ), EAnimationType::pulse, 4 ),
                        750.f,
                        Renderer::outline,
                        72,
                        4.f
                    );
                }


                g_render->circle_minimap( inst.position, Color( 255, 255, 25 ), 600.f, -1, 2.f );
            }
        }
    }

    auto Tracker::draw_notifications( ) -> void{
        int   count{ };
        float placement_increment{ };

        const auto multiplier = ( g_config->awareness.jungletrack_notification_size->get< int >( ) + 1 ) * 0.25f;

        const Vec2 base_position{
            static_cast< float >( g_render_manager->get_width( ) ) / 2.f,
            static_cast< float >( g_render_manager->get_width( ) ) * 0.03f
        };
        const Vec2 box_size{ 360.f * multiplier, 80.f * multiplier };


        const auto texture_y_size = ( 76.f + ( g_config->awareness.jungletrack_notification_size->get< int >( ) - 3 ) )
            *
            multiplier;
        const Vec2 texture_size{ 75.f * multiplier, texture_y_size };
        const int  font_size = 24 * multiplier; 

        auto box_background = Color( 25, 25, 25, 160 );

        for ( const auto inst : m_notifications ) {
            if ( inst.end_time <= *g_time ) {
                remove_notification( inst.text );
                continue;
            }

            constexpr auto animation_duration{ 1.f };
            auto           animation_modifier{ 1.f };

            if ( *g_time <= inst.start_time + 0.8f ) {
                const auto time_left = inst.start_time + 0.8f - *g_time;
                animation_modifier   = 1.f - utils::ease::ease_in_out_quart( time_left / 0.8f );
            } else if ( *g_time >= inst.end_time - animation_duration ) {
                const auto time_left = inst.end_time - *g_time;
                animation_modifier   = utils::ease::ease_in_out_quart( time_left / animation_duration );
            }

            const Vec2 draw_box_size = { box_size.x, box_size.y * animation_modifier };

            const Vec2 dynamic_position{ base_position.x, base_position.y + placement_increment };
            const Vec2 box_position{ dynamic_position.x - draw_box_size.x / 2.f, dynamic_position.y };

            //box_position.y *= animation_modifier;
            //box_size.y *= animation_modifier;


            const Vec2 texture_draw_size = { texture_size.x * animation_modifier, texture_size.y * animation_modifier };

            g_render->filled_box( box_position, draw_box_size, box_background.alpha( 160 * animation_modifier ), 0 );
            g_render->box(
                box_position,
                draw_box_size,
                Color( 60.f, 60.f, 60.f, 255.f * animation_modifier ),
                0.f,
                2.f
            );

            if ( inst.texture )
                g_render->image(
                    { box_position.x + 2.f, box_position.y + 2.f },
                    texture_draw_size,
                    inst.texture
                );

            const auto text_size = g_render->get_text_size(
                inst.text,
                g_fonts->get_zabel( ),
                font_size * animation_modifier
            );

            Vec2 text_position{
                box_position.x + draw_box_size.x / 2.f + texture_draw_size.x / 2.f,
                box_position.y + draw_box_size.y / 2.f
            };

            text_position -= Vec2( text_size.x / 2.f, text_size.y / 2.f );
            placement_increment += draw_box_size.y + 10.f * animation_modifier;

            g_render->text_shadow(
                text_position,
                Color( 255.f, 255.f, 255.f, 255.f * animation_modifier ),
                g_fonts->get_zabel( ),
                inst.text.c_str( ),
                font_size * animation_modifier
            );
        }
    }

    void draw_blurred_box(Vec2 position, Vec2 size, Color color, float rounding, float thickness, int layers) {

        float alpha_step_modifier = color.a / static_cast<float>(layers);

        for (int i = 0; i < layers; i++) {

            g_render->box(position, size, color.alpha(static_cast<int>(color.a - alpha_step_modifier * i)), -1, thickness);

            position.x += thickness;
            position.y += thickness;
            size.x -= thickness * 2.f;
            size.y -= thickness * 2.f;
        }
    }

    auto Tracker::draw_jungle_alerts() -> void {
        float placement_increment{};

        const auto multiplier = static_cast<float>(g_config->awareness.jungle_alert_scale->get<int>()) / 100.f;

        const Vec2 base_position{ static_cast<float>(g_config->awareness.jungle_alert_position_x->get<int>()),
                                 static_cast<float>( g_config->awareness.jungle_alert_position_y->get<int>() )};
        const Vec2 box_size{ 275.f * multiplier, 64.f * multiplier };

        Vec2 header_size = { 275.f * multiplier, 25.f * multiplier };

        if (!m_repositioning && GetAsyncKeyState(0x01))
        {
            auto cursor = g_input->get_cursor_position();

            m_repositioning = true;

            if (cursor.x >= base_position.x && cursor.x <= base_position.x + header_size.x &&
                cursor.y >= base_position.y && cursor.y <= base_position.y + header_size.y)
            {
                m_reposition_start = cursor;
                m_ignore_changes   = false;
            }
            else
                m_ignore_changes = true;
        }
        else if (m_repositioning)
        {
            if (!GetAsyncKeyState(0x01)) { m_repositioning = false; }
            else if (!m_ignore_changes)
            {
                auto cursor = g_input->get_cursor_position();

                auto delta = Vec2(cursor.x - m_reposition_start.x, cursor.y - m_reposition_start.y);

                g_config->awareness.jungle_alert_position_x->get<int>() += delta.x;
                g_config->awareness.jungle_alert_position_y->get<int>() += delta.y;

                m_reposition_start = cursor;
            }
        }

        const auto texture_y_size =
            (60.f + (g_config->awareness.jungletrack_notification_size->get<int>() - 3)) * multiplier;
        const Vec2 texture_size{ 60.f * multiplier, texture_y_size };
        const int  font_size = 20.f * multiplier;

        auto box_background = Color(0, 0, 0, 180);

        g_render->filled_box(base_position, header_size, Color(0, 0, 0, 225));
        std::string header_text = "jungle alerts";

        auto text_size = g_render->get_text_size(header_text, g_fonts->get_zabel(), 20.f);

        Vec2 text_position = { base_position.x + header_size.x / 2.f - text_size.x / 2.f,
                               base_position.y + header_size.y / 2.f - text_size.y / 2.f };

        g_render->text(text_position, Color::white(), g_fonts->get_zabel(), header_text.data(), 20.f);
        Vec2 line_start = { base_position.x, base_position.y + header_size.y };
        Vec2 line_end   = { line_start.x + header_size.x, line_start.y };
       // g_render->line(line_start, line_end, Color(75, 225, 75), 2.f);

        Vec2 message_position = { base_position.x, base_position.y + header_size.y };

        for (const auto inst : m_notifications)
        {
            if (inst.end_time <= *g_time)
            {
                remove_notification(inst.text);
                continue;
            }

            constexpr auto animation_in_duration{ 1.25f };
            constexpr auto delayed_animation_duration{ 1.5f };
            constexpr auto animation_duration{ 1.f };
            auto           animation_modifier{ 1.f };
            auto           color_modifier{ 1.f };

            auto delayed_modifier{ 1.f };
            auto adjusted_animation_duration = animation_duration - 0.5f;

            if (*g_time <= inst.start_time + animation_in_duration)
            {
                const auto time_left = inst.start_time + animation_in_duration - *g_time;
                animation_modifier   = 1.f - utils::ease::ease_in_out_circ( time_left / animation_in_duration);
                //animation_modifier   = utils::ease::ease_in_expo(animation_modifier);
                color_modifier       = 1.f - utils::ease::ease_in_out_quart(time_left / animation_in_duration);

            }
            else if (*g_time >= inst.end_time - animation_duration)
            {
                const auto time_left = inst.end_time - *g_time;
                animation_modifier   = utils::ease::ease_in_out_quart(time_left / animation_duration);
            }

            if (*g_time <= inst.start_time + adjusted_animation_duration + delayed_animation_duration)
            {
                const auto time_left =
                    inst.start_time + adjusted_animation_duration + delayed_animation_duration - *g_time;
                delayed_modifier = 1.f - utils::ease::ease_in_out_expo(time_left / delayed_animation_duration);
                // animation_modifier   = utils::ease::ease_in_expo(animation_modifier);
                // color_modifier = 1.f - utils::ease::ease_in_out_quart(time_left / animation_in_duration);
            }

            const Vec2 draw_box_size = { box_size.x, box_size.y * animation_modifier };

            const Vec2 dynamic_position{ message_position.x// - (150.f - 150.f * animation_modifier)
                , message_position.y + placement_increment };
            const Vec2 box_position{ dynamic_position.x, dynamic_position.y };

            // box_position.y *= animation_modifier;
            // box_size.y *= animation_modifier;

            float min_color  = 10.f;
            float dynamic_color = 255.f - (255.f - min_color) * color_modifier;
            Color color_glow    = Color(dynamic_color, dynamic_color, dynamic_color);

            const Vec2 texture_draw_size = { texture_size.x * animation_modifier, texture_size.y * animation_modifier };


            if (*g_time > inst.start_time + 0.5f &&
                *g_time <= inst.start_time + animation_duration + delayed_animation_duration)
            {

                Vec2 effect_size = { 100.f * multiplier, 0.f };

                Vec2 adjusted_position = { box_position.x - effect_size.x * delayed_modifier, box_position.y };
                Vec2 adjusted_box_size = { draw_box_size.x + effect_size.x * 2.f * delayed_modifier, draw_box_size.y };

                Color effect_color =
                    Color(255.f, inst.type == notification_type::aggro ? 255.f : 255.f - 255.f * animation_modifier,
                          255.f - 255.f * animation_modifier, 255.f - 255.f * delayed_modifier);

                 g_render->filled_box(adjusted_position, adjusted_box_size, effect_color,
                                     0);
            }

            g_render->filled_box(box_position, draw_box_size, box_background.alpha(225 * animation_modifier), 0);
            //g_render->box(box_position, draw_box_size, color_glow, 0.f,
            //              2.f);

             if (inst.texture)
                 g_render->image({ box_position.x + 2.f, box_position.y + 2.f }, texture_draw_size, inst.texture);

            if( *g_time > inst.start_time + 0.5f )draw_blurred_box(box_position, draw_box_size,
                              inst.type == notification_type::aggro
                                  ? Color(255, 255, 0, static_cast<int>(255.f * animation_modifier))
                                      : g_features->orbwalker->get_pulsing_color().alpha(
                                            static_cast<int>(255.f * animation_modifier)),
                              0.f, 2.f,
                              1);

           

            const auto text_size =
                g_render->get_text_size(inst.text, g_fonts->get_zabel(), font_size * animation_modifier);

            Vec2 text_position{ box_position.x + draw_box_size.x / 2.f + texture_draw_size.x / 2.f,
                                box_position.y + draw_box_size.y / 2.f };

            text_position -= Vec2(text_size.x / 2.f, text_size.y / 2.f);
            placement_increment += draw_box_size.y; //+ 10.f * animation_modifier;

            g_render->text_shadow(
                text_position,
                inst.type == notification_type::aggro
                    ? Color(255, 255, 0, static_cast<int>(255.f * animation_modifier))
                    : g_features->orbwalker->get_pulsing_color().alpha(static_cast<int>(255.f * animation_modifier)),
                                  g_fonts->get_zabel(), inst.text.c_str(), font_size * animation_modifier);
        }

        //g_render->filled_box(line_start, { header_size.x, -3.f * multiplier }, Color(75, 225, 75));
    }


    auto Tracker::update_spell_tracker( ) -> void{
        for ( auto obj : g_entity_list.get_enemies( ) ) {
            if ( !obj || obj->is_dead( ) || is_hero_spells_tracked( obj->index ) ) continue;

            auto          champ_name = std::string( obj->champion_name.text );
            HeroSpellInfo hero_spells{ };
            hero_spells.index = obj->index;
            hero_spells.valid = true;

            auto spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) continue;

            SpellSlotInfo q{ };
            q.cooldown = spell_slot->get_cooldown( );
            q.level    = spell_slot->level;
            q.is_ready = spell_slot->level > 0;


            q.spell_texture =
                g_render
                ->load_texture_from_file(
                    path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            "spells",
                            champ_name + "_q.png"
                        }
                    ).value( )
                )
                .get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::w );
            if ( !spell_slot ) continue;

            SpellSlotInfo w{ };
            w.is_ready      = spell_slot->level > 0;
            w.cooldown      = spell_slot->get_cooldown( );
            w.level         = spell_slot->level;
            w.spell_texture =
                g_render
                ->load_texture_from_file(
                    path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            "spells",
                            champ_name + "_w.png"
                        }
                    ).value( )
                )
                .get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) continue;

            SpellSlotInfo e{ };
            e.is_ready      = spell_slot->level > 0;
            e.cooldown      = spell_slot->get_cooldown( );
            e.level         = spell_slot->level;
            e.spell_texture =
                g_render
                ->load_texture_from_file(
                    path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            "spells",
                            champ_name + "_e.png"
                        }
                    ).value( )
                )
                .get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) continue;

            SpellSlotInfo r{ };
            r.is_ready      = spell_slot->level > 0;
            r.cooldown      = spell_slot->get_cooldown( );
            r.level         = spell_slot->level;
            r.spell_texture =
                g_render
                ->load_texture_from_file(
                    path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            "spells",
                            champ_name + "_r.png"
                        }
                    ).value( )
                )
                .get( );

            hero_spells.spell_q = q;
            hero_spells.spell_w = w;
            hero_spells.spell_e = e;
            hero_spells.spell_r = r;

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::d );
            if ( !spell_slot ) continue;

            SpellSlotInfo summoner1{ };
            auto          spell_info = spell_slot->get_spell_info( );
            if ( !spell_info ) continue;

            auto spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) continue;

            auto name = spell_data->get_name( );

            summoner1.is_ready        = spell_slot->is_ready( );
            summoner1.cooldown_expire = spell_slot->cooldown_expire;
            summoner1.cooldown        = spell_slot->cooldown;
            summoner1.level           = spell_slot->level;

            summoner1.spell_texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "summoners",
                        name + ".png",
                    }
                ).value( )
            ).get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::f );
            if ( !spell_slot ) continue;

            SpellSlotInfo summoner2{ };
            spell_info = spell_slot->get_spell_info( );
            if ( !spell_info ) continue;

            spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) continue;

            name = spell_data->get_name( );

            summoner2.is_ready        = spell_slot->is_ready( );
            summoner2.cooldown_expire = spell_slot->cooldown_expire;
            summoner2.cooldown        = spell_slot->cooldown;
            summoner2.level           = spell_slot->level;
            summoner2.spell_texture   = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "summoners",
                        name + ".png",
                    }
                ).value( )
            ).get( );

            hero_spells.summoner_d = summoner1;
            hero_spells.summoner_f = summoner2;

            m_summoner_cooldown_info.push_back({ obj->index, 0.f });

            m_hero_spells.push_back( hero_spells );
            m_experience.push_back( { obj->index, obj->level, obj->get_experience( ) } );
        }

        for ( auto obj : g_entity_list.get_allies( ) ) {
            if ( !obj || obj->is_dead( ) || is_hero_spells_tracked( obj->index ) ) continue;

            auto          champ_name = std::string( obj->champion_name.text );
            HeroSpellInfo hero_spells{ };
            hero_spells.index = obj->index;
            hero_spells.valid = true;

            auto spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) continue;

            SpellSlotInfo q{ };
            q.cooldown = spell_slot->get_cooldown( );
            q.level    = spell_slot->level;
            q.is_ready = true;


            q.spell_texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "champions",
                        champ_name,
                        "spells",
                        champ_name + "_q.png"
                    }
                ).value( )
            ).get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::w );
            if ( !spell_slot ) continue;

            SpellSlotInfo w{ };
            w.is_ready      = true;
            w.cooldown      = spell_slot->get_cooldown( );
            w.level         = spell_slot->level;
            w.spell_texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "champions",
                        champ_name,
                        "spells",
                        champ_name + "_w.png"
                    }
                ).value( )
            ).get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::e );
            if ( !spell_slot ) continue;

            SpellSlotInfo e{ };
            e.is_ready      = true;
            e.cooldown      = spell_slot->get_cooldown( );
            e.level         = spell_slot->level;
            e.spell_texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "champions",
                        champ_name,
                        "spells",
                        champ_name + "_e.png"
                    }
                ).value( )
            ).get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::r );
            if ( !spell_slot ) continue;

            SpellSlotInfo r{ };
            r.is_ready      = true;
            r.cooldown      = spell_slot->get_cooldown( );
            r.level         = spell_slot->level;
            r.spell_texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "champions",
                        champ_name,
                        "spells",
                        champ_name + "_r.png"
                    }
                ).value( )
            ).get( );

            hero_spells.spell_q = q;
            hero_spells.spell_w = w;
            hero_spells.spell_e = e;
            hero_spells.spell_r = r;

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::d );
            if ( !spell_slot ) continue;

            SpellSlotInfo summoner1{ };
            auto          spell_info = spell_slot->get_spell_info( );
            if ( !spell_info ) continue;

            auto spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) continue;

            auto name = spell_data->get_name( );

            summoner1.is_ready        = spell_slot->is_ready( );
            summoner1.cooldown_expire = spell_slot->cooldown_expire;
            summoner1.cooldown        = spell_slot->cooldown;
            summoner1.level           = spell_slot->level;
            summoner1.spell_texture   = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "summoners",
                        name + ".png",
                    }
                ).value( )
            ).get( );

            spell_slot = obj->spell_book.get_spell_slot( ESpellSlot::f );
            if ( !spell_slot ) continue;

            SpellSlotInfo summoner2{ };
            spell_info = spell_slot->get_spell_info( );
            if ( !spell_info ) continue;

            spell_data = spell_info->get_spell_data( );
            if ( !spell_data ) continue;

            name = spell_data->get_name( );

            summoner2.is_ready        = spell_slot->is_ready( );
            summoner2.cooldown_expire = spell_slot->cooldown_expire;
            summoner2.cooldown        = spell_slot->cooldown;
            summoner2.level           = spell_slot->level;
            summoner2.spell_texture   = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "summoners",
                        name + ".png",
                    }
                ).value( )
            ).get( );

            hero_spells.summoner_d = summoner1;
            hero_spells.summoner_f = summoner2;

            m_hero_spells.push_back( hero_spells );
            m_experience.push_back( { obj->index, obj->level, obj->get_experience( ) } );
        }

        for ( auto& inst : m_hero_spells ) {
            auto& hero = g_entity_list.get_by_index( inst.index );
            if ( !hero ) continue;
            hero.update( );
            // auto hero = obj->create_updated_copy( );
            if ( hero->is_dead( ) || hero->is_invisible( ) && hero->team != g_local->team ) continue;

            auto spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::q );
            if ( !spell_slot ) continue;

            auto sci = hero->spell_book.get_spell_cast_info( );
            if ( sci && hero->team != g_local->team ) sci.force_update( );

            //SpellSlotInfo spell{ };

            // update spells values
            for ( auto i = 0; i < 6; i++ ) {
                switch ( i ) {
                case 0:

                    inst.spell_q.is_ready = inst.spell_q.cooldown_expire <= *g_time && spell_slot->level > 0;
                    inst.spell_q.level     = spell_slot->level;
                    inst.spell_q.mana_cost = spell_slot->get_manacost( );

                    if ( sci && sci->slot == 0 && !sci->is_autoattack && !sci->is_special_attack ) {
                        inst.spell_q.last_cast_time = sci->server_cast_time;
                        inst.spell_q.cooldown       = spell_slot->get_cooldown( ) * ( 1.f - hero->
                            get_cooldown_reduction_percent( ) );
                        inst.spell_q.cooldown_expire = sci->server_cast_time + inst.spell_q.cooldown;
                    }

                    break;
                case 1:
                    spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::w );
                    if ( !spell_slot ) continue;

                    inst.spell_w.is_ready  = inst.spell_w.cooldown_expire <= *g_time && spell_slot->level > 0;
                    inst.spell_w.level     = spell_slot->level;
                    inst.spell_w.mana_cost = spell_slot->get_manacost( );

                    if ( sci && sci->slot == 1 && !sci->is_autoattack && !sci->is_special_attack ) {
                        inst.spell_w.last_cast_time = sci->server_cast_time;
                        inst.spell_w.cooldown       =
                            spell_slot->get_cooldown( ) * ( 1.f - hero->get_cooldown_reduction_percent( ) );
                        inst.spell_w.cooldown_expire = sci->server_cast_time + inst.spell_w.cooldown;
                    }

                    break;
                case 2:
                    spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::e );
                    if ( !spell_slot ) continue;

                    inst.spell_e.is_ready  = inst.spell_e.cooldown_expire <= *g_time && spell_slot->level > 0;
                    inst.spell_e.level     = spell_slot->level;
                    inst.spell_e.mana_cost = spell_slot->get_manacost( );

                    if ( sci && sci->slot == 2 && !sci->is_autoattack && !sci->is_special_attack ) {
                        inst.spell_e.last_cast_time = sci->server_cast_time;
                        inst.spell_e.cooldown       =
                            spell_slot->get_cooldown( ) * ( 1.f - hero->get_cooldown_reduction_percent( ) );
                        inst.spell_e.cooldown_expire = sci->server_cast_time + inst.spell_e.cooldown;
                    }

                    break;
                case 3:
                    spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::r );
                    if ( !spell_slot ) continue;

                    inst.spell_r.is_ready  = inst.spell_r.cooldown_expire <= *g_time && spell_slot->level > 0;
                    inst.spell_r.level     = spell_slot->level;
                    inst.spell_r.mana_cost = spell_slot->get_manacost( );

                    if ( sci && sci->slot == 3 && !sci->is_autoattack && !sci->is_special_attack ) {
                        inst.spell_r.last_cast_time = sci->server_cast_time;
                        inst.spell_r.cooldown       = spell_slot->get_cooldown( ) * ( 1.f - hero->
                            get_cooldown_reduction_percent( ) );
                        inst.spell_r.cooldown_expire = sci->server_cast_time + inst.spell_r.cooldown;
                    }

                    break;
                case 4:
                    spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::d );
                    if ( !spell_slot ) continue;

                    inst.summoner_d.is_ready        = spell_slot->is_ready( );
                    inst.summoner_d.cooldown_expire = spell_slot->cooldown_expire;
                    inst.summoner_d.cooldown        = spell_slot->cooldown;
                    break;
                case 5:
                    spell_slot = hero->spell_book.get_spell_slot( ESpellSlot::f );
                    if ( !spell_slot ) continue;

                    inst.summoner_f.is_ready        = spell_slot->is_ready( );
                    inst.summoner_f.cooldown_expire = spell_slot->cooldown_expire;
                    inst.summoner_f.cooldown        = spell_slot->cooldown;
                    break;
                default:
                    break;
                }
            }
        }
    }

    auto Tracker::update_clones( ) -> void{
        if ( !g_config->awareness.show_clones->get< bool >( ) ) return;

        for ( const auto hero : g_entity_list.get_enemies( ) ) {
            if ( !hero || hero->is_dead( ) || !is_clone_champion( rt_hash( hero->champion_name.text ) ) ) continue;

            const auto hero_name = rt_hash( hero->champion_name.text );


            for ( const auto minion : g_entity_list.get_enemy_minions( ) ) {
                if ( !minion || !minion->is_alive( ) || is_clone_tracked( minion->index ) || minion->is_invisible( ) ||
                    rt_hash( minion->get_name().c_str() ) != hero_name )
                    continue;

                m_clones.push_back( minion->index );
            }
        }

        for ( const auto& index : m_clones ) {
            auto& clone = g_entity_list.get_by_index( index );
            if ( !clone ) {
                remove_clone( index );
                continue;
            }

            // auto clone = obj->create_copy( );
            clone.update( );

            if ( clone->is_dead( ) ) remove_clone( index );
        }
    }

    auto Tracker::update_enemies( ) -> void{
        const auto enemies = g_entity_list.get_enemies( );
        if ( enemies.size( ) == m_enemies.size( ) ) return;

        //for ( auto entity : g_entity_list.get_allies(  ))
        //  enemies.push_back( entity );


        for ( const auto obj : enemies ) {
            if ( !obj || is_enemy_saved( obj->index ) ) continue;

            m_enemies.push_back( obj->index );

            if ( is_hero_tracked( obj->index ) ) continue;

            LastSeenInfo info{ };

            info.index             = obj->index;
            info.last_seen_time    = *g_time;
            info.last_raw_position = obj->position;
            info.last_position     = obj->position;


            //auto spell = obj->spell_book.get_spell_slot( ESpellSlot::q );
            //if ( spell ) std::cout << obj->name.text << "ENEMY spellQ: " << std::hex << spell.get_address( ) << " | " << spell->get_name(  ) << std::endl;

            auto aimgr = obj->get_ai_manager( );
            //if ( aimgr ) std::cout << "[ tracker.cpp/update_enemies() ] enemy aimgr " << obj->name.text
            //            << " address: " << std::hex << aimgr.get_address( ) << std::endl;


            auto object = g_entity_list.get_by_index( obj->index );
            if ( object && aimgr )
                std::cout << "[ " << obj->name.text << " ] Address: " << std::hex << object.get_address( )
                    << " | Index: " << std::dec << obj->index << " AIMGR: " << std::hex << aimgr.get_address( ) <<
                    std::endl;

            m_last_seen_data.push_back( info );
        }

        for ( const auto& inst : m_last_seen_data ) {
            auto& object = g_entity_list.get_by_index( inst.index );
            if ( !object ) continue;

            object.update( );
        }
    }


    auto Tracker::update_wards( ) -> void{
        if ( !g_config->awareness.ward_tracker->get< bool >( ) ) return;

        remove_invalid_wards( );

        auto debug_ally_wards{ false };

        for ( const auto obj :
              debug_ally_wards ? g_entity_list.get_ally_minions( ) : g_entity_list.get_enemy_minions( ) ) {
            if ( !obj || is_invalid_ward( obj->network_id ) || obj->is_dead( ) ||
                is_ward_active( obj->index ) )
                continue;

            const auto type = obj->get_ward_type( );
            if ( type == static_cast< sdk::game::Object::EWardType >( 0 ) ) {
                m_invalid_wards.push_back( obj->network_id );
                continue;
            }

            WardInstance ward{ };

            ward.index          = obj->index;
            ward.type           = type;
            ward.expire_time    = *g_time + obj->mana;
            ward.start_time     = *g_time;
            ward.does_expire    = true;
            ward.detection_time = *g_time;

            ward.is_visible  = obj->is_visible( );
            ward.was_visible = ward.is_visible;

            switch ( type ) {
            case Object::EWardType::control:
            case Object::EWardType::blue:
                ward.does_expire = false;
                break;
            case Object::EWardType::maokai_sproutling:
                ward.was_updated = true;
                ward.expire_time = *g_time + 31.f;
                break;
            case Object::EWardType::normal:
            {
                if ( rt_hash( obj->get_name( ).data( ) ) == ct_hash( "YellowTrinket" ) ) {
                    auto buff = g_features->buff_cache->get_buff( obj->index, ct_hash( "relicyellowward" ) );

                    if ( buff ) {

                        auto object = g_entity_list.get_by_index( obj->index );
                        ward.expire_time = buff->buff_data->end_time + 0.5f;
                        ward.start_time  = buff->buff_data->start_time;
                        ward.was_updated = true;

                        if (object)
                        {
                                std::cout << "[ WARD Tracking ] Updated endtime for " << obj->get_name()
                                          << " | manual endtime: " << *g_time + obj->mana
                                          << " | buff endtime: " << ward.expire_time << " [ 0x" << std::hex
                                          << object.get_address() << " ]" << std::endl;
                        }
                       
                    }
                } else {
                    auto buff = g_features->buff_cache->get_buff( obj->index, ct_hash( "ItemGhostWard" ) );

                    if ( buff ) {
                        ward.expire_time = buff->buff_data->end_time + 0.5f;
                        ward.start_time  = buff->buff_data->start_time;
                        ward.was_updated = true;

                        std::cout << "[ WARD Tracking ] Updated endtime for " << obj->get_name( )
                            << " | manual endtime: " << *g_time + obj->mana
                            << " | buff endtime: " << ward.expire_time << std::endl;
                    }
                }

                break;
            }
            case Object::EWardType::caitlyn_trap:
            {
                auto duration = 30.f;

                auto hero_spells = get_hero_spells( obj->get_owner_index( ) );
                if ( hero_spells.has_value( ) ) {
                    switch ( hero_spells.value( ).spell_w.level ) {
                    case 2:
                        duration = 35.f;
                        break;
                    case 3:
                        duration = 40.f;
                        break;
                    case 4:
                        duration = 45.f;
                        break;
                    case 5:
                        duration = 50.f;
                        break;
                    default:
                        break;
                    }
                }

                ward.should_get_vision_polygon = false;
                ward.was_updated               = true;
                ward.expire_time               = *g_time + duration + 0.5f;
                break;
            }
            default:
                break;
            }

            m_wards.push_back( ward );
        }

        for ( auto& inst : m_wards ) {
            if ( !inst.should_get_vision_polygon || !inst.vision_polygon.points.empty( ) ) continue;

            auto& ward = g_entity_list.get_by_index( inst.index );
            if ( !ward || ward->is_dead( ) ) continue;

            ward.update( );

            auto vision_radius{ 900.f };

            switch ( inst.type ) {
            case Object::EWardType::blue:
                vision_radius = 650.f;
            case Object::EWardType::fiddlesticks_effigy:
            case Object::EWardType::zombie:
            case Object::EWardType::control:
            case Object::EWardType::normal:
            {
                auto pos = ward->position;

                auto bypass_poly = Circle( pos, vision_radius ).to_polygon( 0, -1, 64 );
                auto in_bush = static_cast< int >( g_navgrid->get_collision( ward->position ) ) & static_cast< int >(
                                   ECollisionFlag::grass )
                                   ? true
                                   : false;

                constexpr auto bypass_angle  = 120.f;
                constexpr auto bypass_radius = 70.f;

                for ( auto& point : bypass_poly.points ) {
                    auto brushPhase = in_bush ? 1 : 0;
                    int  bypass_count{ };

                    for ( auto i = 1; i <= 100; i++ ) {
                        auto check = pos.extend( point, static_cast< float >( i ) * ( vision_radius / 100.f ) );
                        check.y = g_navgrid->get_height( check );
                        const auto collision = static_cast< int >( g_navgrid->get_collision( check ) );

                        if ( collision & static_cast< int >( ECollisionFlag::wall ) ) {
                            auto polygon = Sector(
                                check,
                                check.extend( pos, -50.f ),
                                bypass_angle,
                                bypass_radius
                            ).to_polygon_new( );
                            bool found_bypass{ };

                            for ( auto& position : polygon.points ) {
                                if ( position.dist_to( check ) <= 5.f ) continue;

                                auto coll = static_cast< int >( g_navgrid->get_collision( position ) );
                                if ( coll & static_cast< int >( ECollisionFlag::wall ) ) continue;

                                bypass_count++;
                                found_bypass = true;
                                break;
                            }

                            if ( found_bypass ) continue;

                            point = check;
                            break;
                        }

                        if ( collision & static_cast< int >( ECollisionFlag::grass ) ) {
                            if ( brushPhase == 2 || brushPhase == 0 ) {
                                auto polygon = Sector(
                                    check,
                                    check.extend( pos, -50.f ),
                                    bypass_angle,
                                    bypass_radius
                                ).to_polygon_new( );
                                bool found_bypass{ };

                                for ( auto& position : polygon.points ) {
                                    if ( position.dist_to( check ) <= 5.f ) continue;

                                    auto coll = static_cast< int >( g_navgrid->get_collision( position ) );
                                    if ( coll & static_cast< int >( ECollisionFlag::wall ) || coll & static_cast<
                                        int >( ECollisionFlag::grass ) )
                                        continue;

                                    bypass_count++;
                                    found_bypass = true;
                                    break;
                                }

                                if ( found_bypass ) continue;

                                point = check;
                                break;
                            }
                        } else if ( brushPhase == 1 ) brushPhase = 2;
                    }
                }

                inst.vision_polygon = bypass_poly;

                debug_log( "added vision poly for {}", ward->get_name( ) );
                break;
            }
            default:
                inst.should_get_vision_polygon = false;
                break;
            }
        }

        auto to_remove = std::ranges::remove_if(
            m_wards,
            [&]( const WardInstance& ward ){
                auto w = g_entity_list.get_by_index( ward.index );
                if ( !w ) return true;

                w.update( );

                if ( w->is_dead( ) || w->get_ward_type( ) == Object::EWardType::unknown ||
                    !debug_ally_wards && w->team == g_local->team )
                    return true;

                return false;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_wards.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::update_recalls( ) -> void{
        if ( !g_config->awareness.recall_tracker->get< bool >( ) ) return;

        for ( const auto enemy : g_entity_list.get_enemies( ) ) {
            if ( !enemy || enemy->is_dead( ) ) continue;

            if ( is_recall_active( enemy->index ) ) {
                const auto i = get_recall_index( enemy->index );

                if ( !i || !m_recalls[ *i ].canceled ) continue;

                bool found_particle{ };
                for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                    if ( !particle || particle->position.dist_to( enemy->position ) > 5.f ||
                        is_particle_ignored( particle->network_id ) )
                        continue;

                    auto name = particle->get_alternative_name( );
                    if ( rt_hash( name.data( ) ) != ct_hash( "TeleportHome.troy" ) &&
                        rt_hash( name.data( ) ) != ct_hash( "Super_TeleportHome.troy" ) ) {
                        m_ignored_particles.push_back( particle->network_id );
                        continue;
                    }

                    found_particle = true;
                    break;
                }

                if ( !found_particle ) continue;

                auto to_remove = std::ranges::remove_if(
                    m_recalls,
                    [&]( const RecallInfo& inf ) -> bool{ return inf.index == enemy->index; }
                );

                if ( to_remove.empty( ) ) continue;

                m_recalls.erase(
                    to_remove.begin( ),
                    to_remove.end( )
                );
            }

            if ( !enemy->champion_name.is_valid( ) ) continue;

            int16_t particle_index{ };
            bool    found_particle{ };
            bool    is_super{ };

            for ( const auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                if ( !particle || particle->position.dist_to( enemy->position ) > 5.f ||
                    is_particle_ignored( particle->network_id ) )
                    continue;

                auto name = particle->get_alternative_name( );
                if ( rt_hash( name.data( ) ) != ct_hash( "TeleportHome.troy" ) &&
                    rt_hash( name.data( ) ) != ct_hash( "Super_TeleportHome.troy" ) ) {
                    m_ignored_particles.push_back( particle->network_id );
                    continue;
                }

                found_particle = true;
                particle_index = particle->index;
                is_super       = rt_hash( name.data( ) ) == ct_hash( "Super_TeleportHome.troy" );
                break;
            }

            if ( !found_particle ) continue;

            RecallInfo recall{ };

            recall.object_name    = enemy->champion_name.text;
            recall.index          = enemy->index;
            recall.canceled       = false;
            recall.particle_index = particle_index;
            recall.start_time     = *g_time;

            if ( is_super ) {
                recall.type             = ERecallType::empowered;
                recall.finish_time      = *g_time + 3.75f;
                recall.channel_duration = 3.75f;
            } else {
                recall.type             = ERecallType::normal;
                recall.finish_time      = *g_time + 7.75f;
                recall.channel_duration = 7.75f;
            }

            m_recalls.push_back( recall );
        }

        for ( auto& inst : m_recalls ) {
            auto& enemy = g_entity_list.get_by_index( inst.index );
            if ( !enemy ) {
                inst.should_remove = true;
                continue;
            }

            enemy.update( );
            if ( !enemy || enemy->is_dead( ) ) {
                inst.should_remove = true;
                continue;
            }

            auto       is_recalling{ !inst.finished && !inst.canceled };
            const auto index = get_last_seen_index( inst.index );

            if ( is_recalling ) {
                const auto& particle = g_entity_list.get_by_index( inst.particle_index );
                if ( !particle ) is_recalling = false;
            }

            if ( !inst.canceled && inst.finish_time > *g_time && !is_recalling ) {
                // on recall cancel
                inst.canceled    = true;
                inst.cancel_time = *g_time;
            } else if ( !inst.canceled && !inst.finished && inst.finish_time < *g_time ) {
                // on recall finish
                if ( index ) {
                    if ( enemy->team == 100 ) m_last_seen_data[ *index ].last_position = Vec3( 394.f, 182.f, 462.f );
                    else m_last_seen_data[ *index ].last_position = Vec3( 14310.f, 171.f, 14386.f );
                    m_last_seen_data[ *index ].last_seen_time = *g_time;

                    m_last_seen_data[ *index ].total_recall_time = 0.f;
                    m_last_seen_data[ *index ].time_in_recall    = 0.f;
                    m_last_seen_data[ *index ].is_recalling      = false;
                }

                inst.finished = true;
            }
        }

        auto should_remove = std::ranges::remove_if(
            m_recalls,
            []( const RecallInfo& inst ) -> bool{
                if ( inst.canceled && *g_time - inst.cancel_time > 2.f || inst.should_remove ) { return true; }
                if ( *g_time - inst.finish_time > 2.f ) { return true; }

                return false;
            }
        );

        if ( should_remove.empty( ) ) return;

        m_recalls.erase( should_remove.begin( ), should_remove.end( ) );
    }

    auto Tracker::is_ward_active( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_wards,
            [&]( const WardInstance& ward ) -> bool{ return ward.index == index; }
        ) != m_wards.end( );
    }

    auto Tracker::is_invalid_ward( const unsigned network_id ) -> bool{
        return std::ranges::find_if(
            m_invalid_wards,
            [ & ]( const unsigned& nid ) -> bool{ return nid == network_id; }
        ) != m_invalid_wards.end( );
    }


    auto Tracker::remove_invalid_wards( ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_wards,
            [&]( const WardInstance& ward ) -> bool{ return ward.does_expire && ward.expire_time < *g_time; }
        );

        if ( to_remove.empty( ) ) return;

        m_wards.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::is_recall_active( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_recalls,
            [&]( const RecallInfo& recall ) -> bool{ return recall.index == index; }
        ) != m_recalls.end( );
    }

    // auto c_tracker::remove_recall( const int16_t index ) -> void {
    //     const auto to_remove = std::ranges::remove_if(
    //         m_recalls,
    //         [&]( const recall_info_t& recall ) -> bool{
    //             return recall.index == index;
    //         }
    //     );
    //
    //     if ( to_remove.empty( ) ) return;
    //
    //     m_recalls.erase( to_remove.begin( ), to_remove.end( ) );
    // }


    auto Tracker::get_last_seen_index( const int16_t index ) const -> std::optional< int32_t >{
        for ( size_t i = 0u; i < m_last_seen_data.size( ); i++ ) {
            if ( m_last_seen_data[ i ].index == index ) return i;
        }

        return std::nullopt;
    }

    auto Tracker::get_recall_index( const int16_t index ) const -> std::optional< int32_t >{
        if ( m_recalls.empty( ) ) return std::nullopt;
        for ( size_t i = 0u; i < m_recalls.size( ); i++ ) {
            if ( m_recalls[ i ].index == index ) return static_cast< int32_t >( i );
        }

        return std::nullopt;
    }

    auto Tracker::get_recall( const int16_t index ) const -> std::optional< Tracker::RecallInfo >{
        if ( m_recalls.empty( ) ) return std::nullopt;

        const auto found = std::ranges::find_if(
            m_recalls,
            [&]( const RecallInfo& recall ) -> bool{ return recall.index == index; }
        );

        if ( found == m_recalls.end( ) ) return std::nullopt;

        return *found;
    }

    auto Tracker::get_last_seen_data( const int16_t index ) -> std::optional< LastSeenInfo >{
        const auto found = std::ranges::find_if(
            m_last_seen_data,
            [&]( const LastSeenInfo& inst ) -> bool{ return inst.index == index; }
        );

        if ( found == m_last_seen_data.end( ) ) return std::nullopt;
        return *found;
    }


    auto Tracker::get_hero_spells( const int16_t index ) const -> std::optional< Tracker::HeroSpellInfo >{
        for ( auto inst : m_hero_spells ) if ( inst.index == index ) return std::make_optional( inst );

        return std::nullopt;
    }

    auto Tracker::get_hud_info( const int16_t index ) -> HudInfo*{
        for ( auto& inst : m_hud_infos ) { if ( inst.index == index ) { return &inst; } }
        return nullptr;
    }

    auto Tracker::is_path_tracked( const int16_t index ) -> bool{
        const auto found = std::ranges::find_if(
            m_paths,
            [&]( const PathingInfo& path ) -> bool{ return path.index == index; }
        );

        return found != m_paths.end( );
    }

    auto Tracker::is_monster_tracked( const int16_t index ) const -> bool{
        for ( const auto& inst : m_jungle_camps ) {
            for ( const auto data : inst.objects ) { if ( data.index == index ) return true; }
        }

        return false;
    }


    auto Tracker::initialize_camp( std::string name, bool topside ) const -> CampInstance{
        CampInstance inst{ };
        auto         hash = rt_hash( name.data() );

        switch ( hash ) {
        case ct_hash( "Sru_Crab" ):
            inst.type = ECampType::scuttle_crab;
            inst.position   = topside ? m_topside_crab : m_botside_crab;
            inst.is_botside = !topside;
            inst.name       = name;

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "crab_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Scuttlecrab" );
            break;
        case ct_hash( "SRU_Gromp" ):
            inst.type = ECampType::gromp;
            inst.position   = topside ? m_topside_gromp : m_botside_gromp;
            inst.is_botside = !topside;
            inst.name       = name;

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "gromp_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Gromp" );
            break;
        case ct_hash( "SRU_Krug" ):
        case ct_hash( "SRU_KrugMini" ):
            inst.type = ECampType::krugs;
            inst.position         = topside ? m_topside_krugs : m_botside_krugs;
            inst.is_botside       = !topside;
            inst.name             = "SRU_Krug";
            inst.alternative_name = "SRU_KrugMini";

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "krug_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Ancient Krug" );

            break;
        case ct_hash( "SRU_Murkwolf" ):
        case ct_hash( "SRU_MurkwolfMini" ):
            inst.type = ECampType::wolves;
            inst.position         = topside ? m_topside_wolves : m_botside_wolves;
            inst.is_botside       = !topside;
            inst.name             = "SRU_Murkwolf";
            inst.alternative_name = "SRU_MurkwolfMini";

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "murkwolf_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Murkwolf" );

            break;
        case ct_hash( "SRU_Razorbeak" ):
        case ct_hash( "SRU_RazorbeakMini" ):
            inst.type = ECampType::raptors;
            inst.position         = topside ? m_topside_raptors : m_botside_raptors;
            inst.is_botside       = !topside;
            inst.name             = "SRU_Razorbeak";
            inst.alternative_name = "SRU_RazorbeakMini";


            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "razorbeakmini_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Razorbeak" );

            break;
        case ct_hash( "SRU_Blue" ):
            inst.type = ECampType::blue;
            inst.position   = topside ? m_topside_blue : m_botside_blue;
            inst.is_botside = !topside;
            inst.name       = name;
            inst.texture    = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "bluesentinel_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Blue" );
            break;
        case ct_hash( "SRU_Red" ):
            inst.type = ECampType::red;
            inst.position   = topside ? m_topside_red : m_botside_red;
            inst.is_botside = !topside;
            inst.name       = name;
            inst.texture    = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "brambleback_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Red" );
            break;
        case ct_hash( "SRU_Dragon" ):
            inst.type = ECampType::dragon;
            inst.position   = m_dragon;
            inst.is_botside = true;
            inst.name       = name;

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "dragon_square.png"
                    }
                ).value( )
            );
            inst.display_name = "Dragon";
            break;
        case ct_hash( "SRU_RiftHerald" ):
            inst.type = ECampType::herald;
            inst.position   = m_herald;
            inst.is_botside = false;
            inst.name       = name;
            inst.texture    = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "sruriftherald_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Rift Herald" );
            break;
        case ct_hash( "SRU_Baron" ):
            inst.type = ECampType::baron;
            inst.position   = m_herald;
            inst.is_botside = false;
            inst.name       = name;

            inst.texture = g_render->load_texture_from_file(
                path::join(
                    {
                        directory_manager::get_resources_path( ),
                        "jungle",
                        "baron_square.png"
                    }
                ).value( )
            );
            inst.display_name = _( "Baron" );
            break;
        default:
            break;
        }

        return inst;
    }

    auto Tracker::get_path_position(
        const Vec3&                start_position,
        const std::vector< Vec3 >& path,
        const int                  waypoint_index,
        const float                distance
    ) -> Vec3{
        if ( path.empty( ) || waypoint_index == path.size( ) || path.size( ) == 1 ||
            static_cast< size_t >( waypoint_index ) >= path.size( ) )
            return start_position;

        auto current_waypoint = path[ waypoint_index ];
        current_waypoint.y    = g_navgrid->get_height( current_waypoint );


        auto       movable_units     = distance;
        const auto waypoint_distance = start_position.dist_to( current_waypoint );

        auto calculated_position{ start_position };

        if ( movable_units > waypoint_distance && static_cast< int32_t >( path.size( ) ) > waypoint_index + 1 ) {
            movable_units -= waypoint_distance;
            auto current_node = current_waypoint;

            for ( auto i = waypoint_index + 1u; i < path.size( ); i++ ) {
                auto next_node = path[ i ];
                current_node   = i == waypoint_index + 1 ? current_waypoint : path[ i - 1 ];

                current_node.y = g_navgrid->get_height(
                    current_node
                ); // setting y for waypoints to get correct distance between them
                next_node.y = g_navgrid->get_height( next_node );


                const auto node_distance = current_node.dist_to( next_node );

                // if next simulated node is too far to walk to, calculate position between current and next node which
                // target will reach
                if ( node_distance >= movable_units || i == path.size( ) - 1 ) {
                    calculated_position =
                        current_node.extend( next_node, node_distance > movable_units ? movable_units : node_distance );
                    break;
                }

                movable_units -= node_distance;
            }
        } else {
            calculated_position = start_position.extend(
                current_waypoint,
                movable_units > waypoint_distance ? waypoint_distance : movable_units
            );
        }

        return Vec3{ calculated_position.x, g_navgrid->get_height( calculated_position ), calculated_position.z };
    }

    auto Tracker::get_fow_particle_position( const hash_t hero_name ) -> std::optional< Vec3 >{
        Vec3 particle_position{ };

        switch ( hero_name ) {
        case ct_hash( "Elise" ):
        {
            std::vector< std::string > particle_names{ "Elise_Base_R_cas", "Elise_Base_Spider_R_cas" };

            for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                if ( !particle ) continue;

                auto name = particle->get_alternative_name( );

                bool found_particle{ };

                for ( auto p_name : particle_names ) {
                    if ( name.find( p_name ) == std::string::npos ) continue;

                    found_particle = true;
                    break;
                }

                if ( !found_particle ) continue;

                particle_position = particle->position;
                break;
            }

            if ( particle_position.length( ) <= 0.f ) return std::nullopt;

            break;
        }
        case ct_hash( "Udyr" ):
        {
            std::vector< std::string > particle_names{
                "Udyr_Base_BearStance",
                "Udyr_Base_BoarStance",
                "Udyr_Base_RamStance"
            };

            for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                if ( !particle ) continue;

                auto name = particle->get_alternative_name( );

                bool found_particle{ };

                for ( auto p_name : particle_names ) {
                    if ( name.find( p_name ) == std::string::npos ) continue;

                    found_particle = true;
                    break;
                }

                if ( !found_particle ) continue;

                particle_position = particle->position;
                break;
            }

            if ( particle_position.length( ) <= 0.f ) return std::nullopt;

            break;
        }
        case ct_hash( "Zac" ):
        {
            std::vector< std::string > particle_names{ "Zac_Base_Q_tar" };

            for ( auto particle : g_entity_list.get_enemy_uncategorized( ) ) {
                if ( !particle ) continue;

                auto name = particle->get_alternative_name( );

                bool found_particle{ };

                for ( auto p_name : particle_names ) {
                    if ( name.find( p_name ) == std::string::npos ) continue;

                    found_particle = true;
                    break;
                }

                if ( !found_particle ) continue;

                particle_position = particle->position;
                break;
            }

            if ( particle_position.length( ) <= 0.f ) return std::nullopt;

            break;
        }
        default:
            return std::nullopt;
        }

        return particle_position.length( ) <= 0.f ? std::nullopt : std::make_optional( particle_position );
    }

    auto Tracker::get_draw_style( const int32_t selection ) -> int32_t{
        switch ( selection ) {
        case 0:
            return Renderer::E3dCircleFlags::outline;
        case 1:
            return Renderer::E3dCircleFlags::filled;
        case 2:
            return Renderer::E3dCircleFlags::outline | Renderer::E3dCircleFlags::filled;
        default:
            return Renderer::E3dCircleFlags::outline;
        }
    }

    auto Tracker::is_hero_tracked( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_last_seen_data,
            [&]( const LastSeenInfo& last_seen ) -> bool{ return last_seen.index == index; }
        ) != m_last_seen_data.end( );
    }

    auto Tracker::is_clone_tracked( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_clones,
            [&]( const int16_t& clone_index ) -> bool{ return clone_index == index; }
        ) != m_clones.end( );
    }

    auto Tracker::is_enemy_turret_tracked( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_enemy_turrets,
            [&]( const int16_t& turret_index ) -> bool{ return turret_index == index; }
        ) != m_enemy_turrets.end( );
    }


    auto Tracker::is_hero_spells_tracked( const int16_t index ) -> bool{
        return std::ranges::find_if(
            m_hero_spells,
            [&]( const HeroSpellInfo& hero_spells ) -> bool{ return hero_spells.index == index; }
        ) != m_hero_spells.end( );
    }

    auto Tracker::is_enemy_saved( const int16_t index ) const -> bool{
        return std::ranges::any_of(
            m_enemies,
            [&]( const short i ) -> bool{ return i == index; }
        );
    }


    auto Tracker::remove_turret( const int16_t index ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_enemy_turrets,
            [&]( const int16_t& turret_index ) -> bool{ return turret_index == index; }
        );

        if ( !to_remove.empty( ) ) m_enemy_turrets.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::remove_clone( const int16_t index ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_clones,
            [&]( const int16_t& clone_index ) -> bool{ return clone_index == index; }
        );

        if ( to_remove.empty( ) ) return;

        m_clones.erase( to_remove.begin( ), to_remove.end( ) );
    }



    auto Tracker::is_clone_champion( const hash_t name ) const -> bool{
        for ( const auto inst : m_clone_champions ) { if ( inst == name ) return true; }

        return false;
    }


    auto Tracker::remove_notification( const std::string& message ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_notifications,
            [&]( const NotificationInstance& inst ) -> bool{
                return rt_hash( inst.text.data() ) == rt_hash( message.data() );
            }
        );

        if ( to_remove.empty( ) ) return;

        m_notifications.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::remove_text_popup( const int16_t target_index, const float creation_time ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_text_popups,
            [&]( const FloatingTextInstance& inst ) -> bool{
                return inst.object_index == target_index && inst.creation_time - creation_time == 0.f;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_text_popups.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::add_text_popup(
        const int16_t      target_index,
        const std::string& message,
        const Color        message_color
    ) -> void{
        const FloatingTextInstance inst{ target_index, message, message_color, *g_time };

        m_text_popups.push_back( inst );
    }


    auto Tracker::add_notification(
        const std::string&                          message,
        const float                                 duration,
        const std::shared_ptr< Renderer::Texture >& texture,
        const notification_type type
    ) -> void{
        const NotificationInstance inst{ message, *g_time, *g_time + duration, true, false, texture, type };

        m_notifications.push_back( inst );
    }


    auto Tracker::send_cooldown_in_chat(
        const Vec2          base_position,
        const Vec2          size,
        const LeagueString& champion_name,
        const int16_t       index,
        const ESpellSlot    slot,
        const unsigned      network_id
    ) -> void{
        if ( !g_config->hud.send_chat_cooldown->get< bool >( )
            || *g_time - m_last_chat_time <= 0.5f )
            return;

        if ( !GetAsyncKeyState( 0x01 ) ) {
            if ( m_did_click ) m_did_click = false;

            return;
        }

        if ( m_did_click ) return;

        const auto& hero = g_entity_list.get_by_index( index );
        if ( !hero ) return;

        const auto spell = hero->spell_book.get_spell_slot( slot );
        if ( !spell ) return;

        const Vec2 cursor = { g_input->get_cursor_position( ) };

        const Vec2 max_position{ base_position.x + size.x, base_position.y + size.y };

        // if cursor is not inside spell rect
        if ( cursor.x > max_position.x || cursor.x < base_position.x ||
            cursor.y > max_position.y || cursor.y < base_position.y )
            return;

        send_chat_cooldown( champion_name.text, spell, slot, network_id, true );
    }

    auto Tracker::send_chat_cooldown(
        std::string                      champion_name,
        utils::MemoryHolder< SpellSlot > spell,
        ESpellSlot                       slot,
        unsigned                         network_id,
        bool                             was_manual
    ) -> bool{
        if ( !was_manual && g_config->misc.chat_spell_cooldowns->get< int >( ) == 0 ) return true;

        auto text{ champion_name };

        text[ 0 ] = tolower( text[ 0 ] );

        switch ( rt_hash( champion_name.c_str( ) ) ) {
        case ct_hash( "AurelionSol" ):
            text = _( "asol" );
            break;
        case ct_hash( "Blitzcrank" ):
            text = _( "blitz" );
            break;
        case ct_hash( "Caitlyn" ):
            text = _( "cait" );
            break;
        case ct_hash( "Cassiopeia" ):
            text = _( "cassio" );
            break;
        case ct_hash( "Chogath" ):
            text = _( "cho" );
            break;
        case ct_hash( "DrMundo" ):
            text = _( "mundo" );
            break;
        case ct_hash( "Evelynn" ):
            text = _( "eve" );
            break;
        case ct_hash( "Fiddlesticks" ):
            text = _( "fiddle" );
            break;
        case ct_hash( "Gangplank" ):
            text = _( "gp" );
            break;
        case ct_hash( "Hecarim" ):
            text = _( "heca" );
            break;
        case ct_hash( "Heimerdinger" ):
            text = _( "heimer" );
            break;
        case ct_hash( "JarvanIV" ):
            text = _( "j4" );
            break;
        case ct_hash( "Kassadin" ):
            text = _( "kassa" );
            break;
        case ct_hash( "Katarina" ):
            text = _( "kata" );
            break;
        case ct_hash( "KhaZix" ):
            text = _( "kha" );
            break;
        case ct_hash( "KogMaw" ):
            text = _( "kog" );
            break;
        case ct_hash( "KSante" ):
            text = _( "ksante" );
            break;
        case ct_hash( "Leblanc" ):
            text = _( "leb" );
            break;
        case ct_hash( "LeeSin" ):
            text = _( "lee" );
            break;
        case ct_hash( "Lucian" ):
            text = _( "luc" );
            break;
        case ct_hash( "Malphite" ):
            text = _( "malph" );
            break;
        case ct_hash( "Malzahar" ):
            text = _( "malz" );
            break;
        case ct_hash( "MasterYi" ):
            text = _( "yi" );
            break;
        case ct_hash( "MissFortune" ):
            text = _( "mf" );
            break;
        case ct_hash( "MonkeyKing" ):
            text = _( "wuk" );
            break;
        case ct_hash( "Mordekaiser" ):
            text = _( "morde" );
            break;
        case ct_hash( "Morgana" ):
            text = _( "morg" );
            break;
        case ct_hash( "Nautilus" ):
            text = _( "naut" );
            break;
        case ct_hash( "Nidalee" ):
            text = _( "nida" );
            break;
        case ct_hash( "Nocturne" ):
            text = _( "noc" );
            break;
        case ct_hash( "Orianna" ):
            text = _( "ori" );
            break;
        case ct_hash( "Pantheon" ):
            text = _( "panth" );
            break;
        case ct_hash( "RekSai" ):
            text = _( "reksai" );
            break;
        case ct_hash( "Rengar" ):
            text = _( "reng" );
            break;
        case ct_hash( "Renekton" ):
            text = _( "rene" );
            break;
        case ct_hash( "Samira" ):
            text = _( "sami" );
            break;
        case ct_hash( "Sejuani" ):
            text = _( "sej" );
            break;
        case ct_hash( "Seraphine" ):
            text = _( "sera" );
            break;
        case ct_hash( "Shyvana" ):
            text = _( "shyv" );
            break;
        case ct_hash( "TahmKench" ):
            text = _( "tahm" );
            break;
        case ct_hash( "Taliyah" ):
            text = _( "tali" );
            break;
        case ct_hash( "Tristana" ):
            text = _( "tris" );
            break;
        case ct_hash( "Tryndamere" ):
            text = _( "trynd" );
            break;
        case ct_hash( "TwistedFate" ):
            text = _( "tf" );
            break;
        case ct_hash( "VelKoz" ):
            text = _( "vkoz" );
            break;
        case ct_hash( "Viktor" ):
            text = _( "vik" );
            break;
        case ct_hash( "Vladimir" ):
            text = _( "vlad" );
            break;
        case ct_hash( "Volibear" ):
            text = _( "voli" );
            break;
        case ct_hash( "Warwick" ):
            text = _( "ww" );
            break;
        case ct_hash( "Xerath" ):
            text = _( "xer" );
            break;
        case ct_hash( "XinZhao" ):
            text = _( "xin" );
            break;
        case ct_hash( "Yasuo" ):
            text = _( "yas" );
            break;
        case ct_hash( "Zilean" ):
            text = _( "zil" );
            break;
        case ct_hash( "Qiyana" ):
            text = _( "qiya" );
            break;
        default:
            break;
        }

        if ( text.empty( ) ) return false;

        auto raw_name = spell->get_name( );

        std::string spell_name{ _( "uhhh" ) };

        switch ( rt_hash( raw_name.c_str() ) ) {
        case ct_hash( "SummonerFlash" ):
            spell_name = "f";
            break;
        case ct_hash( "SummonerHaste" ):
            spell_name = "ghost";
            break;
        case ct_hash( "SummonerHeal" ):
            spell_name = "heal";
            break;
        case ct_hash( "SummonerDot" ):
            spell_name = "ignite";
            break;
        case ct_hash( "S5_SummonerSmiteDuel" ):
        case ct_hash( "S5_SummonerSmitePlayerGanker" ):
        case ct_hash( "SummonerSmite" ):
            spell_name = "smite";
            break;
        case ct_hash( "SummonerBarrier" ):
            spell_name = "barrier";
            break;
        case ct_hash( "SummonerTeleport" ):
        case ct_hash( "S12_SummonerTeleportUpgrade" ):
            spell_name = "tp";
            break;
        case ct_hash( "SummonerBoost" ):
            spell_name = "cleanse";
            break;
        case ct_hash( "SummonerExhaust" ):
            spell_name = "exh";
            break;
        default:
            spell_name = "r";
            break;
        }

        text += " " + spell_name;

        if ( spell->is_ready( ) ) return false;

        auto total_cooldown = spell->cooldown_expire;

        auto minutes = static_cast< int >( std::floor( total_cooldown / 60.f ) );
        auto seconds = static_cast< int >( std::floor( total_cooldown - minutes * 60.f ) );

        seconds += 10 - ( seconds % 10 );
        if ( seconds >= 60 ) seconds = 0;

        text += " " + std::to_string( minutes );
        if ( seconds < 10 && seconds >= 1 ) text += "0" + std::to_string( seconds );
        else text += std::to_string( seconds );

        if ( seconds == 0 ) text += "0";

        debug_log( "msg: {}", text );
        g_function_caller->send_chat( text.c_str( ), false );

        if ( was_manual && g_features->activator->is_cooldown_logged( network_id, slot ) ) {
            g_features->activator->remove_cooldown( network_id, slot );
        }

        m_last_chat_time = *g_time;
        m_did_click      = was_manual;

        return true;
    }

    auto Tracker::is_ignored_object( const unsigned network_id ) const -> bool{
        for ( const auto nid : m_ignored_objects ) if ( nid == network_id ) return true;

        return false;
    }

    auto Tracker::is_particle_ignored( const unsigned network_id ) const -> bool{
        for ( const auto nid : m_ignored_particles ) if ( nid == network_id ) return true;

        return false;
    }

    auto Tracker::draw_local_attack_range( ) const -> void{
        return;
        if ( !g_config->visuals.local_attack_range->get< bool >( ) ) return;

        if ( !g_local ) return;

        // auto copy = g_local.create_updated_copy( );
        g_local.update( );

        if ( g_local->is_dead( ) ) return;

        const auto autoattack_range = helper::get_current_hero( ) == EHeroes::zeri ? 500.f : g_local->attack_range;


        const auto draw_color{
            g_config->orbwalker.draw_rainbow_attack_range->get< bool >( )
                ? m_rainbow_color
                : g_config->visuals.local_attack_range_color->get< Color >( )
        };

        const auto bounding_radius = g_features->orbwalker->get_bounding_radius( );

        g_render->circle_3d(
            g_local->position,
            draw_color,
            autoattack_range + bounding_radius,
            Renderer::outline,
            -1,
            3.f
        );
    }

    auto Tracker::add_gank( const int16_t index ) -> void{ m_ganks.push_back( { index, *g_time, *g_time + 8.f } ); }

    auto Tracker::remove_dash( const int16_t index, const EDashType type, const float start_time ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_dashes,
            [ & ]( const DashInstance& dash ) -> bool{
                return dash.index == index && dash.type == type && dash.start_time == start_time;
            }
        );

        if ( to_remove.empty( ) ) return;

        m_dashes.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::is_dash_active( const int16_t index, const Vec3 start_position ) const -> bool{
        for ( const auto dash : m_dashes ) {
            if ( dash.index == index && dash.start_position.dist_to( start_position ) <= 1.f ) return true;
        }

        return false;
    }

    auto Tracker::remove_gank( const int16_t index ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_ganks,
            [ & ]( const GankInstance& inst ) -> bool{ return inst.index == index; }
        );

        if ( to_remove.empty( ) ) return;

        m_ganks.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::remove_item( const int16_t index, const EItemId item ) -> void{
        const auto to_remove = std::ranges::remove_if(
            m_hero_items,
            [ & ]( const ItemInstance& inst ) -> bool{ return inst.owner_index == index && inst.item == item; }
        );

        if ( to_remove.empty( ) ) return;

        m_hero_items.erase( to_remove.begin( ), to_remove.end( ) );
    }

    auto Tracker::get_items( const int16_t index ) const -> std::vector< ItemInstance >{
        std::vector< ItemInstance > items{ };

        for ( const auto inst : m_hero_items ) {
            if ( inst.owner_index != index ) continue;

            items.push_back( inst );
        }

        return items;
    }

    auto Tracker::is_item_tracked( const int16_t index, const EItemId item ) const -> bool{
        for ( const auto inst : m_hero_items ) {
            if ( inst.owner_index != index || inst.item != item ) continue;

            return true;
        }

        return false;
    }

    auto Tracker::draw_jungler_tracker( ) -> void{
        if ( !g_config->awareness.missing_indicator_enabled->get< bool >( ) || m_enemy_jungler.index == 0 ) return;

        //g_config->awareness.missing_indicator_x->get< int >( ) = 200.f;
        //g_config->awareness.missing_indicator_y->get< int >( ) = 250.f;

        const auto foundation_position =
            Vec2{
                static_cast< float >( g_config->awareness.missing_indicator_x->get< int >( ) ),
                static_cast< float >( g_config->awareness.missing_indicator_y->get< int >( ) )
            };

        const Vec2 instance_size{ 275.f, 125.f };
        const Vec2 texture_size       = { 80.f, 80.f };
        const Vec2 event_texture_size = { 25.f, 25.f };

        constexpr auto font_size       = 35;
        constexpr auto event_font_size = 20;

        if ( !m_repositioning && GetAsyncKeyState( 0x01 ) ) {
            auto cursor = g_input->get_cursor_position( );

            m_repositioning = true;

            if ( cursor.x >= foundation_position.x && cursor.x <= foundation_position.x + instance_size.x &&
                cursor.y >= foundation_position.y && cursor.y <= foundation_position.y + instance_size.y ) {
                m_reposition_start = cursor;
                m_ignore_changes   = false;
            } else m_ignore_changes = true;
        } else if ( m_repositioning ) {
            if ( !GetAsyncKeyState( 0x01 ) ) { m_repositioning = false; } else if ( !m_ignore_changes ) {
                auto cursor = g_input->get_cursor_position( );

                auto delta = Vec2( cursor.x - m_reposition_start.x, cursor.y - m_reposition_start.y );

                g_config->awareness.missing_indicator_x->get< int >( ) += delta.x;
                g_config->awareness.missing_indicator_y->get< int >( ) += delta.y;

                m_reposition_start = cursor;
            }
        }


        g_render->filled_box(
            foundation_position + Vec2( 1.f, 1.f ),
            instance_size + Vec2( 2.f, 2.f ),
            Color( 0, 0, 0, 210 ),
            -1
        );
        g_render->filled_box( foundation_position, instance_size, Color( 30, 30, 30, 215 ), -1 );
        g_render->box( foundation_position, instance_size, Color( 70, 70, 70, 255 ), -1, 3.f );
        //g_render->filled_box( foundation_position, { total_width, 8.f }, Color( 255, 255, 255, 255 ), -1 );

        const Vec2 base_position = { foundation_position.x + 3.f, foundation_position.y + 3.f };
        const auto health_color  = Color( 15, 200, 15, 255 );


        auto unit = g_entity_list.get_by_index( m_enemy_jungler.index );
        if ( !unit || !unit->is_hero( ) ) return;


        const std::string champ_name = unit->champion_name.text;
        auto              texture    = g_render->load_texture_from_file(
            path::join(
                { directory_manager::get_resources_path( ), "champions", champ_name, champ_name + "_square.png" }
            ).value( )
        );

        g_render->image( base_position, texture_size, texture );

        if ( unit->is_dead( ) ) {
            g_render->filled_box( base_position, texture_size, Color( 0, 0, 0, 75 ), -1 );

            std::string text      = _( "DEAD" );
            auto        text_size = g_render->get_text_size( text, g_fonts->get_zabel( ), font_size * 0.8f );

            Vec2 text_position = Vec2(
                base_position.x + texture_size.x / 2.f - text_size.x / 2.f,
                base_position.y + texture_size.y / 2.f - text_size.y / 2.f
            );

            g_render->filled_box(
                Vec2( text_position.x - texture_size.x / 2.f + text_size.x / 2.f, text_position.y ),
                Vec2( texture_size.x, text_size.y ),
                Color( 0, 0, 0, 200 ),
                -1
            );

            g_render->text_shadow(
                text_position,
                Color::white( ),
                g_fonts->get_zabel( ),
                text.data( ),
                font_size * 0.8f
            );
        } else {
            if ( unit->is_invisible( ) ) {
                g_render->filled_box( base_position, texture_size, Color( 0, 0, 0, 75 ), -1 );

                auto data = get_last_seen_data( m_enemy_jungler.index );
                if ( data ) {
                    auto time_missing = *g_time - data->last_seen_time;

                    auto timer = std::to_string( static_cast< int >( std::ceil( time_missing ) ) );
                    if ( timer.size( ) > 2 ) timer = _( "99+" );

                    auto timer_size = g_render->get_text_size( timer, g_fonts->get_zabel( ), font_size );

                    auto timer_position = Vec2(
                        base_position.x + texture_size.x / 2.f - timer_size.x / 2.f,
                        base_position.y + texture_size.y / 2.f - timer_size.y / 2.f
                    );

                    g_render->text_shadow(
                        timer_position,
                        Color::white( ),
                        g_fonts->get_zabel( ),
                        timer.data( ),
                        font_size
                    );
                }
            } else {
            }

            auto bar_width = 14.f;
            auto spacing   = 2.f;
            Vec2 hp_start  = { base_position.x, base_position.y + texture_size.x };

            hp_start.y -= bar_width + spacing;
            hp_start.x += spacing;

            auto modifier   = unit->health / unit->max_health;
            auto bar_length = texture_size.x * modifier - spacing * 2.f;
            if ( bar_length < 3.f ) bar_length = 3.f;

            Vec2 bar_size = { bar_length, bar_width };

            // 245, 78, 66
            g_render->filled_box(
                hp_start,
                { texture_size.x - spacing * 2.f, bar_width },
                Color( 0, 0, 0, 170 )
            );

            //g_render->filled_box( hp_start, { bar_length, bar_width }, Color( 10, 10, 10, 90 ) );
            g_render->filled_box( hp_start, bar_size, health_color );

            /// emboss effect
            g_render->line(
                { hp_start.x - 1.f, hp_start.y + bar_width - 1.f },
                { hp_start.x + bar_length + 1.f, hp_start.y + bar_width - 1.f },
                Color( 0, 0, 0, 125 ),
                5.f
            );


            if ( *g_time - m_enemy_jungler.last_cast_time < 30.f ) {
                auto event_position =
                    Vec2(
                        foundation_position.x + instance_size.x - event_texture_size.x - spacing * 2.f,
                        foundation_position.y + instance_size.y - event_texture_size.y - spacing
                    );

                auto event_texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "event_yellow.png" } ).value( )
                );
                if ( event_texture ) {
                    g_render->image( event_position, event_texture_size, event_texture );

                    auto time_since_event = *g_time - m_enemy_jungler.last_cast_time;
                    auto timer            = std::to_string( time_since_event );
                    timer.resize( 4 );

                    auto dummy_text = "30.0";

                    auto timer_size = g_render->get_text_size( dummy_text, g_fonts->get_zabel( ), event_font_size );

                    g_render->text_shadow(
                        Vec2(
                            event_position.x - timer_size.x,
                            event_position.y + event_texture_size.y / 2.f - timer_size.y / 2.f
                        ),
                        Color( 255, 220, 0, 255 ),
                        g_fonts->get_zabel( ),
                        timer.data( ),
                        event_font_size
                    );
                }
            }
        }
    }

    auto Tracker::initialize_jungler_tracking( ) -> void{
        if ( !m_initialized_jungler_tracking ) {
            //if ( m_enemies.size( ) < 5 ) return;

            const auto index = g_features->activator->get_enemy_jungler_index( );
            if ( !index ) return;

            const auto hero = g_entity_list.get_by_index( index.value( ) );
            if ( !hero ) return;

            m_enemy_jungler = MissingInstance{ hero->index, hero->network_id, *g_time, false, 0.f, 0.f, 0 };
            m_initialized_jungler_tracking = true;

#if __DEBUG
            std::cout << "[ forester tracker ] Initialized forester: " << hero->get_name( ) << std::endl;
#endif
        }

        const auto enemy = g_entity_list.get_by_index( m_enemy_jungler.index );
        if ( !enemy ) return;
        if ( enemy->is_visible( ) ) {
            m_enemy_jungler.last_cast_time  = 0.f;
            m_enemy_jungler.last_cast_value = enemy->get_spellcast_state( );
            return;
        }

        const auto spellcast_state = enemy->get_spellcast_state( );
        if ( spellcast_state != m_enemy_jungler.last_cast_value ) {
            m_enemy_jungler.last_cast_time  = *g_time;
            m_enemy_jungler.last_cast_value = spellcast_state;
        }
    }

    auto Tracker::update_experience_tracker( ) -> void{
        if ( g_config->awareness.show_experience_mode->get< int >( ) == 0 ) return;

        for ( auto& inst : m_experience ) {
            auto unit = g_entity_list.get_by_index( inst.index );
            if ( !unit ) continue;

            unit.update( );

            if ( inst.initialized && unit->level == inst.level && inst.experience < unit->get_experience( ) ) {
                inst.gain_amount          = unit->get_experience( ) - inst.experience;
                inst.experience_gain_time = *g_time;
            } else if ( *g_time - inst.experience_gain_time > 1.f ) inst.gain_amount = 0.f;

            inst.experience = unit->get_experience( );

            if ( unit->level == inst.level && inst.initialized ) continue;

            inst.level = unit->level;

            const auto next_level = inst.level + 1;
            auto       level_threshold{ 0.f };
            switch ( next_level ) {
            case 2:
                level_threshold = 280.f;
                break;
            case 3:
                level_threshold = 660.f;
                break;
            case 4:
                level_threshold = 1140.f;
                break;
            case 5:
                level_threshold = 1720.f;
                break;
            case 6:
                level_threshold = 2400.f;
                break;
            case 7:
                level_threshold = 3180.f;
                break;
            case 8:
                level_threshold = 4060.f;
                break;
            case 9:
                level_threshold = 5040.f;
                break;
            case 10:
                level_threshold = 6120.f;
                break;
            case 11:
                level_threshold = 7300.f;
                break;
            case 12:
                level_threshold = 8580.f;
                break;
            case 13:
                level_threshold = 9960.f;
                break;
            case 14:
                level_threshold = 11440.f;
                break;
            case 15:
                level_threshold = 13020.f;
                break;
            case 16:
                level_threshold = 14700.f;
                break;
            case 17:
                level_threshold = 16480.f;
                break;
            case 18:
                level_threshold = 18360.f;
                break;
            default:
                break;
            }

            inst.required_experience_amount = level_threshold;
            inst.experience_cap             = 80.f + ( unit->level + 1 ) * 100.f;

            inst.gain_amount          = unit->get_experience( ) - ( level_threshold - inst.experience_cap );
            inst.experience_gain_time = *g_time;

            inst.initialized = true;
        }
    }

    auto Tracker::draw_experience_tracker( ) const -> void{
        if ( g_config->awareness.show_experience_mode->get< int >( ) == 0 || g_features->orbwalker->get_mode( ) ==
            Orbwalker::EOrbwalkerMode::combo )
            return;

        const auto enemies_only = g_config->awareness.show_experience_mode->get< int >( ) == 1;

        for ( const auto inst : m_experience ) {
            const auto unit = g_entity_list.get_by_index( inst.index );
            if ( !unit || enemies_only && unit->team == g_local->team || unit->is_dead( ) || unit->
                is_invisible( ) )
                continue;

            Vec2 sp{ };
            if ( !world_to_screen( unit->position, sp ) ) continue;

            constexpr auto base_x = 1920.f;
            constexpr auto base_y = 1080.f;

            const auto width_ratio  = base_x / static_cast< float >( g_render_manager->get_width( ) );
            const auto height_ratio = base_y / static_cast< float >( g_render_manager->get_height( ) );

            const auto width_offset  = width_ratio * 0.056f;
            const auto height_offset = height_ratio * 0.0222f;

            const auto bar_length = width_offset * static_cast< float >( g_render_manager->get_width( ) );
            const auto bar_height = height_offset * static_cast< float >( g_render_manager->get_height( ) );

            auto base_position = unit->get_hpbar_position( );

            base_position.x -= bar_length * 0.45f;
            base_position.y -= bar_height * 1.225f;

            const auto max_width = bar_length;
            const auto modifier  = std::clamp(
                ( unit->get_experience( ) - ( inst.required_experience_amount - inst.experience_cap ) - inst.
                    gain_amount ) / inst.experience_cap,
                0.f,
                1.f
            );
            const auto gain_modifier = inst.gain_amount > 0.f
                                           ? std::clamp( inst.gain_amount / inst.experience_cap, 0.f, 1.f )
                                           : 0.f;
            auto animation_modifier = inst.gain_amount > 0.f
                                          ? std::clamp( ( *g_time - inst.experience_gain_time ) / 0.8f, 0.f, 1.f )
                                          : 1.f;
            const auto movement_modifier = utils::ease::ease_out_expo(
                inst.gain_amount > 0.f ? std::clamp( ( *g_time - inst.experience_gain_time ) / 0.65f, 0.f, 1.f ) : 1.f
            );
            animation_modifier = utils::ease::ease_in_quad( animation_modifier );


            auto       experience_color = Color( 130, 10, 250 );
            const Vec2 baseline_end     = { base_position.x + max_width * modifier, base_position.y };

            g_render->line(
                base_position,
                { base_position.x + max_width, base_position.y },
                Color( 10, 10, 10, 130 ),
                4.f
            );
            g_render->line( base_position, baseline_end, experience_color, 4.f );

            if ( inst.gain_amount > 0.f ) {
                const Color max_pulse_value = {
                    std::max( 255 - experience_color.r, 0 ),
                    std::max( 255 - experience_color.g, 0 ),
                    std::max( 255 - experience_color.b, 0 )
                };

                const auto pulse_modifier = 1.f - animation_modifier;

                const Color pulse_amount = {
                    static_cast< int32_t >( max_pulse_value.r * pulse_modifier ),
                    static_cast< int32_t >( max_pulse_value.g * pulse_modifier ),
                    static_cast< int32_t >( max_pulse_value.b * pulse_modifier )
                };

                auto animated_color = experience_color;
                animated_color.r += pulse_amount.r;
                animated_color.g += pulse_amount.g;
                animated_color.b += pulse_amount.b;
                animated_color.a = 255;

                const auto movement_amount = max_width - ( baseline_end.x - base_position.x ) - max_width *
                    gain_modifier;
                const Vec2 gained_start = {
                    baseline_end.x + ( movement_amount - movement_amount * movement_modifier ),
                    base_position.y
                };
                const Vec2 gained_end = {
                    gained_start.x + max_width * gain_modifier * ( 3.f - 2.f * movement_modifier ),
                    base_position.y
                };

                g_render->line( gained_start, gained_end, animated_color, 4.f );
            }
        }
    }

    auto Tracker::draw_ward_tracker( ) -> void{
        if ( !g_config->awareness.ward_tracker->get< bool >( ) ) return;

        std::vector< sdk::math::Polygon > poly_list{ };

        for ( auto& inst : m_wards ) {
            Vec2  sp;
            auto& ward = g_entity_list.get_by_index( inst.index );
            if ( !ward ) continue;

            ward.update( );

            const auto type = ward->get_ward_type( );
            if ( type == Object::EWardType::unknown || ward->is_dead( ) ) continue;

            /* std::cout << std::endl << std::endl;
             for ( auto buff : ward->buff_manager.get_all( ) ) {
                if ( !buff || !buff->get_buff_data(  ) || !buff->get_buff_data( )->get_buff_info( ) ) continue;

                debug_log( "Buff: {} STACKS: {}  TYPE: {}  END: {}",
                           buff->get_buff_data( )->get_buff_info( )->name,
                           buff->get_buff_data( )->stack,
                           ( int )buff->get_buff_data( )->type,
                           buff->get_buff_data( )->end_time );
            }*/


            const auto     on_screen = sdk::math::world_to_screen( ward->position, sp );
            const auto     minimap   = sdk::math::world_to_minimap( ward->position );
            constexpr auto ward_size{ 4 };

            if ( inst.does_expire && !inst.was_updated ) {
                switch ( type ) {
                default:
                    inst.expire_time = *g_time + ward->mana + 1.f;
                    inst.was_updated = true;
                    break;
                }
            }

            constexpr auto glow_diffuse = 2;
            constexpr auto glow_size    = 4;

            if ( inst.is_visible && !ward->is_visible( ) ) {
                inst.was_visible       = true;
                inst.is_visible        = false;
                inst.last_visible_time = *g_time;
            } else if ( !inst.is_visible && ward->is_visible( ) ) {
                inst.is_visible        = true;
                inst.last_visible_time = *g_time;
            }

            if ( !inst.was_pinged ) {
                switch ( type ) {
                case Object::EWardType::control:
                    if ( !ward->is_visible( ) && !inst.was_visible ) break;
                case Object::EWardType::normal:
                case Object::EWardType::blue:
                case Object::EWardType::zombie:

                    if ( g_features->activator->is_ward_pingable( ward->network_id ) ) {
                        debug_log( "[ WARD ] Added pingable ward {}", ward->get_name( ) );
                        g_features->activator->add_pingable_ward( ward->index, ward->network_id );
                    }

                    inst.was_pinged = true;
                    break;
                default:
                    inst.was_pinged = true;
                    break;
                }
            }

            auto scale = g_config->awareness.ward_scale->get< int >( ) / 100.f;
            Vec2 texture_scale{ 28.f * scale, 28.f * scale };
            auto font_scale{ 20.f * scale };

            auto small_font_scale{ 18.f * scale };
            Vec2 small_texture_scale{ 20.f * scale, 20.f * scale };

            const auto draw_timer =
                [ & ](
                const std::shared_ptr< Renderer::Texture >& texture,
                const Vec2                                  texture_area,
                const float                                 font_size
            ) -> void{
                const auto time_to_death = inst.expire_time - *g_time;
                const auto multiplier = std::clamp( time_to_death / ( inst.expire_time - inst.start_time ), 0.f, 1.f );

                auto bar_color = Color(
                    255.f - 255.f * std::clamp( ( multiplier - 0.5f ) / 0.5f, 0.f, 1.f ),
                    255.f * std::clamp( multiplier / 0.5f, 0.f, 1.f ),
                    0.f
                );

                constexpr float bar_height = 0;


                const auto texture_size = texture_area;

                const int minutes_left = std::floor( ( inst.expire_time - *g_time + 1.f ) / 60.f );
                const int seconds_left = std::ceil( ( inst.expire_time - *g_time ) - minutes_left * 60.f );

                auto text         = " " + std::to_string( static_cast< int >( minutes_left ) ) + ":";
                auto seconds_text = std::to_string( static_cast< int >( seconds_left ) );
                if ( seconds_left < 10 ) seconds_text = "0" + std::to_string( static_cast< int >( seconds_left ) );

                text += seconds_text;

                const std::string dummy_text = " 0:60.";

                const auto timer_size = g_render->get_text_size( text, g_fonts->get_zabel( ), font_size );
                const auto dummy_size = g_render->get_text_size( dummy_text, g_fonts->get_zabel( ), font_size );

                const Vec2 background_size = { texture_size.x + dummy_size.x, texture_size.y + bar_height };

                Vec2 texture_position = { sp.x - background_size.x / 2.f, sp.y - texture_size.y / 2.f };

                const Vec2 countdown_text_position = { texture_position.x + texture_size.x, sp.y - timer_size.y / 2.f };

                Vec2 progress_position = { texture_position.x, texture_position.y + texture_size.y };
                Vec2 progress_size     = { background_size.x * multiplier, bar_height };

                if ( *g_time - inst.detection_time <= 0.75f ) {
                    Vec2 animated_size = { texture_size.x * 1.5f, texture_size.y * 1.5f };

                    if ( *g_time - inst.detection_time <= 0.25f ) {
                        texture_position = { sp.x - animated_size.x / 2.f, sp.y - animated_size.y / 2.f };

                        g_render->image( texture_position, animated_size, texture );

                        g_render->box( texture_position, animated_size, Color( 255, 255, 255, 255 ), -1, 2.f * scale );
                    } else {
                        auto modifier = std::clamp( ( *g_time - inst.detection_time - 0.25f ) / 0.5f, 0.f, 1.f );
                        modifier      = utils::ease::ease_out_quint( modifier );

                        animated_size.x -= texture_size.x / 2.f * modifier;
                        animated_size.y -= texture_size.y / 2.f * modifier;

                        const Vec2 current_position = { sp.x - animated_size.x / 2.f, sp.y - animated_size.y / 2.f };

                        const auto difference = current_position.x - texture_position.x;

                        const Vec2 animated_position = {
                            current_position.x - difference * modifier,
                            sp.y - animated_size.y / 2.f
                        };

                        const auto animation_size = background_size.x - animated_size.x;


                        g_render->filled_box(
                            animated_position,
                            { animated_size.x + animation_size * modifier, animated_size.y + bar_height * modifier },
                            Color( 0.f, 0.f, 0.f, 175.f * modifier ),
                            -1
                        );

                        g_render->image( animated_position, animated_size, texture );


                        auto animated_text_position = countdown_text_position;

                        animated_text_position.x += dummy_size.x - dummy_size.x * modifier;

                        g_render->text_shadow(
                            animated_text_position,
                            Color( 255.f, 255.f, 255.f, 255.f * modifier ),
                            g_fonts->get_zabel( ),
                            text.c_str( ),
                            font_size
                        );

                        /* Vec2 current_progress_position = { texture_position.x,
                                                            animated_position.y + animated_size.y };

                         current_progress_position.y += bar_height - bar_height * modifier;

                         g_render->filled_box(
                             current_progress_position,
                             { progress_size.x, progress_size.y },
                             Color( bar_color.r, bar_color.g, bar_color.b, 255 * modifier ),
                             -1 );*/

                        g_render->box(
                            animated_position,
                            animated_size,
                            Color(
                                255 - 255 * modifier,
                                255 - 255 * modifier,
                                255 - 255 * modifier,
                                255 - 255 * modifier
                            ),
                            -1,
                            2.f * scale
                        );
                    }
                } else {
                    g_render->filled_box( texture_position, background_size, Color( 0, 0, 0, 175 ), -1 );

                    g_render->image( texture_position, texture_size, texture );

                    g_render->text_shadow(
                        countdown_text_position,
                        Color::white( ),
                        g_fonts->get_zabel( ),
                        text.c_str( ),
                        font_size
                    );

                    // g_render->filled_box( progress_position, progress_size, bar_color, -1 );


                    if ( false ) {
                        Vec2 image_position = {
                            progress_position.x + progress_size.x,
                            progress_position.y + progress_size.y / 2.f
                        };
                        Vec2 image_size = { bar_height * 2.f, bar_height * 2.f };

                        if ( image_position.x + image_size.x / 2.f >= texture_position.x + background_size.
                            x )
                            image_position.x = texture_position.x + background_size.x - image_size.x / 2.f;

                        g_render->image(
                            { image_position.x - image_size.x / 2.f, image_position.y - image_size.y / 2.f },
                            image_size,
                            texture
                        );
                    }
                }
            };

            const auto                                      draw_permanent_timer = [ & ](
                const std::shared_ptr< Renderer::Texture >& texture,
                Vec2                                        texture_area,
                const float                                 font_size,
                const bool                                  is_controlward
            ) -> void{
                const auto texture_size{ texture_scale };

                const std::string text = " --:-- ";

                auto end_color = is_controlward ? Color::red( ) : Color( 55, 105, 210 );

                const auto timer_size = g_render->get_text_size( text + ".", g_fonts->get_zabel( ), font_size );

                const Vec2 background_size = { texture_size.x + timer_size.x, texture_size.y };

                Vec2 texture_position = { sp.x - background_size.x / 2.f, sp.y - texture_size.y / 2.f };

                const Vec2 countdown_text_position = { texture_position.x + texture_size.x, sp.y - timer_size.y / 2.f };

                if ( *g_time - inst.detection_time <= 1.5f ) {
                    Vec2 animated_size = { texture_size.x * 1.5f, texture_size.y * 1.5f };

                    if ( *g_time - inst.detection_time <= 0.5f ) {
                        texture_position = { sp.x - animated_size.x / 2.f, sp.y - animated_size.y / 2.f };

                        g_render->image( texture_position, animated_size, texture );

                        g_render->box( texture_position, animated_size, Color( 255, 255, 255, 255 ), -1, 2.f * scale );
                    } else {
                        auto modifier = std::clamp( ( *g_time - inst.detection_time - 0.5f ) / 0.8f, 0.f, 1.f );
                        modifier      = utils::ease::ease_out_quint( modifier );

                        auto late_modifier = std::clamp( ( *g_time - inst.detection_time - 0.7f ) / 0.75f, 0.f, 1.f );
                        late_modifier      = utils::ease::ease_out_expo( late_modifier );

                        animated_size.x -= texture_size.x / 2.f * modifier;
                        animated_size.y -= texture_size.y / 2.f * modifier;

                        const Vec2 current_position = { sp.x - animated_size.x / 2.f, sp.y - animated_size.y / 2.f };

                        const auto difference     = current_position.x - texture_position.x;
                        const auto animation_size = background_size.x - animated_size.x;

                        const Vec2 animated_position = {
                            current_position.x - difference * modifier,
                            sp.y - animated_size.y / 2.f
                        };


                        g_render->filled_box(
                            animated_position,
                            { animated_size.x + animation_size * modifier, animated_size.y },
                            Color( 0.f, 0.f, 0.f, 175.f * modifier ),
                            -1
                        );

                        g_render->image( animated_position, animated_size, texture );


                        const Vec2 animated_text_position = {
                            animated_position.x + animated_size.x,
                            animated_position.y + animated_size.y / 2.f -
                            timer_size.y / 2.f
                        };

                        // float diff = animated_position.x - texture_position.x;

                        // animated_text_position.x += diff;

                        g_render->text_shadow(
                            animated_text_position,
                            Color(
                                end_color.r,
                                end_color.g,
                                end_color.b,
                                static_cast< int32_t >(
                                    255.f * late_modifier
                                )
                            ),
                            g_fonts->get_zabel( ),
                            text.c_str( ),
                            font_size
                        );

                        /* Vec2 current_progress_position = { texture_position.x,
                                                            animated_position.y + animated_size.y };

                         current_progress_position.y += bar_height - bar_height * modifier;

                         g_render->filled_box( current_progress_position,
                                               { progress_size.x, progress_size.y },
                                               color( 252, 219, 3, 255 * modifier ),
                                               -1 );*/

                        g_render->box(
                            animated_position,
                            { animated_size.x + animation_size * modifier, animated_size.y },
                            Color( 255.f, 255.f, 255.f, 255.f - 255.f * modifier ),
                            -1,
                            2.f * scale
                        );

                        g_render->box(
                            animated_position,
                            { animated_size.x + animation_size * modifier, animated_size.y },
                            end_color.alpha( 255 * modifier ),
                            -1,
                            2.f * scale
                        );
                    }
                } else {
                    g_render->filled_box( texture_position, background_size, Color( 0, 0, 0, 175 ), -1 );

                    g_render->image( texture_position, texture_size, texture );

                    g_render->box( texture_position, background_size, end_color, -1, 2.f * scale );

                    g_render->text_shadow(
                        countdown_text_position,
                        end_color,
                        g_fonts->get_zabel( ),
                        text.c_str( ),
                        font_size
                    );
                }
            };

            switch ( type ) {
            case Object::EWardType::control:
            {
                auto texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "jammer.png" } ).value( )
                );

                if ( on_screen ) {
                    if ( g_config->awareness.show_ward_range->get< bool >( ) ) {
                        // g_render->polygon_3d( inst.vision_polygon, Color( 255, 255, 255, 75 ), Renderer::outline, 1
                        // );

                        poly_list.push_back( inst.vision_polygon );
                    }

                   // std::cout << "ward: " << std::hex << ward.get_address() << std::endl;
                    const auto owner = g_entity_list.get_by_index( ward->get_owner_index( ) );
                    if ( owner ) {
                        std::string champ_name    = owner->champion_name.text;
                        auto        owner_texture =
                            g_render->load_texture_from_file(
                                path::join(
                                    {
                                        directory_manager::get_resources_path( ),
                                        "champions",
                                        champ_name,
                                        champ_name + "_square.png"
                                    }
                                ).value( )
                            );

                        draw_permanent_timer( owner_texture, texture_scale, font_scale, true );
                    }
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 40, 40 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                if ( texture ) g_render->image( { minimap.x - 16.f, minimap.y - 16.f }, { 32.f, 32.f }, texture );
                else g_render->filled_circle( minimap, Color::red( ), ward_size );

                break;
            }
            case Object::EWardType::blue:
            {
                auto texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "blue_ward.png" } ).value( )
                );

                if ( on_screen ) {
                    if ( g_config->awareness.show_ward_range->get< bool >( ) )
                        poly_list.push_back(
                            inst.vision_polygon
                        );

                    const auto owner = g_entity_list.get_by_index( ward->get_owner_index( ) );
                    if ( owner ) {
                        std::string champ_name    = owner->champion_name.text;
                        auto        owner_texture =
                            g_render->load_texture_from_file(
                                path::join(
                                    {
                                        directory_manager::get_resources_path( ),
                                        "champions",
                                        champ_name,
                                        champ_name + "_square.png"
                                    }
                                ).value( )
                            );

                        draw_permanent_timer( owner_texture, texture_scale, font_scale, false );
                    }
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 3, 102, 252 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }


                if ( texture ) g_render->image( { minimap.x - 16.f, minimap.y - 16.f }, { 32.f, 32.f }, texture );
                else g_render->filled_circle( minimap, Color( 40, 89, 250, 255 ), ward_size );

                break;
            }
            case Object::EWardType::normal:
            {
                auto texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "disabled_ward.png" } ).value( )
                );
                auto map_texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "ward.png" } ).value( )
                );

                if ( on_screen ) {
                    if ( g_config->awareness.show_ward_range->get< bool >( ) )
                        poly_list.push_back(
                            inst.vision_polygon
                        );

                    auto owner = g_entity_list.get_by_index( ward->get_owner_index( ) );

                    if ( owner && inst.expire_time > *g_time ) {
                        auto time_to_death = inst.expire_time - *g_time;
                        auto multiplier    =
                            std::clamp( time_to_death / ( inst.expire_time - inst.start_time ), 0.f, 1.f );

                        auto bar_color = Color(
                            255.f - 255.f * std::clamp( ( multiplier - 0.5f ) / 0.5f, 0.f, 1.f ),
                            255.f * std::clamp( multiplier / 0.5f, 0.f, 1.f ),
                            0.f
                        );

                        auto bar_height = texture_scale.x / 3.5f;

                        std::string champ_name    = owner->champion_name.text;
                        auto        owner_texture =
                            g_render->load_texture_from_file(
                                path::join(
                                    {
                                        directory_manager::get_resources_path( ),
                                        "champions",
                                        champ_name,
                                        champ_name + "_square.png"
                                    }
                                ).value( )
                            );
                        if ( owner_texture ) {
                            auto texture_size{ texture_scale };

                            int minutes_left = std::floor( ( inst.expire_time - *g_time + 1.f ) / 60.f );
                            int seconds_left = std::ceil( ( inst.expire_time - *g_time ) - minutes_left * 60.f );

                            auto text         = " " + std::to_string( static_cast< int >( minutes_left ) ) + ":";
                            auto seconds_text = std::to_string( static_cast< int >( seconds_left ) );
                            if ( seconds_left < 10 )
                                seconds_text = "0" + std::to_string(
                                    static_cast< int >( seconds_left )
                                );

                            text += seconds_text;

                            std::string dummy_text = " 0:60:";

                            auto timer_size = g_render->get_text_size( text, g_fonts->get_zabel( ), font_scale );
                            auto dummy_size = g_render->get_text_size( dummy_text, g_fonts->get_zabel( ), font_scale );

                            Vec2 background_size = { texture_size.x + dummy_size.x, texture_size.y + bar_height };

                            Vec2 texture_position = { sp.x - background_size.x / 2.f, sp.y - texture_size.y / 2.f };

                            Vec2 countdown_text_position = {
                                texture_position.x + texture_size.x,
                                sp.y - timer_size.y / 2.f
                            };

                            Vec2 progress_position = { texture_position.x, texture_position.y + texture_size.y };
                            Vec2 progress_size     = { background_size.x * multiplier, bar_height };

                            if ( *g_time - inst.detection_time <= 1.5f ) {
                                Vec2 animated_size = { texture_size.x * 1.5f, texture_size.y * 1.5f };

                                if ( *g_time - inst.detection_time <= 0.5f ) {
                                    texture_position = { sp.x - animated_size.x / 2.f, sp.y - animated_size.y / 2.f };

                                    g_render->image( texture_position, animated_size, owner_texture );

                                    g_render->box(
                                        texture_position,
                                        animated_size,
                                        Color( 255, 255, 255, 255 ),
                                        -1,
                                        2.f * scale
                                    );
                                } else {
                                    auto modifier =
                                        std::clamp( ( *g_time - inst.detection_time - 0.5f ) / 0.8f, 0.f, 1.f );
                                    modifier = utils::ease::ease_out_quint( modifier );

                                    animated_size.x -= texture_size.x / 2.f * modifier;
                                    animated_size.y -= texture_size.y / 2.f * modifier;

                                    Vec2 current_position = {
                                        sp.x - animated_size.x / 2.f,
                                        sp.y - animated_size.y / 2.f
                                    };

                                    auto difference = current_position.x - texture_position.x;

                                    Vec2 animated_position = {
                                        current_position.x - difference * modifier,
                                        sp.y - animated_size.y / 2.f
                                    };

                                    auto animation_size = background_size.x - animated_size.x;


                                    g_render->filled_box(
                                        animated_position,
                                        {
                                            animated_size.x + animation_size * modifier,
                                            animated_size.y + bar_height * modifier
                                        },
                                        Color( 0.f, 0.f, 0.f, 175.f * modifier ),
                                        -1
                                    );

                                    g_render->image( animated_position, animated_size, owner_texture );


                                    auto animated_text_position = countdown_text_position;

                                    animated_text_position.x += dummy_size.x - dummy_size.x * modifier;

                                    g_render->text_shadow(
                                        animated_text_position,
                                        Color( 255.f, 255.f, 255.f, 255.f * modifier ),
                                        g_fonts->get_zabel( ),
                                        text.c_str( ),
                                        font_scale
                                    );

                                    Vec2 current_progress_position = {
                                        texture_position.x,
                                        animated_position.y + animated_size.y
                                    };

                                    current_progress_position.y += bar_height - bar_height * modifier;

                                    g_render->filled_box(
                                        current_progress_position,
                                        { progress_size.x, progress_size.y },
                                        Color(
                                            bar_color.r,
                                            bar_color.g,
                                            bar_color.b,
                                            static_cast< int32_t >(
                                                255.f * modifier
                                            )
                                        ),
                                        -1
                                    );

                                    g_render->box(
                                        animated_position,
                                        animated_size,
                                        Color(
                                            255 - 255 * modifier,
                                            255 - 255 * modifier,
                                            255 - 255 * modifier,
                                            255 - 255 * modifier
                                        ),
                                        -1,
                                        2.f * scale
                                    );
                                }
                            } else {
                                g_render->filled_box( texture_position, background_size, Color( 0, 0, 0, 175 ), -1 );

                                g_render->image( texture_position, texture_size, owner_texture );

                                g_render->text_shadow(
                                    countdown_text_position,
                                    Color::white( ),
                                    g_fonts->get_zabel( ),
                                    text.c_str( ),
                                    font_scale
                                );

                                g_render->filled_box( progress_position, progress_size, bar_color, -1 );


                                if ( false ) {
                                    Vec2 image_position = {
                                        progress_position.x + progress_size.x,
                                        progress_position.y + progress_size.y / 2.f
                                    };
                                    Vec2 image_size = { bar_height * 2.f, bar_height * 2.f };

                                    if ( image_position.x + image_size.x / 2.f >=
                                        texture_position.x + background_size.x )
                                        image_position.x = texture_position.x + background_size.x - image_size.x / 2.f;

                                    g_render->image(
                                        {
                                            image_position.x - image_size.x / 2.f,
                                            image_position.y - image_size.y / 2.f
                                        },
                                        image_size,
                                        texture
                                    );
                                }
                            }
                        }
                    }


                    // if ( texture ) g_render->image( { sp.x - 22.f, sp.y - 32.f }, { 44.f, 44.f }, texture );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 244, 252, 3 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }


                if ( map_texture )
                    g_render->image(
                        { minimap.x - 16.f, minimap.y - 16.f },
                        { 32.f, 32.f },
                        map_texture
                    );
                else g_render->filled_circle( minimap, Color( 255, 255, 0, 255 ), ward_size );
                break;
            }
            case Object::EWardType::zombie:
            {
                auto texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "zombie_ward.png" } ).value( )
                );
                auto map_texture = g_render->load_texture_from_file(
                    path::join( { directory_manager::get_resources_path( ), "common", "ward.png" } ).value( )
                );

                if ( on_screen ) {
                    if ( g_config->awareness.show_ward_range->get< bool >( ) )
                        poly_list.push_back(
                            inst.vision_polygon
                        );

                    if ( inst.expire_time > *g_time ) {
                        auto time_to_death = inst.expire_time - *g_time;
                        auto multiplier    = time_to_death / ( inst.expire_time - inst.start_time );

                        auto bar_width = 40.f;

                        Vec2 bar_origin{ sp.x - bar_width / 2.f, sp.y };
                        Vec2 bar_size{ bar_width, 5.f };

                        Vec2 progress_origin{ bar_origin.x + 1.f, bar_origin.y + 1.f };
                        Vec2 progress_size{ ( bar_width - 2.f ) * multiplier, 3.f };

                        g_render->filled_box( bar_origin, bar_size, Color( 5, 5, 5 ), 0 );
                        g_render->filled_box( progress_origin, progress_size, Color( 255, 255, 255 ), 0 );

                        auto timer_text =
                            std::to_string( static_cast< int >( std::ceil( inst.expire_time - *g_time ) ) );

                        Vec2 timer_position = { bar_origin.x + bar_width / 2.f, bar_origin.y + 13.f };
                        auto timer_size     = g_render->get_text_size( timer_text, g_fonts->get_default_bold( ), 16 );

                        g_render->text_shadow(
                            { timer_position.x - timer_size.x / 2.f, timer_position.y - timer_size.y / 2.f },
                            Color::white( ),
                            g_fonts->get_default_bold( ),
                            timer_text.c_str( ),
                            16
                        );
                    }

                    if ( texture ) g_render->image( { sp.x - 22.f, sp.y - 32.f }, { 44.f, 44.f }, texture );
                }

                if ( false && m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) &&
                    !inst.glow_enabled && g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 69, 252, 3 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }


                if ( map_texture )
                    g_render->image(
                        { minimap.x - 16.f, minimap.y - 16.f },
                        { 32.f, 32.f },
                        map_texture
                    );
                else g_render->filled_circle( minimap, Color( 252, 102, 3, 255 ), ward_size );
                break;
            }
            case Object::EWardType::teemo_shroom:
            {
                if ( on_screen ) {
                    auto texture = g_render->load_texture_from_file(
                        path::join(
                            { directory_manager::get_resources_path( ), "champions", "Teemo", "spells", "Teemo_r.png" }
                        ).value( )
                    );


                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::shaco_box:
            {
                if ( on_screen ) {
                    auto texture = g_render->load_texture_from_file(
                        path::join(
                            { directory_manager::get_resources_path( ), "champions", "Shaco", "spells", "Shaco_w.png" }
                        ).value( )
                    );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::jhin_trap:
            {
                if ( on_screen ) {
                    auto texture = g_render->load_texture_from_file(
                        path::join(
                            { directory_manager::get_resources_path( ), "champions", "Jhin", "spells", "Jhin_e.png" }
                        ).value( )
                    );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::nidalee_trap:
            {
                if ( on_screen ) {
                    auto texture =
                        g_render->load_texture_from_file(
                            path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    "Nidalee",
                                    "spells",
                                    "Nidalee_w.png"
                                }
                            ).value( )
                        );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::maokai_sproutling:
            {
                if ( on_screen ) {
                    auto texture =
                        g_render->load_texture_from_file(
                            path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    "Maokai",
                                    "spells",
                                    "Maokai_e.png"
                                }
                            ).value( )
                        );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::fiddlesticks_effigy:
            {
                if ( on_screen ) {
                    if ( g_config->awareness.show_ward_range->get< bool >( ) )
                        poly_list.push_back(
                            inst.vision_polygon
                        );

                    auto texture =
                        g_render->load_texture_from_file(
                            path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    "Fiddlesticks",
                                    "spells",
                                    "Fiddlesticks_passive.png"
                                }
                            ).value( )
                        );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            case Object::EWardType::caitlyn_trap:
            {
                if ( on_screen ) {
                    auto texture =
                        g_render->load_texture_from_file(
                            path::join(
                                {
                                    directory_manager::get_resources_path( ),
                                    "champions",
                                    "Caitlyn",
                                    "spells",
                                    "Caitlyn_w.png"
                                }
                            ).value( )
                        );

                    if ( inst.expire_time > *g_time ) draw_timer( texture, small_texture_scale, small_font_scale );
                }

                if ( m_glow_working && g_config->awareness.internal_glow_toggle->get< bool >( ) && !inst.glow_enabled &&
                    g_function_caller->is_glow_queueable( ) ) {
                    g_function_caller->enable_glow(
                        ward->network_id,
                        D3DCOLOR_ARGB( 255, 255, 20, 20 ),
                        0,
                        glow_size,
                        glow_diffuse
                    );
                    inst.glow_enabled = true;
                }

                break;
            }
            default:
                break;
            }
        }

        if ( poly_list.empty( ) ) return;

        auto draw_poly = Geometry::to_polygons( Geometry::clip_polygons( poly_list ) );

        for ( const auto poly : draw_poly ) {
            g_render->polygon_3d( poly, Color( 255, 255, 255, 80 ), Renderer::outline, 1.f );
        }
    }

    auto Tracker::draw_gank_alert( ) -> void{
        if ( !g_config->awareness.show_gank_alert->get< bool >( ) ) return;

        for ( const auto inst : m_ganks ) {
            if ( inst.end_time <= *g_time ) {
                remove_gank( inst.index );
                continue;
            }

            const auto hero = g_entity_list.get_by_index( inst.index );
            if ( !hero || hero->is_invisible( ) || hero->is_dead( ) ) {
                remove_gank( inst.index );
                continue;
            }

            if ( hero->dist_to_local( ) <= 500.f ) continue;

            auto range_modifier = std::clamp( ( *g_time - inst.start_time ) / 1.3f, 0.f, 1.f );
            range_modifier      = utils::ease::ease_out_circ( range_modifier );

            const auto opacity_modifier = std::clamp( ( *g_time - ( inst.start_time + 0.5f ) ) / 0.5f, 0.f, 1.f );

            const int outline_opacity = 255 - 255 * opacity_modifier;

            constexpr auto angle        = 90.f;
            constexpr auto rotate_angle = angle / 2.f * ( 3.14159265359f / 180.f );
            auto           draw_color   = Color( 255, 50, 50 );

            const auto wave_color = Color( 255, 50, 50, outline_opacity );

            const auto first_wave_range  = ( hero->dist_to_local( ) + 150.f ) * range_modifier;
            auto       second_wave_range = hero->dist_to_local( ) * range_modifier;

            g_render->circle_3d(
                hero->position,
                wave_color,
                first_wave_range,
                Renderer::outline,
                32,
                6.f,
                angle,
                ( g_local->position - hero->position ).rotated( -rotate_angle )
            );

            const auto line_length = 350.f +
                std::clamp( ( hero->dist_to_local( ) - 500.f ) / 3000.f, 0.f, 1.f ) * 400.f;

            Color alert_color{ 255, 0, 0 };

            //g_render->line_3d(
            //    g_local->position, g_local->position.extend( hero->position, line_length ), alert_color, 4.f );

            auto draw_position = g_local->position.extend( hero->position, line_length );

            const Vec2 texture_size{ 27.f, 27.f };

            Vec2 sp{ };
            if ( world_to_screen( draw_position, sp ) ) {
                std::string champ_name = hero->champion_name.text;
                auto        texture    = g_render->load_texture_from_file(
                    path::join(
                        {
                            directory_manager::get_resources_path( ),
                            "champions",
                            champ_name,
                            champ_name + "_square.png"
                        }
                    ).value( )
                );
                if ( !texture ) continue;

                const Vec2 texture_position{ sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                auto       distance_text = std::to_string( static_cast< int >( hero->dist_to_local( ) ) );
                auto       dummy         = distance_text + "..";
                const auto size          = g_render->get_text_size( dummy, g_fonts->get_zabel_16px( ), 16 );


                const Vec2 background_origin = { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                const Vec2 background_size   = { texture_size.x + size.x, texture_size.y };

                g_render->filled_box( background_origin, background_size, Color( 0, 0, 0, 175 ), -1 );

                if ( texture ) g_render->image( texture_position, texture_size, texture );
                const Vec2 text_position{ sp.x + texture_size.x * 0.55f, sp.y - size.y / 2.f };

                g_render->text_shadow(
                    text_position,
                    Color( 255, 255, 255 ),
                    g_fonts->get_zabel_16px( ),
                    distance_text.data( ),
                    16
                );

                g_render->box(
                    { background_origin.x - 1.f, background_origin.y - 1.f },
                    { background_size.x + 2.f, background_size.y + 2.f },
                    Color( 0, 0, 0 ),
                    -1,
                    4.f
                );

                g_render->box( background_origin, background_size, alert_color, -1, 2.f );
            }
        }
    }

    auto Tracker::draw_nearby_enemies() -> void {

        if (!g_config->awareness.show_nearby_enemies->get<bool>()) return;

        float scale_modifier = (float)(g_config->awareness.nearby_enemies_scale->get<int>()) / 100.f;

        for (auto inst : m_last_seen_data) {

            auto object = g_entity_list.get_by_index(inst.index);
            if (!object || object->is_dead()) continue;

            Vec2 texture_size{ 25.f * scale_modifier, 25.f * scale_modifier };

            if ( object->is_visible( ) ) {

                if (object->dist_to_local() > 3000.f || object->dist_to_local() <= 900.f) continue;

                float fow_modifier =  1.f - std::clamp( ( *g_time - inst.vision_gain_time ) / 0.75f, 0.f, 1.f );
                float fast_fow_modifier = utils::ease::ease_in_expo(fow_modifier);
                float slow_fow_modifier = utils::ease::ease_in_quad(fow_modifier);
                fow_modifier       = utils::ease::ease_out_expo(fow_modifier);

                float fow_size_modifier = std::clamp( inst.last_fow_duration / 8.f ,0.f, 1.f);

                Vec3 root_position = g_local->position.extend(object->position, 125.f);
                float modifier       = std::clamp(object->dist_to_local() / 3250.f, 0.f, 1.f);
                float extend_distance = 750.f * modifier;

                texture_size += Vec2(5.f + (15.f * scale_modifier) * fow_size_modifier,
                                     5.f + (15.f * scale_modifier) * fow_size_modifier) *
                    fow_modifier;

                g_render->line_3d(root_position, root_position.extend(object->position, extend_distance),
                                  Color(255, 75, 60), 4.f * scale_modifier);

                Vec2 sp{};
                if (!world_to_screen(root_position.extend(object->position, extend_distance), sp)) continue;

                 std::string champ_name = object->champion_name.text;
                auto        texture =
                    g_render->load_texture_from_file(path::join({ directory_manager::get_resources_path(), "champions",
                                                                  champ_name, champ_name + "_square.png" })
                                                         .value());
                if (!texture) continue;

                const Vec2 texture_position{ sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                //auto       dummy         = distance_text + "..";
                //const auto size          = g_render->get_text_size(dummy, g_fonts->get_zabel_16px(), 16);


                const Vec2 background_origin = { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                const Vec2 background_size   = { texture_size.x, texture_size.y };

                float glow_effect_value   = (10.f * scale_modifier) - (10.f * scale_modifier) * fast_fow_modifier;
                Vec2  glow_effect_origin = { background_origin.x - glow_effect_value,
                                              background_origin.y - glow_effect_value };
                Vec2  glow_effect_size    = { background_size.x + glow_effect_value * 2.f,
                                              background_size.y + glow_effect_value * 2.f };

                g_render->filled_box(glow_effect_origin, glow_effect_size,
                                     Color(255, 75, 75, (int)(255.f * slow_fow_modifier)),
                                     -1);

                g_render->image(texture_position, texture_size, texture);

                 g_render->box({ background_origin.x - 1.f, background_origin.y - 1.f },
                              { background_size.x + 2.f, background_size.y + 2.f }, Color(255, 75, 60), -1,
                              2.f * scale_modifier);
            }
            else {

                  if (object->dist_to_local() > 3500.f) continue;


                  float time_since_update = *g_time - inst.last_seen_time;
                  if (time_since_update > 4.325f) continue;

                  float fade_modifier = std::clamp( time_since_update / 4.f, 0.f, 1.f);
                  float icon_fade_modifier = std::clamp((time_since_update - 4.f) / 0.5f, 0.f, 1.f);
                  icon_fade_modifier       = utils::ease::ease_in_out_circ(icon_fade_modifier);

                  bool was_fow_update = inst.last_seen_time > inst.last_visible_time + 0.25f;

                  float fow_modifier      = 1.f - std::clamp((*g_time - inst.last_seen_time) / 0.6f, 0.f, 1.f);
                  float fast_fow_modifier = utils::ease::ease_in_expo(fow_modifier);
                  float slow_fow_modifier = utils::ease::ease_in_quad(fow_modifier);
                  fow_modifier            = utils::ease::ease_out_expo(fow_modifier);

                  float fow_size_modifier = was_fow_update ? 0.4f : 0.f;

                  Vec3  root_position   = g_local->position.extend(object->position, 125.f);
                  float modifier        = std::clamp(object->dist_to_local() / 3250.f, 0.f, 1.f);
                  float extend_distance = 750.f * modifier;

                  g_render->line_3d(root_position, root_position.extend(object->position, extend_distance),
                                    Color(255, 255 ,
                                          255 ,
                                          static_cast<int>(255.f - 200.f * fade_modifier)),
                                    4.f * scale_modifier);

                  Vec2 sp{};
                  if (!world_to_screen(root_position.extend(object->position, extend_distance), sp)) continue;

                  std::string champ_name = object->champion_name.text;
                  auto        texture    = g_render->load_texture_from_file(
                      path::join({ directory_manager::get_resources_path(), "champions", champ_name,
                                             champ_name + "_square.png" })
                          .value());
                if (!texture) continue;

                texture_size -= texture_size * icon_fade_modifier;

                 if (was_fow_update)
                texture_size += Vec2((5.f * scale_modifier + 15.f * scale_modifier) * fow_size_modifier,
                                     (5.f * scale_modifier + 15.f * scale_modifier) * fow_size_modifier) *
                    fow_modifier;

                  const Vec2 texture_position{ sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                  // auto       dummy         = distance_text + "..";
                  // const auto size          = g_render->get_text_size(dummy, g_fonts->get_zabel_16px(), 16);


                  const Vec2 background_origin = { sp.x - texture_size.x / 2.f, sp.y - texture_size.y / 2.f };
                  const Vec2 background_size   = { texture_size.x, texture_size.y };

                if (was_fow_update) {

                      float glow_effect_value  = (5.f * scale_modifier) - (5.f * scale_modifier) * fast_fow_modifier;
                Vec2  glow_effect_origin = { background_origin.x - glow_effect_value,
                                             background_origin.y - glow_effect_value };
                Vec2  glow_effect_size   = { background_size.x + glow_effect_value * 2.f,
                                             background_size.y + glow_effect_value * 2.f };

                        g_render->filled_box(glow_effect_origin, glow_effect_size,
                                     Color(255, 255, 255, (int)(255.f * slow_fow_modifier)), -1);

                }

                  g_render->image(texture_position, texture_size, texture);

                  g_render->filled_box(background_origin, background_size, Color(0, 0, 0, 50), -1);

                  g_render->box({ background_origin.x - 1.f, background_origin.y - 1.f },
                                { background_size.x + 2.f, background_size.y + 2.f },
                                Color(255, 255, 255,
                                      static_cast<int>(255.f - 200.f * fade_modifier)), -1,
                                2.f * scale_modifier);
            }
        }
    }


    auto Tracker::draw_enemy_spell_ranges( ) const -> void{
        if ( !g_config->awareness.show_enemy_spell_range->get< bool >( ) ) return;

        Vec2 sp{ };

        for ( const auto index : m_enemies ) {
            const auto enemy = g_entity_list.get_by_index( index );
            if ( !enemy || enemy->is_dead( ) || enemy->is_invisible( ) || !
                world_to_screen( enemy->position, sp ) )
                continue;

            constexpr auto range =
                0.f;  // g_features->target_selector->get_champion_range( rt_hash( enemy->champion_name.text ) );
            if ( range <= 0.f ) continue;

            g_render->circle_3d( enemy->position, Color( 255, 255, 255 ), range, Renderer::outline, 60, 3.f );
        }
    }

    auto Tracker::update_items( ) -> void{
        if ( !g_config->awareness.show_item_cooldowns->get< bool >( ) ) return;

        if ( g_config->awareness.show_ally_items->get< bool >( ) ) {
            for ( const auto hero : g_entity_list.get_allies( ) ) {
                if ( !hero || hero->network_id == g_local->network_id || hero->is_dead( ) || hero->
                    is_invisible( ) )
                    continue;

                for ( auto i = 1; i < 7;
                      i++ ) {
                    const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
                    auto       spell             = hero->spell_book.get_spell_slot( spell_slot_object );

                    if ( !spell ) continue;

                    auto slot = hero->inventory.get_inventory_slot( i );
                    if ( !slot ) continue;
                    auto item_base = slot->get_base_item( );
                    if ( !item_base ) continue;
                    auto item_data = item_base->get_item_data( );
                    if ( !item_data ) continue;

                    const auto item_id = static_cast< EItemId >( item_data->id );

                    if ( is_item_tracked( hero->index, item_id ) ) continue;

                    switch ( item_id ) {
                    default:
                        break;
                    case EItemId::zhonyas_hourglass:
                    case EItemId::mercurial_scimitar:
                    case EItemId::quicksilver_sash:
                    case EItemId::galeforce:
                    case EItemId::shieldbow:
                    case EItemId::stopwatch:
                    {
                        ItemInstance inst{
                            hero->index,
                            item_id,
                            spell_slot_object,
                            i,
                            spell->is_ready( ),
                            spell->cooldown_expire,
                            spell->cooldown
                        };
                        m_hero_items.push_back( inst );
                        break;
                    }
                    }
                }
            }
        }

        if ( g_config->awareness.show_enemy_items->get< bool >( ) ) {
            for ( const auto hero : g_entity_list.get_enemies( ) ) {
                if ( !hero || hero->is_dead( ) || hero->is_invisible( ) ) continue;

                for ( auto i = 1; i < 7;
                      i++ ) {
                    const auto spell_slot_object = static_cast< ESpellSlot >( 5 + i );
                    auto       spell             = hero->spell_book.get_spell_slot( spell_slot_object );

                    if ( !spell ) continue;

                    auto slot = hero->inventory.get_inventory_slot( i );
                    if ( !slot ) continue;
                    auto item_base = slot->get_base_item( );
                    if ( !item_base ) continue;
                    auto item_data = item_base->get_item_data( );
                    if ( !item_data ) continue;

                    const auto item_id = static_cast< EItemId >( item_data->id );

                    if ( is_item_tracked( hero->index, item_id ) ) continue;

                    switch ( item_id ) {
                    default:
                        break;
                    case EItemId::zhonyas_hourglass:
                    case EItemId::mercurial_scimitar:
                    case EItemId::quicksilver_sash:
                    case EItemId::galeforce:
                    case EItemId::shieldbow:
                    case EItemId::stopwatch:
                    {
                        ItemInstance inst{
                            hero->index,
                            item_id,
                            spell_slot_object,
                            i,
                            spell->is_ready( ),
                            spell->cooldown_expire,
                            spell->cooldown
                        };
                        m_hero_items.push_back( inst );
                        break;
                    }
                    }
                }
            }
        }

        for ( auto& inst : m_hero_items ) {
            auto owner = g_entity_list.get_by_index( inst.owner_index );
            if ( !owner || owner->is_dead( ) || owner->is_invisible( ) ) continue;

            if ( !owner->is_hero( ) ||
                owner->team == g_local->team && !g_config->awareness.show_ally_items->get< bool >( ) ||
                owner->team != g_local->team && !g_config->awareness.show_enemy_items->get< bool >( ) ) {
                remove_item( inst.owner_index, inst.item );
                continue;
            }

            auto slot = owner->inventory.get_inventory_slot( inst.inventory_slot );
            if ( !slot ) {
                remove_item( inst.owner_index, inst.item );
                continue;
            }

            auto item_base = slot->get_base_item( );
            if ( !item_base ) {
                remove_item( inst.owner_index, inst.item );
                continue;
            }

            auto item_data = item_base->get_item_data( );
            if ( !item_data ) {
                remove_item( inst.owner_index, inst.item );
                continue;
            }

            const auto item_id = static_cast< EItemId >( item_data->id );
            if ( item_id != inst.item ) {
                bool found_item{ };

                for ( auto i = 1; i < 8; i++ ) {
                    slot = owner->inventory.get_inventory_slot( inst.inventory_slot );
                    if ( !slot ) continue;
                    item_base = slot->get_base_item( );
                    if ( !item_base ) continue;
                    item_data = item_base->get_item_data( );
                    if ( !item_data ) continue;

                    const auto current_item_id = static_cast< EItemId >( item_data->id );
                    if ( current_item_id != inst.item ) continue;

                    found_item          = true;
                    inst.inventory_slot = i;
                    inst.spell_slot     = static_cast< ESpellSlot >( 5 + i );
                    break;
                }

                if ( !found_item ) {
                    remove_item( inst.owner_index, inst.item );
                    continue;
                }
            }

            auto spell = owner->spell_book.get_spell_slot( inst.spell_slot );
            if ( !spell ) {
                remove_item( inst.owner_index, inst.item );
                continue;
            }

            inst.is_ready        = spell->is_ready( );
            inst.cooldown        = spell->cooldown;
            inst.cooldown_expire = spell->cooldown_expire;
        }
    }

    auto Tracker::draw_items( ) -> void{
        if ( !g_config->awareness.show_item_cooldowns->get< bool >( ) ) return;

        const auto scale = g_config->awareness.item_tracker_scale->get< int >( ) / 100.f;

        const Vec2 texture_size{ 24.f * scale, 24.f * scale };
        const auto font_size = 14.f * scale;
        const auto font      = font_size > 16.f ? g_fonts->get_zabel( ) : g_fonts->get_zabel_16px( );

        Vec2 sp{ };
        for ( auto hero : g_entity_list.get_allies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->is_invisible( ) || !world_to_screen( hero->position, sp ) ||
                rt_hash( hero->champion_name.text ) == ct_hash( "Yuumi" ) )
                continue;

            auto items = get_items( hero->index );
            if ( items.empty( ) ) continue;

            auto base_position = hero->get_hpbar_position( );
            auto bar_length    = static_cast< float >( g_render_manager->get_width( ) ) * 0.0546875f;
            auto bar_height    = static_cast< float >( g_render_manager->get_height( ) ) * 0.022222222f;

            base_position.x -= bar_length * 0.7f;
            base_position.y -= static_cast< float >( g_render_manager->get_height( ) ) * 0.003f;

            //g_render->filled_circle( base_position, Color::white( ), 2.f, 16 );

            float height_offset{ };


            for ( const auto inst : items ) {
                const Vec2 draw_position = {
                    base_position.x - texture_size.x,
                    base_position.y + height_offset - texture_size.y
                };

                const auto texture = get_item_texture( inst.item );
                if ( !texture ) continue;

                g_render->image( draw_position, texture_size, texture );
                g_render->box( draw_position, texture_size, Color( 0, 0, 0, 255 ), -1, 2.f );

                if ( !inst.is_ready ) {
                    g_render->filled_box( draw_position, texture_size, Color( 0, 0, 0, 100 ), -1 );

                    const auto cooldown_left = inst.cooldown_expire - *g_time;

                    auto timer_text = std::to_string( cooldown_left );
                    if ( cooldown_left >= 100.f || cooldown_left < 10.f ) timer_text.resize( 3 );
                    else timer_text.resize( 2 );

                    const auto timer_size = g_render->get_text_size( timer_text, font, font_size );

                    const Vec2 text_position = {
                        draw_position.x + texture_size.x / 2.f - timer_size.x / 2.f,
                        draw_position.y + texture_size.y / 2.f - timer_size.y / 2.f
                    };

                    g_render->text_shadow(
                        text_position,
                        Color::white( ),
                        font,
                        timer_text.c_str( ),
                        font_size
                    );
                }

                height_offset += texture_size.y;
            }
        }

        for ( auto hero : g_entity_list.get_enemies( ) ) {
            if ( !hero || hero->is_dead( ) || hero->is_invisible( ) || !world_to_screen( hero->position, sp ) ||
                rt_hash( hero->champion_name.text ) == ct_hash( "Yuumi" ) )
                continue;

            auto items = get_items( hero->index );
            if ( items.empty( ) ) continue;

            auto base_position = hero->get_hpbar_position( );
            auto bar_length    = static_cast< float >( g_render_manager->get_width( ) ) * 0.0546875f;
            auto bar_height    = static_cast< float >( g_render_manager->get_height( ) ) * 0.022222222f;

            base_position.x -= bar_length * 0.7f;
            base_position.y -= static_cast< float >( g_render_manager->get_height( ) ) * 0.003f;

            // g_render->filled_circle( base_position, Color::white( ), 2.f, 16 );

            float height_offset{ };


            for ( const auto inst : items ) {
                const Vec2 draw_position = {
                    base_position.x - texture_size.x,
                    base_position.y + height_offset - texture_size.y
                };

                const auto texture = get_item_texture( inst.item );
                if ( !texture ) continue;

                g_render->image( draw_position, texture_size, texture );
                g_render->box( draw_position, texture_size, Color( 0, 0, 0, 255 ), -1, 2.f );

                if ( !inst.is_ready ) {
                    g_render->filled_box( draw_position, texture_size, Color( 0, 0, 0, 100 ), -1 );

                    const auto cooldown_left = inst.cooldown_expire - *g_time;

                    auto timer_text = std::to_string( cooldown_left );
                    if ( cooldown_left >= 100.f || cooldown_left < 10.f ) timer_text.resize( 3 );
                    else timer_text.resize( 2 );

                    const auto timer_size = g_render->get_text_size( timer_text, font, font_size );

                    const Vec2 text_position = {
                        draw_position.x + texture_size.x / 2.f - timer_size.x / 2.f,
                        draw_position.y + texture_size.y / 2.f - timer_size.y / 2.f
                    };

                    g_render->text_shadow( text_position, Color::white( ), font, timer_text.c_str( ), font_size );
                }

                height_offset += texture_size.y;
            }
        }
    }

    auto Tracker::get_item_texture( const EItemId item ) -> std::shared_ptr< Renderer::Texture >{
        switch ( item ) {
        case EItemId::zhonyas_hourglass:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "zhonyas.png" } ).value( )
            );
        case EItemId::quicksilver_sash:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "qss.png" } ).value( )
            );
        case EItemId::mercurial_scimitar:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "mercurial.png" } ).value( )
            );
        case EItemId::galeforce:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "galeforce.png" } ).value( )
            );
        case EItemId::shieldbow:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "shieldbow.png" } ).value( )
            );
        case EItemId::stopwatch:
            return g_render->load_texture_from_file(
                path::join( { directory_manager::get_resources_path( ), "common", "stopwatch.png" } ).value( )
            );
        default:
            break;
        }

        return { };
    }

}
