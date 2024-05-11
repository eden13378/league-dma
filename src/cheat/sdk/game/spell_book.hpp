#pragma once
#include "../../lua-v2/custom_structs.hpp"
#include "..\..\utils\memory_holder.hpp"

namespace sdk::game {
    enum class ESpellSlot;
    class SpellCastInfo;
    class SpellSlot;

    class SpellBook {
    public:
        auto get_spell_slot( ESpellSlot id ) const -> utils::MemoryHolder< SpellSlot >;
        auto get_spell_cast_info( ) const -> utils::MemoryHolder< SpellCastInfo >;

        auto get_spell_slot_raw( ESpellSlot id ) const -> std::optional< lua::LuaSpellSlot >;
        auto get_spell_cast_info_raw( ) const -> std::optional< lua::LuaSpellCastInfo >;

        char pad_0000[64]; //0x0000
        class SpellCastInfo *spell_cast_info; //0x0040
        char pad_0048[2752]; //0x0048
        class SpellSlot *spell_slot_q; //0x0B08
        class SpellSlot *spell_slot_w; //0x0B10
        class SpellSlot *spell_slot_e; //0x0B18
        class SpellSlot *spell_slot_r; //0x0B20
        class N00000AE7 *N000008DE; //0x0B28
        class N00000AFB *N000008DF; //0x0B30
        class N00000B0F *N000008E0; //0x0B38
        class N00000B23 *N000008E1; //0x0B40
        class N00000B37 *N000008E2; //0x0B48
        class N00000B4B *N000008E3; //0x0B50
        class N00000B5F *N000008E4; //0x0B58
        class N00000B73 *N000008E5; //0x0B60
        class N00000B87 *N000008E6; //0x0B68
        class N00000B9B *N000008E7; //0x0B70
        class N00000BAF *N000008E8; //0x0B78
        class N00000BC3 *N000008E9; //0x0B80
        class N00000BD7 *N000008EA; //0x0B88
        class N00000BEB *N000008EB; //0x0B90
        class N00000BFF *N000008EC; //0x0B98
        class N00000C13 *N000008ED; //0x0BA0
        class N00000C27 *N000008EE; //0x0BA8
        class N00000C3B *N000008EF; //0x0BB0
        class N00000C4F *N000008F0; //0x0BB8
        class N00000C63 *N000008F1; //0x0BC0
        class N00000C77 *N000008F2; //0x0BC8
        class N00000C8B *N000008F3; //0x0BD0
        class N00000C9F *N000008F4; //0x0BD8
        class N00000CB3 *N000008F5; //0x0BE0
        class N00000CC7 *N000008F6; //0x0BE8
        class N00000CDB *N000008F7; //0x0BF0
        class N00000CEF *N000008F8; //0x0BF8
        class N00000D03 *N000008F9; //0x0C00
        class N00000D17 *N000008FA; //0x0C08
        class N00000D2B *N000008FB; //0x0C10
        class N00000D3F *N000008FC; //0x0C18
        class N00000D53 *N000008FD; //0x0C20
        class N00000D67 *N000008FE; //0x0C28
        class N00000D7B *N000008FF; //0x0C30
        class N00000D8F *N00000900; //0x0C38
        class N00000DA3 *N00000901; //0x0C40
        class N00000DB7 *N00000902; //0x0C48
        class N00000DCB *N00000903; //0x0C50
        class N00000DDF *N00000904; //0x0C58
        class N00000DF3 *N00000863; //0x0C60
        class N00000E07 *N00000691; //0x0C68
        char pad_0C70[952]; //0x0C70
    };
}
