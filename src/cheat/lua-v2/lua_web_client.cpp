#include "pch.hpp"
#include "lua_web_client.hpp"

#include "logger.hpp"

namespace lua {
    auto curl__http_write_callback_web_client(
        char*             ptr,
        const std::size_t size,
        const std::size_t nmemb,
        void*             user_data
    ) -> std::size_t{
        auto* data = static_cast< std::string* >( user_data );
        data->append( ptr, size * nmemb );
        return size * nmemb;
    }

    WebClient::HttpOptions::HttpOptions( EMethod method, sol::table table ){
        for ( auto v : table ) {
            if ( v.first.get_type( ) != sol::type::string ) continue;

            std::string key = v.first.as< std::string >( );

            switch ( rt_hash( key.c_str( ) ) ) {
            case ct_hash( "headers" ):
                if ( v.second.get_type( ) != sol::type::table ) break;
                {
                    auto table = v.second.as< sol::table >( );
                    for ( auto t : table ) {
                        if ( t.first.get_type( ) != sol::type::string ||
                            t.second.get_type( ) != sol::type::string
                        )
                            continue;

                        headers.push_back(
                            std::make_pair( t.first.as< std::string >( ), t.second.as< std::string >( ) )
                        );
                    }
                }
                break;
            case ct_hash( "useragent" ):
                if ( v.second.get_type( ) != sol::type::string ) break;
                useragent = v.second.as< std::string >( );
                break;
            case ct_hash( "body" ):
                if ( v.second.get_type( ) == sol::type::string ) body = v.second.as< std::string >( );
                else if ( v.second.get_type( ) == sol::type::table ) {
                    try {
                        body = Response::table_to_json( v.second.as< sol::table >( ) ).dump( );

                        headers.push_back( std::make_pair( "content-type", "application/json" ) );
                    } catch (
                        std::exception& ex ) { lua_logger->error( "{}", ex.what( ) ); } catch ( ... ) { body = "null"; }
                }
                break;
            case ct_hash( "timeout" ):
                if ( v.second.get_type( ) != sol::type::number ) break;
                timeout = v.second.as< int32_t >( );
                break;
            }
        }
    }

    auto WebClient::Response::json( ) -> sol::object{
        try {
            const auto json = nlohmann::json::parse( m_response );

            return get_object_from_json( json );
        } catch ( const std::exception ex ) { lua_logger->error( "{}", ex.what( ) ); } catch ( ... ) {
            return sol::nil;
        }

        return sol::nil;
    }

    auto WebClient::Response::to_file( std::string path ) -> bool{
        try {
            if ( std::filesystem::exists( path ) ) std::filesystem::remove( path );

            std::ofstream out( path );

            out << text( );
            out.flush( );
            out.close( );
            return true;
        } catch ( std::exception& ex ) { lua_logger->error( "Error saving response to file: {}", ex.what( ) ); } catch (
            ... ) { lua_logger->error( "Unknown error while trying to save response to file" ); }

        return false;
    }

    auto WebClient::Response::bytes( ) -> std::vector< std::byte >{
        std::vector< std::byte > out( m_response.size( ) + 1 );
        std::transform(
            m_response.begin( ),
            m_response.end( ),
            out.begin( ),
            []( char c ){ return std::byte( c ); }
        );

        return out;
    }

    auto WebClient::make_request( std::string url, HttpOptions options ) const -> WebClient::Response{
        const auto curl_data = curl_easy_init( );
        if ( !curl_data ) return { };

        struct curl_slist* headers = nullptr;

        std::string response;
        curl_easy_setopt( curl_data, CURLOPT_URL, url.data( ) );

        headers = curl_slist_append( headers, _( "Expect:" ) );
        headers = curl_slist_append( headers, _( "Content-Type: application/json" ) );
        for ( auto& h : options.headers ) {
            headers = curl_slist_append(
                headers,
                std::format( "{}: {}", h.first, h.second ).c_str( )
            );
        }

        switch ( options.method ) {
        case EMethod::post:
            curl_easy_setopt( curl_data, CURLOPT_POST, 1L );
            break;
        case EMethod::put:
            curl_easy_setopt( curl_data, CURLOPT_PUT, 1L );
            break;
        case EMethod::del:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "DELETE" );
            break;
        case EMethod::head:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "HEAD" );
            break;
        case EMethod::connect:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "CONNECT" );
            break;
        case EMethod::options:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "OPTIONS" );
            break;
        case EMethod::trace:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "TRACE" );
            break;
        case EMethod::patch:
            curl_easy_setopt( curl_data, CURLOPT_CUSTOMREQUEST, "PATCH" );
            break;
        default: ;
        }

        if ( !options.body.empty( ) ) curl_easy_setopt( curl_data, CURLOPT_POSTFIELDS, options.body.data( ) );

        curl_easy_setopt( curl_data, CURLOPT_HTTPHEADER, headers );

        curl_easy_setopt( curl_data, CURLOPT_TIMEOUT, options.timeout );
        curl_easy_setopt( curl_data, CURLOPT_SSL_VERIFYPEER, 1L );
        curl_easy_setopt( curl_data, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3 );
        curl_easy_setopt( curl_data, CURLOPT_MAXFILESIZE, 1024 * 1024 * 1024 );
        curl_easy_setopt( curl_data, CURLOPT_WRITEDATA, &response );
        curl_easy_setopt( curl_data, CURLOPT_WRITEFUNCTION, curl__http_write_callback_web_client );
        curl_easy_setopt(
            curl_data,
            CURLOPT_USERAGENT,
            options.useragent.empty( ) ? _( "slotted-web-client" ) : options.useragent.data( )
        );

        const auto curl_response = curl_easy_perform( curl_data );
        curl_slist_free_all( headers );
        curl_easy_cleanup( curl_data );

        if ( curl_response == CURLE_OK ) {
            long response_code;
            curl_easy_getinfo( curl_data, CURLINFO_RESPONSE_CODE, &response_code );

            return Response( response, static_cast< int32_t >( response_code ) );
        }

        return Response( );
    }
}
