#pragma once

#include "..\utils\memory_holder.hpp"
#include "game/object_manager.hpp"
#include "game/camera_config.hpp"
#include "game/navgrid.hpp"
#include "game/tactical_map.hpp"
#include "game/menugui.hpp"
#include "game/e_game_state.hpp"
#include "../utils/holder.hpp"
#include "game/pw_hud.hpp"

#include <d3d9types.h>


class c_matrix_holder {
public:
    D3DMATRIX view_matrix;       //0x0058
    D3DMATRIX projection_matrix; // 0x0098
};

extern CHolder                                         g_local;
extern utils::MemoryHolder< sdk::game::ObjectManager > g_objects;
extern utils::MemoryHolder< sdk::game::CameraConfig >  g_camera_config;
extern intptr_t                                        g_league_base;
extern utils::MemoryHolder< float >                    g_time;
extern utils::MemoryHolder< sdk::game::EGameState >    g_state;
extern bool                                            g_game_focused;
extern utils::MemoryHolder< sdk::game::TacticalMap >   g_minimap;
extern utils::MemoryHolder< sdk::game::Navgrid >       g_navgrid;
extern utils::MemoryHolder< sdk::game::MenuGui >       g_menugui;

extern utils::MemoryHolder< c_matrix_holder > g_render_matrix;
// extern unload_t g_unload;

namespace sdk {
    enum class EInitializeGlobalsError {
        unknown,
        process_not_initialized,
        memory_not_initialized,
        league_not_running,
        request_exit,
        offset_incorrect_object_manager,
        offset_incorrect_time,
        offset_incorrect_render_manager,
        offset_incorrect_pw_hud,
        offset_incorrect_navgrid,
        offset_incorrect_minimap_instance,
        offset_incorrect_camera_config,
        offset_incorrect_menu_gui,
        app_memory_process_not_initialized,
        max,
    };

    /**
     * \brief Sets up global variables.
     * \param process target process to initialize globals for.
     */
    auto initialize_globals( const sdk::memory::Process& process ) noexcept -> std::expected< void, EInitializeGlobalsError >;
}
