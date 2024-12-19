#include "tac_keyboard_input.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/containers/tac_array.h"

namespace Tac::Keyboard
{
  struct KeyboardInputFrame
  {
    bool      IsKeyDown( const Key );
    String    GetPressedKeyDescriptions();

    Array< bool, ( int )Key::Count > mCurrDown;
  };



  //struct KeyboardInputFrame
  //{
  //  bool    IsKeyDown( Key );
  //  String  GetPressedKeyDescriptions();

  //  bool    mCurrDown[ ( int )Key::Count ] = {};
  //};

  static bool operator == ( const KeyboardInputFrame& a, const KeyboardInputFrame& b )
  {
    return a.mCurrDown == b.mCurrDown;
  }
  //{
  //  for( int i = 0; i < ( int )Key::Count; ++i )
  //    if( a[ i ] != b[ i ] )
  //      return false;

  //    return true;
  //}

  struct KeyboardInput
  {
    bool               HasKeyJustBeenReleased( Key );
    bool               IsKeyJustDown( Key );
    bool               IsKeyDown( Key );
    bool               WasKeyDown( Key );
    void               DebugImgui();
    void               DebugPrintWhenKeysChange();
    void               SetIsKeyDown( Key, bool );

    // Called after input has been gathered, but before the game updates
    void               BeginFrame();
    void               EndFrame();


    KeyboardInputFrame mCurr             {};
    KeyboardInputFrame mPrev             {};
    Codepoint          mWMCharPressedHax {};

  };

  // -----------------------------------------------------------------------------------------------

  static KeyboardInput gKeyboardInput;

  // -----------------------------------------------------------------------------------------------

  bool      KeyboardHasKeyJustBeenReleased( Key key)
  {
    return gKeyboardInput.HasKeyJustBeenReleased(key);
  }

  bool      KeyboardIsKeyJustDown( Key key )
  {
    return gKeyboardInput.IsKeyJustDown(key);
  }

  bool      KeyboardIsKeyDown( Key key)
  {
    return gKeyboardInput.IsKeyDown(key);
  }

  bool      KeyboardWasKeyDown( Key key )
  {
    return gKeyboardInput.WasKeyDown(key);
  }

  void      KeyboardDebugImgui()
  {
    gKeyboardInput.DebugImgui();
  }

  void      KeyboardDebugPrintWhenKeysChange()
  {
    gKeyboardInput.DebugPrintWhenKeysChange();
  }

  void      KeyboardSetIsKeyDown( Key key, bool down)
  {
    gKeyboardInput.SetIsKeyDown(key, down);
  }


  Codepoint KeyboardGetWMCharPressedHax() 
  {
    return gKeyboardInput.mWMCharPressedHax;
  }

  void      KeyboardSetWMCharPressedHax(Codepoint codepoint)
  {
    gKeyboardInput.mWMCharPressedHax = codepoint;
  }


  // Called after input has been gathered, but before the game updates
  void      KeyboardBeginFrame()
  {
    gKeyboardInput.BeginFrame();
  }

  void      KeyboardEndFrame()
  {
    gKeyboardInput.EndFrame();
  }

  String    ToString( const Key key )
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
      default: TAC_ASSERT_INVALID_CASE( key ); return "";
    }
  }


  bool      KeyboardInputFrame::IsKeyDown( const Key key )
  {
    return mCurrDown[ ( int )key ];
  }

  String    KeyboardInputFrame::GetPressedKeyDescriptions()
  {
    Vector< String > keyNames;
    for( int i {}; i < ( int )Key::Count; ++i )
    {
      const Key key = ( Key )i;
      if( !IsKeyDown( key ) )
        continue;

      const String keyName { ToString( key ) };
      keyNames.push_back( keyName );
    }

    const String keysDownText { "Keys Down: " + (
      keyNames.empty()
      ? String( "none" )
      : Join( keyNames, ", " ) ) };
    return keysDownText;
  }

  bool KeyboardInput::IsKeyJustDown( const Key key )
  {
    return !mPrev.IsKeyDown( key ) && IsKeyDown( key );
  }

  bool KeyboardInput::HasKeyJustBeenReleased( const Key key )
  {
    return mPrev.IsKeyDown( key ) && !IsKeyDown( key );
  }

  bool KeyboardInput::IsKeyDown( const Key key )
  {
    return mCurr.IsKeyDown( key );
  }

  bool KeyboardInput::WasKeyDown( const Key key )
  {
    return mPrev.IsKeyDown( key );
  }

  void KeyboardInput::DebugImgui()
  {
    //ImGui::Text( mCurr.GetPressedKeyDescriptions() );
  }

  void KeyboardInput::SetIsKeyDown( const Key key, const bool isDown )
  {
    mCurr.mCurrDown[ ( int )key ] = isDown;
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
    mWMCharPressedHax = 0;
  }

  void KeyboardInput::DebugPrintWhenKeysChange()
  {
    if( mCurr != mPrev )
    {
    }
    String currkeysDown = mCurr.GetPressedKeyDescriptions();
    String lastkeysDown = mPrev.GetPressedKeyDescriptions();
    if( currkeysDown == lastkeysDown )
      return;
    OS::OSDebugPrintLine(currkeysDown.c_str());
  }

} // namespace Tac::Keyboard

// -------------------------------------------------------------------------------------------------

namespace Tac::Mouse
{

  struct MouseInputFrame
  {
    bool    IsDown( Button ) const;

    bool    mCurrDown[ ( int )Button::Count ] {};
    v2      mScreenspaceCursorPos             {};
    int     mMouseScroll                      {};
  };

  struct MouseInput
  {
    void BeginFrame();
    void EndFrame();

    bool      ButtonJustBeenReleased( Button );
    bool      ButtonJustDown( Button );
    bool      ButtonIsDown( Button );
    bool      ButtonWasDown( Button );
    void      ButtonSetIsDown( Button, bool );

    MouseInputFrame mCurr             {};
    MouseInputFrame mPrev             {};
    v2              mMouseDeltaPos    {};
    int             mMouseDeltaScroll {};
  };

  // -----------------------------------------------------------------------------------------------

  bool      MouseInputFrame::IsDown( const Button key ) const
  {
    return mCurrDown[ ( int )key ];
  }

  // -----------------------------------------------------------------------------------------------

  void MouseInput::BeginFrame()
  {
    //OSGetScreenspaceCursorPos( mCurr.mScreenspaceCursorPos, mCurr.mScreenspaceCursorPosErrors );
    //if( mCurr.mScreenspaceCursorPosErrors.empty() && mPrev.mScreenspaceCursorPosErrors.empty() )
    //  mMouseDeltaPosScreenspace = mCurr.mScreenspaceCursorPos - mPrev.mScreenspaceCursorPos;
    //else
    //  mMouseDeltaPosScreenspace = {};
    //mMouseDeltaScroll = mCurr.mMouseScroll - mPrev.mMouseScroll;
  }

  void MouseInput::EndFrame()
  {
    mPrev = mCurr;
    mMouseDeltaScroll = {};
    mMouseDeltaPos = {};
  }

  bool      MouseInput::ButtonJustBeenReleased( Button button )
  {
    return mPrev.IsDown( button ) && !ButtonIsDown( button );
  }

  bool      MouseInput::ButtonJustDown( Button button )
  {
    return !mPrev.IsDown( button ) && ButtonIsDown( button );
  }

  bool      MouseInput::ButtonIsDown( Button button )
  {
    return mCurr.IsDown( button );
  }

  bool      MouseInput::ButtonWasDown( Button button )
  {
    return mPrev.IsDown( button );
  }

  void      MouseInput::ButtonSetIsDown( Button button, bool isDown )
  {
    mCurr.mCurrDown[ ( int )button ] = isDown;
  }

  String    ToString( const Button key )
  {
    switch( key )
    {
      case Button::MouseLeft: return "lclick";
      case Button::MouseRight: return "rclick";
      case Button::MouseMiddle: return "mclick";
      default: TAC_ASSERT_INVALID_CASE( key ); return "";
    }
  }

  // -----------------------------------------------------------------------------------------------

  static Timestamp           mMouseMovementConsummation;
  const  TimestampDifference kConsumeDelta = 0.1f;
  static StackFrame          sConsumeFrame;
  static MouseInput          sMouseInput;

  // -----------------------------------------------------------------------------------------------

  void      TryConsumeMouseMovement( Timestamp* savedT, const StackFrame& stackFrame )
  {
    const Timestamp oldSavedT = *savedT;
    const Timestamp curTime = Timestep::GetElapsedTime();
    const bool consumedBySomebody = curTime - mMouseMovementConsummation < kConsumeDelta;
    const bool isThatSomebodyUs =
      *savedT >= mMouseMovementConsummation && // bugfix: >=, not >
      *savedT < mMouseMovementConsummation  + kConsumeDelta;
    if( consumedBySomebody && !isThatSomebodyUs )
    {
      *savedT = 0.0;
      return;
    }

    mMouseMovementConsummation = curTime;
    *savedT = curTime;
    sConsumeFrame = stackFrame;
  }

  v2        GetMouseDeltaPos()
  {
    return sMouseInput.mMouseDeltaPos;
  }

  void      MouseWheelEvent(int delta)
  {
    sMouseInput.mCurr.mMouseScroll += delta;
    sMouseInput.mMouseDeltaScroll =
      sMouseInput.mCurr.mMouseScroll -
      sMouseInput.mPrev.mMouseScroll;
  }

  int       GetMouseDeltaScroll()
  {
    return sMouseInput.mMouseDeltaScroll;
  }

  v2        GetScreenspaceCursorPos()
  {
    return sMouseInput.mCurr.mScreenspaceCursorPos;
  }

  void      SetScreenspaceCursorPos(v2 screenspaceCursorPos)
  {
      sMouseInput.mCurr.mScreenspaceCursorPos = screenspaceCursorPos;
      sMouseInput.mMouseDeltaPos =
        sMouseInput.mCurr.mScreenspaceCursorPos -
        sMouseInput.mPrev.mScreenspaceCursorPos;
  }

  void MouseBeginFrame()
  {
    sMouseInput.BeginFrame();
  }

  void MouseEndFrame()
  {
    sMouseInput.EndFrame();

  }
   
  bool      ButtonJustBeenReleased( Button button )
  {
    return sMouseInput.ButtonJustBeenReleased( button );
  }

  bool      ButtonJustDown( Button button )
  {
    return sMouseInput.ButtonJustDown( button);
  }

  bool      ButtonIsDown( Button button )
  {
    return sMouseInput.ButtonIsDown( button);
  }

  bool      ButtonWasDown( Button button )
  {
    return sMouseInput.ButtonWasDown( button);
  }

  void      ButtonSetIsDown( Button button, bool isDown )
  {
    sMouseInput.ButtonSetIsDown( button, isDown );
  }

} // namespace Tac::Mouse

