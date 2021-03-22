#include "src/space/presentation/tacVoxelGIPresentation.h"
#include "src/common/tacCamera.h"
#include "src/common/tacMemory.h"

namespace Tac
{
  struct WorldVoxelGIState
  {

  };

  void               VoxelGIPresentationInit( Errors& )
  {

  }

  void               VoxelGIPresentationUninit()
  {

  }

  WorldVoxelGIState* VoxelGIPresentationCreateState( World* )
  {
    auto worldVoxelGIState = TAC_NEW WorldVoxelGIState;
    return worldVoxelGIState;

  }

  void VoxelGIPresentationRender( World* world,
                                  const Camera* camera,
                                  int viewWidth,
                                  int viewHeight,
                                  Render::ViewHandle viewHandle )
  {

    TAC_UNUSED_PARAMETER( world );
    TAC_UNUSED_PARAMETER( camera );
    TAC_UNUSED_PARAMETER( viewWidth );
    TAC_UNUSED_PARAMETER( viewHeight );
    TAC_UNUSED_PARAMETER( viewHandle );
  }
}


