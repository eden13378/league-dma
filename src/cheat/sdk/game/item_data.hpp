#pragma once
#include <cstdint>

namespace sdk::game {
    class ItemData {
    public:
        char    pad_0000[ 156 ]; //0x0000
        int32_t id; //0x009C
        int32_t max_stacks; //0x00A0
        char    pad_0070[ 52 ]; //0x00A4
        int32_t is_consumable; //0x00D8
        char    pad_009C[ 100 ]; //0x009C
    };
}
