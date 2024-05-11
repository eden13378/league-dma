#pragma once
#include <cstdint>

namespace sdk::game {
    class ObjectManager {
    public:
        char               pad_0000[ 24 ]; //0x0000
        class ObjectArray* object_array; //0x0018
        int64_t            last_max_max_offset; //0x0020
        char               pad_0028[ 32 ]; //0x0028
        int64_t            objects_amount; //0x0048
        char               pad_0050[ 48 ]; //0x0050
    };
}
