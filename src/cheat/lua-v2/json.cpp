#include "pch.hpp"

#include "json.hpp"

#include "logger.hpp"
#include "state.hpp"

namespace lua::json {
    auto intl_parse( nlohmann::json j ) -> sol::object{
        if ( j.is_boolean( ) ) { return sol::make_object( g_lua_state2, j.get< bool >( ) ); }

        if ( j.is_number_integer( ) ) { return sol::make_object( g_lua_state2, j.get< int32_t >( ) ); }

        if ( j.is_number_float( ) ) { return sol::make_object( g_lua_state2, j.get< float >( ) ); }
        if ( j.is_number_unsigned( ) ) { return sol::make_object( g_lua_state2, j.get< uint32_t >( ) ); }

        if ( j.is_string( ) ) { return sol::make_object( g_lua_state2, j.get< std::string >( ) ); }
        if ( j.is_null( ) ) return sol::nil;

        if ( j.is_array( ) ) {
            std::vector< sol::object > objects;

            objects.reserve( j.size( ) );

            for ( const auto& i : j ) { objects.push_back( intl_parse( i ) ); }

            return sol::make_object( g_lua_state2, sol::as_table( objects ) );
        }

        if ( j.is_object( ) ) {
            sol::table table = sol::make_object( g_lua_state2, sol::create );

            for ( auto& [ key, value ] : j.items( ) ) { table[ key ] = intl_parse( value ); }

            return table;
        }
        return sol::nil;
    }

    auto parse( const std::string& json ) -> sol::object{
        try {
            const auto parsed = nlohmann::json::parse( json );

            return intl_parse( parsed );
        } catch ( std::exception& ex ) { lua_logger->error( "error parsing json: {}", ex.what( ) ); } catch ( ... ) {
            lua_logger->error( "unknown error parsing json" );
        }

        return sol::nil;
    }

    auto object_to_json( sol::object object ) -> nlohmann::json;

    auto intl_encode( sol::table table ) -> nlohmann::json{
        nlohmann::json j;

        for ( auto t : table ) { j[ object_to_json( t.first ) ] = object_to_json( t.second ); }

        return j;
    }

    auto object_to_json( sol::object object ) -> nlohmann::json{
        switch ( object.get_type( ) ) {
        case sol::type::nil:
            return nlohmann::json( );
        case sol::type::string:
            return object.as< std::string >( );
        case sol::type::number:
            return object.as< float >( );
        case sol::type::boolean:
            return object.as< bool >( );
        case sol::type::function:
        case sol::type::thread:
        case sol::type::poly:
            return nlohmann::json( { } );
        case sol::type::table:
            return intl_encode( object.as< sol::table >( ) );
        case sol::type::userdata:
        case sol::type::lightuserdata:
        {
            if ( object.is< Vec2 >( ) ) {
                auto v = object.as< Vec2 >( );
                return { { "x", v.x }, { "y", v.y } };
            }
            if ( object.is< Vec3 >( ) ) {
                auto v = object.as< Vec3 >( );
                return { { "x", v.x }, { "y", v.y }, { "z", v.z } };
            }
            if ( object.is< Color >( ) ) {
                auto v = object.as< Color >( );
                return { { "r", v.r }, { "g", v.g }, { "b", v.b }, { "a", v.a } };
            }

            return nlohmann::json( { } );
        }
        default: ;
        }

        return nlohmann::json( { } );
    }

    auto stringify( sol::object obj ) -> std::string{
        if ( obj.get_type( ) == sol::type::table ) { return intl_encode( obj.as< sol::table >( ) ).dump( ); }

        return object_to_json( obj ).dump( );
    }
}
