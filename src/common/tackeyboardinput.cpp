#include "tackeyboardinput.h"
#include "tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "imgui.h" 
#include "tacUtility.h"

TacString ToString( TacKey key )
{
  switch( key )
  {
  case TacKey::UpArrow: return "uparrow";
  case TacKey::DownArrow: return "downarrow";
  case TacKey::LeftArrow: return "leftarrow";
  case TacKey::RightArrow: return "rightarrow";
  case TacKey::Space: return "space";
  case TacKey::Debug: return "debug";
  case TacKey::Back: return "back";
  case TacKey::Backtick: return "backtick";
  case TacKey::MouseLeft: return "lclick";
  case TacKey::MouseRight: return "rclick";
  case TacKey::MouseMiddle: return "mclick";
  case TacKey::Modifier: return "mod";
  case TacKey::Q: return "q";
  case TacKey::E: return "e";
  case TacKey::F: return "f";
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
  ImGui::Text( mCurr.GetPressedKeyDescriptions() );
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
void TacKeyboardInput::BeforePoll()
{
  mPrev = mCurr;
}
void TacKeyboardInput::DebugPrintWhenKeysChange()
{
  TacString currkeysDown = mCurr.GetPressedKeyDescriptions();
  TacString lastkeysDown = mPrev.GetPressedKeyDescriptions();
  if( currkeysDown == lastkeysDown )
    return;
  std::cout << currkeysDown << std::endl;
}

