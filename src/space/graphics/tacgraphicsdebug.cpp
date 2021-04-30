#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/common/tacPreprocessor.h"

namespace Tac
{
  void GraphicsDebugImgui( System* system )
  {
    TAC_UNUSED_PARAMETER( system );

    //auto graphics = ( Graphics* )system;
    ImGuiText( "graphics stuff" );

  }

}

