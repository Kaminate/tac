#include "common/taccontrollerinput.h"
#include "common/tacPreprocessor.h"

TacControllerBitfield ToBitfield( TacControllerButton controllerButton )
{
  return 1 << ( TacControllerBitfield )controllerButton;
}

const char* TacToString( TacControllerButton controllerButton )
{
  switch( controllerButton )
  {
  case TacControllerButton::DPadUp: return "DPadUp";
  case TacControllerButton::DPadLeft: return "DPadLeft";
  case TacControllerButton::DPadDown: return "DPadDown";
  case TacControllerButton::DPadRight: return "DPadRight";
  case TacControllerButton::Start: return "Start";
  case TacControllerButton::Back: return "Back";
  case TacControllerButton::LeftThumb: return "LeftThumb";
  case TacControllerButton::RightThumb: return "RightThumb";
  case TacControllerButton::LeftShoulder: return "LeftShoulder";
  case TacControllerButton::RightShoulder: return "RightShoulder";
  case TacControllerButton::A: return "A";
  case TacControllerButton::B: return "B";
  case TacControllerButton::X: return "X";
  case TacControllerButton::Y: return "Y";
    TacInvalidDefaultCase( controllerButton );
  }
  return nullptr;
}

bool TacControllerState::IsButtonDown( TacControllerButton controllerButton )
{
  return mButtons & ToBitfield( controllerButton );
}
void TacControllerState::DebugImgui()
{
  //ImGui::DragFloat2( "Left stick", mLeftStick.data() );
  //ImGui::DragFloat2( "Right stick", mRightStick.data() );
  //ImGui::DragFloat( "Left Trigger", &mLeftTrigger );
  //ImGui::DragFloat( "Right Trigger", &mRightTrigger );
  //for( int iButton = 0; iButton < ( int )TacControllerButton::Count; ++iButton )
  //{
  //  auto controllerButton = ( TacControllerButton )iButton;
  //  bool down = IsButtonDown( controllerButton );
  //  ImGui::Checkbox( TacToString( controllerButton ), &down );
  //}
}

TacController::TacController()
{
  mDebugging = true;
}
TacController::~TacController()
{
  TacControllerIndex iController = FindControllerIndex();
  mInput->mControllers[ iController ] = nullptr;
}
TacControllerIndex TacController::FindControllerIndex()
{
  for( TacControllerIndex iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController )
    if( mInput->mControllers[ iController ] == this )
      return iController;
  TacInvalidCodePath;
  return TAC_CONTROLLER_COUNT_MAX;
}
bool TacController::IsButtonDown( TacControllerButton controllerButton )
{
  return mControllerStateCurr.IsButtonDown( controllerButton );
}
bool TacController::IsButtonChanged( TacControllerButton controllerButton )
{
  return mControllerStateCurr.IsButtonDown( controllerButton ) !=
    mControllerStatePrev.IsButtonDown( controllerButton );
}
bool TacController::IsButtonJustPressed( TacControllerButton controllerButton )
{
  return mControllerStateCurr.IsButtonDown( controllerButton ) &&
    IsButtonChanged( controllerButton );
}
bool TacController::IsButtonJustReleased( TacControllerButton controllerButton )
{
  return !mControllerStateCurr.IsButtonDown( controllerButton ) &&
    IsButtonChanged( controllerButton );
}
void TacController::DebugImgui()
{
  //ImGui::Text( mName );
  //mControllerStateCurr.DebugImgui();
  //DebugImguiInner();
}
void TacController::DebugImguiInner()
{
}

TacControllerInput::TacControllerInput()
{
  mDebugging = true;


}
TacControllerInput::~TacControllerInput()
{

  for( TacController* controller : mControllers )
  {
    delete controller;
  }

}
void TacControllerInput::DebugImgui()
{
  //ImGui::Checkbox( "Input", &mDebugging );
  //if( !mDebugging )
  //  return;

  //ImGui::Begin( "Input", &mDebugging );
  //OnDestruct( ImGui::End() );

  //ImGui::Checkbox( "Use index override", &mForceIndexOverride );
  //if( mForceIndexOverride )
  //  ImGui::SliderInt( "index override", &mIndexOverride, 0, 3 );

  //DebugImguiInner();

  //for( int iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController )
  //{
  //  TacString name = "Controller " + TacToString( iController );
  //  TacController* controller = mControllers[ iController ];
  //  if( !controller )
  //  {
  //    name += "disconnected";
  //    ImGui::Text( name );
  //    continue;
  //  }

  //  ImGui::PushID( iController );
  //  OnDestruct( ImGui::PopID() );

  //  ImGui::Checkbox( name, &controller->mDebugging );
  //  if( !controller->mDebugging )
  //    continue;

  //  if( mForceIndexOverride )
  //  {
  //    float intensity = 0.2f;
  //    ImVec4 red( intensity, 0, 0, 1 );
  //    ImVec4 green( 0, intensity, 0, 1 );
  //    ImVec4 color = mIndexOverride == iController ? green : red;
  //    ImGui::PushStyleColor( ImGuiCol_WindowBg, color );
  //  }
  //  OnDestruct( if( mForceIndexOverride ) ImGui::PopStyleColor() );

  //  ImGui::Begin( name.c_str(), &controller->mDebugging );
  //  controller->DebugImgui();
  //  ImGui::End();
  //}
}
void TacControllerInput::Update()
{
  for( TacController* controller : mControllers )
    if( controller )
      controller->mControllerStatePrev = controller->mControllerStateCurr;
  UpdateInner();
}
void TacControllerInput::DebugImguiInner()
{
}
void TacControllerInput::UpdateInner()
{
}
bool TacControllerInput::CanAddController()
{
  auto c = GetConnectedControllerCount();
  bool result = c < TAC_CONTROLLER_COUNT_MAX;
  return result;
}
TacControllerIndex TacControllerInput::GetConnectedControllerCount()
{
  TacControllerIndex connectedControllerCount = 0;
  for( TacController* controller : mControllers )
  {
    if( !controller )
      continue;
    connectedControllerCount++;
  }
  return connectedControllerCount;
}
TacControllerIndex TacControllerInput::AddController( TacController* controller )
{
  TacAssert( CanAddController() );
  for( TacControllerIndex iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController )
  {
    if( mControllers[ iController ] )
      continue;
    controller->mInput = this;
    mControllers[ iController ] = controller;
    return iController;
  }
  TacInvalidCodePath;
  return TAC_CONTROLLER_COUNT_MAX;
}
