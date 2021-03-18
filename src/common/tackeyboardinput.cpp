#include "src/common/tacKeyboardinput.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacUtility.h"
#include "src/common/shell/tacShell.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/tacOS.h"

#include <iostream>

namespace Tac
{


  String ToString( Key key )
  {
    switch( key )
    {
      case Key::UpArrow: return "uparrow";
      case Key::DownArrow: return "downarrow";
      case Key::LeftArrow: return "leftarrow";
      case Key::RightArrow: return "rightarrow";
      case Key::Spacebar: return "spacebar";
      case Key::Debug: return "debug";
      case Key::Backspace: return "backspace";
      case Key::Delete: return "delete";
      case Key::Backtick: return "backtick";
      case Key::Escape: return "escape";
      case Key::MouseLeft: return "lclick";
      case Key::MouseRight: return "rclick";
      case Key::MouseMiddle: return "mclick";
      case Key::Modifier: return "mod";
      case Key::A: return "A";
      case Key::B: return "B";
      case Key::C: return "C";
      case Key::D: return "D";
      case Key::E: return "E";
      case Key::F: return "F";
      case Key::G: return "G";
      case Key::H: return "H";
      case Key::I: return "I";
      case Key::J: return "J";
      case Key::K: return "K";
      case Key::L: return "L";
      case Key::M: return "M";
      case Key::N: return "N";
      case Key::O: return "O";
      case Key::P: return "P";
      case Key::Q: return "Q";
      case Key::R: return "R";
      case Key::S: return "S";
      case Key::T: return "T";
      case Key::U: return "U";
      case Key::V: return "V";
      case Key::W: return "W";
      case Key::X: return "X";
      case Key::Y: return "Y";
      case Key::Z: return "Z";
      case Key::F5: return "f5";
      default: TAC_CRITICAL_ERROR_INVALID_CASE( key ); return "";
    }
  }

  bool KeyboardInputFrame::IsKeyDown( Key key )
  {
    return mCurrDown[ ( int )key ];
  }
  String KeyboardInputFrame::GetPressedKeyDescriptions()
  {
    Vector< String > keyNames;
    for( auto key : mCurrDown )
    {
      auto keyName = ToString( key );
      keyNames.push_back( keyName );
    }
    String keysDownText = "Keys Down: " + ( !keyNames.empty() ? Join( keyNames, ", " ) : "none" );
    return keysDownText;
  }


  bool KeyboardInput::IsKeyJustDown( Key key )
  {
    return !mPrev.IsKeyDown( key ) && IsKeyDown( key );
  }
  bool KeyboardInput::HasKeyJustBeenReleased( Key key )
  {
    return mPrev.IsKeyDown( key ) && !IsKeyDown( key );
  }
  bool KeyboardInput::IsKeyDown( Key key )
  {
    return mCurr.IsKeyDown( key );
  }
  void KeyboardInput::DebugImgui()
  {
    //ImGui::Text( mCurr.GetPressedKeyDescriptions() );
  }
  void KeyboardInput::SetIsKeyDown( Key key, bool isDown )
  {
    mCurr.mCurrDown[ ( int )key ] = isDown;
  }

  KeyboardInput gKeyboardInput;

  // should only be used in the stuffthread
  KeyboardInput::KeyboardInput()
  {
    //OSGetScreenspaceCursorPos( mCurr.mScreenspaceCursorPos, mCurr.mScreenspaceCursorPosErrors );
    //OSGetScreenspaceCursorPos( mPrev.mScreenspaceCursorPos, mPrev.mScreenspaceCursorPosErrors );
  }
  void KeyboardInput::BeginFrame()
  {
    //OSGetScreenspaceCursorPos( mCurr.mScreenspaceCursorPos, mCurr.mScreenspaceCursorPosErrors );
    //if( mCurr.mScreenspaceCursorPosErrors.empty() && mPrev.mScreenspaceCursorPosErrors.empty() )
    //  mMouseDeltaPosScreenspace = mCurr.mScreenspaceCursorPos - mPrev.mScreenspaceCursorPos;
    //else
    //  mMouseDeltaPosScreenspace = {};
    //mMouseDeltaScroll = mCurr.mMouseScroll - mPrev.mMouseScroll;
  }
  void KeyboardInput::EndFrame()
  {
    mPrev = mCurr;
    mMouseDeltaScroll = {};
    mMouseDeltaPosScreenspace = {};
    mWMCharPressedHax = 0;
  }
  void KeyboardInput::DebugPrintWhenKeysChange()
  {
    String currkeysDown = mCurr.GetPressedKeyDescriptions();
    String lastkeysDown = mPrev.GetPressedKeyDescriptions();
    if( currkeysDown == lastkeysDown )
      return;
    std::cout << currkeysDown.c_str() << std::endl;
  }


  // non-threadlocal
  static double       mMouseMovementConsummation = 0;
  const double        kConsumeDelta = 0.1f;
  void                TryConsumeMouseMovement( double* savedT )
  {
    const double curTime = ShellGetElapsedSeconds();
    const bool consumedBySomebody = curTime - mMouseMovementConsummation < kConsumeDelta;
    const bool isThatSomebodyUs =
      *savedT > mMouseMovementConsummation &&
      *savedT - mMouseMovementConsummation < kConsumeDelta;
    if( consumedBySomebody && !isThatSomebodyUs )
    {
      *savedT = 0;
      return;
    }

    mMouseMovementConsummation = curTime;
    *savedT = curTime;
  }
      

}
