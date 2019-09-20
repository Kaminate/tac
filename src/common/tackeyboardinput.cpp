#include "common/tackeyboardinput.h"
#include "common/tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "common/tacUtility.h"
#include "common/tacOS.h"

TacString ToString( TacKey key )
{
  switch( key )
  {
  case TacKey::UpArrow: return "uparrow";
  case TacKey::DownArrow: return "downarrow";
  case TacKey::LeftArrow: return "leftarrow";
  case TacKey::RightArrow: return "rightarrow";
  case TacKey::Spacebar: return "spacebar";
  case TacKey::Debug: return "debug";
  case TacKey::Backspace: return "backspace";
  case TacKey::Delete: return "delete";
  case TacKey::Backtick: return "backtick";
  case TacKey::MouseLeft: return "lclick";
  case TacKey::MouseRight: return "rclick";
  case TacKey::MouseMiddle: return "mclick";
  case TacKey::Modifier: return "mod";
  case TacKey::A: return "A";
  case TacKey::B: return "B";
  case TacKey::C: return "C";
  case TacKey::D: return "D";
  case TacKey::E: return "E";
  case TacKey::F: return "F";
  case TacKey::G: return "G";
  case TacKey::H: return "H";
  case TacKey::I: return "I";
  case TacKey::J: return "J";
  case TacKey::K: return "K";
  case TacKey::L: return "L";
  case TacKey::M: return "M";
  case TacKey::N: return "N";
  case TacKey::O: return "O";
  case TacKey::P: return "P";
  case TacKey::Q: return "Q";
  case TacKey::R: return "R";
  case TacKey::S: return "S";
  case TacKey::T: return "T";
  case TacKey::U: return "U";
  case TacKey::V: return "V";
  case TacKey::W: return "W";
  case TacKey::X: return "X";
  case TacKey::Y: return "Y";
  case TacKey::Z: return "Z";
  case TacKey::F5: return "f5";
    TacInvalidDefaultCase( key );
  }
  return "";
}

bool TacKeyboardInputFrame::IsKeyDown( TacKey key )
{
  return mCurrDown.find( key ) != mCurrDown.end();
}
TacString TacKeyboardInputFrame::GetPressedKeyDescriptions()
{
  TacVector< TacString > keyNames;
  for( auto key : mCurrDown )
  {
    auto keyName = ToString( key );
    keyNames.push_back( keyName );
  }
  TacString keysDownText = "Keys Down: " + ( !keyNames.empty() ? TacSeparateStrings( keyNames, ", " ) : "none" );
  return keysDownText;
}


bool TacKeyboardInput::IsKeyJustDown( TacKey key )
{
  return !mPrev.IsKeyDown( key ) && IsKeyDown( key );
}
bool TacKeyboardInput::HasKeyJustBeenReleased( TacKey key )
{
  return mPrev.IsKeyDown( key ) && !IsKeyDown( key );
}
bool TacKeyboardInput::IsKeyDown( TacKey key )
{
  return mCurr.IsKeyDown( key );
}
void TacKeyboardInput::DebugImgui()
{
  //ImGui::Text( mCurr.GetPressedKeyDescriptions() );
}
void TacKeyboardInput::SetIsKeyDown( TacKey key, bool isDown )
{
  if( isDown )
  {
    mCurr.mCurrDown.insert( key );
  }
  else
  {
    mCurr.mCurrDown.erase( key );
  }
}

TacKeyboardInput* TacKeyboardInput::Instance;
TacKeyboardInput::TacKeyboardInput()
{
  Instance = this;
  TacErrors ignored;
  TacOS::Instance->GetScreenspaceCursorPos( mCurr.mScreenspaceCursorPos, ignored );
  TacOS::Instance->GetScreenspaceCursorPos( mPrev.mScreenspaceCursorPos, ignored );
}
void TacKeyboardInput::BeginFrame()
{
  TacErrors ignored;
  TacOS::Instance->GetScreenspaceCursorPos( mCurr.mScreenspaceCursorPos, ignored );
  mMouseDeltaPosScreenspace =
    mCurr.mScreenspaceCursorPos -
    mPrev.mScreenspaceCursorPos;
  mMouseDeltaScroll = mCurr.mMouseScroll - mPrev.mMouseScroll;
}
void TacKeyboardInput::EndFrame()
{
  mPrev = mCurr;
  mWMCharPressedHax = 0;
}
void TacKeyboardInput::DebugPrintWhenKeysChange()
{
  TacString currkeysDown = mCurr.GetPressedKeyDescriptions();
  TacString lastkeysDown = mPrev.GetPressedKeyDescriptions();
  if( currkeysDown == lastkeysDown )
    return;
  std::cout << currkeysDown << std::endl;
}

