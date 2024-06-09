#include "tac_level_editor_widget_renderer.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/graphics/tac_renderer_util.h" // PremultipliedAlpha
#include "tac-std-lib/math/tac_matrix4.h"


namespace Tac
{

  static Render::ProgramParams GetProgramParams3DTest()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DTest" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  static void AddDrawCall( Render::IContext* renderContext,
                           const Mesh* mesh )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const Render::DrawArgs drawArgs
      {
        .mIndexCount    { subMesh.mIndexCount },
      };

      renderContext->SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      renderContext->SetShader( CreationGameWindow::Instance->m3DShader );
      renderContext->SetBlendState( CreationGameWindow::Instance->mBlendState );
      renderContext->SetDepthState( CreationGameWindow::Instance->mDepthState );
      renderContext->SetRasterizerState( CreationGameWindow::Instance->mRasterizerState );
      renderContext->SetVertexFormat( CreationGameWindow::Instance->m3DVertexFormat );
      renderContext->SetSamplerState( { CreationGameWindow::Instance->mSamplerState }  );

      renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
      renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
      renderContext->Draw( drawArgs );
    }
  }


  // -----------------------------------------------------------------------------------------------

  void WidgetRenderer::Init( Errors& errors )
  {
    Render::IDevice* renderDevice { Render::RenderApi::GetRenderDevice() };

    const Render::ProgramParams programParams3DTest{ GetProgramParams3DTest() };
    TAC_CALL( m3DShader = renderDevice->CreateProgram( programParams3DTest, errors ) );
  }

  void WidgetRenderer::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DShader);
    renderDevice->DestroyPipeline( m3DPipeline);
  }

  void WidgetRenderer::RenderEditorWidgetsSelection( Render::IContext* renderContext,
                                                     const WindowHandle viewHandle,
                                                     Errors& errors )
  {
    if( !sGizmosEnabled || gCreation.mSelectedEntities.empty() )
      return;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Editor Selection" );

    const float w { ( float )windowSize.x };
    const float h { ( float )windowSize.y };
    const Render::DefaultCBufferPerFrame perFrameData { GetPerFrame( w, h ) };
    const Render::ConstantBufferHandle hPerFrame { Render::DefaultCBufferPerFrame::Handle };
    const int perFrameSize{ sizeof( Render::DefaultCBufferPerFrame ) };
    Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );

    const v3 selectionGizmoOrigin{ gCreation.mSelectedEntities.GetGizmoOrigin() };
    const m4 rots[]{
      m4::RotRadZ( -3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( 3.14f / 2.0f ), };

    const v3 axises[ 3 ]
    {
      v3( 1, 0, 0 ),
      v3( 0, 1, 0 ),
      v3( 0, 0, 1 ),
    };

    for( int i{}; i < 3; ++i )
    {

      const v3 axis{ axises[ i ] };
      const Render::PremultipliedAlpha axisPremultipliedColor{
        Render::PremultipliedAlpha::From_sRGB( axis ) };


      // Widget Translation Arrow
      {
        const bool picked{
          pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
          pickData.arrowAxis == i };

        const bool usingTranslationArrow{
          gCreation.mSelectedGizmo &&
          gCreation.mTranslationGizmoDir == axis };

        const bool shine { picked || usingTranslationArrow };

        Render::PremultipliedAlpha arrowColor { axisPremultipliedColor };
        if( shine )
        {
          float t { float( Sin( Timestep::GetElapsedTime() * 6.0 ) ) };
          t *= t;
          arrowColor.mColor = Lerp( v4( 1, 1, 1, 1 ), axisPremultipliedColor.mColor, t );

        }


        const m4 World
        { m4::Translate( selectionGizmoOrigin )
        * rots[ i ]
        * m4::Scale( v3( 1, 1, 1 ) * mArrowLen ) };

        const Render::DefaultCBufferPerObject perObjectData
        {
          .World { World },
          .Color { arrowColor },
        };

        Render::UpdateConstantBuffer( Render::DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( Render::DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        AddDrawCall( renderContext, mArrow, viewHandle );
      }


      // Widget Scale Cube
      // ( it is not current interactable )
      if( false )
      {
        const m4 World{
          m4::Translate( selectionGizmoOrigin ) *
          m4::Translate( axis * ( mArrowLen * 1.1f ) ) *
          rots[ i ] *
          m4::Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f ) };

        const Render::DefaultCBufferPerObject perObjectData
        {
          .World { World },
          .Color { axisPremultipliedColor },
        };

        Render::UpdateConstantBuffer( Render::DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( Render::DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );

        AddDrawCall( renderContext, mCenteredUnitCube, viewHandle );
      }
    }
  }
} // namespace Tac
