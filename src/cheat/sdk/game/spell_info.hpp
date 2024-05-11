#pragma once
#include <cstdint>
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class SpellData;

    class SpellInfo {
    public:
        SpellInfo( ) = default;

        [[nodiscard]] auto get_spell_data( ) const -> utils::MemoryHolder< SpellData >;

        [[nodiscard]] auto get_spell_data_raw( ) const -> std::optional< SpellData >;

        char pad_0000[40]; //0x0000
        char *owner_character; //0x0028
        char pad_0030[8]; //0x0030
        int64_t owner_character_size; //0x0038
        char pad_0040[16]; //0x0040
        class SpellData *spell_data; //0x0050
        char pad_0058[112]; //0x0058
    };
}
