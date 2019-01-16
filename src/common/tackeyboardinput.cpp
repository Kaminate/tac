#include "tackeyboardinput.h"
#include "tacPreprocessor.h"
#include "common/containers/tacVector.h"
#include "imgui.h" 
#include "tacUtility.h"

TacString ToString( TacKey key )
{
  switch( key )
  {
  case TacKey::Up: return "up";
  case TacKey::Down: return "down";
  case TacKey::Left: return "left";
  case TacKey::Right: return "right";
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


bool TacKeyboardInput::IsKeyDown( TacKey key )
{
  return mCurrDown.find( key ) != mCurrDown.end();
}
bool TacKeyboardInput::IsKeyUp( TacKey key )
{
  return !IsKeyDown( key );
}
bool TacKeyboardInput::WasKeyDown( TacKey key )
{
  return mPrevDown.find( key ) != mPrevDown.end();
}
bool TacKeyboardInput::WasKeyUp( TacKey key )
{
  return !WasKeyDown( key );
}
bool TacKeyboardInput::IsKeyJustDown( TacKey key )
{
  return IsKeyDown( key ) && !WasKeyDown( key );
}
bool TacKeyboardInput::IsKeyJustUp( TacKey key )
{
  return IsKeyUp( key ) && !WasKeyUp( key );
}

TacString TacKeyboardInput::GetKeysDownText()
{
  TacVector< TacString > keyNames;
  for( auto key : mCurrDown )
  {
    auto keyName = ToString( key );
    keyNames.push_back( keyName );
  }
  TacString keysDownText = "Keys Down: " + (!keyNames.empty() ? TacSeparateStrings( keyNames, ", " ) : "none" );
  return keysDownText;
}
void TacKeyboardInput::DebugImgui()
{
  TacString keysDownText = GetKeysDownText();
  ImGui::Text( keysDownText.c_str() );
}


