#include "tac_game_presentation.h" // self-inc


#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/skybox/tac_skybox_component.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/presentation/mesh/tac_mesh_presentation.h"
#include "tac-ecs/presentation/material/tac_material_presentation.h"
#include "tac-ecs/presentation/radiosity/tac_radiosity_bake_presentation.h"
#include "tac-ecs/presentation/shadow/tac_shadow_presentation.h"
#include "tac-ecs/presentation/skybox/tac_skybox_presentation.h"
#include "tac-ecs/presentation/terrain/tac_terrain_presentation.h"
#include "tac-ecs/presentation/voxel/tac_voxel_gi_presentation.h"
#include "tac-ecs/presentation/infinitegrid/tac_infinite_grid_presentation.h"
#include "tac-ecs/presentation/jpptpresentation/tac_jppt_presentation.h"
#include "tac-ecs/terrain/tac_terrain.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/tac_ints.h"


#if TAC_GAME_PRESENTATION_ENABLED()

namespace Tac
{
  static bool                          mRenderEnabledDebug3D{ true };
  static bool                          sInitialized;
}

void Tac::GamePresentation::Init( Errors& errors )
{
  if( sInitialized )
    return;

#if TAC_MESH_PRESENTATION_ENABLED()
  TAC_CALL( MeshPresentation::Init( errors ) );
#endif

#if TAC_MATERIAL_PRESENTATION_ENABLED()
  TAC_CALL( MaterialPresentation::Init( errors ) );
#endif

#if TAC_JPPT_PRESENTATION_ENABLED()
  TAC_CALL( JPPTPresentation::Init( errors ) );
#endif

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()
  TAC_CALL( InfiniteGrid::Init( errors ) );
#endif

#if TAC_SKYBOX_PRESENTATION_ENABLED()
  TAC_CALL( SkyboxPresentation::Init( errors ) );
#endif

  TAC_CALL( ShadowPresentation::Init( errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    TAC_CALL( VoxelGIPresentationInit( errors ) );
#endif

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
    TAC_CALL( RadiosityBakePresentation::Init( errors ) );
#endif

  sInitialized = true;
}

void Tac::GamePresentation::Uninit()
{
  if( sInitialized )
  {

#if TAC_MESH_PRESENTATION_ENABLED()
    MeshPresentation::Uninit();
#endif

#if TAC_JPPT_PRESENTATION_ENABLED()
    JPPTPresentation::Uninit();
#endif

#if TAC_MATERIAL_PRESENTATION_ENABLED()
    MaterialPresentation::Uninit();
#endif

#if TAC_SKYBOX_PRESENTATION_ENABLED()
    SkyboxPresentation::Uninit();
#endif

#if TAC_SHADOW_PRESENTATION_ENABLED()
    ShadowPresentation::Uninit();
#endif

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()
    InfiniteGrid::Uninit();
#endif

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationUninit();
#endif

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
    RadiosityBakePresentation::Uninit();
#endif

#if TAC_TERRAIN_PRESENTATION_ENABLED()
    TerrainPresentation::Uninit();
#endif

    sInitialized = false;
  }

}


void Tac::GamePresentation::Render( RenderParams renderParams, Errors& errors )
{
  Render::IContext* renderContext { renderParams.mContext };
  const World* world { renderParams.mWorld };
  const Camera* camera { renderParams.mCamera };
  const v2i viewSize { renderParams.mViewSize };
  const Render::TextureHandle dstColorTex { renderParams.mColor };
  const Render::TextureHandle dstDepthTex { renderParams.mDepth };
  Debug3DDrawBuffers* debug3DDrawBuffers { renderParams.mBuffers };

  const Render::Targets renderTargets
  {
    .mColors { dstColorTex },
    .mDepth  { dstDepthTex },
  };

  renderContext->DebugEventBegin( "GamePresentation::Render" );
  renderContext->SetViewport( viewSize );
  renderContext->SetScissor( viewSize );
  renderContext->SetRenderTargets( renderTargets );

#if 0
  TAC_CALL( ShadowPresentationRender( world, errors ) );
#endif

#if TAC_MESH_PRESENTATION_ENABLED()
  TAC_CALL( MeshPresentation::Render( renderContext,
                                      world,
                                      camera,
                                      viewSize,
                                      dstColorTex,
                                      dstDepthTex,
                                      errors ) );
#endif

#if TAC_MATERIAL_PRESENTATION_ENABLED()
  TAC_CALL( MaterialPresentation::Render( renderContext,
                                          world,
                                          camera,
                                          viewSize,
                                          dstColorTex,
                                          dstDepthTex,
                                          errors ) );
#endif


#if 0
#if TAC_TERRAIN_PRESENTATION_ENABLED()
  TAC_CALL( TerrainPresentation::Render( renderContext,
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

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()
  TAC_CALL( InfiniteGrid::Render( renderContext,
                                  camera,
                                  viewSize,
                                  dstColorTex,
                                  dstDepthTex,
                                  errors ) );
#endif

  if( renderParams.mIsLevelEditorWorld )
  {
#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
    TAC_CALL( RadiosityBakePresentation::Render( renderContext,
                                                 world,
                                                 camera,
                                                 errors ) );
#endif

#if TAC_JPPT_PRESENTATION_ENABLED()
    TAC_CALL( JPPTPresentation::Render( renderContext,
                                        world,
                                        camera,
                                        viewSize,
                                        dstColorTex,
                                        dstDepthTex,
                                        errors ) );
#endif
  }

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

void Tac::GamePresentation::DebugImGui( Graphics* graphics )
{
  TAC_UNUSED_PARAMETER( graphics );
  if( !ImGuiCollapsingHeader( "Game Presentation", ImGuiNodeFlags_DefaultOpen ) )
    return;

  TAC_IMGUI_INDENT_BLOCK;
  ImGuiCheckbox( "Debug 3D", &mRenderEnabledDebug3D );

#if TAC_MESH_PRESENTATION_ENABLED()
  MeshPresentation::DebugImGui();
#endif

#if TAC_MATERIAL_PRESENTATION_ENABLED()
  MaterialPresentation::DebugImGui();
#endif

#if TAC_SHADOW_PRESENTATION_ENABLED()
  ShadowPresentation::DebugImGui();
#endif

#if TAC_SKYBOX_PRESENTATION_ENABLED()
  SkyboxPresentation::DebugImGui();
#endif

#if TAC_INFINITE_GRID_PRESENTATION_ENABLED()
  InfiniteGrid::DebugImGui();
#endif

#if TAC_TERRAIN_PRESENTATION_ENABLED()
  TerrainPresentation::DebugImGui();
#endif

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
  RadiosityBakePresentation::DebugImGui();
#endif

#if TAC_JPPT_PRESENTATION_ENABLED()
  JPPTPresentation::DebugImGui();
#endif
}

#endif
