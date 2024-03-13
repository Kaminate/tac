#include "tac_controller_input.h" // self-inc

#include "tac-engine-core/input/tac_controller_internal.h"
//#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/error/tac_assert.h"

namespace Tac::Controller
{
  void UpdateJoysticks()
  {
    ControllerInput* input = ControllerInput::Instance;
    if( input )
      input->Update();
  }

  const char* ToString( ControllerButton controllerButton )
  {
    switch( controllerButton )
    {
      case ControllerButton::DPadUp:        return "DPadUp";
      case ControllerButton::DPadLeft:      return "DPadLeft";
      case ControllerButton::DPadDown:      return "DPadDown";
      case ControllerButton::DPadRight:     return "DPadRight";
      case ControllerButton::Start:         return "Start";
      case ControllerButton::Back:          return "Back";
      case ControllerButton::LeftThumb:     return "LeftThumb";
      case ControllerButton::RightThumb:    return "RightThumb";
      case ControllerButton::LeftShoulder:  return "LeftShoulder";
      case ControllerButton::RightShoulder: return "RightShoulder";
      case ControllerButton::A:             return "A";
      case ControllerButton::B:             return "B";
      case ControllerButton::X:             return "X";
      case ControllerButton::Y:             return "Y";
      default: TAC_ASSERT_INVALID_CASE( controllerButton ); return nullptr;
    }
  }

  Controller* GetController( ControllerIndex ci )
  {
    ControllerInput* input = ControllerInput::Instance;
    return input->GetController( ci );
  }

  bool        IsButtonJustPressed( ControllerButton button, Controller* controller )
  {
    return controller->IsButtonJustPressed( button );
  }
} // namespace Tac::Controller

