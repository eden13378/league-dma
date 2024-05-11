#include "pch.hpp"

#include "c_config_system.hpp"

#include <filesystem>
#include <fstream>

#include "../security/src/base64.hpp"
#include "../globals.hpp"
#if enable_new_lua
#include "../lua-v2/lua_def.hpp"
#endif

#include "../mixpanel/mixpanel.hpp"

#include "../utils/debug_logger.hpp"
#if enable_lua
#include "../include/sol/sol.hpp"
#endif

// #include <fmt/core.h>

config::ConfigSystem* g_config_system = new config::ConfigSystem( );

namespace config {
#if enable_lua
    auto ConfigSystem::lua_add_int( int32_t value, sol::object var_name ) -> std::shared_ptr< ConfigVar >{
        lua_arg_check_ct( var_name, std::string, "string" )

        auto name = std::format( ( "lua_{}" ), var_name.as< std::string >( ) );

        const auto hashed = rt_hash( name.data( ) );
        if ( auto found = find_by_name( hashed ) ) return found;

        return add_item( value, name, hashed );
    }

    auto ConfigSystem::lua_add_float( float value, sol::object var_name ) -> std::shared_ptr< ConfigVar >{
        lua_arg_check_ct( var_name, std::string, "string" )

        auto name = std::format( ( "lua_{}" ), var_name.as< std::string >( ) );

        const auto hashed = rt_hash( name.data( ) );
        if ( auto found = find_by_name( hashed ) ) return found;

        return add_item( value, name, hashed );
    }

    auto ConfigSystem::lua_add_bool( bool value, sol::object var_name ) -> std::shared_ptr< ConfigVar >{
        lua_arg_check_ct( var_name, std::string, "string" )

        auto name = std::format( ( "lua_{}" ), var_name.as< std::string >( ) );

        const auto hashed = rt_hash( name.data( ) );
        if ( auto found = find_by_name( hashed ) ) return found;

        return add_item( value, name, hashed );
    }

#endif


#if __DEBUG
    auto ConfigSystem::export_named( std::string out_path ) -> void{
        nlohmann::json j;

        for ( auto i : m_unknown_data.items( ) ) { j[ "unknown" ][ i.key( ) ] = i.value( ); }

        for ( const auto& var : m_config_vars ) {
            j[ var->get_name( ) ] = var->get_config_formatter( )->get_json_named( );
        }

        std::ofstream t( out_path );
        t << j.dump( 2 );
    }
#endif

    auto ConfigSystem::save( const char* name ) -> void{
        nlohmann::json j;

        for ( auto i : m_unknown_data.items( ) ) { j[ i.key( ) ] = i.value( ); }

        for ( const auto& var : m_config_vars ) {
            j[ var->get_name_hash_string( ) ] = var->get_config_formatter( )->get_json( );
        }

#if __DEBUG
        // debug_log( "{}", j.dump( ) );
        // fmt::print( "{}", j.dump( ) );
#endif
        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        const auto path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, name );
        debug_log( "path: {}", path );
        if ( !std::filesystem::exists( std::format( ( "C:\\{}" ), hwid_short_form ) ) ) {
            std::filesystem::create_directories( std::format( ( "C:\\{}" ), hwid_short_form ) );
        }

        std::ofstream t( path );
        // pretty print json
        auto s = j.dump( );
        // for ( auto& c : s ) c ^= 's';


        t << s;
    }

    auto ConfigSystem::load( const char* name ) -> void{
        app->logger->info( _( "loading config" ) );

        //         lua_arg_check_ct( value, int32_t, "number" )
        // lua_arg_check_ct( var_name, std::string, "string" )
        //
        // auto name = std::format( ( "lua_{}" ), var_name.as< std::string >( ) );
        //
        //         const auto hashed = rt_hash( name.data( ) );
        //         if ( auto found = find_by_name( hashed ) ) return found;
        //
        //         return add_item( value.as< int32_t >( ), name, hashed );

        // std::unique_lock lock( m_mutex );
        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        auto       path            = std::format( ( "C:\\{}\\{}" ), hwid_short_form, name );
        if ( !std::filesystem::exists( path ) ) return;
        std::ifstream     t( path );
        std::stringstream buffer;
        buffer << t.rdbuf( );

        std::string s;
        if ( !buffer.str( ).starts_with( "{" ) ) {
            // decode old config style
            s = xbt::security::base64::decode( buffer.str( ) );
            for ( auto& c : s ) c ^= 's';
        } else { s = buffer.str( ); }

        nlohmann::json j;
        try { j = nlohmann::json::parse( s ); } catch ( ... ) {
            if ( app && app

                ->
                logger
            )
                app->logger->error( "error parsing config file" );
        }

        for ( const auto var : m_config_vars ) {
            const auto found = j.find( var->get_name_hash_string( ) );
            if ( found == j.end( ) ) { continue; }
            try { var->get_config_formatter( )->from_json( *found ); } catch ( ... ) {
            }
        }

        for ( auto i : j.items( ) ) {
            auto found = std::ranges::find_if(
                m_config_vars,
                [&]( std::shared_ptr< ConfigVar >& v ) -> bool{ return v->get_name_hash_string( ) == i.key( ); }
            );

            if ( found != m_config_vars.end( ) ) { continue; }

            m_unknown_data[ i.key( ) ] = i.value( );

            if ( i.value( ).is_number_integer( ) ) {
                auto item = add_item( i.value( ).get< int32_t >( ), i.key( ), std::stoul( i.key( ) ) );
                item->get_config_formatter( )->from_json( i.value( ) );
            } else if ( i.value( ).is_boolean( ) ) {
                auto item = add_item( i.value( ).get< bool >( ), i.key( ), std::stoul( i.key( ) ) );
                item->get_config_formatter( )->from_json( i.value( ) );
            } else if ( i.value( ).is_number_float( ) ) {
                auto item = add_item( i.value( ).get< float >( ), i.key( ), std::stoul( i.key( ) ) );
                item->get_config_formatter( )->from_json( i.value( ) );
            }
        }

        static auto did_submit = false;

        if ( !did_submit && app && app->mixpanel ) {
            std::vector< mixpanel::Mixpanel::Event > events;

            for ( auto& var : m_config_vars ) {
                const auto name = var->get_name( );

                if ( var->get_config_formatter_ref( )->m_value.type( ).hash_code( ) == typeid( bool ).hash_code( ) ) {
                    if ( !var->get< bool >( ) ) continue;
                    if ( name.contains( "orbwalker" ) ||
                        // name.contains( "awareness" ) ||
                        name.contains( "prediction" ) ||
                        name.contains( "misc" ) ||
                        // name.contains( "visuals" ) ||
                        name.contains( "target_selector" ) ||
                        name.contains( "activator" ) ||
                        name.contains( "evade" ) ) {
                        if ( name.contains( "evade_spells" ) ) continue;


                        mixpanel::Mixpanel::Event ev(
                            "config value",
                            { { "enabled feature", std::make_any< std::string >( var->get_name( ) ) } }
                        );

                        events.push_back( ev );
                    }
                }
            }

            if ( !events.empty( ) ) app->mixpanel->track_events( events );

            did_submit = true;
        }
    }
}
