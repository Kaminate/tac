#include "tac_keyboard_backend.h" // self-inc

#include "tac-desktop-app/desktop_event/tac_desktop_event.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/shell/tac_shell_time.h"

#if TAC_SHOULD_IMPORT_STD()
  import std;
#else
  #include <mutex>
#endif

namespace Tac
{
  AppKeyboardApiBackend AppKeyboardApiBackend::sGameKeyboardBackend;
  AppKeyboardApiBackend AppKeyboardApiBackend::sUIKeyboardBackend;

  // -----------------------------------------------------------------------------------------------

  void AppKeyboardApiBackend::SetKeyState( Key key, KeyState state )
  {
    KeyState& currKeySate{ mCurr.mKeyStates[ ( int )key ] };
    if( currKeySate == state )
      return;

    currKeySate = state;
    mCurr.mToggles[ ( int )key ]++;
    mCurr.mKeyTimes[ ( int )key ] = RealTime::Now();
  }
  void AppKeyboardApiBackend::SetCodepoint( Codepoint codepoint )
  {
    mCurr.mCodepointDelta.push_back( codepoint );
  }
  void AppKeyboardApiBackend::SetScreenspaceMousePos( v2 screenspace )
  {
    mCurr.mMousePosScreenspace = screenspace;

    // prevent frame 1 jump when calling getmouse when calling GetMousePosDelta()
    if( static bool sInitialized; !sInitialized )
    {
      sInitialized = true;
      mPrev.mMousePosScreenspace = screenspace;
    }
  }
  void AppKeyboardApiBackend::AddMouseWheelDelta( float wheelDelta )
  {
    mCurr.mMouseWheel += wheelDelta;
  }
  void AppKeyboardApiBackend::Sync()
  {
    mPrev = mCurr;
    mCurr.mTime = RealTime::Now();
    mCurr.mToggles = {};
    mCurr.mCodepointDelta = {};
  }

  bool AppKeyboardApiBackend::GetIsPressed( Key key )
  {
    return mCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Down;
  }
  bool AppKeyboardApiBackend::GetIsDepressed( Key key )
  {
    if( key == Key::Myself ) { return true; }
    return mCurr.mKeyStates[ ( int )key ] == AppKeyboardApiBackend::KeyState::Up;
  }
  bool AppKeyboardApiBackend::GetIsJustPressed( Key key )
  {
    const int toggleCount { mCurr.mToggles[ ( int )key ] };
    return GetIsPressed( key ) && toggleCount >= 1;
  }
  bool AppKeyboardApiBackend::GetIsJustDepressed( Key key )
  {
    const int toggleCount { mCurr.mToggles[ ( int )key ] };
    return GetIsDepressed( key ) && toggleCount >= 1;
  }
  auto AppKeyboardApiBackend::GetHeldSeconds( Key key ) -> RealTimeDelta
  {
    if( !GetIsPressed( key ) ) { return {}; }
    return mCurr.mTime - mCurr.mKeyTimes[ ( int )key ];
  }
  auto AppKeyboardApiBackend::GetCodepoints() -> CodepointView
  {
    return mCurr.mCodepointDelta;
  }
  auto AppKeyboardApiBackend::GetMouseWheelDelta() -> float
  {
    return mCurr.mMouseWheel - mPrev.mMouseWheel;
  }
  auto AppKeyboardApiBackend::GetMousePosScreenspace() -> v2i
  {
    return mCurr.mMousePosScreenspace;
  }
  auto AppKeyboardApiBackend::GetMousePosDelta() -> v2i
  {
    return mCurr.mMousePosScreenspace - mPrev.mMousePosScreenspace;
  }

  bool AppKeyboardApi::IsPressed( Key key )                    { return AppKeyboardApiBackend::sGameKeyboardBackend.GetIsPressed( key ); }
  bool AppKeyboardApi::IsDepressed( Key key )                  { return AppKeyboardApiBackend::sGameKeyboardBackend.GetIsDepressed( key ); }
  bool AppKeyboardApi::JustPressed( Key key )                  { return AppKeyboardApiBackend::sGameKeyboardBackend.GetIsJustPressed( key ); }
  bool AppKeyboardApi::JustDepressed( Key key )                { return AppKeyboardApiBackend::sGameKeyboardBackend.GetIsJustDepressed( key ); } 
  auto AppKeyboardApi::HeldSeconds( Key key ) -> RealTimeDelta { return AppKeyboardApiBackend::sGameKeyboardBackend.GetHeldSeconds( key ); }
  auto AppKeyboardApi::GetCodepoints() -> CodepointView        { return AppKeyboardApiBackend::sGameKeyboardBackend.GetCodepoints(); }
  auto AppKeyboardApi::GetMouseWheelDelta() -> float           { return AppKeyboardApiBackend::sGameKeyboardBackend.GetMouseWheelDelta(); }
  auto AppKeyboardApi::GetMousePosScreenspace() -> v2i         { return AppKeyboardApiBackend::sGameKeyboardBackend.GetMousePosScreenspace(); }
  auto AppKeyboardApi::GetMousePosDelta() -> v2i               { return AppKeyboardApiBackend::sGameKeyboardBackend.GetMousePosDelta(); }

  bool UIKeyboardApi::IsPressed( Key key )                     { return AppKeyboardApiBackend::sUIKeyboardBackend.GetIsPressed( key ); }
  bool UIKeyboardApi::IsDepressed( Key key )                   { return AppKeyboardApiBackend::sUIKeyboardBackend.GetIsDepressed( key ); }
  bool UIKeyboardApi::JustPressed( Key key )                   { return AppKeyboardApiBackend::sUIKeyboardBackend.GetIsJustPressed( key ); }
  bool UIKeyboardApi::JustDepressed( Key key )                 { return AppKeyboardApiBackend::sUIKeyboardBackend.GetIsJustDepressed( key ); } 
  auto UIKeyboardApi::HeldSeconds( Key key ) -> RealTimeDelta  { return AppKeyboardApiBackend::sUIKeyboardBackend.GetHeldSeconds( key ); }
  auto UIKeyboardApi::GetCodepoints() -> CodepointView         { return AppKeyboardApiBackend::sUIKeyboardBackend.GetCodepoints(); }
  auto UIKeyboardApi::GetMouseWheelDelta() -> float            { return AppKeyboardApiBackend::sUIKeyboardBackend.GetMouseWheelDelta(); }
  auto UIKeyboardApi::GetMousePosScreenspace() -> v2i          { return AppKeyboardApiBackend::sUIKeyboardBackend.GetMousePosScreenspace(); }
  auto UIKeyboardApi::GetMousePosDelta() -> v2i                { return AppKeyboardApiBackend::sUIKeyboardBackend.GetMousePosDelta(); }

  bool UIKeyboardApi::sWantCaptureKeyboard;
  bool UIKeyboardApi::sWantCaptureMouse;

  // -----------------------------------------------------------------------------------------------

} // namespace Tac
