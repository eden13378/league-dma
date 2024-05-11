#pragma once
#include "../build.hpp"

#if enable_lua

#include "framework/c_base_window.hpp"
#include "framework/menu_shared.hpp"

namespace menu {
    class CConsole : public framework::c_base_window {
    public:
        auto draw_contents( ) -> void override;

        auto initialize( ) -> void override{
            c_base_window::initialize( );
        }

        auto wnd_proc_handler( HWND hwnd, UINT msg, WPARAM w_param, LPARAM l_param ) -> void override;
        auto process_input( ) -> void override;

    private:
        auto draw_text_input_field( ) -> void;

    private:
        std::string m_input_buffer{ };
        int32_t m_input_position{ };
    };
}

#endif
