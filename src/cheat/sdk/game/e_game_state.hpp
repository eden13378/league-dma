#pragma once

namespace sdk::game {
    enum class EGameState {
        pre_game,
        spawn,
        game_loop,
        disconnected,
        game_end,
        pre_exit,
        exit
    };
}
