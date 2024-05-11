#include "pch.hpp"
#include "runtime2.hpp"

#include "../feature.hpp"

#include "../../menu/menu.hpp"

#include "../../utils/utils.hpp"

#include "../champion_modules/lua_module.hpp"
#include "../champion_modules/modules.hpp"
#include "../champion_modules/module.hpp"
#include "../activator/activator.hpp"

namespace features {
    auto Runtime::initialize_champion_module( ) noexcept -> void{
        m_module_initialized = false;
        debug_fn_call( )
#if enable_lua
        for ( const auto feature : g_features->all_features ) { feature->on_lua_load( ); }
        // run_catching( g_entity_list->on_lua_load( ); )
#endif

        if ( !g_local ) return;

        const auto current_champion_name = rt_hash( g_local->champion_name.text );
#if enable_lua
        auto       lua_champion_modules = g_lua2->get_champion_modules( );
        const auto lua_module           = std::ranges::find_if(
            lua_champion_modules,
            [&]( const std::shared_ptr< lua::LuaState::ChampionModule >& it ) -> bool{
                return it->champion_name == current_champion_name;
            }
        );

        if ( lua_module != lua_champion_modules.end( ) ) {
            g_features->current_module = std::make_shared< champion_modules::LuaModule >( *lua_module );
            g_window->m_mutex.lock( );
            g_features->current_module->initialize( );
            g_features->current_module->initialize_menu( );
            g_window->m_mutex.unlock( );
        } else if ( !g_features->current_module )
#endif
        {
            champion_modules::initialize( );
            g_window->m_mutex.lock( );
            g_features->current_module = g_features->get_module_for_champion( rt_hash( g_local->champion_name.text ) );
            g_features->modules.clear( );
            if ( g_features->current_module ) {
                g_features->current_module->initialize( );
                g_features->current_module->initialize_menu( );
            }
            g_window->m_mutex.unlock( );
        }

        m_module_initialized = true;
    }

    auto Runtime::load_encouragements( ) noexcept -> void{
        try {
            const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
            auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "encouragements.txt" );
            if ( !std::filesystem::exists( path ) ) {
                std::ofstream s( path );
                s << "" << std::endl;
                return;
            }


            if ( auto file = utils::read_file_to_string( path ) ) {
                std::vector< std::string > words;
                std::stringstream          ss;
                for ( auto c : *file ) {
                    if ( c == '\n' ) {
                        words.push_back( ss.str( ) );
                        ss.clear( );
                        continue;
                    }

                    ss << c;
                }

                for ( auto w : words ) { g_features->activator->add_encouragement( w ); }
            }
        } catch ( ... ) { app->logger->error( "error loading encouragements" ); }
    }

    auto Runtime::load_taunts( ) noexcept -> void{
        try {
            const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
            auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "taunts.txt" );
            if ( !std::filesystem::exists( path ) ) {
                std::ofstream s( path );
                s << "" << std::endl;
                return;
            }

            if ( auto file = utils::read_file_to_string( path ) ) {
                std::vector< std::string > words;
                std::stringstream          ss;
                for ( auto c : *file ) {
                    if ( c == '\n' ) {
                        if ( !ss.str( ).empty( ) ) words.push_back( ss.str( ) );
                        ss.clear( );
                        continue;
                    }

                    ss << c;
                }

                for ( auto w : words ) { g_features->activator->add_encouragement( w ); }
            }
        } catch ( ... ) { app->logger->error( "error loading taunts" ); }
    }

    auto Runtime::run_menu_initializers( ) noexcept -> void{
        for ( const auto feature : g_features->all_features )
            run_catching( feature->initialize_menu( ); )
    }

    auto Runtime::run_feature_list(
        const std::vector< std::shared_ptr< IFeature > >& list,
        const bool                                        run_threaded
    ) noexcept -> void{
        // run c_feature::run on all features
        std::vector< std::thread > threads;
        for ( auto feature : list ) {
            if ( !feature->should_run_in_feature_loop( ) ) continue;
            if ( run_threaded && !feature->force_run_sync( ) ) {
                threads.emplace_back(
                    [feature]( ) -> void{
#if __DEBUG && profiling_overlay
                    utils::c_timer timer;
#endif
#if enable_lua
                        // utils::error_logger::set_call_context( );
                        run_catching( feature->lua_pre_run( ); )
#endif
                        // utils::error_logger::set_call_context( );
                        run_catching( feature->pre_run( ); )
                        run_catching( feature->run( ); )
                        run_catching( feature->post_run( ); )
#if enable_lua
                        // utils::error_logger::set_call_context( );
                        run_catching( feature->lua_post_run( ); )
#endif
#if __DEBUG && profiling_overlay
                    if ( g_features->visuals->debug_profiling_times.find( feature->get_name( ) ) == g_features->visuals->debug_profiling_times.end( ) ) {
                        auto& debug_feature_time = g_features->visuals->debug_profiling_times[ feature->get_name( ) ];
                        debug_feature_time.time = static_cast< int32_t >( timer.get_ms_since_start( ).count( ) );
                        debug_feature_time.name = feature->get_full_name( );
                    }
                    else {
                        g_features->visuals->debug_profiling_times[ feature->get_name( ) ].update( static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ) );
                    }
#endif
                    }

                );
            } else {
#if __DEBUG && profiling_overlay
                utils::c_timer timer;
#endif
                if ( feature->is_enabled( ) ) {
#if enable_lua
                    // utils::error_logger::set_call_context( );
                    run_catching( feature->lua_pre_run( ); )
#endif
                    // utils::error_logger::set_call_context( );
                    run_catching( feature->pre_run( ); )
                    run_catching( feature->run( ); )
                    run_catching( feature->post_run( ); )
#if enable_lua
                    // utils::error_logger::set_call_context( );
                    run_catching( feature->lua_post_run( ); )
#endif
                }
#if __DEBUG && profiling_overlay
                if ( g_features->visuals->debug_profiling_times.find( feature->get_name( ) ) == g_features->visuals->debug_profiling_times.end( ) ) {
                    auto& debug_feature_time = g_features->visuals->debug_profiling_times[ feature->get_name( ) ];
                    debug_feature_time.time = static_cast< int32_t >( timer.get_ms_since_start( ).count( ) );
                    debug_feature_time.name = feature->get_full_name( );
                }
                else {
                    g_features->visuals->debug_profiling_times[ feature->get_name( ) ].update( static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ) );
                }
#endif
            }
        }

        if ( run_threaded ) {
            // wait for feature threads to end
            for ( auto& thread : threads ) { if ( thread.joinable( ) ) thread.join( ); }
        }
    }
}
