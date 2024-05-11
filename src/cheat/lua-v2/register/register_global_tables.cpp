#include "pch.hpp"

#include "../lua_def.hpp"
#include "../state.hpp"
#include "../../globals.hpp"
#include "../../features/feature.hpp"
#include "../../features/helper.hpp"
#include "../../features/champion_modules/lua_module.hpp"
#include "../../features/champion_modules/modules.hpp"
#include "../../renderer/c_renderer.hpp"
#include "../../utils/c_function_caller.hpp"
#include "../../utils/directory_manager.hpp"
#include "../../utils/ease.hpp"
#include "../../utils/input.hpp"

namespace lua {
        auto LuaState::register_global_tables( sol::state& state ) -> void{
        state.create_named_table(
            "cheat",
            "register_callback",
            []( const char* name, const sol::object callback ) -> void{
                lua_arg_check_ct_v( callback, sol::function, "function" );

                const auto cb = callback.as< sol::function >( );

                switch ( rt_hash( name ) ) {
                case ct_hash( "feature" ):
                    g_lua2->register_callback( ct_hash( "features.run" ), callback.as< sol::function >( ) );
                    break;
                case ct_hash( "render" ):
                    g_lua2->register_callback( ct_hash( "renderer.draw" ), callback.as< sol::function >( ) );
                    break;
                case ct_hash( "pre_feature" ):
                    g_lua2->register_callback( ct_hash( "features.pre_run" ), callback.as< sol::function >( ) );
                    break;
                case ct_hash( "champion_create" ):
                    g_lua2->register_callback( ct_hash( "champion.create" ), callback.as< sol::function >( ) );
                    break;
                case ct_hash( "champion_delete" ):
                    g_lua2->register_callback( ct_hash( "champion.delete" ), callback.as< sol::function >( ) );
                    break;
                default: ;
                }
            },
            "on",
            []( const char* name, sol::object callback ) -> void{
                lua_arg_check_ct_v( callback, sol::function, "function" );

                switch ( const auto callback_name = rt_hash( name ) ) {
                case ct_hash( "lua.unload" ):
                case ct_hash( "lua.load" ):
                case ct_hash( "local.cast_spell" ):
                case ct_hash( "local.issue_order_attack" ):
                case ct_hash( "local.issue_order_move" ):
                case ct_hash( "champion.create" ):
                case ct_hash( "champion.delete" ):
                case ct_hash( "features.run" ):
                case ct_hash( "features.pre_run" ):
                case ct_hash( "features.orbwalker" ):
                case ct_hash( "features.prediction" ):
                case ct_hash( "features.entity_list" ):
                case ct_hash( "features.buff_cache" ):
                case ct_hash( "features.evade" ):
                case ct_hash( "features.target_selector" ):
                case ct_hash( "renderer.draw" ):
                case ct_hash( "features.orbwalker.on_spell_casted" ):
                case ct_hash( "features.orbwalker.on_auto_attacked" ):
                {
                    g_lua2->register_callback( callback_name, callback.as< sol::function >( ) );
                    break;
                }
                default: ;
                }
            },
#if !__DEBUG
            "get_username",
            []( ) -> std::string{
                return
#if enable_auth
                    g_user->get_username( );
#else

     "";
#endif
            },
#endif
            "register_module",
            [&]( sol::object table ) -> bool{
                if ( table.get_type( ) != sol::type::table ) return false;
                const auto t      = table.as< sol::table >( );
                const auto module = std::make_shared< ChampionModule >( );

                for ( auto value : t ) {
                    if ( value.first.get_type( ) != sol::type::string ) continue;
                    const auto name = rt_hash( value.first.as< std::string >( ).data( ) );

                    switch ( name ) {
                    case ct_hash( "champion_name" ):
                        if ( value.second.get_type( ) != sol::type::string ) continue;
                        module->champion_name = rt_hash( value.second.as< std::string >( ).c_str( ) );
                        break;
                    case ct_hash( "on_draw" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->on_draw = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "spell_q" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->spell_q = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "spell_w" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->spell_w = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "spell_e" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->spell_e = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "spell_r" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->spell_r = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "initialize" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->initialize = value.second.as< sol::function >( );
                        break;
                    case ct_hash( "get_priorities" ):
                        if ( value.second.get_type( ) != sol::type::function ) continue;
                        module->get_priorities = value.second.as< sol::function >( );
                        break;
                    default: ;
                    }
                }

                if ( module->is_valid( ) ) {
                    g_lua2->push_champion_module( module );

                    if ( g_local ) {
                        const auto current_champion_name = rt_hash( g_local->champion_name.text );

                        auto       lua_champion_modules = g_lua2->get_champion_modules( );
                        const auto lua_module           = std::ranges::find_if(
                            lua_champion_modules,
                            [&]( std::shared_ptr< ChampionModule > it ) -> bool{
                                debug_log( "{} == {}", it->champion_name, current_champion_name );
                                return it->champion_name == current_champion_name;
                            }
                        );

                        if ( lua_module != lua_champion_modules.end( ) ) {
                            debug_log( "set lua module" );

                            // todo: here should be a mutex lock for g_features->current_module
                            g_features->current_module = std::make_shared< features::champion_modules::LuaModule >(
                                *lua_module
                            );
                            // mutex.unlock( );
                            g_features->current_module->initialize( );
                            // g_features->current_module->initialize_menu( );
                            // mutex.lock( );
                        }
                    }

                    return true;
                }

                return false;
            },
            "get_module_by_champion",
            []( const sol::object name ) -> std::shared_ptr< features::champion_modules::IModule >{
                lua_arg_check_ct( name, std::string, "string" )

                features::champion_modules::initialize( );
                const auto hashed = rt_hash( name.as<std::string>( ).data( ) );
                const auto found  = std::ranges::find_if(
                    g_features->modules,
                    [hashed]( const std::shared_ptr< features::champion_modules::IModule > module ) -> bool{
                        return module->get_champion_name( ) == hashed;
                    }
                );

                if ( found == g_features->modules.end( ) ) return nullptr;
                auto f = *found;
                g_features->modules.clear( );
                return f;
            },
            "get_resource_path",
            []( ) -> std::string{
                static auto path = directory_manager::get_resources_path( );
                return path;
            },
            "get_local_attack_cast_delay",
            []( ) -> float{ return g_function_caller->attack_cast_delay( ); },
            "get_local_attack_delay",
            []( ) -> float{ return g_function_caller->attack_delay( ); },
            "get_default_target_selector",
            []( ) -> std::shared_ptr< features::ITargetSelector >{ return g_features->default_target_selector; },
            "reload_lua",
            []( ) -> void{ g_lua2->set_reload_lua( ); }
        );

        state.globals( ).set( "g_render", g_render );

        state.new_usertype< Renderer >(
            "c_renderer",
            "line",
            sol::resolve( &Renderer::line_lua ),
            "box",
            sol::resolve( &Renderer::box_lua ),
            "filled_box",
            sol::resolve( &Renderer::filled_box_lua ),
            "circle",
            sol::resolve( &Renderer::circle_lua ),
            "triangle",
            sol::resolve( &Renderer::triangle_lua ),
            "filled_triangle",
            sol::resolve( &Renderer::filled_triangle_lua ),
            "filled_circle",
            sol::resolve( &Renderer::filled_circle_lua ),
            "circle_3d",
            sol::resolve( &Renderer::circle_3d_lua ),
            "text",
            sol::resolve( &Renderer::text_lua ),
            "get_text_size",
            sol::resolve( &Renderer::get_text_size_lua ),
            "load_texture_from_file",
            sol::resolve( &Renderer::load_texture_from_file_lua ),
            "image",
            sol::resolve( &Renderer::image_lua ),
            "get_screensize",
            sol::resolve( &Renderer::get_screensize_lua ),
            "circle_minimap",
            sol::resolve( &Renderer::circle_minimap_lua ),
            "line_3d",
            sol::resolve( &Renderer::lua_line_3d )
        );


        state.new_usertype< Renderer::Texture >(
            "texture_t",
            "width",
            sol::readonly( &Renderer::Texture::width ),
            "height",
            sol::readonly( &Renderer::Texture::height ),
            "name",
            sol::readonly( &Renderer::Texture::name )
        );

        state.create_named_table(
            "helper",
            "calculate_damage",
            sol::resolve( &features::helper::calculate_damage ),
            "get_aa_damage",
            sol::resolve( &features::helper::get_aa_damage ),
            "get_on_hit_damage",
            sol::resolve( &features::helper::get_onhit_damage ),
            "get_real_health",
            sol::resolve( &features::helper::get_real_health )
        );

        state.create_named_table(
            "easing",
            "ease_out",
            []( sol::object progress, const sol::object factor ) -> float{
                lua_arg_check_ct( progress, float, "number" )

                if ( factor.get_type( ) == sol::type::number ) {
                    return utils::ease::ease_out( progress.as< float >( ), factor.as< int32_t >( ) );
                }
                return utils::ease::ease_out( progress.as< float >( ) );
            },
            "ease_in",
            []( sol::object progress, const sol::object factor ) -> float{
                lua_arg_check_ct( progress, float, "number" )


                if ( factor.get_type( ) == sol::type::number ) {
                    return utils::ease::ease_in( progress.as< float >( ), factor.as< int32_t >( ) );
                }
                return utils::ease::ease_in( progress.as< float >( ) );
            },
            "ease_in_out",
            sol::resolve( &utils::ease::ease_in_out ),
            "ease_in_quad",
            sol::resolve( &utils::ease::ease_in_quad ),
            "ease_out_quad",
            sol::resolve( &utils::ease::ease_out_quad ),
            "ease_in_out_quad",
            sol::resolve( &utils::ease::ease_in_out_quad ),
            "ease_in_cubic",
            sol::resolve( &utils::ease::ease_in_cubic ),
            "ease_out_cubic",
            sol::resolve( &utils::ease::ease_out_cubic ),
            "ease_in_out_cubic",
            sol::resolve( &utils::ease::ease_in_out_cubic ),
            "ease_in_quart",
            sol::resolve( &utils::ease::ease_in_quart ),
            "ease_out_quart",
            sol::resolve( &utils::ease::ease_out_quart ),
            "ease_in_out_quart",
            sol::resolve( &utils::ease::ease_in_out_quart ),
            "ease_in_quint",
            sol::resolve( &utils::ease::ease_in_quint ),
            "ease_out_quint",
            sol::resolve( &utils::ease::ease_out_quint ),
            "ease_in_out_quint",
            sol::resolve( &utils::ease::ease_in_out_quint ),
            "ease_in_sine",
            sol::resolve( &utils::ease::ease_in_sine ),
            "ease_out_sine",
            sol::resolve( &utils::ease::ease_out_sine ),
            "ease_in_out_sine",
            sol::resolve( &utils::ease::ease_in_out_sine ),
            "ease_in_expo",
            sol::resolve( &utils::ease::ease_in_expo ),
            "ease_out_expo",
            sol::resolve( &utils::ease::ease_out_expo ),
            "ease_in_out_expo",
            sol::resolve( &utils::ease::ease_in_out_expo ),
            "ease_in_circ",
            sol::resolve( &utils::ease::ease_in_circ ),
            "ease_out_circ",
            sol::resolve( &utils::ease::ease_out_circ ),
            "ease_in_out_circ",
            sol::resolve( &utils::ease::ease_in_out_circ )
        );

        state.new_usertype< utils::Input >(
            "c_input",
            "set_cursor_position",
            sol::resolve( &utils::Input::set_cursor_position_lua ),
            "get_cursor_position",
            sol::resolve( &utils::Input::get_cursor_position ),
            "get_cursor_position_game",
            sol::resolve( &utils::Input::get_cursor_position_game ),
            "send_key_event",
            sol::resolve( &utils::Input::send_key_event_lua ),
            "send_mouse_key_event",
            sol::resolve( &utils::Input::send_mouse_key_event_lua ),
            "issue_order_move",
            sol::resolve( &utils::Input::issue_order_move_lua ),
            "issue_order_attack",
            sol::resolve( &utils::Input::issue_order_attack_lua ),
            "cast_spell",
            sol::resolve( &utils::Input::cast_spell_lua ),
            "release_chargeable",
            sol::resolve( &utils::Input::release_chargeable_lua ),
            "is_key_pressed",
            sol::resolve( &utils::Input::is_key_pressed_lua ),
            "block_cast_spell",
            sol::resolve( &utils::Input::lua_block_cast_spell ),
            "block_issue_order",
            sol::resolve( &utils::Input::lua_block_issue_order ),
            "is_cast_spell_blocked",
            sol::resolve( &utils::Input::is_cast_spell_blocked ),
            "is_issue_order_blocked",
            sol::resolve( &utils::Input::lua_is_issue_order_blocked )
        );

        state.globals( ).set( "g_input", g_input.get( ) );
    }
}
