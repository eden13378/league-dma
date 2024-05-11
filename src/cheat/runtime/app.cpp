#include "pch.hpp"
#include "app.hpp"

#include "../version.hpp"
#include "../lua-v2/state.hpp"

#include "../utils/crash_detector.hpp"


std::unique_ptr< runtime::App > app = std::make_unique< runtime::App >( );

namespace runtime {
    auto App::unload( ) -> void{
        app->logger->info( "unloading" );

        try { crash_detector::clean( ); } catch ( ... ) {
        }

#if enable_sentry
        if ( app->sentry ) {
            app->sentry->add_context_object(
                "unload",
                []( auto builder ) -> void{ builder->add_bool( "should_unload", true ); }
            );
            app->sentry->shutdown( );
        }
#endif

        m_should_unload = true;

        // add force unload thread
        std::thread(
            []( ) -> void{
                std::this_thread::sleep_for( std::chrono::seconds( 10 ) );
                std::abort( );
            }
        ).detach( );
    }

    auto App::add_unload_callback( const UnloadCB& callback ) -> void{ m_unload_callbacks.push_back( callback ); }

    auto App::setup_logger( ) -> void{ logger = std::make_shared< Logger >( ); }

    auto App::setup_sentry( ) -> void{
        sentry = std::make_unique< utils::Sentry >(
            _( "https://b541cd3b6574445c88e1fd09956c0985@o1321659.ingest.sentry.io/6578338" ),
#ifdef __BETA
            std::format( "{}_beta", _(release_version) )
#else
            _( release_version )
#endif
        );
    }

    auto App::setup_mixpanel( ) -> void{
        this->mixpanel = std::make_unique< mixpanel::Mixpanel >( _( "11a8e753ad218bca64842c1eb4e79707" ) );
    }

    auto App::run_unload_callbacks( ) const -> void{
        for ( auto unload_callback : m_unload_callbacks )
            try { unload_callback( ); } catch ( std::exception& ex ) { logger->error( ex.what( ) ); }
    }

    auto App::update_lua_scripts( ) -> void{
        try {
            const std::string path = std::format( ( "C:\\{}\\lua" ), user::c_hwid_system( ).get_hwid_short_form( ) );

            if ( !std::filesystem::exists( path ) ) {
                std::filesystem::create_directories( path );
                return;
            }

            const auto scripts = m_scripts;

            for ( const auto& entry : std::filesystem::directory_iterator( path ) ) {
                if ( entry.path( ).extension( ) == _( ".lua" ) || entry.path( ).extension( ) == _( ".slua" ) ) {
                    const auto script = lua::LuaScript( entry.path( ) );

                    auto script_exists_already = false;
                    for ( auto& s : scripts ) {
                        if ( s.path == script.path ) {
                            script_exists_already = true;
                            break;
                        }
                    }

                    if ( script_exists_already ) continue;

                    m_scripts.push_back( script );
                }
            }
        } catch ( ... ) {
        }
    }

    auto App::get_scripts( ) -> std::deque< lua::LuaScript >{ return m_scripts; }
}
