#pragma once

namespace utils::resource_updater {
    auto get( const std::string_view url, const uint32_t timeout ) -> std::expected< std::string, const char* >;
    auto download_to_file( const std::string& local_path, const std::string& url ) -> void;
    auto update( ) -> void;
}
