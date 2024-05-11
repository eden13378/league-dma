#include "pch.hpp"
#include "cache_manager.hpp"

#include "path.hpp"
#include "web_client.hpp"


namespace cache_manager {
    auto get_base_path( ) -> std::expected< std::string, const char* >{
        static auto hwid = user::c_hwid_system( );

        auto joined = path::join( { "C:\\", hwid.get_hwid_short_form( ), "cache" } );

        if ( !joined ) return std::unexpected( "could not join base path" );

        return *joined;
    }

    auto get_local_path( const std::string& name ) -> std::expected< std::string, const char* >{
        static const auto cache_path = get_base_path( );

        if ( !cache_path ) return std::unexpected( cache_path.error( ) );

        try {
            if ( !std::filesystem::exists( *cache_path ) ) std::filesystem::create_directories( *cache_path );

            const auto hashed = rt_hash( name.data( ) );

            auto out_path = path::join( { *cache_path, std::to_string( hashed ) + ".scch" } );

            if ( !out_path ) return std::unexpected( "could not join local path" );

            return *out_path;
        } catch ( ... ) { return std::unexpected( "error getting local path" ); }
    }

    auto get_cache( const std::string& name ) -> std::optional< std::vector< unsigned char > >{
        const auto path = get_local_path( name );

        if ( !path ) return { };

        try {
            if ( !std::filesystem::exists( *path ) ) return std::nullopt;

            std::ifstream                stream( *path, std::ios::binary );
            std::vector< unsigned char > data(
                ( std::istreambuf_iterator< char >( stream ) ),
                std::istreambuf_iterator< char >( )
            );

            if ( std::filesystem::exists( *path ) && path::get_existence_time( *path ) > 1000 * 60 * 60 * 24 * 3 ) {
                // std::filesystem::remove( path );
                return std::nullopt;
            }

            return std::make_optional( data );
        } catch ( ... ) { return std::nullopt; }
    }

    auto write_cache( const std::string& name, std::vector< unsigned char > data ) -> void{
        const auto path = get_local_path( name );

        if ( !path ) return;

        try {
            if ( std::filesystem::exists( *path ) ) std::filesystem::remove( *path );

            std::ofstream stream( *path, std::ios::binary );
            stream.write( reinterpret_cast< char* >( data.data( ) ), data.size( ) );

            stream.flush( );
        } catch ( ... ) {
        }
    }

    auto get( const std::string& url ) -> std::vector< unsigned char >{
        const auto cache = get_cache( url );
        if ( cache ) return *cache;

        WebClient client;
        auto      response = client.get( url );
        if ( !response ) return { };

        std::vector< unsigned char > data( response->begin( ), response->end( ) );
        write_cache( url, data );

        return data;
    }
}
