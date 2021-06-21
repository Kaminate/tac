#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/common/tacPreprocessor.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"

namespace Tac
{
  void GraphicsDebugImgui( System* system )
  {
    TAC_UNUSED_PARAMETER( system );

    //auto graphics = ( Graphics* )system;
    if( ImGuiCollapsingHeader( "Game Presentation" ) )
    {
      bool& enabled = GamePresentationGetEnabled();
      ImGuiCheckbox( "Game Presentation Enabled", &enabled );
    }

    if( ImGuiCollapsingHeader( "Voxel GI Presentation" ) )
    {
      bool& enabled = VoxelGIPresentationGetEnabled();
      ImGuiCheckbox( "Enabled", &enabled );

      bool& debugEnabled = VoxelGIPresentationGetDebugEnabled();
      ImGuiCheckbox( "Debug Enabled", &debugEnabled );
    }
  }
}

