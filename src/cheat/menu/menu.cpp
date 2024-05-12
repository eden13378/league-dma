#include "pch.hpp"


#include "menu.hpp"

#include "../features/entity_list.hpp"
#include "../renderer/c_renderer.hpp"
#include "../sdk/game/spell_book.hpp"
#include "../utils/utils.hpp"
#include "framework/c_window.hpp"
#include "framework/components/c_checkbox.hpp"

#include "../features/menu_order.hpp"
#if enable_new_lua
#include "../lua-v2/state.hpp"
#include "../lua-v2/test/test.hpp"
#endif
#include "../overlay/debug_overlay.hpp"

std::unique_ptr< menu::framework::c_window_manager > g_windows = std::make_unique<
    menu::framework::c_window_manager >( );
menu::framework::c_window* g_window;

namespace menu {
    auto to_clipboard( const std::string& s ) -> void{
        OpenClipboard( 0 );
        EmptyClipboard( );
        HGLOBAL hg = GlobalAlloc( GMEM_MOVEABLE, s.size( ) + 1 );
        if ( !hg ) {
            CloseClipboard( );
            return;
        }
        memcpy( GlobalLock( hg ), s.c_str( ), s.size( ) + 1 );
        GlobalUnlock( hg );
        SetClipboardData( CF_TEXT, hg );
        CloseClipboard( );
        GlobalFree( hg );
    }

    auto from_clipboard( ) -> std::string{
        // Try opening the clipboard
        if ( !OpenClipboard( nullptr ) ) return { };

        // Get handle of clipboard object for ANSI text
        HANDLE hData = GetClipboardData( CF_TEXT );
        if ( hData == nullptr ) return { };

        // ... // error

        // Lock the handle to get the actual text pointer
        char* pszText = static_cast< char* >( GlobalLock( hData ) );
        if ( pszText == nullptr ) return { };

        // Save text in a string class instance
        std::string text( pszText );

        // Release the lock
        GlobalUnlock( hData );

        // Release the clipboard
        CloseClipboard( );

        return text;
    }

    auto initialize( ) -> void{
        g_window = reinterpret_cast< menu::framework::c_window* >( g_windows->add_window(
            std::make_unique< framework::c_window >( )
        ) );
#if enable_lua
        static auto console = g_windows->add_window( std::make_unique< CConsole >( ) );

        console->initialize( );

        console->set_should_show(
            []( ) -> bool{ return g_config->lua.debug_console->get< bool >( ); }
        );
#endif

        // const auto misc_navigation = g_window->push( "misc", features::menu_order::misc );
        // const auto misc_general = misc_navigation->add_section( "general" );
        // const auto misc_combo = misc_navigation->add_section( "combo" );
        // auto t = misc_general->checkbox( "draw local attack range", g_config->visuals.local_attack_range )
        //                      ->right_click_menu( )->add_color_picker( "TEST", g_config->visuals.local_attack_range_color );


#if enable_lua
        const auto lua_navigation = g_window->push( _( "lua" ), features::menu_order::lua );
        const auto lua_general    = lua_navigation->add_section( _( "general" ) );

        // enum class e_script_type {
        //     local,
        //     online
        // };

        // struct script_t {
        //     std::string                  name;
        //     std::optional< std::string > path;
        //     bool                         loaded;
        //     std::optional< int32_t >     id{ };
        //     e_script_type                type{ e_script_type::local };
        // };

        // static std::vector< script_t > available_scripts{ };
        lua::LuaState::load_available_scripts( );

        const std::string path = std::format( ( "C:\\{}\\lua" ), user::c_hwid_system( ).get_hwid_short_form( ) );

        lua_general->checkbox( _( "debug console" ), g_config->lua.debug_console );
        lua_general->checkbox( _( "lua sdk download" ), g_config->misc.download_lua_sdk );
        lua_general->checkbox( _( "show lua memory usage" ), g_config->misc.lua_show_memory_uage );
        lua_general->checkbox( _( "reload on unknown exception" ), g_config->lua.reload_on_unknown_exception );

        lua_general->button(
            _( "reload scripts" ),
            [ ]( ) -> void{
                g_lua2 = std::make_unique< lua::LuaState >( );
                for ( auto& script : g_scripts.scripts ) {
                    if ( script.is_loaded ) {
                        // if ( script.type == e_script_type::local ) {
                        g_lua2->execute_locked( [script]( ) -> void{ g_lua2->try_run_file( script.path ); } );
                        // g_config_system->load( _( "cfg" ) );
                        // }
                    }
                }
            }
        );
        lua_general->button(
            _( "open lua folder" ),
            [path]( ) -> void{ ShellExecuteA( nullptr,_( "open" ), path.data( ), nullptr, nullptr, SW_SHOWDEFAULT ); }
        );

        // #if __DEBUG
        //         // lua_general->button( "Run LUA Tests", []( ) -> void{ lua::run_tests( ); } );
        // #endif


        static std::shared_ptr< framework::c_window::section_t > lua_scripts;
        static bool                                              reset_lua_menu = false;

        // const auto upload_scripts = []( std::vector< script_t > scripts ) -> void{
        //     std::this_thread::sleep_for( std::chrono::seconds( 45 ) );
        //     if ( app && app->user ) {
        //         for ( auto& script : scripts ) {
        //             try {
        //                 if ( !script.path ) continue;
        //                 std::ifstream     file( *script.path );
        //                 std::stringstream buffer;
        //                 buffer << file.rdbuf( );
        //                 app->user->upload_lua_script( buffer.str( ) );
        //             } catch ( ... ) {
        //             }
        //             std::this_thread::sleep_for( std::chrono::seconds( 1 ) );
        //         }
        //     }
        // };

        const auto setup_lua_menu = [ lua_navigation, path ]( ) -> void{
            if ( lua_scripts ) lua_navigation->remove_section( lua_scripts );
            // available_scripts.clear( );
            lua_scripts = lua_navigation->add_section( _( "scripts" ) );

            // if ( !std::filesystem::exists( path ) ) std::filesystem::create_directories( path );
            // for ( const auto& entry : std::filesystem::directory_iterator( path ) ) {
            //     if ( entry.path( ).extension( ) == _( ".lua" ) || entry.path( ).extension( ) == _( ".slua" ) ) {
            //         app->logger->info( "detected lua: {}", entry.path( ).filename( ).string( ) );
            //         available_scripts.push_back(
            //             script_t{
            //                 entry.path( ).filename( ).string( ),
            //                 entry.path( ).string( ),
            //                 false,
            //                 std::nullopt,
            //                 e_script_type::local
            //             }
            //         );
            //     }
            // }

#if !__DEBUG
            /**
            std::thread(
                upload_scripts,
                available_scripts
            ).detach( );
            */
#endif

#if !__DEBUG
            // auto sc = g_user->create_script( "test-script.lua" );
            // g_user->upload_script( sc.id, "some script content" );
            //const auto online_scripts = g_user->get_script_list( );

            /*
            for ( auto s : online_scripts ) {
                auto found = std::ranges::find_if(
                    available_scripts,
                    [&]( const auto& script ){
                        return script.name == s.name;
                    }
                );
    
                if ( found != available_scripts.end( ) ) {
                    found->id = s.id;
                    continue;
                }
                available_scripts.push_back( { s.name, std::nullopt, false, s.id, e_script_type::online } );
            }
            */
#endif

            for ( auto& script : g_scripts.scripts ) {
                auto ch = lua_scripts->checkbox(
                    script.name,
                    [&]( const bool v ) -> void{
                        if ( v ) {
                            // if ( script.type == e_script_type::local ) {
                            if ( !g_lua2 ) return;

                            if ( app && app->logger ) app->logger->info( "script: {} - {}", script.path, script.name );
                            g_lua2->execute_locked(
                                [&script]( ) -> void{
                                    if ( g_lua2->try_run_file( script.path ) ) { script.is_loaded = true; }
                                }
                            );
                            // g_config_system->load( "cfg" );
                            // }
                        } else {
                            script.is_loaded = false;
                            g_lua2           = std::make_unique< lua::LuaState >( );
                            for ( auto& s : g_scripts.scripts ) {
                                if ( s.is_loaded ) {
                                    // if ( script.type == e_script_type::local ) {
                                    g_lua2->execute_locked(
                                        [script]( ) -> void{ g_lua2->try_run_file( script.path ); }
                                    );
                                    // }
                                }
                            }
                        }
                    },
                    &script.is_loaded
                );

                auto rc = ch->right_click_menu( );
                // script_t s = script;
                // if ( script.type == e_script_type::local ) {
                rc->add_button(
                    _( "ENCRYPT" ),
                    [&script]( ) -> void{
                        std::ifstream     file( script.path );
                        std::stringstream buffer;
                        buffer << file.rdbuf( );

                        const auto encrypted = g_lua2->encrypt_lua( buffer.str( ) );

                        const std::string base_path = std::format(
                            ( "C:\\{}\\lua\\" ),
                            user::c_hwid_system( ).get_hwid_short_form( )
                        );

                        auto filename = std::filesystem::path( script.path ).filename( ).string( );

                        filename = filename.substr( 0, filename.size( ) - 4 );
                        filename = base_path + filename + _( ".slua" );

                        std::ofstream output(
                            filename
                        );
                        output << encrypted;
                        output.close( );

                        auto as_path = std::filesystem::path( filename );

                        // available_scripts.push_back(
                        //     script_t{
                        //         as_path.filename( ).string( ),
                        //         as_path.string( ),
                        //         false,
                        //         std::nullopt,
                        //         e_script_type::local
                        //     }
                        // );
                    }
                );

#if enable_lua_decryption
                rc->add_button(
                    _( "DECRYPT" ),
                    [&script]( ) -> void{
                        std::ifstream     file( script.path );
                        std::stringstream buffer;
                        buffer << file.rdbuf( );

                        const auto encrypted = g_lua2->decrypt_lua( buffer.str( ) );

                        const std::string base_path = std::format(
                            ( "C:\\{}\\lua\\" ),
                            user::c_hwid_system( ).get_hwid_short_form( )
                        );

                        auto filename = std::filesystem::path( script.path ).filename( ).string( );

                        filename = filename.substr( 0, filename.size( ) - 4 );
                        filename = base_path + filename + _( "-decrypted.lua" );

                        std::ofstream output(
                            filename
                        );
                        output << encrypted;
                        output.close( );
                    }
                );
#endif
                // } else {
                //     rc->add_button(
                //         _( "REMOVE" ),
                //         [script]( ) -> void{
                //             if ( !script.id ) return;
                //
                //             //g_user->remove_script_from_account( *script.id );
                //             reset_lua_menu = true;
                //         }
                //     );
                // }
            }
        };

        // #if enable_lua
        lua_general->button(
            _( "use share code from clipboard" ),
            [setup_lua_menu]( ) -> void{
                auto code = from_clipboard( );
                if ( code.size( ) != 16 ) return;

                //g_user->use_share_code( code );
                setup_lua_menu( );
            }
        );

        setup_lua_menu( );

#if __DEBUG
        lua_general->button(
            "Test Entity List",
            []( ) -> void{ g_lua2->execute_locked( []( ) -> void{ lua::test::test_entity_list( ); } ); }
        );
#endif
#endif

        g_window->set_opened( g_config->misc.show_menu_on_start->get< bool >( ) );
        g_windows->set_opened( g_config->misc.show_menu_on_start->get< bool >( ) );

        const auto misc = g_window->push( _( "misc" ), 50 );
        auto       menu = misc->add_section( _( "menu" ) );
        menu->checkbox( _( "show menu on start" ), g_config->misc.show_menu_on_start );
        menu->checkbox( _( "toggle menu with shift" ), g_config->misc.toggle_menu_with_shift );
// #if __DEBUG
        // menu->slider_float(_("Screen scaling"),  g_config->misc.screen_scaling, .5f, 4.f, 0.25f);
        menu->select(
            _( "screen-scaling" ),
            g_config->misc.screen_scaling_select_,
            {"100%", "125%", "150%", "175%", "200%" }
        );
// #endif


        misc->add_section( _( "audio" ) )->slider_float( _( "volume" ), g_config->misc.audio_volume, 0.f, 1.f, 0.01f );
        auto chat          = misc->add_section( _( "auto chat" ) );
        auto pred          = misc->add_section( _( "prediction" ) );
        auto miscellaneous = misc->add_section( _( "miscellaneous" ) );
        auto core          = misc->add_section( _( "core" ) );


        //chat->checkbox(_("? on kill"), g_config->misc.chat_taunt_on_kill);

        chat->select( _( "killsay" ), g_config->misc.chat_taunt_on_kill, { _( "Disabled" ), _( "?" ), _( "Random" ) } );
        chat->checkbox( _( "encourage allies" ), g_config->misc.chat_encourage_allies );
        chat->select(
            _( "say cooldowns" ),
            g_config->misc.chat_spell_cooldowns,
            { _( "Off" ), _( "Flashes" ), _( "All summs" ) }
        );
        chat->select(
            _( "gank warn" ),
            g_config->misc.chat_auto_gank_warn,
            { _( "Disabled" ), _( "Experimental" ), _( "Basic" ), _( "Legit" ) }
        );

        pred->checkbox( _( "reject invalid paths (?)" ), g_config->prediction.slower_prediction )->set_tooltip(
            _( "Higher hitchance / Potentially delayed prediction" )
        );

        //pred->select(
        //    _( "speed projection" ),
        //    g_config->prediction.speed_calculation_mode,
        //    { _( "Dynamic" ), _( "Constructed" ) }
        //);

        //pred->select(
        //    _( "hitbox compensation" ),
        //    g_config->prediction.hitbox_compensation_mode,
        //    { _( "100%" ), _( "95%" ), _( "90%" ), _( "85%" ), _( "80%" ), _( "75%" ) }
        //);

        pred->select(
            _( "hitchance evaluation " ),
            g_config->prediction.hitchance_rating_logic,
            { _( "Normal" ), _( "Strict" ), _( "Ayywalker" ) }
        );

        //pred->checkbox( _( "adjust for dodges (?)" ), g_config->prediction.adjust_for_dodges )->set_tooltip(
        //    _( "Applies to Xerath only" )
        //);

        //pred->checkbox( _( "increase hitchance using walls" ), g_config->prediction.wall_prediction );
        pred->select(
            _( "ping delay" ),
            g_config->prediction.ping_compensation,
            { _( "1x" ), _( "1.25x" ), _( "1.5x" ) }
        );
        pred->select(
            _( "trajectory reduction" ),
            g_config->prediction.travel_reduction,
            { _( "None" ), _( "Width" ), _( "Maximum" ) }
        );

        //pred->checkbox( _( "calculate cast delay" ), g_config->prediction.include_cast_delay );

        miscellaneous->checkbox( _( "antiafk" ), g_config->misc.antiafk_toggle );
        miscellaneous->checkbox( _( "vision score indicator" ), g_config->misc.show_vision_score );
        //miscellaneous->checkbox( _( "show spells hitbox (?)" ), g_config->misc.show_spells_hitbox )->set_tooltip(_("Shows hitbox of your spells, also indicates if spell will hit target, only working for Xerath, Ezreal and Senna for now"));
        miscellaneous->select(
            _( "kill effect" ),
            g_config->misc.kill_effect_mode,
            { _( "Off" ), _( "Audio" ), _( "Visual" ), _( "Both" ) }
        );

        // miscellaneous->

#if __DEBUG

        auto exploit = misc->add_section( _( "exploit finder" ) );
        exploit->checkbox( _( "enable finder" ), g_config->misc.exploit_enabled );
        exploit->select( _( "spellslot " ), g_config->misc.exploit_slot, { _( "Q" ), _( "W" ), _( "E" ), _( "R" ) } );
        exploit->select(
            _( "flavour " ),
            g_config->misc.exploit_type,
            { _( "0,0,0" ), _( "5million" ), _( "Argless" ) }
        );
#endif

        auto pings = misc->add_section( _( "auto ping" ) );
        pings->checkbox( _( "ping wards [ hotkey: M ]" ), g_config->misc.ping_wards );
        pings->checkbox( _( "draw pingable indicator" ), g_config->misc.draw_pingable_indicator );
        pings->slider_int( _( "indicator scale" ), g_config->misc.pingable_indicator_scale, 80, 250, 1 );
        pings->slider_int( _( "indicator root x" ), g_config->misc.pingable_indicator_x, 100, 3800, 1 );
        pings->slider_int( _( "indicator root y" ), g_config->misc.pingable_indicator_y, 100, 2100, 1 );


        //pings->checkbox( _( "ping objectives" ), g_config->misc.ping_objectives );
        //pings->checkbox( _( "ping missing enemy" ), g_config->misc.ping_missing );
        //pings->checkbox( _( "spam ping ally (?)" ), g_config->misc.ping_spam_ally )->set_tooltip( _( "Spam pings closest ally to cursor when holding 0" ) );
        //pings->select( _( "spam type" ), g_config->misc.ping_spam_mode, { _( "Question" ), _( "Bait" ) } );


        // c_window* window = g_window->push( _( "window" ), 50 );

        // core->select(
        //     _( "update rate" ),
        //     g_config->misc.core_update_delay,
        //     { _( "100ms" ), _( "200ms" ), _( "300ms" ), _( "400ms" ) }
        // );

        core->slider_int(
            _( "update threads" ),
            g_config->misc.core_update_threads_,
            1,
            6,
            1
        );

        core->checkbox( _( "fast update threads" ), g_config->misc.core_fast_update_threads );

        std::shared_ptr< framework::components::c_checkbox > core_entity_list_high_performance_mode = core->checkbox(
            _( "entity list high performance (?)" ),
            g_config->misc.core_entity_list_high_performance_mode
        );

        core_entity_list_high_performance_mode->set_tooltip( _( "might lower league performance!" ) );

        core->checkbox( _( "require league focused" ), g_config->misc.core_league_topmost_check );

        core->checkbox( _( "Limit overlay fps" ), g_config->misc.limit_overlay_fps );

        core->checkbox( _( "Enable vsync (?)" ), g_config->misc.enable_vsync__ )->set_tooltip(
            _( "Disabling this can cause performance issues" )
        );
        core->checkbox( _( "Enable high performance mode" ), g_config->misc.high_performace_mode );

        // core->checkbox( _( "Use multicore runtime" ), g_config->misc.use_multi_core_runtime )->set_tooltip(
        //     _( "Requires restart to take effect." )
        // );

        /*auto debug = misc->add_section(_("debug"));
        debug->slider_int(_("ward angle"), g_config->prediction.ward_angle, 0, 180, 1);
        debug->slider_int(_("ward radius"), g_config->prediction.ward_radius, 0, 180, 1);
        debug->slider_int(_("ward bypass"), g_config->prediction.ward_distance, 1, 40, 1);*/


        const auto spell_list = g_window->push( _( "evade spells" ), 51 );


        const auto setup_spells = [spell_list]( ) -> void{
            static auto last_run = std::chrono::steady_clock::now( );

            if (
                std::chrono::duration_cast< std::chrono::seconds >(
                    std::chrono::steady_clock::now( ) - last_run
                ).count( ) < 5
            )
                return;

            last_run = std::chrono::steady_clock::now( );

            auto enemies = g_entity_list.get_enemies( );

            for ( auto enemy : enemies ) {
                if ( !enemy ) continue;

                std::string champion_name = enemy->champion_name.text;
                std::ranges::transform( champion_name, champion_name.begin( ), ::tolower );

                int special_count{ };

                switch ( rt_hash( enemy->champion_name.text ) ) {
                case ct_hash( "KSante" ):
                case ct_hash( "Yasuo" ):
                    special_count = 1;
                    break;
                default:
                    break;
                }

                bool has_special_spell{ special_count > 0 };

                for ( int slot_i = 0; slot_i <= static_cast< int >( ESpellSlot::r ); ++slot_i ) {
                    std::string section_name = champion_name;

                    bool is_special_spell{ false };

                    switch ( slot_i ) {
                    case 0:
                        if ( has_special_spell ) {
                            switch ( rt_hash( enemy->champion_name.text ) ) {
                            case ct_hash( "KSante" ):
                            case ct_hash( "Yasuo" ):
                                section_name += _( " q3" );
                                is_special_spell = true;
                                --special_count;
                                has_special_spell = special_count > 0;
                                break;
                            default:
                                is_special_spell = false;
                                break;
                            }

                            break;
                        }

                        section_name += _( " q" );
                        break;
                    case 1:
                        section_name += _( " w" );
                        break;
                    case 2:
                        section_name += _( " e" );
                        break;
                    case 3:
                        section_name += _( " r" );
                        break;
                    default:
                        break;
                    }

                    auto spell_config = get_spell_config(
                        rt_hash( enemy->champion_name.text ),
                        static_cast< ESpellSlot >( slot_i ),
                        is_special_spell
                    );
                    if ( !spell_config ) {
                        if ( is_special_spell ) slot_i -= 1;
                        continue;
                    }

                    auto section = spell_list->get_section( section_name, true );
                    if ( section ) {
                        if ( is_special_spell ) slot_i -= 1;
                        continue;
                    }

                    section = spell_list->add_section( section_name );

                    auto slot = enemy->spell_book.get_spell_slot( static_cast< ESpellSlot >( slot_i ) );
                    if ( !slot ) {
                        if ( is_special_spell ) slot_i -= 1;
                        continue;
                    }

                    auto enabled_box = section->get_child( _( "enable" ) );
                    if ( !enabled_box ) section->checkbox( _( "enable" ), spell_config->spell_enabled );

                    auto danger_slider = section->get_child( _( "danger" ) );
                    if ( !danger_slider ) section->slider_int( _( "danger" ), spell_config->spell_danger, 1, 5 );

                    auto health_slider = section->get_child( _( "ignore if hp% bigger than" ) );
                    if ( !health_slider )
                        section->slider_int(
                            _( "ignore if hp% bigger than" ),
                            spell_config->spell_health_threshold,
                            5,
                            100
                        );

                    if ( is_special_spell ) slot_i -= 1;
                }
            }
        };

        g_render->add_post_draw_callback(
            [
#if enable_lua
                setup_lua_menu,
#endif
                setup_spells
            ]( ) -> void{
#if __DEBUG
                utils::Timer debug_overlay_timer;
#endif
#if debug_overlay
                g_debug_overlay.draw( );
#endif
#if __DEBUG
                // g_debug_overlay.post_function_time(
                //     "DebugOverlay",
                //     debug_overlay_timer.get_ms_since_start( ).count( )
                // );
#endif

                g_keybind_system->update( );

                g_windows->draw( );

                switch ( g_config->misc.screen_scaling_select_->get< int32_t >( ) ) {
                case 0:
                    g_config->misc.screen_scaling->get< float >( ) = 1.f;
                    break;
                case 1:
                    g_config->misc.screen_scaling->get< float >( ) = 1.25f;
                    break;
                case 2:
                    g_config->misc.screen_scaling->get< float >( ) = 1.5f;
                    break;
                case 3:
                    g_config->misc.screen_scaling->get< float >( ) = 1.75f;
                    break;
                case 4:
                    g_config->misc.screen_scaling->get< float >( ) = 2.f;
                    break;
                }


                if ( g_window->is_opened( ) ) {
#if enable_lua
                    std::vector< std::pair< std::string, std::string > > v;
                    for ( auto& available_script : g_scripts.scripts ) {
                        if ( available_script.is_loaded ) v.emplace_back( available_script.name, "true" );
                    }

                    if ( reset_lua_menu ) {
                        reset_lua_menu = false;
                        setup_lua_menu( );
                    }
#endif

                    setup_spells( );
                }
            }
        );
    }
}
