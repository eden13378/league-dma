#pragma once

namespace sdk::offsets {
    /**
     * Signatures:
     * 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B F8 48 85 C0 75 ? 48 83 C4 ? 5F C3 48 8B 00 48 8B CF 48 89 5C 24 ? FF 50 ? 48 8B D8 E8 ? ? ? ? 4C 8B C0 48 8B D7 48 8B CB 48 8B 5C 24 ? 48 83 C4 ? 5F E9 ? ? ? ? ? ? 40 57 48 83 EC ? 48 8B 0D ? ? ? ?
     */
    constexpr auto object_manager = 0x18ffac8;
    /**
     * Signatures:
     * 0F 84 ? ? ? ? 48 89 5C 24 ? 48 8D 0D ? ? ? ? 48 8B 98 ? ? ? ?
     */
    constexpr auto local_player = 0x19216f0;
    /**
     * Signatures:
     * 48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 89 0D ? ? ? ? 48 8D 05 ? ? ? ? 48 89 01 48 8B F9
     * E8 ? ? ? ? EB 03 49 8B C6 48 8D 0D ? ? ? ?  -> inside function
     * qword in last else statement
     *
     * 74 17 0F B6 40 18
     *
 *   *(_WORD *)(a1 + 82) = v65;
   if ( qword_7FF7DBEBC4E8 ) <- this qword
   {
    HIBYTE(v66) = 1;
    LOBYTE(v66) = (*(_BYTE *)(qword_7FF7DBEBC4E8 + 24) & 1) == 0;
    *(_WORD *)(a1 + 84) = v66;
   }
     */
    constexpr auto game_time = 0x1905388;
    /**
    * Signatures:
    * 48 8B 0D ? ? ? ? E8 ? ? ? ? 48 8B 9E ? ? ? ? 8B 86 ? ? ? ? 48 8D 3C C3 48 3B DF 74 ? 0F 1F 84 00 ? ? ? ?
    *
   *  48 89 5C 24 ? 48 89 6C 24 ? 48 89 74 24 ? 57 48 83 EC ? 48 8B F1 0F B6 EA
   *   (*(void (__fastcall **)(__int64))(*(_QWORD *)qword_7FF7DBEC8BC0 + 16i64))(qword_7FF7DBEC8BC0);
   *   sub_7FF7DAB32FE0(qword_7FF7DBF250D0); <- this qword
    */
    constexpr auto render_manager = 0x197de68;

    /**
    * Signatures:
    * 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 ? E9 ? ? ? ? ? ? ? ? B9 ? ? ? ?
    * 48 83 EC ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? E8 ? ? ? ? 48 8D 0D ? ? ? ? 48 83 C4 ? E9 ? ? ? ? ? ? ? ? 48 8D 0D ? ? ? ?
    */
    constexpr auto render_matrix = 0x1974cf0;

    /**
    * Signatures:
    * 48 8B 0D ? ? ? ? 8B 57 ?
    */
    constexpr auto pw_hud = 0x18ffad8;
    /**
    * Signatures:
    * 48 8B 05 ? ? ? ? 48 8B 5C 24 ? 48 89 47 70
    */
    constexpr auto camera_config = 0x190b650;

    /**
  * Signatures:
  * direct reference: [actual address in first opcode] 48 83 EC 70 48 8B 0D ? ? ? ?
  */

    constexpr auto r3d_camera = 0x1905170;

    /**
    * String xref:
    * "game.gameState" - 0x6
    * 40 53 48 83 EC ? 8B 91 ? ? ? ? 48 8B D9 83 EA ?
    * 83 78 ? ? 75 ? C7 83 ? ? ? ? ? ? ? ?
    *
    *       case 2:
    * result = qword_7FF7DBEBC360;
    * if ( *(_BYTE *)(qword_7FF7DBEBC360 + 63) )
    * {
    *   in -> 40 53 48 83 EC ? 8B 91 ? ? ? ? 48 8B D9 83 EA ?
    *
    *
    * 48 8B 48 30 0F B6 81 ? ? ? ?
    */
    constexpr auto game_state = 0x19123e8;

    /**
    * Signatures:
    * C6 80 ? ? ? ? ? 48 8B 1D ? ? ? ?  next line
    */
    constexpr auto minimap_instance = 0x1911ee0;

    /**
     * Signatures:
     * 48 8B 05 ? ? ? ? 45 32 FF   exact instruction
     * 48 8B 0D ? ? ? ? 48 8B 49 ? E9 ? ? ? ? 48 89 5C 24 ? 57 48 83 EC ? FF 41 ?
     * one of the bellow
     * 48 8B 05 ? ? ? ? 48 8B 40 ? 48 85 C0 75 ?
     * 48 8B 40 ? 48 85 C0 75 ? C3 0F B6 80 ? ? ? ?
     */
    constexpr auto navgrid = 0x1900008;

    // 48 83 EC 28 48 8B 0D ? ? ? ? B2 01  only qword in function
    // 48 89 5C 24 ? 57 48 83 EC ? 0F B6 DA 48 8B F9 0F B6 51 ? -> else -> in the if and fn call
    constexpr auto menu_gui = 0x19216f8;
}
