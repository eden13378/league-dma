#pragma once

#include <random>
#include <string>

namespace user::encryption {
    auto xor_string( std::string message, const char key ) -> std::string;
    auto decrypt( const std::string& message ) -> std::string;
    auto get_random_string( uint32_t size ) -> std::string;
    auto get_random_char( ) -> std::string;
    auto encrypt( const std::string& message ) -> std::string;
}
