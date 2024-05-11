#include "pch.hpp"
//
// #include "single_core.hpp"
//
// #include <stdexcept>
//
// namespace features {
//     auto SingleCoreRuntime::initialize( ) noexcept -> void{
// #if enable_sentry
//         if ( app->sentry ) app->sentry->set_tag( _( "runtime" ), _( "single_core" ) );
// #endif
//         start_drawing( );
//     }
//
//     auto SingleCoreRuntime::on_draw( ) noexcept -> void{ throw std::logic_error( "Not implemented" ); }
//     auto SingleCoreRuntime::on_lua_reset( ) noexcept -> void{ throw std::logic_error( "Not implemented" ); }
//     auto SingleCoreRuntime::on_lua_load( ) noexcept -> void{ throw std::logic_error( "Not implemented" ); }
//
//     auto SingleCoreRuntime::start_drawing( ) noexcept -> void{
//         static auto avg_drawing_time = 0.f;
//         static auto last_run_time    = 0.f;
//
//         g_render->add_draw_callback(
//             [&]( ) -> void{
//                 utils::Timer render_timer;
//
//                 update_game_focus_state( );
//
//                 if ( g_lua2 && g_lua2->should_reset( ) && g_config->lua.reload_on_unknown_exception->get< bool >( ) ) {
//                     g_lua2 = std::make_unique< lua::LuaState >( );
//                     for ( auto& script : g_scripts.scripts ) {
//                         if ( script.is_loaded ) {
//                             g_lua2->execute_locked( [&script]( ) -> void{ g_lua2->try_run_file( script.path ); } );
//                             g_config_system->load( _( "cfg" ) );
//                         }
//                     }
//                 }
//
//                 // if ( features_initialized && app->feature_ticks % 10 == 0 &&
//                 //     app->memory &&
//                 //     !app->memory->get_process( )->is_running( )
//                 // ) {
//                 //     app->unload( );
//                 //     return;
//                 // }
//
//                 // todo: fix g_game_focused
//                 if ( !g_game_focused && !g_window->is_opened( ) ) return;
//                 run_catching( update_render_data( ); )
//
//                 // g_threading->render_thread  = std::this_thread::get_id( );
//                 // g_threading->feature_thread = std::this_thread::get_id( );
//
// #if enable_lua
//                 if ( g_lua2 ) {
//                     g_lua2->execute_locked(
//                         []( ) -> void{ g_lua2->run_callback( ct_hash( "renderer.draw" ) ); }
//                     );
//                 }
// #endif
//
//                 // g_render->text(
//                 //     Vec2( 100, 100 ),
//                 //     Color::white( ),
//                 //     g_fonts->get_default( ),
//                 //     std::format( "feature draw time: {:0.3f}ms", avg_drawing_time ).c_str( ),
//                 //     16
//                 // );
//
//                 if ( !features_initialized ) initialize_features( );
//
//                 if ( g_config->misc.lua_show_memory_uage->get< bool >( ) && g_lua2 ) {
//                     g_debug_overlay->track_size( "LUA memory usage", g_lua2->get_memory_usage( ) );
//                 }
//
//                 if ( !g_local ) return;
//
//                 if ( features_initialized && !g_window->is_opened( ) &&
//                     ( g_config->misc.high_performace_mode->get< bool >( ) ||
//                         last_run_time != *g_time )
//                 ) {
//                     run_features( );
//                     last_run_time = *g_time;
//                 }
//
//                 // run c_feature::on_draw on all features
//                 for ( const auto feature : g_features->list ) {
// #if debug_overlay
//                     utils::c_timer timer;
// #endif
//                     if ( feature->is_enabled( ) ) { run_catching( feature->on_draw( ); ) }
// #if debug_overlay
//                     g_debug_overlay.post_function_time(
//                         std::format( "{}-render", feature->get_full_name( ) ),
//                         timer.get_ms_since_start( ).count( )
//                     );
// #endif
//                 }
//
//
//                 if ( g_features->current_module ) {
// #if debug_overlay
//                     utils::c_timer timer;
// #endif
//                     run_catching( g_features->current_module->on_draw( ); )
//
// #if debug_overlay
//                     g_debug_overlay.post_function_time(
//                         "current-module-render",
//                         timer.get_ms_since_start( ).count( )
//                     );
// #endif
//                 }
//
// #if debug_overlay
//                 g_debug_overlay.post_function_time(
//                     "Feature rendering",
//                     render_timer.get_ms_since_start( ).count( )
//                 );
// #endif
//
//                 avg_drawing_time += render_timer.get_ms_since_start( ).count( ) / 1000.f;
//                 avg_drawing_time *= .5f;
//             }
//         );
//     }
//
//     auto SingleCoreRuntime::start_features( ) noexcept -> void{ throw std::logic_error( "Not implemented" ); }
// }
