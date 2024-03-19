#include "tac_controller_internal.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Controller
{

  ControllerBitfield ToBitfield( ControllerButton controllerButton )
  {
    return 1 << ( ControllerBitfield )controllerButton;
  }


  bool ControllerState::IsButtonDown( ControllerButton controllerButton )
  {
    return mButtons & ToBitfield( controllerButton );
  }
  void ControllerState::DebugImgui()
  {
    //ImGui::DragFloat2( "Left stick", mLeftStick.data() );
    //ImGui::DragFloat2( "Right stick", mRightStick.data() );
    //ImGui::DragFloat( "Left Trigger", &mLeftTrigger );
    //ImGui::DragFloat( "Right Trigger", &mRightTrigger );
    //for( int iButton = 0; iButton < ( int )ControllerButton::Count; ++iButton )
    //{
    //  auto controllerButton = ( ControllerButton )iButton;
    //  bool down = IsButtonDown( controllerButton );
    //  ImGui::Checkbox( ToString( controllerButton ), &down );
    //}
  }

  Controller::Controller()
  {
    mDebugging = true;
  }
  Controller::~Controller()
  {
    ControllerIndex iController = FindControllerIndex();
    mInput->mControllers[ iController ] = nullptr;
  }
  ControllerIndex Controller::FindControllerIndex()
  {
    for( ControllerIndex iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController )
      if( mInput->mControllers[ iController ] == this )
        return iController;
    TAC_ASSERT_INVALID_CODE_PATH;
    return TAC_CONTROLLER_COUNT_MAX;
  }
  bool Controller::IsButtonDown( ControllerButton controllerButton )
  {
    return mControllerStateCurr.IsButtonDown( controllerButton );
  }
  bool Controller::IsButtonChanged( ControllerButton controllerButton )
  {
    return mControllerStateCurr.IsButtonDown( controllerButton ) !=
      mControllerStatePrev.IsButtonDown( controllerButton );
  }
  bool Controller::IsButtonJustPressed( ControllerButton controllerButton )
  {
    return mControllerStateCurr.IsButtonDown( controllerButton ) &&
      IsButtonChanged( controllerButton );
  }
  bool Controller::IsButtonJustReleased( ControllerButton controllerButton )
  {
    return !mControllerStateCurr.IsButtonDown( controllerButton ) &&
      IsButtonChanged( controllerButton );
  }
  void Controller::DebugImgui()
  {
    //ImGui::Text( mName );
    //mControllerStateCurr.DebugImgui();
    //DebugImguiInner();
  }
  void Controller::DebugImguiInner()
  {
  }

  ControllerInput* ControllerInput::Instance = nullptr;
  ControllerInput::ControllerInput()
  {
    mDebugging = true;
    Instance = this;
  }
  ControllerInput::~ControllerInput()
  {

    for( Controller* controller : mControllers )
    {
      delete controller;
    }

  }
  void ControllerInput::DebugImgui()
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
    //  String name = "Controller " + ToString( iController );
    //  Controller* controller = mControllers[ iController ];
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
  void ControllerInput::Update()
  {
    for( Controller* controller : mControllers )
      if( controller )
        controller->mControllerStatePrev = controller->mControllerStateCurr;
    UpdateInner();
  }
  void ControllerInput::DebugImguiInner()
  {
  }
  void ControllerInput::UpdateInner()
  {
  }
  bool ControllerInput::CanAddController()
  {
    auto c = GetConnectedControllerCount();
    bool result = c < TAC_CONTROLLER_COUNT_MAX;
    return result;
  }
  ControllerIndex ControllerInput::GetConnectedControllerCount()
  {
    ControllerIndex connectedControllerCount = 0;
    for( Controller* controller : mControllers )
    {
      if( !controller )
        continue;
      connectedControllerCount++;
    }
    return connectedControllerCount;
  }
  ControllerIndex ControllerInput::AddController( Controller* controller )
  {
    TAC_ASSERT( CanAddController() );
    for( ControllerIndex iController = 0; iController < TAC_CONTROLLER_COUNT_MAX; ++iController )
    {
      if( mControllers[ iController ] )
        continue;
      controller->mInput = this;
      mControllers[ iController ] = controller;
      return iController;
    }
    TAC_ASSERT_INVALID_CODE_PATH;
    return TAC_CONTROLLER_COUNT_MAX;
  }
  Controller* ControllerInput::GetController( ControllerIndex ci )
  {
    return mControllers[ ci ];
  }

} // namespace Tac::Controller
