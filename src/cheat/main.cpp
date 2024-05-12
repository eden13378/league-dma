#include "pch.hpp"

#include "build.hpp"
// #if enable_sentry
#pragma comment(lib, "winhttp.lib")
#pragma comment(lib, "Dbghelp.lib")
#pragma comment(lib, "version.lib")
// #endif

#pragma comment(lib, "Ws2_32.lib")
// #pragma comment(lib, "sentry.lib")
#pragma comment(lib, "version.lib")
#pragma comment(lib, "CRYPT32.lib")
#if __DEBUG
#pragma comment(lib, "libcurl-d.lib")
#else
#pragma comment(lib, "libcurl.lib")
#pragma comment(lib, "fmt.lib")
#endif


#include "auth/encryption.hpp"
#include "features\runtime\runtime.hpp"
#include "overlay/overlay.hpp"
#include "renderer/c_renderer.hpp"

#include "config/c_config.hpp"
#if enable_new_lua
#include "lua-v2/state.hpp"
#endif
// #include "lua/c_lua.hpp"

#if enable_new_lua
#include "lua-v2/sdk_download.hpp"
#endif
#include "menu/menu.hpp"
#include "renderer/c_fonts.hpp"
#include "utils/crash_detector.hpp"
#include "utils\keybind_system.hpp"
#include "utils/resource_updater.hpp"
#include "utils/utils.hpp"


#if enable_auth
user::c_user* g_user;
#endif
Renderer*             g_render         = new Renderer( );
Fonts*                g_fonts          = new Fonts( );
utils::KeybindSystem* g_keybind_system = new utils::KeybindSystem( );
config::c_config*     g_config;

#ifndef enable_sentry
#include "version.hpp"
#endif

enum class EStartupError {
    auth_timeout = 0x2,
    login_failure,
    login_exception
};

auto format_error_message( EStartupError code ) -> std::string{
    return std::format( "Error 0x{:x}. Exiting", static_cast< int32_t >( code ) );
}

auto show_error_message_box( const EStartupError code ) -> void{
    static auto did_show = false;
    if ( did_show ) return;
    did_show = true;
    MessageBoxA(
        nullptr,
        format_error_message( code ).c_str( ),
        _( "Error" ),
        MB_OK
    );
}

auto main( int argc, char* argv[ ] ) -> int{
    auto old_log = std::format( ( "C:\\{}\\log.log" ), user::c_hwid_system( ).get_hwid_short_form( ) );

    if ( std::filesystem::exists( old_log ) ) {
        auto new_path = std::format( ( "C:\\{}\\log_o.log" ), user::c_hwid_system( ).get_hwid_short_form( ) );
        if ( std::filesystem::exists( new_path ) ) std::filesystem::remove( new_path );

        std::filesystem::copy_file(
            old_log,
            std::format( ( "C:\\{}\\log_o.log" ), user::c_hwid_system( ).get_hwid_short_form( ) )
        );
    }

    app->setup_mixpanel( );

    spdlog::init_thread_pool( 8192, 1 );

    app->setup_logger( );

#if __DEBUG
    app->logger->info( "enc: {}", user::encryption::encrypt( "3jvcNse9SUl5vXz_AURO274NNNl7S8SpRERMRObsDT_zZojDkRZUaqMqIFoqDuQ_9FvCq5ZiOn4NTPc7QRdL1HVuXT0Wfn2O1z92nWe0HFRGVpuryHQaMh8rUUKmQ3JF" ) );
#endif

#if enable_sentry
    app->setup_sentry( );
#endif

#if enable_auth
    if ( argc <= 1 ) {
#if enable_sentry
        app->sentry->track_exception( _( "started without auth token" ) );
#endif
        return 50;
    }

    std::string api_url = _( "https://slt.strainnews.biz/product" );

    // setup auth
    g_user = new user::c_user(
        api_url,
        _(
            "a5U0DbGIFHBYeKufA2TfmNliRsexy0HXwwCiTmrlKnvBZ-dwJ93aXFyWSZ5EXVItfbYj5jnStoGVwkk-WRz0h2ifS2nwSGs21-iMLaryL_8BasGkirwiu4koxUbrF5ik"
        )
    );
#endif


    app->logger->info( _( "setup config" ) );

    g_config = new config::c_config( );

    app->logger->info( "config setup done" );

#if enable_auth
    const auto login_transaction = app->mixpanel->track_time( "startup", { } );

    if ( argc <= 1 ) return 50;
    const auto start_token = user::encryption::decrypt( argv[ argc - 1 ] );

    bool use_login_v2 = false;
    if ( argc >= 3 && std::string( argv[ argc - 2 ] ) == "v2" ) { use_login_v2 = true; }

    try {
        // app->logger->info( "use_login_v2: {}", use_login_v2 );
        const auto login_result = use_login_v2 ? g_user->login_v2( start_token ) : g_user->login( start_token );

        if ( !login_result ) {
            login_transaction->done( );
            if ( app->logger ) app->logger->error( format_error_message( EStartupError::login_failure ) );
#if enable_sentry
            app->sentry->track_exception( _( "login failed" ) );
#endif
            std::thread(
                []( ) -> void{ show_error_message_box( EStartupError::login_failure ); }
            ).detach( );

            std::this_thread::sleep_for( std::chrono::seconds( 15 ) );

            return 51;
        }
    } catch ( ... ) {
        if ( app->logger ) app->logger->error( format_error_message( EStartupError::login_exception ) );
        std::thread(
            []( ) -> void{ show_error_message_box( EStartupError::login_exception ); }
        ).detach( );

        std::this_thread::sleep_for( std::chrono::seconds( 15 ) );
        return 51;
    }
    g_user->set_exit_function(
        []( ) -> void{
            if ( app->logger ) app->logger->error( format_error_message( EStartupError::auth_timeout ) );

            std::thread(
                []( ) -> void{ show_error_message_box( EStartupError::auth_timeout ); }
            ).detach( );

            app->unload( );
        }
    );
#if !__BETA
    g_user->start_check_thread( );
#endif

    login_transaction->done( );
#endif

#if enable_auth
    app->logger->info( "checking for crashes" );
    crash_detector::check( );
#endif

#if !__DEBUG
    const auto track_config_load_time = app->mixpanel->track_time( "initial_config_load", { } );
#endif
    g_config_system->load( _( "cfg" ) );
#if !__DEBUG
    if ( !track_config_load_time->done( ) ) app->logger->error( "error saving config load time" );
#endif

#if enable_new_lua
    if ( g_config->misc.download_lua_sdk->get< bool >( ) ) {
        auto sdk_downloader = lua::SdkDownloader( );
        sdk_downloader.download_sdk( );
    }
#endif

#if __DEBUG
    {
        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        auto       base_path       = std::format( ( "C:\\{}\\" ), hwid_short_form );
        auto       path            = std::format( ( "{}export_configs\\" ), base_path );
        if ( !std::filesystem::exists( path ) ) std::filesystem::create_directories( path );

        for ( auto path : std::filesystem::recursive_directory_iterator( path ) ) {
            if ( path.is_directory( ) ) continue;
            auto file = path.path( );
            if ( file.extension( ) == ".json" ) continue;

            auto short_path = file.string( ).substr( base_path.size( ) );


            debug_log( "loading {} for export", short_path );
            g_config_system->load( short_path.c_str( ) );
            debug_log( "exporting {} to {}", short_path, base_path + short_path + ".json" );
            g_config_system->export_named( base_path + short_path + ".json" );
        }
    }

#endif

#if enable_sentry && enable_auth
    if ( app->sentry ) {
        app->sentry->identify_user( g_user->get_username( ), std::nullopt, std::nullopt, std::nullopt );

        app->sentry->add_context_object(
            _( "EntityList" ),
            []( auto builder ) -> void{
                builder->add_bool(
                    _( "core_entity_list_high_performance_mode" ),
                    g_config->misc.core_entity_list_high_performance_mode->get< bool >( )
                );
                builder->add_int( "core_fast_update_threads", g_config->misc.core_fast_update_threads->get< bool >( ) );
                builder->add_int( "core_update_threads_", g_config->misc.core_update_threads_->get< int32_t >( ) );
                builder->add_int( "core_update_delay", g_config->misc.core_update_delay->get< int32_t >( ) );
            }
        );
    }
#endif

#if enable_auth
    if ( app->mixpanel ) {
        app->mixpanel->set_username( g_user->get_username( ) );
        app->mixpanel->set_user_id( g_user->get_user_id( ) );
    }
#endif

    auto resource_updater_thread = std::thread(
        []( ) -> void{
            app->logger->info( "checking for resource file updates" );
            try { utils::resource_updater::update( ); } catch ( ... ) {
                if ( app && app->logger ) app->logger->error( "error updating resources" );
            }
        }
    );
    resource_updater_thread.detach( );


    try {
        if ( std::filesystem::exists( "C:\\hwinfo32.dll" ) ) { std::filesystem::remove( "C:\\hwinfo32.dll" ); }
    } catch ( ... ) {
    }
#if enable_auth
    try {
        if ( std::filesystem::exists( "C:\\hwinfo64.dll" ) ) { std::filesystem::remove( "C:\\hwinfo64.dll" ); }
    } catch ( ... ) {
    }
#endif

    app->logger->info( "features" );
    // if ( g_config->misc.use_multi_core_runtime->get< bool >( ) ) {
        app->logger->info( "initializing multi core" );

        overlay::initialize( );
        menu::initialize( );
        features::initialize_multi_core( );
    // } else {
    //     app->logger->info( "initializing single core" );
    //     utils::set_current_thread_priority( THREAD_PRIORITY_HIGHEST );
    //
    //     menu::initialize( );
    //     features::initialize_single_core( );
    //     overlay::initialize( );
    // }

    debug_fn_call( )
    if ( app && !app->should_run( ) ) app->unload( );
    crash_detector::clean( );

    return 0;
}
