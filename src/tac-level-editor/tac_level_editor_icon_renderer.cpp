#include "tac_level_editor_icon_renderer.h" // self-inc

#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-std-lib/math/tac_matrix4.h"


namespace Tac
{
  static Render::ProgramParams GetProgramParams3DSprite()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DSprite" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }
  // -----------------------------------------------------------------------------------------------
  void IconRenderer::Init( float lightWidgetSize, Errors& errors )
  {
    mLightWidgetSize = lightWidgetSize;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::ProgramParams programParams3DSprite{ GetProgramParams3DSprite() };
    TAC_CALL( mSpriteShader = renderDevice->CreateProgram( programParams3DSprite, errors ) );
  }

  void IconRenderer::Uninit()
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( mSpriteShader);
    renderDevice->DestroyPipeline( mSpritePipeline);
  }

  void IconRenderer::RenderLights( const World* world,
                                   Render::IContext* renderContext,
                                   WindowHandle viewHandle,
                                   Errors& errors )
  {
    TAC_RENDER_GROUP_BLOCK( renderContext, "light widgets" );

    struct : public LightVisitor
    {
      void operator()( Light* light ) override { mLights.push_back( light ); }
      Vector< const Light* > mLights;
    } lightVisitor;


    const float w { ( float )windowSize.x };
    const float h { ( float )windowSize.y };
    const Render::DefaultCBufferPerFrame perFrameData { GetPerFrame( w, h ) };
    const Render::ConstantBufferHandle hPerFrame { Render::DefaultCBufferPerFrame::Handle };
    const int perFrameSize { sizeof( Render::DefaultCBufferPerFrame ) };
    Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );

    const Graphics* graphics { GetGraphics( world ) };
    graphics->VisitLights( &lightVisitor );

    const Render::TextureHandle textureHandle{
      TextureAssetManager::GetTexture( "assets/editor/light.png", errors ) };

    for( int iLight {}; iLight < lightVisitor.mLights.size(); ++iLight )
    {
      const Light* light { lightVisitor.mLights[ iLight ] };

      // Q: why am ii only scaling the m00, and not the m11 and m22?
      m4 world { light->mEntity->mWorldTransform };
      world.m00 = mLightWidgetSize;

      const Render::DefaultCBufferPerObject perObjectData
      {
        .World { world },
      };

      const Render::ConstantBufferHandle hPerObj{ Render::DefaultCBufferPerObject::Handle };
      const int perObjSize{ sizeof( Render::DefaultCBufferPerObject ) };

      const String groupname{ String() + "Editor light " + ToString( iLight ) };
      TAC_RENDER_GROUP_BLOCK( renderContext, groupname );

      const Render::DrawArgs drawArgs
      {
        .mVertexCount { 6 },
      };

      renderContext->SetPipeline( mSpritePipeline );
      renderContext->SetVertexBuffer( {} );
      renderContext->SetIndexBuffer( {}  );
      renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
      renderContext->SetTexture( { textureHandle } );
      renderContext->Render::UpdateConstantBuffer( hPerObj,
                                                   &perObjectData,
                                                   perObjSize,
                                                   TAC_STACK_FRAME );
      renderContext->Draw( drawArgs );
    }
  }
}
