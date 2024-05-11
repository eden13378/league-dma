#pragma once

#include <expected>

namespace path {
    inline auto join(
        const std::initializer_list< std::string >& second
    ) noexcept -> std::expected< std::string, std::string >{
        // auto p = std::filesystem::path( first );
        std::optional< std::filesystem::path > p = std::nullopt;

        try {
            for ( auto i : second ) {
                if ( !p ) p = std::filesystem::path( i );
                else *p     = *p / std::filesystem::path( i );
            }
        } catch ( ... ) { return std::unexpected( "unknown error joining paths" ); }

        if ( !p ) return std::unexpected( "path::join: p is empty" );

        return p->string( );
    }

    inline auto get_existence_time( const std::string& path ) -> long long{
        const auto t = std::filesystem::last_write_time( path );

        const auto d = t.time_since_epoch( );

        return std::chrono::duration_cast< std::chrono::seconds >(
            std::chrono::system_clock::now( ).time_since_epoch( ) - d
        ).count( );
    }
}
