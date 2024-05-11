#pragma once
#include <cstdint>
#include <ostream>

namespace sdk::game {
    class LeagueString {
    public:
        char        text[ 16 ]; //0x0000
        int32_t     size; //0x0010
        char        pad_0014[ 4 ]; //0x0014
        friend auto operator<<( std::ostream& os, const LeagueString& obj ) -> std::ostream&{
            return os << "lstring( " << obj.text << " )";
        }

        auto is_valid( ) const -> bool{ return text && size > 0 && size <= 16; }
    };
}
