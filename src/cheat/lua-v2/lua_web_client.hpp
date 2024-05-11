#pragma once
#include "state.hpp"

#define lua_web_client_request_method_sync( method )         auto method( std::string url, sol::table options ) -> Response{ \
                                                                 HttpOptions http_options( EMethod::##method, options ); \
                                                                 return make_request( url, http_options ); \
                                                             }
#define lua_web_client_request_method_async( method ) auto async_##method( std::string url, sol::table options, sol::function callback ) -> void{ \
                                                                  HttpOptions http_options( EMethod::##method, options ); \
                                                                  std::thread( \
                                                                      [this, http_options, url, callback]( ) -> void{ \
                                                                          const auto response = make_request( url, http_options ); \
                                                                          g_lua2->execute_locked( [&]( ) -> void{ callback( sol::make_object( g_lua_state2, response ) ); } ); \
                                                                      } \
                                                                  ).detach( ); \
                                                              }

namespace lua {
    class WebClient {
        enum class EMethod {
            get,
            post,
            put,
            del,
            head,
            connect,
            options,
            trace,
            patch
        };

        struct HttpOptions {
            explicit HttpOptions( EMethod method, sol::table table );

            std::vector< std::pair< std::string, std::string > > headers{ };
            std::string                                          useragent{ };
            EMethod                                              method{ };
            std::string                                          body{ };
            int32_t                                              timeout{ 30 };
        };

    public:
        class Response {
        public:
            Response( const std::string& response, int32_t status ): m_response( response ), m_status{ status }{
            }

            Response( ): m_status{ -1 }{
            }

            auto json( ) -> sol::object;

            auto text( ) -> std::string{ return m_response; }
            auto to_file( std::string path ) -> bool;
            auto bytes( ) -> std::vector< std::byte >;

            auto get_status_code( ) const -> int32_t{ return m_status; }
            auto is_success( ) const -> bool{ return get_status_code( ) >= 200 && get_status_code( ) < 300; }

        private:
            static auto get_object_from_json( nlohmann::json j ) -> sol::object{
                if ( j.is_boolean( ) ) { return sol::make_object( g_lua_state2, j.get< bool >( ) ); }

                if ( j.is_number_integer( ) ) { return sol::make_object( g_lua_state2, j.get< int32_t >( ) ); }

                if ( j.is_number_float( ) ) { return sol::make_object( g_lua_state2, j.get< float >( ) ); }
                if ( j.is_number_unsigned( ) ) { return sol::make_object( g_lua_state2, j.get< uint32_t >( ) ); }

                if ( j.is_string( ) ) { return sol::make_object( g_lua_state2, j.get< std::string >( ) ); }
                if ( j.is_null( ) ) return sol::nil;

                if ( j.is_array( ) ) {
                    std::vector< sol::object > objects;

                    objects.reserve( j.size( ) );

                    for ( const auto& i : j ) { objects.push_back( get_object_from_json( i ) ); }

                    return sol::make_object( g_lua_state2, sol::as_table( objects ) );
                }

                if ( j.is_object( ) ) {
                    sol::table table = sol::make_object( g_lua_state2, sol::create );

                    for ( auto& [ key, value ] : j.items( ) ) { table[ key ] = get_object_from_json( value ); }

                    return table;
                }
                return sol::nil;
            }

            static auto object_to_json( sol::object object ) -> nlohmann::json{
                switch ( object.get_type( ) ) {
                case sol::type::nil:
                    return nlohmann::json( );
                case sol::type::string:
                    return object.as< std::string >( );
                case sol::type::number:
                    return object.as< float >( );
                case sol::type::thread:
                    return nlohmann::json( { } );
                case sol::type::boolean:
                    return object.as< bool >( );
                case sol::type::function:
                    return nlohmann::json( { } );
                case sol::type::userdata:
                    return nlohmann::json( { } );
                case sol::type::lightuserdata:
                    return nlohmann::json( { } );
                case sol::type::table:
                    return table_to_json( object.as< sol::table >( ) );
                case sol::type::poly:
                    return nlohmann::json( { } );
                default: ;
                }

                return nlohmann::json( { } );
            }

        public:
            static auto table_to_json( sol::table table ) -> nlohmann::json{
                nlohmann::json j;

                for ( auto t : table ) { j[ object_to_json( t.first ) ] = object_to_json( t.second ); }

                return j;
            }

        private:
            std::string m_response{ };
            int32_t     m_status{ };
        };

    public:
        WebClient( ) = default;

        lua_web_client_request_method_sync( get )
        lua_web_client_request_method_sync( post )
        lua_web_client_request_method_sync( put )
        lua_web_client_request_method_sync( del )
        lua_web_client_request_method_sync( head )
        lua_web_client_request_method_sync( connect )
        lua_web_client_request_method_sync( options )
        lua_web_client_request_method_sync( trace )
        lua_web_client_request_method_sync( patch )

        lua_web_client_request_method_async( get )
        lua_web_client_request_method_async( post )
        lua_web_client_request_method_async( put )
        lua_web_client_request_method_async( del )
        lua_web_client_request_method_async( head )
        lua_web_client_request_method_async( connect )
        lua_web_client_request_method_async( options )
        lua_web_client_request_method_async( trace )
        lua_web_client_request_method_async( patch )

    private:
        auto make_request( std::string url, HttpOptions options ) const -> Response;
    };
}
