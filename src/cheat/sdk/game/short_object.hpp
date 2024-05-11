#pragma once
#include <cstdint>

#include "league_string.hpp"
#include "../math/vec3.hpp"

namespace sdk::game {
    class ShortObject {
    public:
        char                      pad_0000[ 4 ]; //0x0000
        class c_game_object_type* object_type; //0x0004
        char                      pad_0008[ 24 ]; //0x0008
        int16_t                   object_index; //0x0020
        int16_t                   next_object_index; //0x0022
        char                      pad_0024[ 40 ]; //0x0024
        int32_t                   team; //0x004C
        char                      pad_0050[ 28 ]; //0x0050
        LeagueString              name; //0x006C
        char                      pad_0080[ 76 ]; //0x0080
        int32_t                   network_id; //0x00CC
        char                      pad_00D0[ 216 ]; //0x00D0
        bool                      is_visible_on_screen; //0x01A8
        char                      pad_01A9[ 11 ]; //0x01A9
        int32_t                   framecount; //0x01B4
        char                      pad_01B8[ 32 ]; //0x01B8
        math::Vec3                position; //0x01D8
        char                      pad_01E4[ 28 ]; //0x01E4
        int32_t                   is_alive_flag; //0x0200
        char                      pad_0204[ 20 ]; //0x0204
        int8_t                    is_dead_flag; //0x0218
    };
}
