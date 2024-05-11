#pragma once

namespace lua::json {
    auto parse( const std::string& json ) -> sol::object;
    auto stringify( sol::object obj ) -> std::string;
}
