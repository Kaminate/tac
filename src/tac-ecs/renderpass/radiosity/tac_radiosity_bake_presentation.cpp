#include "tac_radiosity_bake_presentation.h" // self-inc

//#include "tac-ecs/entity/tac_entity.h"
//#include "tac-ecs/graphics/material/tac_material.h"
//#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/renderpass/radiosity/tac_radiosity_baker.h"
//#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
//#include "tac-std-lib/os/tac_os.h"
#include "tac-ecs/world/tac_world.h"

#if TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()

//#if TAC_SHOULD_IMPORT_STD()
//  import std;
//#else
//  #include <limits>
//#endif

namespace Tac
{
  static bool          sInitialized;
  static PreBakeScene* sPreBakeScene;
  static bool          sRequestBake;

  // -----------------------------------------------------------------------------------------------

  void RadiosityBakePresentation::Init( Errors& )
  {
    if( sInitialized )
      return;

    sInitialized = true;

#if 0
    if( sPipeline.IsValid() )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams
    {
      .mInputs{ "jppt" },
    };

    TAC_CALL( sProgram = renderDevice->CreateProgram( programParams, errors ) );
    const Render::PipelineParams pipelineParams
    {
      .mProgram{ sProgram },
    };
    TAC_CALL( sPipeline = renderDevice->CreatePipeline( pipelineParams, errors ) );

    Render::IShaderVar* outputTexture {
      renderDevice->GetShaderVariable( sPipeline, "sOutputTexture" ) };
    outputTexture->SetResource( sTexture );
#endif
  }

  void RadiosityBakePresentation::Render( Render::IContext*,
                                          const World* world,
                                          const Camera*,
                                          Errors& )
  {
    if( sRequestBake )
    {
      sRequestBake = false;
      TAC_DELETE sPreBakeScene;
      sPreBakeScene = TAC_NEW PreBakeScene;
      sPreBakeScene->Init( world );
      Job::JobQueuePush( sPreBakeScene );
    }

    if( sPreBakeScene )
    {
      for( const auto& inst : sPreBakeScene->mInstances )
      {
        for( const auto& patch : inst.mPatchPowers )
        {
          const v3 p0{ patch.mTriVerts[ 0 ] };
          const v3 p1{ patch.mTriVerts[ 1 ] };
          const v3 p2{ patch.mTriVerts[ 2 ] };
          const v3 color{ patch.mTotalPower / patch.mArea / 3.14f };

          world->mDebug3DDrawData->DebugDraw3DTriangle( p0, p1, p2, color );
        }
      }

      if( sPreBakeScene->mDebugLine )
      {
        const v3 srcPos{ sPreBakeScene->mDebugSrcPos };
        const v3 dstPos{ sPreBakeScene->mDebugDstPos };
        const auto srcPatch{ sPreBakeScene->mDebugSrcPatch };
        const auto dstPatch{ sPreBakeScene->mDebugDstPatch };
        world->mDebug3DDrawData->DebugDraw3DArrow( srcPos, dstPos );
        world->mDebug3DDrawData->DebugDraw3DLine( srcPos, dstPos );
        world->mDebug3DDrawData->DebugDraw3DTriangle( srcPatch->mTriVerts[ 0 ],
                                                      srcPatch->mTriVerts[ 1 ],
                                                      srcPatch->mTriVerts[ 2 ],
                                                      v3( 0, 1, 0 ) );
        world->mDebug3DDrawData->DebugDraw3DTriangle( dstPatch->mTriVerts[ 0 ],
                                                      dstPatch->mTriVerts[ 1 ],
                                                      dstPatch->mTriVerts[ 2 ],
                                                      v3( 1, 0, 0 ) );
      }
    }
  }

  void RadiosityBakePresentation::Uninit()
  {
    if( sInitialized )
    {
      sInitialized = false;
    }
  }

  void RadiosityBakePresentation::DebugImGui()
  {
#if 0
    if( !ImGuiCollapsingHeader( "Radiosity Bake" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;
#endif

    if( !sPreBakeScene || sPreBakeScene->GetStatus() == JobState::ThreadFinished )
    {
      sRequestBake |= ImGuiButton( "Bake Radiosity" );
    }

    if( sPreBakeScene )
    {
      if( sPreBakeScene->GetStatus() == JobState::ThreadRunning )
      {
        ;

        ImGuiText( "Baking" + String( "...", ( int )Timestep::GetElapsedTime() % 4 )
                   + " ("
                   + FormatFrameTime( sPreBakeScene->mElapsed.mSeconds ) 
                   + ")" );
      }

      if( sPreBakeScene->GetStatus() == JobState::ThreadFinished )
      {
        static Errors saveErrors;
        static bool saved;
        if( ImGuiButton( "Save to file" ))
        {
          saveErrors = {};
          sPreBakeScene->SaveToFile( saveErrors );
          if( !saveErrors )
            saved = true;
        }

        if( saved )
        {
          ImGuiSameLine();
          ImGuiText( "Saved!" );
        }


        if( saveErrors )
          ImGuiText( saveErrors.ToString() );
      }
    }



  }
}

#endif // TAC_RADIOSITY_BAKE_PRESENTATION_ENABLED()
