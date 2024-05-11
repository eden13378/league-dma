#include "pch.hpp"

#include "sdk.hpp"

#include <thread>
// #include <fmt/printf.h>

#include "globals.hpp"
#include "offsets.hpp"
#include "game/render_manager.hpp"
#include "game/tactical_map.hpp"
#include "game/menugui.hpp"
#include "game/pw_hud.hpp"
#include "game/navgrid.hpp"
#include "../utils/c_function_caller.hpp"
#include "../utils/debug_logger.hpp"
#include "game/camera_config.hpp"
#include "g_unload.hpp"
#include "../runtime/app.hpp"

CHolder                                      g_local;
utils::MemoryHolder< ObjectManager >         g_objects;
utils::MemoryHolder< RenderManager >         g_render_manager;
utils::MemoryHolder< TacticalMap >           g_minimap;
utils::MemoryHolder< CameraConfig >          g_camera_config;
utils::MemoryHolder< PwHud >                 g_pw_hud;
utils::MemoryHolder< float >                 g_time;
utils::MemoryHolder< sdk::game::EGameState > g_state;
utils::MemoryHolder< sdk::game::Navgrid >    g_navgrid;
utils::MemoryHolder< c_matrix_holder >       g_render_matrix;
utils::MemoryHolder< MenuGui >               g_menugui;
UnloadT                                      g_unload;

intptr_t g_league_base;
bool     g_game_focused = false;

namespace sdk {
#if __DEBUG
    auto print_local_flags( ) -> void{
        auto flags = g_local->get_raw_flags( );

        if ( !flags ) {
            debug_log( "flags is null" );
            return;
        }

        auto f = flags->get( );

        auto check_flag = [&]( Object::EObjectTypeFlags flag, std::string name ) -> void{
            bool h = f & static_cast< int >( flag );
            if ( h )
                debug_log( "local is {}", name, h );
        };

        debug_log( "--------------[ local flags ]--------------" );

        debug_log( "flag: {}", f );

        check_flag( Object::EObjectTypeFlags::game_object, "game_object" );
        check_flag( Object::EObjectTypeFlags::neutral_camp, "neutral_camp" );
        check_flag( Object::EObjectTypeFlags::dead_object, "dead_object" );
        check_flag( Object::EObjectTypeFlags::invalid_object, "invalid_object" );
        check_flag( Object::EObjectTypeFlags::ai_base_common, "ai_base_common" );
        check_flag( Object::EObjectTypeFlags::attackable_unit, "attackable_unit" );
        check_flag( Object::EObjectTypeFlags::ai, "ai" );
        check_flag( Object::EObjectTypeFlags::minion, "minion" );
        check_flag( Object::EObjectTypeFlags::hero, "hero" );
        check_flag( Object::EObjectTypeFlags::turret, "turret" );
        check_flag( Object::EObjectTypeFlags::missile, "missile" );
        check_flag( Object::EObjectTypeFlags::building, "building" );

        debug_log( "--------------[ end local flags ]--------------" );
    }
#endif

    auto initialize_globals(
        const sdk::memory::Process& process
    ) noexcept -> std::expected< void, EInitializeGlobalsError >{
        debug_fn_call( )

        if ( !process ) return std::unexpected( EInitializeGlobalsError::process_not_initialized );

        // setup global vars
        app->memory = std::make_unique< sdk::memory::Memory >( std::make_unique< sdk::memory::Process >( process ) );

        if ( !app->memory ) {
            return std::unexpected(
                EInitializeGlobalsError::memory_not_initialized
            );
        }

        const auto p = app->memory->get_process( );

        if ( !p ) return std::unexpected( EInitializeGlobalsError::app_memory_process_not_initialized );

        if ( !p->get_module( ct_hash( "league of legends.exe" ) ) ) {
            return std::unexpected( EInitializeGlobalsError::league_not_running );
        }

        g_league_base = app->memory->get_process( )->get_module( ct_hash( "league of legends.exe" ) )->get_base( );

        debug_log( "g_objects: 0x{:x}", g_objects.get_address( ) );

        // wait for local player object to be initialized
        while ( !g_local ) {
            debug_log( "local not found" );
            const auto offset = app->memory->read< intptr_t >( g_league_base + offsets::local_player );

            if ( !offset ) continue;

            debug_log( "local offset: {:x}", *offset );

            if ( !app->memory->read< Object >( *offset, g_local.get_feature_object( ) ) ) {
                std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
                continue;
            }

            g_local.set( *offset );
            g_local.set_index( g_local.get_unchecked( )->index );
            // g_local.set_is_local( true );
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
            if ( !app->should_run( ) ) return { };
        }

        debug_log( "local->is_invisible(): {}", g_local->is_invisible( ) );

#if __DEBUG
        print_local_flags( );
#endif

        g_state = utils::MemoryHolder< sdk::game::EGameState >(
            *app->memory->read< intptr_t >( g_league_base + offsets::game_state ) + 0xC
        );

        while ( !g_state || *g_state != EGameState::game_loop ) {
            g_state = utils::MemoryHolder< sdk::game::EGameState >(
                *app->memory->read< intptr_t >( g_league_base + offsets::game_state ) + 0xC
            );
            std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );
            if ( !app->should_run( ) ) return std::unexpected( EInitializeGlobalsError::request_exit );
        }

        g_function_caller = std::make_unique< utils::c_function_caller >( );
#if enable_function_caller
        g_function_caller->initialize( );
#endif

#if __DEBUG
        fmt::print( "local: {:x}\n", g_local.get_address( ) );
        fmt::print( "Minions: {:x}\n", *app->memory->read< intptr_t >( g_league_base + 0x01844634 ) );
        fmt::print( "Heroes: {:x}\n", *app->memory->read< intptr_t >( g_league_base + 0x0184252c ) );
        fmt::print( "Turrets: {:x}\n", *app->memory->read< intptr_t >( g_league_base + 0x030d4ac8 ) );
        fmt::print( "Missiles: {:x}\n", *app->memory->read< intptr_t >( g_league_base + 0x030e023c ) );
        fmt::print( "map: {:X}\n", *app->memory->read< intptr_t >( g_league_base + offsets::minimap_instance ) );
        fmt::print( "render: {:X}\n", *app->memory->read< intptr_t >( g_league_base + offsets::render_manager ) );
        fmt::print(
            "navgrid: {:X}\n",
            *app->memory->read< intptr_t >(
                *app->memory->read< intptr_t >( g_league_base + offsets::navgrid ) + 0x8
            ) //x64
        );
        fmt::print( "menugui: {:x}\n", *app->memory->read< intptr_t >( g_league_base + offsets::menu_gui ) );
#endif
        const auto objects_address = app->memory->read< intptr_t >( g_league_base + offsets::object_manager );

        if ( !objects_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_object_manager );

        g_objects = utils::MemoryHolder< ObjectManager >(
            *objects_address
        );

        debug_log( "g_objects: {:x}", g_objects.get_address( ) );

        const auto time_address = app->memory->read< intptr_t >( g_league_base + offsets::game_time );

        if ( !time_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_time );

        g_time = utils::MemoryHolder< float >(
            *time_address + 0x20
        );

        debug_log( "g_time: {:x}", g_time.get_address( ) );

        const auto render_manager_address = app->memory->read< intptr_t >( g_league_base + offsets::render_manager );

        if ( !render_manager_address ) {
            return std::unexpected(
                EInitializeGlobalsError::offset_incorrect_render_manager
            );
        }

        g_render_manager = utils::MemoryHolder< RenderManager >(
            *render_manager_address
        );

        debug_log( "g_render_manager: {:x}", g_render_manager.get_address( ) );

        const auto pw_hud_address = app->memory->read< intptr_t >( g_league_base + offsets::pw_hud );

        if ( !pw_hud_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_pw_hud );

        g_pw_hud = utils::MemoryHolder< PwHud >( *pw_hud_address );

        debug_log( "g_pw_hud: {:x}", g_pw_hud.get_address( ) );

        const auto camera_config_address = app->memory->read< intptr_t >( g_league_base + offsets::camera_config );

        if ( !camera_config_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_camera_config );

        g_camera_config = utils::MemoryHolder< CameraConfig >(
            *camera_config_address
        );

        debug_log( "g_camera_config: {:x}", g_camera_config.get_address( ) );

        const auto minimap_instance_address = app->memory->read< intptr_t >(
            g_league_base + offsets::minimap_instance
        );

        if ( !minimap_instance_address ) {
            return std::unexpected(
                EInitializeGlobalsError::offset_incorrect_minimap_instance
            );
        }

         debug_log("pre g_minimap: {:x}", *minimap_instance_address);

        g_minimap = utils::MemoryHolder< TacticalMap >(
            *app->memory->read< intptr_t >(
                *minimap_instance_address + 0x240 //x64
            )
        );

        debug_log( "g_minimap: {:x}", g_minimap.get_address( ) );

        const auto navgrid_address = app->memory->read< intptr_t >( g_league_base + offsets::navgrid );

        if ( !navgrid_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_navgrid );

        g_navgrid = utils::MemoryHolder< Navgrid >(
            *app->memory->read< intptr_t >(
                *navgrid_address + 0x8
            ) // x64
        );

        debug_log( "g_navgrid: {:x}", g_navgrid.get_address( ) );

        debug_log( "g_navgrid->is_river( g_local->position ): {}", g_navgrid->is_river( g_local->position ) );
        debug_log( "g_navgrid->is_bush( g_local->position ): {}", g_navgrid->is_bush( g_local->position ) );
        debug_log("g_navgrid->get_collision: {}", static_cast<int32_t>(g_navgrid->get_collision( g_local->position )) );


        const auto menugui_address = app->memory->read< intptr_t >( g_league_base + offsets::menu_gui );

        if ( !menugui_address ) return std::unexpected( EInitializeGlobalsError::offset_incorrect_menu_gui );

        g_menugui = utils::MemoryHolder< MenuGui >(
            *menugui_address
        );

        debug_log( "g_menugui: {:x}", g_menugui.get_address( ) );

        fmt::print( "pwhud: {:x}\n", g_pw_hud.get_address( ) );
        fmt::print( "cconfig: {:x}\n", g_camera_config.get_address( ) );

        g_render_matrix = utils::MemoryHolder< c_matrix_holder >( g_league_base + offsets::render_matrix );

        std::thread(
            []( ) -> void{
                while ( app->should_run( ) ) {
                    std::this_thread::sleep_for( std::chrono::milliseconds( 500 ) );

                    if ( app &&
                        app->memory &&
                        app->memory->get_process( ) &&
                        !app->memory->get_process( )->is_running( )
                    ) {
                        std::thread(
                            []( ) -> void{
                                std::this_thread::sleep_for( std::chrono::milliseconds( 1500 ) );
                                std::exit( 0 );
                            }
                        ).detach( );

                        app->unload( );
                    }
                }
            }
        ).detach( );

        return { };
    }
}
