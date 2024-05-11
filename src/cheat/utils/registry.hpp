#pragma once
#include <string>
#include <windows.h>

namespace registry {
    enum class ERegistryError {
        unknown,
        none_success_code
    };

    auto get_string_reg_key(
        HKEY               key,
        const std::string& str_value_name,
        std::string&       str_value,
        const std::string& str_default_value
    ) -> std::expected< LONG, ERegistryError >;
}
