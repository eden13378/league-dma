#include "pch.hpp"
#include "web_client.hpp"
#if enable_new_lua
#include "../lua-v2/lua_def.hpp"
#include "../lua-v2/state.hpp"
#endif

#if enable_lua
#include <sol/sol.hpp>
#endif

auto curl__http_write_callback(
    char*             ptr,
    const std::size_t size,
    const std::size_t nmemb,
    void*             user_data
) -> std::size_t{
    auto* data = static_cast< std::string* >( user_data );
    data->append( ptr, size * nmemb );
    return size * nmemb;
}

auto WebClient::get( std::string url ) const -> std::optional< std::string >{
    const auto curl_data = curl_easy_init( );
    if ( !curl_data ) return { };

    struct curl_slist* headers = nullptr;

    std::string response;
    curl_easy_setopt( curl_data, CURLOPT_URL, url.data( ) );

    headers = curl_slist_append( headers, _( "Expect:" ) );
    headers = curl_slist_append( headers, _( "Content-Type: application/json" ) );
    for ( auto& h : m_default_headers ) {
        headers = curl_slist_append(
            headers,
            std::format( "{}: {}", h.first, h.second ).c_str( )
        );
    }

    curl_easy_setopt( curl_data, CURLOPT_HTTPHEADER, headers );

    curl_easy_setopt( curl_data, CURLOPT_TIMEOUT, m_timeout );
    curl_easy_setopt( curl_data, CURLOPT_SSL_VERIFYPEER, 1L );
    curl_easy_setopt( curl_data, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3 );
    curl_easy_setopt( curl_data, CURLOPT_MAXFILESIZE, 1024 * 1024 * 1024 );
    curl_easy_setopt( curl_data, CURLOPT_WRITEDATA, &response );
    curl_easy_setopt( curl_data, CURLOPT_WRITEFUNCTION, curl__http_write_callback );
    curl_easy_setopt( curl_data, CURLOPT_USERAGENT, _( "slc-webclient" ) );

    const auto curl_response = curl_easy_perform( curl_data );
    curl_slist_free_all( headers );
    curl_easy_cleanup( curl_data );

    return curl_response == CURLE_OK ? std::make_optional( response ) : std::nullopt;
}

auto WebClient::post( std::string url, std::string body ) const -> std::optional< std::string >{
    const auto curl_data = curl_easy_init( );
    if ( !curl_data ) return { };

    struct curl_slist* headers = nullptr;

    std::string response;
    curl_easy_setopt( curl_data, CURLOPT_URL, url.data( ) );

    headers = curl_slist_append( headers, _( "Expect:" ) );
    headers = curl_slist_append( headers, _( "Content-Type: application/json" ) );
    for ( auto& h : m_default_headers ) {
        headers = curl_slist_append(
            headers,
            std::format( "{}: {}", h.first, h.second ).c_str( )
        );
    }

    curl_easy_setopt( curl_data, CURLOPT_HTTPHEADER, headers );

    curl_easy_setopt( curl_data, CURLOPT_POST, 1L );
    curl_easy_setopt( curl_data, CURLOPT_POSTFIELDS, body.data( ) );
    curl_easy_setopt( curl_data, CURLOPT_TIMEOUT, m_timeout );
    curl_easy_setopt( curl_data, CURLOPT_SSL_VERIFYPEER, 1L );
    curl_easy_setopt( curl_data, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_3 );
    curl_easy_setopt( curl_data, CURLOPT_MAXFILESIZE, 1024 * 1024 * 1024 );
    curl_easy_setopt( curl_data, CURLOPT_WRITEDATA, &response );
    curl_easy_setopt( curl_data, CURLOPT_WRITEFUNCTION, curl__http_write_callback );
    curl_easy_setopt( curl_data, CURLOPT_USERAGENT, _( "slc-webclient" ) );

    const auto curl_response = curl_easy_perform( curl_data );
    curl_slist_free_all( headers );
    curl_easy_cleanup( curl_data );

    return curl_response == CURLE_OK ? std::make_optional( response ) : std::nullopt;
}

#if enable_lua
// ReSharper disable once CppPassValueParameterByConstReference
auto WebClient::lua_get( const sol::object url ) -> sol::object{
    lua_arg_check_ct( url, std::string, "string" )

    const auto response = get( url.as< std::string >( ) );
    if ( !response ) return sol::nil;

    return sol::make_object( g_lua_state2, *response );
}

auto WebClient::lua_post(
    // ReSharper disable CppPassValueParameterByConstReference
    const sol::object url,
    const sol::object body
    // ReSharper restore CppPassValueParameterByConstReference
) -> sol::object{
    lua_arg_check_ct( url, std::string, "string" )
    lua_arg_check_ct( body, std::string, "string" )

    const auto response = post( url.as< std::string >( ), body.as< std::string >( ) );
    if ( !response ) return sol::nil;

    return sol::make_object( g_lua_state2, *response );
}

// ReSharper disable once CppPassValueParameterByConstReference
auto WebClient::lua_set_timeout( const sol::object timeout ) -> void{
    lua_arg_check_ct_v( timeout, int32_t, "number" )

    set_timeout( timeout.as< int32_t >( ) );
}

auto WebClient::lua_add_default_header(
    // ReSharper disable CppPassValueParameterByConstReference
    const sol::object name,
    const sol::object content
    // ReSharper restore CppPassValueParameterByConstReference
) -> void{
    lua_arg_check_ct_v( name, std::string, "string" )
    lua_arg_check_ct_v( content, std::string, "string" )

    add_default_header( name.as< std::string >( ), content.as< std::string >( ) );
}
#endif

auto WebClient::add_default_header( std::string name, std::string content ) -> void{
    m_default_headers.push_back( std::make_pair( name, content ) );
}
