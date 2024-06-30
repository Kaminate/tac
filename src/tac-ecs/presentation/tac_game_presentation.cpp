#include "tac_game_presentation.h" // self-inc


#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/presentation/tac_shadow_presentation.h"
#include "tac-ecs/presentation/tac_mesh_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/presentation/tac_terrain_presentation.h"
#include "tac-ecs/presentation/tac_voxel_gi_presentation.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_terrain.h"


#if TAC_GAME_PRESENTATION_ENABLED()

namespace Tac
{

  struct MeshModelVtx
  {
    v3 mPos;
    v3 mNor;
  };



  static bool                          mRenderEnabledDebug3D{ true };
  static Render::CBufferLights         mDebugCBufferLights{};
  static bool                          sInitialized;
  static Render::VertexDeclarations    m3DVertexFormatDecls;

  static void Create3DVertexFormat()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( MeshModelVtx, mNor ) },
    };

    m3DVertexFormatDecls.clear();
    m3DVertexFormatDecls.push_back( posDecl );
    m3DVertexFormatDecls.push_back( norDecl );
  }


  //static void Debug3DEachTri( Graphics* graphics )
  //{
  //  struct : public ModelVisitor
  //  {
  //    void operator()( Model* model ) override
  //    {
  //      const Mesh* mesh{ MeshPresentationGetModelMesh( model ) };
  //      if( !mesh )
  //        return;
  //      for( const SubMesh& subMesh : mesh->mSubMeshes )
  //      {
  //        for( const SubMeshTriangle& tri : subMesh.mTris )
  //        {
  //          const v3 p0{ ( model->mEntity->mWorldTransform * v4( tri[ 0 ], 1 ) ).xyz() };
  //          const v3 p1{ ( model->mEntity->mWorldTransform * v4( tri[ 1 ], 1 ) ).xyz() };
  //          const v3 p2{ ( model->mEntity->mWorldTransform * v4( tri[ 2 ], 1 ) ).xyz() };
  //          mDrawData->DebugDraw3DTriangle( p0, p1, p2 );
  //        }
  //      }
  //    }
  //    Debug3DDrawData* mDrawData{ nullptr };
  //  } visitor{};
  //  visitor.mDrawData = graphics->mWorld->mDebug3DDrawData;
  //  graphics->VisitModels( &visitor );
  //}

  
  //static void DebugImgui3DTris( Graphics* graphics )
  //{
  //  if( !mRenderEnabledDebug3D )
  //    return;
  //  static bool debugEachTri;
  //  ImGuiCheckbox( "debug each tri", &debugEachTri );
  //  if( debugEachTri )
  //    Debug3DEachTri( graphics );
  //}

}


void        Tac::GamePresentationInit( Errors& errors )
{
  if( sInitialized )
    return;

  TAC_CALL( MeshPresentationInit( errors ) );
  TAC_CALL( SkyboxPresentationInit( errors ) );
  TAC_CALL( ShadowPresentationInit( errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    TAC_CALL( VoxelGIPresentationInit( errors ) );
#endif
  sInitialized = true;
}

void        Tac::GamePresentationUninit()
{
  if( sInitialized )
  {
    Create3DVertexFormat();
    MeshPresentationUninit();
    SkyboxPresentationUninit();
    ShadowPresentationUninit();
#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationUninit();
#endif
    sInitialized = false;
  }

}


void        Tac::GamePresentationRender( Render::IContext* renderContext,
                                         const World* world,
                                         const Camera* camera,
                                         const v2i viewSize,
                                         const Render::TextureHandle dstColorTex,
                                         const Render::TextureHandle dstDepthTex,
                                         Debug3DDrawBuffers* debug3DDrawBuffers,
                                         Errors& errors )
{
  const Render::Targets renderTargets
  {
    .mColors { dstColorTex },
    .mDepth  { dstDepthTex },
  };

  renderContext->DebugEventBegin( "GamePresentationRender" );
  renderContext->SetViewport( viewSize );
  renderContext->SetScissor( viewSize );
  renderContext->SetRenderTargets( renderTargets );

#if 0
  TAC_CALL( ShadowPresentationRender( world, errors ) );
#endif

#if TAC_MESH_PRESENTATION_ENABLED()
  TAC_CALL( MeshPresentationRender( renderContext,
                                    world,
                                    camera,
                                    viewSize,
                                    dstColorTex,
                                    dstDepthTex,
                                    errors ) );
#endif

#if 0
#if TAC_TERRAIN_PRESENTATION_ENABLED()
  TAC_CALL( TerrainPresentationRender( renderContext,
                                       world,
                                       camera,
                                       viewSize,
                                       dstColorTex,
                                       errors ) );
#endif
#endif


#if 0
#if TAC_SKYBOX_PRESENTATION_ENABLED()
  // Skybox should be last to reduce pixel shader invocations
  TAC_CALL( SkyboxPresentationRender( renderContext,
                                      world,
                                      camera,
                                      viewSize,
                                      dstColorTex,
                                      errors ) );
#endif
#endif

  if( mRenderEnabledDebug3D )
  {
    TAC_CALL( const Debug3DDrawBuffers::Buffer * debug3DDrawBuffer{
      debug3DDrawBuffers->Update( renderContext,
                                  world->mDebug3DDrawData->GetVerts(),
                                  errors ) } );
    TAC_CALL( debug3DDrawBuffer->DebugDraw3DToTexture( renderContext,
                                                       dstColorTex,
                                                       dstDepthTex,
                                                       camera,
                                                       viewSize,
                                                       errors ) );
  }

  renderContext->DebugEventEnd();
}

void Tac::GamePresentationDebugImGui( Graphics* graphics )
{
  if( !ImGuiCollapsingHeader( "Game Presentation" ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;
  ImGuiCheckbox( "Game Presentation Enabled Debug3D", &mRenderEnabledDebug3D );

  MeshPresentationDebugImGui();
  ShadowPresentationDebugImGui();
  SkyboxPresentationDebugImGui();
  TerrainPresentationDebugImGui();

  // this logic is in tac level_editor_game_window.h
  //DebugImgui3DTris( graphics );
}

#endif
