#include "tac_key.h" // self-inc

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/error/tac_assert.h"

auto Tac::KeyToString( const Key key ) -> StringView
{
  switch( key )
  {
    case Key::UpArrow:      return "up arrow";
    case Key::DownArrow:    return "down arrow";
    case Key::LeftArrow:    return "left arrow";
    case Key::RightArrow:   return "right arrow";

    case Key::Spacebar:     return "spacebar";
    case Key::Debug:        return "debug";
    case Key::Backspace:    return "backspace";
    case Key::Delete:       return "delete";
    case Key::Backtick:     return "backtick";
    case Key::Escape:       return "escape";
    case Key::Modifier:     return "mod";
    case Key::Enter:        return "enter";

    case Key::A:            return "A";
    case Key::B:            return "B";
    case Key::C:            return "C";
    case Key::D:            return "D";
    case Key::E:            return "E";
    case Key::F:            return "F";
    case Key::G:            return "G";
    case Key::H:            return "H";
    case Key::I:            return "I";
    case Key::J:            return "J";
    case Key::K:            return "K";
    case Key::L:            return "L";
    case Key::M:            return "M";
    case Key::N:            return "N";
    case Key::O:            return "O";
    case Key::P:            return "P";
    case Key::Q:            return "Q";
    case Key::R:            return "R";
    case Key::S:            return "S";
    case Key::T:            return "T";
    case Key::U:            return "U";
    case Key::V:            return "V";
    case Key::W:            return "W";
    case Key::X:            return "X";
    case Key::Y:            return "Y";
    case Key::Z:            return "Z";

    case Key::Digit0:       return "0";
    case Key::Digit1:       return "1";
    case Key::Digit2:       return "2";
    case Key::Digit3:       return "3";
    case Key::Digit4:       return "4";
    case Key::Digit5:       return "5";
    case Key::Digit6:       return "6";
    case Key::Digit7:       return "7";
    case Key::Digit8:       return "8";
    case Key::Digit9:       return "9";

    case Key::F1:           return "f1";
    case Key::F2:           return "f2";
    case Key::F3:           return "f3";
    case Key::F4:           return "f4";
    case Key::F5:           return "f5";
    case Key::F6:           return "f6";
    case Key::F7:           return "f7";
    case Key::F8:           return "f8";
    case Key::F9:           return "f9";
    case Key::F10:          return "f10";
    case Key::F11:          return "f11";
    case Key::F12:          return "f12";

    case Key::MouseLeft:    return "mouse left";
    case Key::MouseMiddle:  return "mouse middle";
    case Key::MouseRight:   return "mouse right";

    default: TAC_ASSERT_INVALID_CASE( key ); return "";
  }
}
