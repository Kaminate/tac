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

  static bool debugEachTri;
  static void Debug3DEachTri( Graphics* graphics )
  {
    if( !GamePresentationGetRenderEnabledDebug3D() )
      return;
    if( !debugEachTri )
      return;

    struct : public ModelVisitor
    {
      void operator()( const Model* model ) override
      {
        Errors errors;
        Mesh* mesh = ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                             model->mModelIndex,
                                                             GamePresentationGetVertexDeclarations(),
                                                             errors );
        if( !mesh )
          return;

        for( const SubMesh& subMesh : mesh->mSubMeshes )
        {
          for( const SubMeshTriangle& tri : subMesh.mTris )
          {
            const v3 p0 = ( model->mEntity->mWorldTransform * mesh->mTransform * v4( tri[ 0 ], 1 ) ).xyz();
            const v3 p1 = ( model->mEntity->mWorldTransform * mesh->mTransform * v4( tri[ 1 ], 1 ) ).xyz();
            const v3 p2 = ( model->mEntity->mWorldTransform * mesh->mTransform * v4( tri[ 2 ], 1 ) ).xyz();
            mDrawData->DebugDraw3DTriangle( p0, p1, p2 );
          }
        }
      }
      Debug3DDrawData* mDrawData;
    } visitor = {};
    visitor.mDrawData = graphics->mWorld->mDebug3DDrawData;

    graphics->VisitModels( &visitor );
  }

  void GraphicsDebugImgui( System* system )
  {
    auto graphics = ( Graphics* )system;

    //auto graphics = ( Graphics* )system;
    if( ImGuiCollapsingHeader( "Game Presentation" ) )
    {
      ImGuiCheckbox( "Game Presentation Enabled Model", &GamePresentationGetRenderEnabledModel() );
      ImGuiCheckbox( "Game Presentation Enabled Skybox", &GamePresentationGetRenderEnabledSkybox() );
      ImGuiCheckbox( "Game Presentation Enabled Terrain", &GamePresentationGetRenderEnabledTerrain() );
      ImGuiCheckbox( "Game Presentation Enabled Debug3D", &GamePresentationGetRenderEnabledDebug3D() );
      if( GamePresentationGetRenderEnabledDebug3D() )
        ImGuiCheckbox( "debug each tri", &debugEachTri );
    }

    if( ImGuiCollapsingHeader( "Voxel GI Presentation" ) )
    {
      bool& enabled = VoxelGIPresentationGetEnabled();
      ImGuiCheckbox( "Enabled", &enabled );

      bool& debugEnabled = VoxelGIPresentationGetDebugEnabled();
      ImGuiCheckbox( "Debug Enabled", &debugEnabled );

      if( debugEnabled )
      {
        VoxelDebugImgui();
      }
    }

    Debug3DEachTri( graphics );
  }
}

