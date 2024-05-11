#pragma once
#include <string>

namespace directory_manager {
    auto get_base_path( ) -> std::expected< std::string, std::string >;
    auto get_resources_path( ) -> std::string;
}
