#pragma once
#include "../../sdk/sdk.hpp"
#include "../../security/src/xorstr.hpp"
#include "../../utils/debug_logger.hpp"
#include "..\..\utils\key.hpp"

namespace menu::framework::helper {
    inline auto cursor_in_rect( const Vec2 cursor, const Vec2 start, const Vec2 size ) -> bool{
        if ( cursor.x < start.x || cursor.y < start.y ) return false;
        if ( cursor.x > start.x + size.x || cursor.y > start.y + size.y ) return false;

        return true;
    }

    inline auto cursor_in_triangle( const Vec2 cursor, const Vec2 left, const Vec2 right, const Vec2 bottom ) -> bool{
        if ( cursor.x < left.x || cursor.y < left.y ) return false;
        if ( cursor.x > right.x ) return false;
        if ( cursor.y > bottom.y ) return false;

        return true;
    }

    inline auto key_to_character( const utils::EKey key ) -> std::string{
        debug_log( "key: {}", static_cast<int32_t>(key) );

        switch ( key ) {
        case utils::EKey::A:
            return _( "a" );
        case utils::EKey::B:
            return _( "b" );
        case utils::EKey::C:
            return _( "c" );
        case utils::EKey::D:
            return _( "d" );
        case utils::EKey::E:
            return _( "e" );
        case utils::EKey::F:
            return _( "f" );
        case utils::EKey::G:
            return _( "g" );
        case utils::EKey::H:
            return _( "h" );
        case utils::EKey::I:
            return _( "i" );
        case utils::EKey::J:
            return _( "j" );
        case utils::EKey::K:
            return _( "k" );
        case utils::EKey::L:
            return _( "l" );
        case utils::EKey::M:
            return _( "m" );
        case utils::EKey::N:
            return _( "n" );
        case utils::EKey::O:
            return _( "o" );
        case utils::EKey::P:
            return _( "p" );
        case utils::EKey::Q:
            return _( "q" );
        case utils::EKey::R:
            return _( "r" );
        case utils::EKey::S:
            return _( "s" );
        case utils::EKey::T:
            return _( "t" );
        case utils::EKey::U:
            return _( "u" );
        case utils::EKey::V:
            return _( "v" );
        case utils::EKey::W:
            return _( "w" );
        case utils::EKey::X:
            return _( "x" );
        case utils::EKey::Y:
            return _( "y" );
        case utils::EKey::Z:
            return _( "z" );
        case utils::EKey::space:
            return _( " " );
        case utils::EKey::_0:
            return _( "0" );
        case utils::EKey::_1:
            return _( "1" );
        case utils::EKey::_2:
            return _( "2" );
        case utils::EKey::_3:
            return _( "3" );
        case utils::EKey::_4:
            return _( "4" );
        case utils::EKey::_5:
            return _( "5" );
        case utils::EKey::_6:
            return _( "6" );
        case utils::EKey::_7:
            return _( "7" );
        case utils::EKey::_8:
            return _( "8" );
        case utils::EKey::_9:
            return _( "9" );
        default:
            return "";
        }
    }

    inline auto key_to_string( const utils::EKey key ) -> std::string{
        switch ( key ) {
        case utils::EKey::lbutton:
            return _( "l mouse" );
        case utils::EKey::rbutton:
            return _( "r mouse" );
        case utils::EKey::cancel:
            return _( "cancel" );
        case utils::EKey::mbutton:
            return _( "m mouse" );
        case utils::EKey::xbutton1:
            return _( "xbutton1" );
        case utils::EKey::xbutton2:
            return _( "xbutton2" );
        case utils::EKey::back:
            return _( "back" );
        case utils::EKey::tab:
            return _( "tab" );
        case utils::EKey::clear:
            return _( "clr" );
        case utils::EKey::return_key:
            return _( "return" );
        case utils::EKey::shift:
            return _( "shift" );
        case utils::EKey::control:
            return _( "ctrl" );
        case utils::EKey::menu:
            return _( "menu" );
        case utils::EKey::pause:
            return _( "pause" );
        case utils::EKey::capital:
            return _( "cap" );
        case utils::EKey::kana:
            return _( "kana" );
        case utils::EKey::hanguel:
            return _( "hanguel" );
        case utils::EKey::hangul:
            return _( "hangul" );
        case utils::EKey::escape:
            return _( "esc" );
        case utils::EKey::convert:
            return _( "convert" );
        case utils::EKey::nonconvert:
            return _( "nonconvert" );
        case utils::EKey::accept:
            return _( "accept" );
        case utils::EKey::modechange:
            return _( "mode change" );
        case utils::EKey::space:
            return _( "space" );
        case utils::EKey::prior:
            return _( "prior" );
        case utils::EKey::next:
            return _( "next" );
        case utils::EKey::end:
            return _( "end" );
        case utils::EKey::home:
            return _( "home" );
        case utils::EKey::left:
            return _( "left" );
        case utils::EKey::up:
            return _( "up" );
        case utils::EKey::right:
            return _( "right" );
        case utils::EKey::down:
            return _( "down" );
        case utils::EKey::select:
            return _( "select" );
        case utils::EKey::print:
            return _( "print" );
        case utils::EKey::execute:
            return _( "execute" );
        case utils::EKey::snapshot:
            return _( "snapshot" );
        case utils::EKey::insert:
            return _( "insert" );
        case utils::EKey::delete_key:
            return _( "delete" );
        case utils::EKey::help:
            return _( "help" );
        case utils::EKey::_0:
            return _( "0" );
        case utils::EKey::_1:
            return _( "1" );
        case utils::EKey::_2:
            return _( "2" );
        case utils::EKey::_3:
            return _( "3" );
        case utils::EKey::_4:
            return _( "4" );
        case utils::EKey::_5:
            return _( "5" );
        case utils::EKey::_6:
            return _( "6" );
        case utils::EKey::_7:
            return _( "7" );
        case utils::EKey::_8:
            return _( "8" );
        case utils::EKey::_9:
            return _( "9" );
        case utils::EKey::A:
            return _( "a" );
        case utils::EKey::B:
            return _( "b" );
        case utils::EKey::C:
            return _( "c" );
        case utils::EKey::D:
            return _( "d" );
        case utils::EKey::E:
            return _( "e" );
        case utils::EKey::F:
            return _( "f" );
        case utils::EKey::G:
            return _( "g" );
        case utils::EKey::H:
            return _( "h" );
        case utils::EKey::I:
            return _( "i" );
        case utils::EKey::J:
            return _( "j" );
        case utils::EKey::K:
            return _( "k" );
        case utils::EKey::L:
            return _( "l" );
        case utils::EKey::M:
            return _( "m" );
        case utils::EKey::N:
            return _( "n" );
        case utils::EKey::O:
            return _( "o" );
        case utils::EKey::P:
            return _( "p" );
        case utils::EKey::Q:
            return _( "q" );
        case utils::EKey::R:
            return _( "r" );
        case utils::EKey::S:
            return _( "s" );
        case utils::EKey::T:
            return _( "t" );
        case utils::EKey::U:
            return _( "u" );
        case utils::EKey::V:
            return _( "v" );
        case utils::EKey::W:
            return _( "w" );
        case utils::EKey::X:
            return _( "x" );
        case utils::EKey::Y:
            return _( "y" );
        case utils::EKey::Z:
            return _( "z" );
        case utils::EKey::n0:
            return _( "num0" );
        case utils::EKey::n1:
            return _( "num1" );
        case utils::EKey::n2:
            return _( "num2" );
        case utils::EKey::n3:
            return _( "num3" );
        case utils::EKey::n4:
            return _( "num4" );
        case utils::EKey::n5:
            return _( "num5" );
        case utils::EKey::n6:
            return _( "num6" );
        case utils::EKey::n7:
            return _( "num7" );
        case utils::EKey::n8:
            return _( "num8" );
        case utils::EKey::n9:
            return _( "num9" );
        case utils::EKey::f1:
            return _( "f1" );
        default:
            return _( "unknown" );
        }
    }
}
