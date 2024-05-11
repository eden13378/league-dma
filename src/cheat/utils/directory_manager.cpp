#include "pch.hpp"
#include "directory_manager.hpp"

#include "path.hpp"

namespace directory_manager {
    static auto hwid = user::c_hwid_system( ).get_hwid_short_form( );

    auto get_base_path( ) -> std::expected< std::string, std::string >{ return path::join( { "C:\\", hwid } ); }

    auto get_resources_path( ) -> std::string{
        const auto base_path = get_base_path( );

        if ( !base_path.has_value( ) ) {
            if ( app && app->logger ) app->logger->error( "get_resources_path: base_path has no value" );
            return "";
        }

        static auto resources_path = path::join( { *base_path, "resources" } );

        if ( !resources_path ) {
            if ( app && app->logger ) app->logger->error( "get_resources_path: error joining paths" );
            return "";
        }

        return *resources_path;
    }
}
