#include "pch.hpp"

#include "c_fonts.hpp"

#include "../security/src/xorstr.hpp"
#include "fonts.hpp"
#include "../utils/cache_manager.hpp"
#include "../utils/debug_logger.hpp"

namespace renderer {
    auto Fonts::initialize( ) -> void{
        if ( m_initialized ) return;
        m_initialized = true;

        auto io = ImGui::GetIO( );

        cache_manager::get(
            "https://cdn.slotted.cc/public%2FOpenSans-VariableFont_wdth%2Cwght.ttf"
        );
        auto open_sans_regular = cache_manager::get_local_path(
            "https://cdn.slotted.cc/public%2FOpenSans-VariableFont_wdth%2Cwght.ttf"
        );

        if ( open_sans_regular ) {
            m_default_draw = io.Fonts->AddFontFromFileTTF(
                open_sans_regular->data( ),
                32
            );

            m_default_navbar = io.Fonts->AddFontFromFileTTF(
                open_sans_regular->data( ),
                38
            );
        }

        cache_manager::get( "https://cdn.slotted.cc/public%2FOpenSans-SemiBold.ttf" );
        auto open_sans_semi_bold = cache_manager::get_local_path(
            "https://cdn.slotted.cc/public%2FOpenSans-SemiBold.ttf"
        );

        if ( open_sans_semi_bold ) {
            m_default_bold = io.Fonts->AddFontFromFileTTF(
                open_sans_semi_bold->data( ),
                32
            );

            m_default_draw_small = io.Fonts->AddFontFromFileTTF(
                open_sans_regular->data( ),
                12
            );
        }

        m_block_draw = io.Fonts->AddFontFromMemoryCompressedTTF(
            fonts::esp_misc_font_compressed_data,
            fonts::esp_misc_font_compressed_size,
            8
        );

        m_block_big_draw = io.Fonts->AddFontFromMemoryCompressedTTF(
            fonts::esp_misc_font_compressed_data,
            fonts::esp_misc_font_compressed_size,
            12
        );

        if ( open_sans_regular ) {
            m_bold_draw = io.Fonts->AddFontFromFileTTF(
                open_sans_regular->data( ),
                32
            );

            m_bold_draw_16px = io.Fonts->AddFontFromFileTTF(
                open_sans_regular->data( ),
                16
            );
        }

        cache_manager::get(
            "https://cdn.slotted.cc/public%2FZabalDEMO-Ultra.ttf"
        );
        auto path = cache_manager::get_local_path(
            "https://cdn.slotted.cc/public%2FZabalDEMO-Ultra.ttf"
        );

        m_zabel_12px = io.Fonts->AddFontFromMemoryCompressedTTF(
            fonts::zabel_bold_compressed_data,
            fonts::zabel_bold_compressed_size,
            12
        );

        m_zabel_16px = io.Fonts->AddFontFromMemoryCompressedTTF(
            fonts::zabel_bold_compressed_data,
            fonts::zabel_bold_compressed_size,
            16
        );

        m_zabel_draw = io.Fonts->AddFontFromMemoryCompressedTTF(
            fonts::zabel_bold_compressed_data,
            fonts::zabel_bold_compressed_size,
            32
        );

        cache_manager::get( "https://cdn.slotted.cc/public%2FZabalDEMO-Ultra.ttf" );
        const auto nexa_path = cache_manager::get_local_path(
            "https://cdn.slotted.cc/public%2FZabalDEMO-Ultra.ttf"
        );

        if ( nexa_path ) {
            m_nexa_draw      = io.Fonts->AddFontFromFileTTF( nexa_path->data( ), 32 );
            m_nexa_draw_16px = io.Fonts->AddFontFromFileTTF( nexa_path->data( ), 16 );
            m_nexa_draw_20px = io.Fonts->AddFontFromFileTTF( nexa_path->data( ), 20 );
        }

        //m_zabel_draw = io.Fonts->AddFontFromFileTTF( path.data( ), 32 );
        //m_zabel_16px = io.Fonts->AddFontFromFileTTF( path.data( ), 16 );
        //m_zabel_12px = io.Fonts->AddFontFromFileTTF( path.data( ), 12 );

        const auto hwid_short_form = user::c_hwid_system( ).get_hwid_short_form( );
        const auto extra_font_path = std::format( ( "C:\\{}\\{}" ), hwid_short_form, _( "fonts" ) );
        if ( !std::filesystem::exists( extra_font_path ) ) std::filesystem::create_directories( extra_font_path );

        for ( auto& f : std::filesystem::directory_iterator( extra_font_path ) ) {
            if ( f.is_directory( ) ) continue;

            if ( f.path( ).extension( ) == ".ttf" ) {
                auto file      = f.path( ).filename( ).string( );
                auto file_name = file.substr( 0, file.size( ) - 4 );
                std::ranges::transform( file_name, file_name.begin( ), ::tolower );
                m_lua_fonts[ rt_hash( file_name.data( ) ) ] = std::make_unique< LuaFont >( io, f.path( ).string( ) );
                debug_log( "font: {} loaded", file_name );
            }
        }
    }

    auto Fonts::get_lua_font( std::string name ) const noexcept -> LuaFont*{
        try {
            std::ranges::transform( name, name.begin( ), ::tolower );
            const auto hashed = rt_hash( name.data( ) );

            for ( auto& f : m_lua_fonts ) if ( f.first == hashed ) return f.second.get( );

            return nullptr;
        } catch ( ... ) { return nullptr; }
    }

    Fonts::LuaFont::LuaFont( ImGuiIO io, std::string path ){
        try {
            _8 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                8
            );
            _10 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                10
            );
            _12 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                12
            );
            _14 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                14
            );
            _16 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                16
            );
            _18 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                18
            );
            _20 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                20
            );
            _24 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                24
            );
            _28 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                28
            );
            _32 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                32
            );
            _36 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                36
            );
            _40 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                40
            );
            _48 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                48
            );
            _56 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                56
            );
            _64 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                64
            );
            _72 = io.Fonts->AddFontFromFileTTF(
                path.data( ),
                72
            );
        } catch ( ... ) { if ( app && app->logger ) app->logger->error( "error setting up lua font" ); }
    }

    auto Fonts::LuaFont::get( int32_t size ) const noexcept -> ImFont*{
        if ( size <= 8 ) return _8;
        if ( size <= 10 ) return _10;
        if ( size <= 12 ) return _12;
        if ( size <= 14 ) return _14;
        if ( size <= 16 ) return _16;
        if ( size <= 18 ) return _18;
        if ( size <= 20 ) return _20;
        if ( size <= 24 ) return _24;
        if ( size <= 28 ) return _28;
        if ( size <= 32 ) return _32;
        if ( size <= 36 ) return _36;
        if ( size <= 40 ) return _40;
        if ( size <= 48 ) return _48;
        if ( size <= 56 ) return _56;
        if ( size <= 64 ) return _64;
        return _72;
    }
}
