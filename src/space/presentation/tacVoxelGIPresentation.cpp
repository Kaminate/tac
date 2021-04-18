#include "src/common/tacCamera.h"
#include "src/common/tacMemory.h"
#include "src/space/graphics/tacgraphics.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"

namespace Tac
{
//   struct WorldVoxelGIState
//   {
// 
//   };

  void               VoxelGIPresentationInit( Errors& )
  {

  }

  void               VoxelGIPresentationUninit()
  {

  }

//   WorldVoxelGIState* VoxelGIPresentationCreateState( World* )
//   {
//     auto worldVoxelGIState = TAC_NEW WorldVoxelGIState;
//     return worldVoxelGIState;
//   }


  void               VoxelGIPresentationRender( World* world,
                                                const Camera* camera,
                                                const int viewWidth,
                                                const int viewHeight,
                                                const Render::ViewHandle viewHandle )
  {
    Graphics* graphics = GetGraphics( world );
    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        TAC_UNUSED_PARAMETER( model );
      }
    } modelVisitor;
    graphics->VisitModels( &modelVisitor );

    TAC_UNUSED_PARAMETER( world );
    TAC_UNUSED_PARAMETER( camera );
    TAC_UNUSED_PARAMETER( viewWidth );
    TAC_UNUSED_PARAMETER( viewHeight );
    TAC_UNUSED_PARAMETER( viewHandle );
  }
}


