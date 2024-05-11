#include "pch.hpp"


#include "../utils/exceptions.hpp"

#include "runtime.hpp"

#include <thread>

#include "../feature.hpp"
#include "../tracker.hpp"

#include "../orbwalker.hpp"
#include "../visuals.hpp"
#include "../prediction.hpp"
#include "../evade.hpp"
#include "../target_selector/target_selector.hpp"
#include "../activator/activator.hpp"
#include "../buff_cache.hpp"
#include "../entity_list.hpp"
#include "../threading.hpp"
#include "../globals.hpp"
#include "../../lua-v2/state.hpp"
// #include "../lua/c_lua.hpp"
// #include "../lua/lua_tests.hpp"
#include "../overlay/debug_overlay.hpp"
#include "../renderer/c_renderer.hpp"
#include "../runtime/app.hpp"
#include "../sdk/globals.hpp"
#include "../sdk/game/buff.hpp"
#include "../sdk/game/render_manager.hpp"
#include "../sdk/game/tactical_map.hpp"
#include "..\..\utils\timer.hpp"
#include "../utils/input.hpp"
#include "../utils/utils.hpp"
#include "../champion_modules/module.hpp"
#include "../champion_modules/modules.hpp"
#include "../utils/c_function_caller.hpp"
#include "../champion_modules/lua_module.hpp"
#include "../sdk/game/pw_hud.hpp"
#include "..\..\utils\destroy_watcher.hpp"

#define register_feature( feature_name, cls ) g_features->feature_name = g_features->create_feature< cls >( )
#define register_pre_feature( feature_name, cls ) g_features->feature_name = g_features->create_pre_feature< cls >( )

#if _DEBUG
#define run_catching( cmd ) try { cmd } catch(InvalidHolderException& e) { app->logger->error("object = null"); }
#define run_catching_lua( cmd ) cmd
#else
#define ___run_catching( cmd, is_lua ) try { cmd } catch(InvalidHolderException& e) {  } catch(InvalidMemoryAddressException& e) {} \
    catch(const std::exception& e) { \
        if(e.what()) { \
            app->logger->error("exception {} {}",_(#cmd), e.what()); \
        } else { \
            app->logger->error("exception {} {}", _(#cmd), _("unknown")); \
        } \
        if constexpr (!is_lua) { \
            if( std::string( e.what() ).find("device or resource busy") == std::string::npos ) { if(app->sentry) app->sentry->track_exception(e.what()); } \
        } \
    } catch(...) { \
        app->logger->error("{} {}", _(#cmd), _("uncaught exception")); \
    }
#define run_catching( cmd ) ___run_catching( cmd, false )
#define run_catching_lua( cmd ) ___run_catching( cmd, true )
#endif
#if _DEBUG && debug_profiling_times
#define profile_func( call, _name, name_hash ) { \
    utils::c_timer timer; \
    call \
    if ( g_features->visuals->debug_profiling_times.find( name_hash ) == g_features->visuals->debug_profiling_times.end( ) ) { \
        auto& debug_feature_time = g_features->visuals->debug_profiling_times[ name_hash ]; \
        debug_feature_time.time = static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ); \
        debug_feature_time.name = _name; \
    } \
    else { \
        g_features->visuals->debug_profiling_times[ name_hash ].update( static_cast< int32_t >( timer.get_ms_since_start( ).count( ) ) ); \
    } \
}
#else
#define profile_func( call, name, name_hash ) call
#endif

namespace features {
    static bool module_initialized = false;

    auto start_drawing_multi_core( ) -> void{
        g_render->add_draw_callback(
            [&]( ) -> void{
#if __DEBUG
                const utils::Timer render_timer;
#endif

                if ( !g_game_focused && !g_window->is_opened( ) ) return;
                std::thread one(
                    []( ) -> void{ run_catching( g_render_manager.force_update(); ) }
                );
                std::thread two(
                    []( ) -> void{ run_catching( g_minimap.force_update(); ) }
                );
                one.join( );
                two.join( );

#if enable_lua
                if ( g_lua2 ) {
                    g_lua2->execute_locked(
                        []( ) -> void{ g_lua2->run_callback( ct_hash( "renderer.draw" ) ); }
                    );
                }
                //
#endif

                if ( g_lua2 && g_lua2->should_reset( ) && g_config->lua.reload_on_unknown_exception->get< bool >( ) ) {
                    if ( g_lua2->should_reset( ) ) {
                        g_lua2 = std::make_unique< lua::LuaState >( );
                        g_lua2->execute_locked(
                            []( ) -> void{
                                for ( auto& script : g_scripts.scripts ) {
                                    if ( script.is_loaded ) g_lua2->try_run_file( script.path );
                                }
                            }
                        );
                    }
                }

                if ( !g_local ) return;

                if ( g_config->misc.lua_show_memory_uage->get< bool >( ) && g_lua2 ) {
                    g_debug_overlay->track_size( "LUA memory usage", g_lua2->get_memory_usage( ) );
                }

                // run c_feature::on_draw on all features
                for ( const auto feature : g_features->list ) {
#if __DEBUG
                    utils::Timer timer;
#endif
                    if ( feature->is_enabled( ) )
                        run_catching( feature->on_draw( ); )
#if __DEBUG
                    // g_debug_overlay.post_function_time(
                    //     std::format( "{}-render", feature->get_full_name( ) ),
                    //     timer.get_ms_since_start( ).count( )
                    // );
#endif
                }


                if ( g_features->current_module ) {
#if __DEBUG
                    const utils::Timer timer;
#endif
                    run_catching( g_features->current_module->on_draw( ); )

#if __DEBUG
                    // g_debug_overlay.post_function_time(
                    //     "current-module-render",
                    //     timer.get_ms_since_start( ).count( )
                    // );
#endif
                }

                // #if __DEBUG
                //                 g_debug_overlay.post_function_time(
                //                     "Feature rendering",
                //                     render_timer.get_ms_since_start( ).count( )
                //                 );
                // #endif
            }
        );
    }

    auto run_feature_list_multi_core(
        const std::vector< std::shared_ptr< IFeature > >& list,
        const bool                                        run_threaded = true
    ) -> void{
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

    auto run_menu_initializers_multi_core( ) -> void{
        for ( const auto feature : g_features->all_features )
            run_catching( feature->initialize_menu( ); )
    }

    namespace chrono = std::chrono;

    auto start_features_multi_core( ) -> void{
        auto last_gc = chrono::high_resolution_clock::now( );

        g_entity_list.start( );

        while ( app->should_run( ) ) {
            utils::Timer timer;

            // exit script when game exits
            // if ( app->feature_ticks % 10 == 0 &&
            //     app->memory &&
            //     app->memory->get_process( ) &&
            //     !app->memory->get_process( )->is_running( )
            // ) {
            //     app->unload( );
            //     return;
            // }

            if ( g_lua2 && g_lua2->should_reset( ) && g_config->lua.reload_on_unknown_exception->get< bool >( ) ) {
                g_lua2 = std::make_unique< lua::LuaState >( );
                for ( auto& script : g_scripts.scripts ) {
                    if ( script.is_loaded ) {
                        app->logger->info( "loading script {} on feature start", script.name );
                        g_lua2->execute_locked( [&script]( ) -> void{ g_lua2->try_run_file( script.path ); } );
                        g_config_system->load( _( "cfg" ) );
                    }
                }
            }


            run_catching( g_local.update( ); )
            if ( !g_local ) {
                // g_debug_overlay.post_chart_time( _( "Feature" ), timer.get_ms_since_start( ).count( ) / 1000000.f );
                std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
                continue;
            }

            if ( !g_features->entity_list ) g_features->entity_list = &g_entity_list;

            if ( !module_initialized && g_features->current_module ) {
                if ( g_entity_list.get_enemies( ).size( ) == 5 && g_entity_list.get_allies( ).size( ) == 5 ) {
                    module_initialized = true;
                    run_catching( g_features->current_module->on_all_champions_loaded( ); )
                }
            }
#if enable_new_lua
            if ( g_lua2 ) { g_lua2->execute_locked( []( ) -> void{ g_lua2->setup_globals( ); } ); }
#endif


            if ( !g_config->misc.core_league_topmost_check->get< bool >( ) || app->feature_ticks % 60 == 0 ) {
#if __DEBUG
                g_game_focused = true;
#else
                if ( g_config->misc.core_league_topmost_check->get< bool >( ) ) {
                    g_game_focused = app->memory->get_process( )->get_pid( ) == utils::get_focused_process_pid( );
                } else { g_game_focused = true; }
#endif
            }

            app->feature_ticks++;

            g_threading->feature_thread = std::this_thread::get_id( );

            if ( !g_game_focused && !g_window->is_opened( ) ) {
                static auto first_open_time = std::chrono::steady_clock::now( );


                // g_debug_overlay.post_chart_time( _( "Feature" ), timer.get_ms_since_start( ).count( ) / 1000000.f );
                std::this_thread::sleep_for( chrono::milliseconds( 5 ) );

                if ( std::chrono::duration_cast< std::chrono::seconds >(
                    std::chrono::steady_clock::now( ) - first_open_time
                ) > std::chrono::seconds( 10 ) )
                    continue;
            }

            if ( g_window->is_opened( ) ) {
                // g_debug_overlay.post_chart_time( _( "Feature" ), timer.get_ms_since_start( ).count( ) / 1000000.f );
                std::this_thread::sleep_for( chrono::milliseconds( 100 ) );
                continue;
            }

            // update time
            run_catching( g_time.update(); )

            // ss
            run_catching( g_pw_hud.update(); )

            // updating objects
            // run_catching( g_entity_list->pre_run( ); )
            run_catching( g_entity_list.run( ); )
            // run_catching( g_entity_list->post_run( ); )


            //updating menu state
            run_catching( g_menugui.update( ); )

#if enable_lua
            if ( g_lua2 ) {
                g_lua2->execute_locked( []( ) -> void{ g_lua2->run_callback( ct_hash( "features.pre_run" ) ); } );
            }
#endif
            run_feature_list_multi_core( g_features->pre_list, false );
#if enable_lua
            if ( g_lua2 ) {
                g_lua2->execute_locked( []( ) -> void{ g_lua2->run_callback( ct_hash( "features.run" ) ); } );
            }

#endif
            run_feature_list_multi_core( g_features->list, false );

            // Run LUA garbage collector
            static auto last_gc_time = std::chrono::high_resolution_clock::now( );
            if ( std::chrono::duration_cast< std::chrono::seconds >(
                std::chrono::high_resolution_clock::now( ) - last_gc_time
            ) > std::chrono::seconds( 1 ) ) {
                last_gc_time = std::chrono::high_resolution_clock::now( );
                if ( g_lua2 ) g_lua2->execute_locked( []( ) -> void{ g_lua2->collect_garbage( ); } );
            }

            // run queued inputs
            try {
                std::thread(
                    []( ) -> void{ run_catching( g_input->process_queue( ); ) }
                ).detach( );
            } catch ( std::exception& e ) { if ( app->sentry ) app->sentry->track_exception( e.what( ) ); }

            g_entity_list.run( );

            // utils::sentry_logger::update_runtime_info( );

            // submit timings
            // g_debug_overlay.post_chart_time( _( "Feature" ), timer.get_ms_since_start( ).count( ) / 1000000.f );

            std::this_thread::sleep_for( chrono::milliseconds( 1 ) );
        }

        debug_log( "feature cleanup" );

        g_threading->feature_thread = std::thread::id( );
        app->run_unload_callbacks( );
    }

    auto load_encuragements_multi_core( ) -> void{
        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "encouragements.txt" );
        if ( !std::filesystem::exists( path ) ) {
            std::ofstream s( path );
            s << "" << std::endl;
            return;
        }

        auto file = utils::read_file_to_string( path );


        if ( file.has_value( ) ) {
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
    }

    auto load_taunts_multi_core( ) -> void{
        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, "taunts.txt" );
        if ( !std::filesystem::exists( path ) ) {
            std::ofstream s( path );
            s << "" << std::endl;
            return;
        }

        auto file = utils::read_file_to_string( path );

        if ( file.has_value( ) ) {
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
    }

    auto initialize_multi_core( ) -> void{
#if enable_sentry
        if ( app->sentry ) app->sentry->set_tag( _( "runtime" ), _( "multi_core" ) );
#endif

        register_pre_feature( buff_cache, BuffCache );
        register_pre_feature( target_selector, TargetSelector );
        g_features->default_target_selector = g_features->target_selector;
        register_pre_feature( spell_detector, SpellDetector );

        register_feature( prediction, Prediction );
        register_feature( evade, Evade );
        register_feature( orbwalker, Orbwalker );
        register_feature( visuals, Visuals );
        register_feature( tracker, Tracker );
        register_feature( activator, Activator );

        load_encuragements_multi_core( );
        load_taunts_multi_core( );

#if enable_lua
        // initialize lua of all features
        g_lua2 = std::make_unique< lua::LuaState >( );
#endif
        run_menu_initializers_multi_core( );

        // g_features->entity_list = g_entity_list;

        // wait for league of legends to be started
        sdk::memory::Process process;
        while ( !process ) {
            process = sdk::memory::Process( ct_hash( "League of Legends.exe" ) );
            if ( !process ) std::this_thread::sleep_for( chrono::milliseconds( 500 ) );
            if ( !app->should_run( ) ) return;
        }

        const auto initialize_sdk_result = sdk::initialize_globals( process );

        if ( !initialize_sdk_result ) {
            static auto last_error = sdk::EInitializeGlobalsError::max;
            if ( last_error != initialize_sdk_result.error( ) ) {
                if ( app->logger ) {
                    app->logger->info(
                        "error initializing globals: {}",
                        static_cast< int32_t >( initialize_sdk_result.error( ) )
                    );
                }
                last_error = initialize_sdk_result.error( );
            }
            return;
        }

        if ( !g_local ) return;

#if !__DEBUG
        static auto did_sent_champion_name_to_analytics = false;
        if ( g_local && !did_sent_champion_name_to_analytics && g_local->champion_name.is_valid( ) ) {
            app->mixpanel->track_event(
                "champion_name",
                {
                    { "champion", std::make_any< std::string >( std::string( g_local->champion_name.text ) ) },
                    { "level", std::make_any< int32_t >( g_local->level ) },
                    { "kills", std::make_any< int32_t >( g_local->get_kills( ) ) }
                }
            );

            did_sent_champion_name_to_analytics = true;
        }
#endif

#if enable_auth
        std::thread(
            []( ) -> void{
                try {
                    if ( !g_local->name.text ) return;

                    auto current_username = std::string( g_local->name.text );
                    g_user->submit_username( current_username );
                } catch ( std::exception& e ) {
                }
            }
        ).detach( );
#endif

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
            run_catching( g_features->current_module->initialize( ); )
            run_catching( g_features->current_module->initialize_menu( ); )
            g_window->m_mutex.unlock( );
        } else if ( !g_features->current_module )
#endif
        {
            champion_modules::initialize( );
            g_window->m_mutex.lock( );
            g_features->current_module = g_features->get_module_for_champion( rt_hash( g_local->champion_name.text ) );
            g_features->modules.clear( );
            if ( g_features->current_module ) {
                //std::lock_guard lock( g_window->m_mutex );
                run_catching( g_features->current_module->initialize( ); )
                run_catching( g_features->current_module->initialize_menu( ); )
            }
            g_window->m_mutex.unlock( );
        }

        app->logger->info( "initialized" );

        // add drawing callback and call drawing function from all features
        start_drawing_multi_core( );
        // run features
        start_features_multi_core( );
    }

    auto on_lua_reset_multi_core( ) -> void{
#if enable_lua
        for ( const auto feature : g_features->all_features )
            run_catching( feature->on_lua_reset( ); )
        // g_entity_list->on_lua_reset( );
#endif
    }

    auto on_lua_load_multi_core( ) -> void{
        module_initialized = false;
        debug_fn_call( )
        g_features->current_module = nullptr;
#if enable_lua
        for ( const auto feature : g_features->all_features ) { run_catching( feature->on_lua_load( ); ) }
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
            run_catching( g_features->current_module->initialize( ); )
            run_catching( g_features->current_module->initialize_menu( ); )
        } else
#endif
        {
            champion_modules::initialize( );
            g_features->current_module = g_features->get_module_for_champion( rt_hash( g_local->champion_name.text ) );
            g_features->modules.clear( );
            if ( g_features->current_module ) {
                run_catching( g_features->current_module->initialize( ); )
                run_catching( g_features->current_module->initialize_menu( ); )
            }
        }
    }
}
