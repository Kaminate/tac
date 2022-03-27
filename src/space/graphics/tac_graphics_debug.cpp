#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/common/tacPreprocessor.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/model/tacmodel.h"
#include "src/space/taccomponent.h"
#include "src/space/tacsystem.h"
#include "src/space/tacentity.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/tacworld.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"

namespace Tac
{


  void GraphicsDebugImgui( System* system )
  {
    auto graphics = ( Graphics* )system;

    GamePresentationDebugImGui(graphics);

    VoxelGIDebugImgui();

  }
}

