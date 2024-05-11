#include "pch.hpp"

#include "state.hpp"

#include "custom_structs.hpp"
#include "../features/entity_list.hpp"
#include "../features/feature.hpp"
#include "../features/champion_modules/lua_module.hpp"
#include "../features/champion_modules/modules.hpp"
#include "../features/runtime/runtime.hpp"
#include "../utils/c_function_caller.hpp"
#include "../utils/directory_manager.hpp"
#include "custom_structs.hpp"
#include "../globals.hpp"
#include "../features/spell_detector.hpp"
#include "../security/src/base64.hpp"

#include <boost/stacktrace.hpp>

#include "cancelable.hpp"
#include "json.hpp"
#include "lua_web_client.hpp"
#include "../menu/menu.hpp"
#include "../overlay/debug_overlay.hpp"

#pragma comment(lib, "lua.lib")

#if enable_new_lua


// #include "../math/vec2.hpp"
#include "exception.hpp"

#include <sol/sol.hpp>

#include "../sdk/math/math.hpp"
#include "../sdk/game/ai_manager.hpp"
#include "../sdk/game/hud_manager.hpp"
#include "../sdk/game/spell_cast_info.hpp"
#include "../sdk/game/unit_info_component.hpp"
#include "../sdk/game/spell_info.hpp"
#include "../sdk/game/spell_data.hpp"

#include "../sdk/game/spell_details.hpp"

#include "../sdk/math/vec2.hpp"
#include "../sdk/math/vec3.hpp"

#include "../features/activator/activator.hpp"
#include "../features/feature.hpp"
#include "../features/buff_cache.hpp"
#include "../features/target_selector/ITargetSelector.hpp"
#include "../features/prediction.hpp"
#include "../features/evade.hpp"
#include "../features/orbwalker.hpp"

#include "../features/champion_modules/module.hpp"

// #pragma comment(lib, "lua.lib")
// #pragma comment(lib, "lua51.lib")

namespace sdk::game {
    class SpellData;
}

lua::LuaScripts g_scripts{ };


#define lua_enable_catch_all __BETA && !__DEBUG && 0

namespace lua {
    LuaState::LuaState( ){
        lua_logger->info( "{}", "initializing lua state" );
        std::lock_guard lock( m_mutex );
        m_state = open_state( );

        m_state.stop_gc( );

        g_lua_state2 = m_state.lua_state( );

        register_math_types( m_state );
        register_sdk_types( m_state );
        register_feature_types( m_state );
        register_global_tables( m_state );
        register_config( m_state );
        register_menu( m_state );
        register_lua_types( m_state );
        register_enums( m_state );
        register_lua_only_types( m_state );
        register_debug_overlay( m_state );

        setup_globals( );

        lua_logger->info( "{}", "initialized lua state" );
    }

    LuaState::~LuaState( ){
        std::lock_guard lock( m_mutex );

        m_state.stop_gc( );

        if ( app && app->should_run( )

        ) {
            if ( g_features &&
                g_features->current_module &&
                g_features->current_module->is_lua_module( )
            )
                g_features->current_module.reset( );

            g_window->on_lua_reset( );
        }
    }

    auto                                                        LuaState::collect_garbage( ) -> void{
        try { m_state.collect_garbage( ); } catch ( sol::error& err ) { handle_error( err ); }
#if lua_enable_catch_all
        catch ( ... ) {
            handle_unknown_error( );
        }
#endif
    }

    auto LuaState::run_function( sol::function fn, sol::object arg ) -> sol::object{
        if ( !g_lua_state2 || g_lua_state2 != m_state.lua_state( ) ) return sol::nil;

        if ( !m_state.lua_state( ) || !fn || !fn.valid( ) || fn.lua_state( ) != m_state.lua_state( ) ) return sol::nil;

        try {
            sol::protected_function        func       = fn;
            sol::protected_function_result run_result = func( arg );

            if ( !run_result.valid( ) ) {
                sol::error  err   = run_result;
                std::string error = err.what( );
                string_replace( error, ": sol: ", ": " );

                lua_logger->error( "{}", error );
            } else { return run_result; }
            return sol::nil;
        } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& e ) {
            std::cout << e.what( ) << std::endl;
        }
#if lua_enable_catch_all
        catch ( ... ) {
            std::cout << "Unknown error" << std::endl;
            handle_unknown_error( );
            // lua_logger->error( "run fn trace: {}", boost::stacktrace::to_string( boost::stacktrace::stacktrace( ) ) );
        }
#endif
        return sol::nil;
    }

    auto lua_object_to_string( const sol::object object ) -> std::string{
        switch ( object.get_type( ) ) {
        case sol::type::string:
            return object.as< std::string >( );
        case sol::type::number:
            return std::to_string( object.as< double >( ) );
        case sol::type::boolean:
            return object.as< bool >( ) ? _( "true" ) : _( "false" );
        case sol::type::table:
            return _( "table" );
        case sol::type::userdata:
        {
            if ( object.is< Object >( ) ) {
                std::stringstream ss;
                ( ss << object.as< Object >( ) );
                return ss.str( );
            }
            if ( object.is< Object* >( ) ) {
                std::stringstream ss;
                ( ss << object.as< Object* >( ) );
                return ss.str( );
            }
            if ( object.is< LeagueString >( ) ) {
                std::stringstream ss;
                ( ss << object.as< LeagueString >( ) );
                return ss.str( );
            }
            if ( object.is< Vec2 >( ) ) {
                std::stringstream ss;
                ( ss << object.as< Vec2 >( ) );
                return ss.str( );
            }
            if ( object.is< Vec3 >( ) ) {
                std::stringstream ss;
                ( ss << object.as< Vec3 >( ) );
                return ss.str( );
            }
            if ( object.is< Color >( ) ) {
                std::stringstream ss;
                ( ss << object.as< Color >( ) );
                return ss.str( );
            }
        }
        break;
        default:
            return "unknown type";
        }

        return "unknown type";
    }

    auto format_variable_size( const char* fmt, const std::deque< sol::object >& args ) -> std::string{
        std::string f = fmt;

        std::vector< std::string > out;
        std::string                current = "";

        for ( auto c : f ) {
            if ( c == '{' ) {
                out.push_back( current );
                current = "";
                continue;
            }
            if ( c == '}' ) continue;

            current += c;
        }

        if ( !current.empty( ) ) out.push_back( current );

        std::string out_str;
        for ( int i = 0; i < out.size( ); ++i ) {
            out_str += out[ i ];
            if ( i < args.size( ) ) { out_str += lua_object_to_string( args[ i ] ); }
        }

        return out_str;
    }

    auto LuaState::load_available_scripts( ) -> void{
        const std::string path = std::format( ( "C:\\{}\\lua" ), user::c_hwid_system( ).get_hwid_short_form( ) );

        if ( !std::filesystem::exists( path ) ) {
            std::filesystem::create_directories( path );
            return;
        }

        // const auto scripts = g_scripts.scripts;

        for ( const auto& entry : std::filesystem::directory_iterator( path ) ) {
            if ( entry.path( ).extension( ) == _( ".lua" ) || entry.path( ).extension( ) == _( ".slua" ) ) {
                const auto script = LuaScript( entry.path( ) );

                auto script_exists_already = false;
                for ( auto& s : g_scripts.scripts ) {
                    if ( s.path == script.path ) {
                        script_exists_already = true;
                        break;
                    }
                }

                if ( script_exists_already ) continue;

                g_scripts.push_script( script );
            }
        }
    }

    auto LuaState::open_state( ) -> sol::state{
        sol::state state;

        state.open_libraries(
            sol::lib::base,
            // require and other package functions
            sol::lib::package,
            // coroutine functions and utilities
            sol::lib::coroutine,
            // string library
            sol::lib::string,
            // functionality from the OS
            sol::lib::os,
            // all things math
            sol::lib::math,
            // the table manipulator and observer functions
            sol::lib::table,
            // the debug library
            sol::lib::debug,
            // the bit library: different based on which you're using
            sol::lib::bit32,
            // input/output library
            sol::lib::io,
            // LuaJIT only
            sol::lib::ffi,
            // LuaJIT only
            sol::lib::jit,
            // library for handling utf8: new to Lua
            sol::lib::utf8
        );

        const auto lua_base = std::format( ( "C:\\{}\\lua\\lib\\" ), user::c_hwid_system( ).get_hwid_short_form( ) );
        if ( !std::filesystem::exists( lua_base ) ) std::filesystem::create_directories( lua_base );
        state[ "package" ][ "path" ] = std::format( "{}{}", lua_base, "?.lua" );

        state.set_function(
            "print",
            []( sol::variadic_args args ) -> void{
                std::deque< sol::object > objects;
                for ( auto arg : args ) { objects.push_back( arg ); }

                if ( objects.size( ) == 1 ) {
                    lua_logger->info( "{}", lua_object_to_string( objects[ 0 ] ) );
                    return;
                }

                if ( objects.size( ) >= 2u ) {
                    try {
                        const auto formattable = objects[ 0 ].as< std::string >( );
                        objects.pop_front( );
                        auto formatted = format_variable_size( formattable.data( ), objects );
                        lua_logger->info( "{}", formatted );
                    } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& ex ) {
                        lua_logger->error( "{}", std::format( ( "error parsing string: {}" ), ex.what( ) ) );
                    }
#if lua_enable_catch_all
                    catch ( ... ) {
                        lua_logger->error( "{}", std::format( ( "error parsing string: unknown" ) ) );
                        g_lua2->set_reload_lua( );
                    }
#endif
                } else if ( !objects.empty( ) ) {
                    const auto formatted = lua_object_to_string( objects[ 0 ] );
                    lua_logger->info( "{}", formatted );
                }
            }
        );

        return state;
    }

    auto LuaState::run_script( const std::string& code ) -> void{
        auto script = m_state.load( code );
        if ( !script.valid( ) ) throw CouldNotLoadScript( code );

        run( script );
    }

    auto LuaState::run_file( const std::string& path ) -> void{
        lua_logger->info( "Loading lua: {}", path );

        if ( std::filesystem::path( path ).extension( ) == _( ".slua" ) ) {
            std::ifstream     t( path );
            std::stringstream buffer;
            buffer << t.rdbuf( );

            try_run_script( decrypt_lua( buffer.str( ) ) );
            return;
        }

        auto script = m_state.load_file( path );
        if ( !script.valid( ) ) throw CouldNotLoadScript( path );

        run( script );
    }

    auto LuaState::run( sol::load_result& script ) -> void{
        if ( !script.valid( ) || script.lua_state( ) != m_state.lua_state( ) ) return;

        try {
            sol::protected_function        func       = script;
            sol::protected_function_result run_result = func( );

            if ( !run_result.valid( ) ) {
                sol::error  err   = run_result;
                std::string error = err.what( );
                string_replace( error, ": sol: ", ": " );

                lua_logger->error( "{}", error );
            }
        } catch ( const sol::error& err ) { handle_error( err ); }
        catch ( std::exception&     e ) { std::cout << e.what( ) << std::endl; }
#if lua_enable_catch_all
        catch ( ... ) { handle_unknown_error( ); }
#endif
    }

    auto LuaState::handle_error( const sol::error& ex ) -> void{
        std::string error_message = ex.what( );
        string_replace( error_message, ": sol: ", ": " );
        string_replace( error_message, "sol.sol::", "" );

        lua_logger->error( "{}", error_message );
    }

    auto LuaState::handle_unknown_error( const std::source_location& location ) -> void{
        lua_logger->error( "unknown error in: {}:{}", location.function_name( ), location.line( ) );

        m_should_reset = true;
    }

    auto LuaState::try_run_script( const std::string& code ) -> bool{
        try {
            run_script( code );
            return true;
        } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& e ) {
            lua_logger->error( "{}", e.what( ) );
            return false;
        }
#if lua_enable_catch_all
        catch ( ... ) {
            handle_unknown_error( );
            return false;
        }
#endif
        return false;
    }

    auto LuaState::try_run_file( const std::string& code ) -> bool{
        try {
            lua_logger->info( "file: {}", code );
            run_file( code );
            return true;
        } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& e ) {
            lua_logger->error( "{}", e.what( ) );
        }
#if lua_enable_catch_all
        catch ( ... ) { handle_unknown_error( ); }
#endif
        return false;
    }

    auto LuaState::execute_locked( const std::function< void( ) >& function ) -> std::expected< void, const char* >{
        std::lock_guard lock( m_mutex );
        try { function( ); } catch ( sol::error& ex ) {
            handle_error( ex );
            return std::unexpected( ex.what( ) );
        }
#if lua_enable_catch_all
        catch ( ... ) {
            handle_unknown_error( );
            return std::unexpected( "unknown error" );
        }
#endif

        return { };
    }

    auto LuaState::run_callback( const hash_t name ) -> std::expected< void, LuaState::ERunLuaCallbackError >{
        const auto callbacks = m_callbacks[ name ];

        if ( callbacks.empty( ) ) return { };

        if ( !g_lua_state2 ) return std::unexpected( ERunLuaCallbackError::invalid_lua_state );

        if ( g_lua_state2 != m_state.lua_state( ) ) {
            return std::unexpected(
                ERunLuaCallbackError::lua_state_dont_match
            );
        }

        for ( const auto& cb : callbacks ) {
            try {
                if ( !cb || cb.lua_state( ) != m_state.lua_state( ) ) continue;
                sol::protected_function func = cb;
                if ( !func.valid( ) ) continue;
                sol::protected_function_result run_result = func( );

                if ( !run_result.valid( ) ) {
                    sol::error  err   = run_result;
                    std::string error = err.what( );
                    string_replace( error, ": sol: ", ": " );

                    lua_logger->error( "{}", error );
                }
            } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& e ) {
                std::cout << e.what( ) << std::endl;
            }
#if lua_enable_catch_all
            catch ( ... ) { handle_unknown_error( ); }
#endif
        }

        return { };
    }

    auto LuaState::run_callback(
        const hash_t name,
        sol::object  object
    ) -> std::expected< void, LuaState::ERunLuaCallbackError
    >{
        const auto callbacks = m_callbacks[ name ];

        if ( callbacks.empty( ) ) return { };

        if ( !g_lua_state2 ) return std::unexpected( ERunLuaCallbackError::invalid_lua_state );

        if ( g_lua_state2 != m_state.lua_state( ) ) {
            return std::unexpected(
                ERunLuaCallbackError::lua_state_dont_match
            );
        }


        for ( const auto& cb : callbacks ) {
            try {
                if ( !cb || cb.lua_state( ) != m_state.lua_state( ) ) continue;
                sol::protected_function func = cb;
                if ( !func.valid( ) ) continue;
                sol::protected_function_result run_result = func( object );

                if ( !run_result.valid( ) ) {
                    sol::error  err   = run_result;
                    std::string error = err.what( );
                    string_replace( error, ": sol: ", ": " );

                    lua_logger->error( "{}", error );
                }
            } catch ( const sol::error& err ) { handle_error( err ); } catch ( std::exception& e ) {
                std::cout << e.what( ) << std::endl;
            }
#if lua_enable_catch_all
            catch ( ... ) { handle_unknown_error( ); }
#endif
        }

        return { };
    }

    auto LuaState::register_callback(
        const hash_t  name,
        sol::function callback
    ) -> std::expected< void,
        ERegisterCallbackError >{
        if ( !callback.valid( ) || callback.lua_state( ) != m_state.lua_state( ) ) {
            return std::unexpected(
                ERegisterCallbackError::callback_invalid
            );
        }
        m_callbacks[ name ].push_back( callback );

        return { };
    }

    auto LuaState::setup_globals( ) noexcept -> void{
        if ( g_features ) {
            m_state.globals( ).set( "g_features", g_features.get( ) );
            m_state.globals( ).set( "features", g_features.get( ) );
        }
        if ( g_local ) m_state.globals( ).set( "g_local", g_local.get( ) );
        if ( g_navgrid ) m_state.globals( ).set( "g_navgrid", g_navgrid.get( ) );
        if ( g_time ) m_state.globals( ).set( "g_time", *g_time.get( ) );
        else m_state.globals( ).set( "g_time", -1.f );
        if ( g_threading ) m_state.globals( ).set( "g_threading", g_threading.get( ) );
        if ( g_debug_overlay ) m_state.globals( ).set( "g_debug_overlay", g_debug_overlay.get( ) );
    }

    auto string_replace( std::string& str, const std::string& from, const std::string& to ) -> void{
        try {
            size_t start_pos = 0;
            while ( ( start_pos = str.find( from, start_pos ) ) != std::string::npos ) {
                str.replace( start_pos, from.length( ), to );
                start_pos += to.length( ); // In case 'to' contains 'from', like replacing 'x' with 'yx'
            }
        } catch ( ... ) {
        }
    }

    LuaScript::LuaScript( std::filesystem::path path ){
        app->logger->info( "detected lua: {}: {}", path.filename( ).string( ), path.string( ) );

        name         = path.filename( ).string( );
        this->path   = path.string( );
        is_loaded    = false;
        is_encrypted = path.extension( ) == ".slua";
    }

    auto LuaState::decrypt_lua( std::string input ) -> std::string{
        input = xbt::security::base64::decode( input );
        for ( auto i = 0u; i < input.size( ); ++i ) {
            input[ i ] ^= 's';

            switch ( i % 3 ) {
            case 0:
                input[ i ] ^= 'u';
            case 1:
                input[ i ] ^= 'l';
            case 2:
                input[ i ] ^= 'a';
            default:
                break;
            }
        }

        return input;
    }

    auto LuaState::encrypt_lua( std::string input ) -> std::string{
        for ( auto i = 0u; i < input.size( ); ++i ) {
            input[ i ] ^= 's';

            switch ( i % 3 ) {
            case 0:
                input[ i ] ^= 'u';
            case 1:
                input[ i ] ^= 'l';
            case 2:
                input[ i ] ^= 'a';
            default: ;
            }
        }

        return xbt::security::base64::encode( input );
    }
}

std::unique_ptr< lua::LuaState > g_lua2;
lua_State*                       g_lua_state2;

#endif
