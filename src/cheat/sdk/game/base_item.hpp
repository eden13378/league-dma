#pragma once
#include <cstdint>
#include "item_data.hpp"

namespace sdk::game {
    class BaseItem {
    public:
        auto get_item_data( ) const -> utils::MemoryHolder< ItemData >{
            if ( !item_data ) return { };

            return utils::MemoryHolder< ItemData >( reinterpret_cast< intptr_t >( item_data ) );
        }

        auto get_item_data_raw( ) const -> std::optional< ItemData >{
            auto data = get_item_data( );

            if ( !data ) return std::nullopt;

            return *data;
        }

        char            pad_0000[ 48 ]; //0x0000
        unsigned char   can_use; //0x0030
        char            pad_0031[ 7 ]; // 0x0031
        class ItemData* item_data; //0x0038
        uint8_t         stacks_left{ }; // 0x0040
        char            pad_0041[ 3 ];  // 0x0031
        float           price; //0x0044
        float           rageblade_damage; // 0x0048
        char            pad_002C[ 88 ]; //0x002C
    };
}
