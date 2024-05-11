#pragma once
#include "../offsets.hpp"

namespace sdk::game {
    class CameraConfig {
    public:
        // Not used atm
        //char pad_0000[ 0x10 ]; //0x0000
        //float acceleration_time_mouse; //0x10
        //float deceleration_time_mouse; //0x14
        //float acceleration_time_keyboard; //0x18
        //float deceleration_time_keyboard; //0x1C
        //float topdown_zoom; //0x20
        //float zoom_min_distance; //0x24
        //float zoom_max_distance; //0x28
        //float zoom_easy_time; //0x2C
        //float zoom_min_speed; //0x30
        //float locked_camera_easing_distance; //0x34
        //float drag_scale; //0x38
        //float drag_momentum_decay; //0x3C
        //float drag_momentum_decay_weight; //0x40
        //float transition_duration_into_cinematic_mode; //0x44

        [[nodiscard]] static auto get_zoom( ) -> float;
        // zoom hack
        static auto set_zoom( float value ) -> bool;
    };
}
