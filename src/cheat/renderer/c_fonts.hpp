#pragma once
#include <format>
// #include "../config/c_config_system.hpp"
// #include "../features/c_spell_detector.hpp"
#include "../security/src/hash_t.hpp"

#include "../include/imgui/imgui.h"

namespace renderer {
    class Fonts {
        struct LuaFont {
            LuaFont( ) = default;

            LuaFont( ImGuiIO io, std::string path );

            ImFont* _8;
            ImFont* _10;
            ImFont* _12;
            ImFont* _14;
            ImFont* _16;
            ImFont* _18;
            ImFont* _20;
            ImFont* _24;
            ImFont* _28;
            ImFont* _32;
            ImFont* _36;
            ImFont* _40;
            ImFont* _48;
            ImFont* _56;
            ImFont* _64;
            ImFont* _72;

            auto get( int32_t size ) const noexcept -> ImFont*;
        };

    public:
        auto initialize( ) -> void;

        auto get_default( ) const noexcept -> ImFont*{ return m_default_draw; }
        auto get_default_bold( ) const noexcept -> ImFont*{ return m_default_bold; }
        auto get_default_small( ) const noexcept -> ImFont*{ return m_default_draw_small; }
        auto get_block( ) const noexcept -> ImFont*{ return m_block_draw; }
        auto get_bold( ) const noexcept -> ImFont*{ return m_bold_draw; }
        auto get_zabel( ) const noexcept -> ImFont*{ return m_zabel_draw; }
        auto get_zabel_16px( ) const noexcept -> ImFont*{ return m_zabel_16px; }
        auto get_zabel_12px( ) const noexcept -> ImFont*{ return m_zabel_12px; }
        auto get_default_navbar( ) const noexcept -> ImFont*{ return m_default_navbar; }

        auto get_nexa( ) const noexcept -> ImFont*{ return m_nexa_draw; }
        auto get_nexa_16px( ) const noexcept -> ImFont*{ return m_nexa_draw_16px; }
        auto get_nexa_20px( ) const noexcept -> ImFont*{ return m_nexa_draw_20px; }

        auto get_bold_16px( ) const noexcept -> ImFont*{ return m_bold_draw_16px; }

        auto get_lua_font( std::string name ) const noexcept -> LuaFont*;

    private:
        ImFont* m_default_draw{ };
        ImFont* m_default_navbar{ };
        ImFont* m_default_bold{ };
        ImFont* m_default_draw_small{ };
        ImFont* m_bold_draw{ };
        ImFont* m_block_draw{ };
        ImFont* m_block_big_draw{ };
        ImFont* m_zabel_draw{ };
        ImFont* m_zabel_16px{ };
        ImFont* m_zabel_12px{ };
        ImFont* m_bold_draw_16px{ };
        ImFont* m_nexa_draw{ };
        ImFont* m_nexa_draw_16px{ };
        ImFont* m_nexa_draw_20px{ };

        bool m_initialized{ false };

        std::map< hash_t, std::unique_ptr< LuaFont > > m_lua_fonts{ };
    };
}

extern Fonts* g_fonts;
