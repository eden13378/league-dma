#pragma once
#include <cstdint>
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    class BuffInfo;

    class BuffData {
    public:
        [[nodiscard]] auto get_buff_info( ) const -> utils::MemoryHolder< BuffInfo >;
        [[nodiscard]] auto is_active( ) const -> bool;
        [[nodiscard]] auto is_hard_cc( ) const -> bool;
        [[nodiscard]] auto is_disabling( ) const -> bool;
        [[nodiscard]] auto is_knock_up( ) const -> bool;
        [[nodiscard]] auto is_silence( ) const -> bool;
        [[nodiscard]] auto is_cripple( ) const -> bool;
        [[nodiscard]] auto is_invincible( ) const -> bool;
        [[nodiscard]] auto is_slow( ) const -> bool;

        [[nodiscard]] auto get_source_index( ) const -> int16_t;
        [[nodiscard]] auto get_senna_passive_attack_damage() const -> float;

        char             pad_0000[ 12 ]; //0x0000
        uint8_t          type; //0x0008
        char             pad_0005[ 3 ]; //0x0009
        class BuffInfo*  buff_info; //0x0010
        float            start_time; //0x0018
        float            end_time; //0x001C
        float            end_time2; //0x0020
        char             pad_0024[ 4 ]; //0x0024
        class BuffData*  self; //0x0028
        class N00001920* N00001860; //0x0030
        int32_t          stack; //0x0038
        int32_t          max_stack; //0x003C
        char             pad_0040[ 76 ]; //0x0040
        int32_t          count; //0x0078
        int32_t          count2; //0x007C
        char             pad_0080[ 84 ]; //0x0080
    };
}
