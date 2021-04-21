#include "src/common/tacCamera.h"
#include "src/common/tacMemory.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/space/graphics/tacgraphics.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"
#include "src/space/model/tacmodel.h"

namespace Tac
{
  static Render::TextureHandle voxTexScene;
  static Render::TextureHandle voxTexRadianceBounce0;
  static Render::TextureHandle voxTexRadianceBounce1;
  static int                   voxGridPow = 0;
  //static int                   GetVoxelGridWidth()
  //{
  //  return 1 << voxGridPow;
  //}


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
    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        TAC_UNUSED_PARAMETER( model );


        Errors errors;
        //Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
        //                                             model->mModelIndex,
        //                                             m3DVertexFormatDecls,
        //                                             errors );

        //Model->


      }
    } modelVisitor;

    Graphics* graphics = GetGraphics( world );
    graphics->VisitModels( &modelVisitor );

    TAC_UNUSED_PARAMETER( world );
    TAC_UNUSED_PARAMETER( camera );
    TAC_UNUSED_PARAMETER( viewWidth );
    TAC_UNUSED_PARAMETER( viewHeight );
    TAC_UNUSED_PARAMETER( viewHandle );
  }
}


