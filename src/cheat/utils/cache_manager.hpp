#pragma once
#include <expected>

namespace cache_manager {
    auto get_local_path( const std::string& name ) -> std::expected<std::string, const char*>;
    auto get_cache( const std::string& name ) -> std::optional< std::vector< unsigned char > >;
    auto write_cache( const std::string& name, std::vector< unsigned char > data ) -> void;

    auto get( const std::string& url ) -> std::vector< unsigned char >;
}
