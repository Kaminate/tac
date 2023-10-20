#include "src/common/graphics/tac_renderer.h"
#include "src/common/math/tac_vector2.h"
#include "src/common/graphics/tac_camera.h"
#include "src/space/tac_world.h"
#include "src/space/presentation/tac_game_presentation.h"
#include "src/game-examples/tac_examples_presentation.h"

namespace Tac
{
  static Render::ViewHandle        exampleView;
  static int                       exampleWidth;
  static int                       exampleHeight;
  static Render::TextureHandle     exampleColor;
  static Render::TextureHandle     exampleDepth;
  static Render::FramebufferHandle exampleFramebuffer;

  static void   ExamplesEnsureView( const v2& size )
  {
    const auto w = ( int )size.x;
    const auto h = ( int )size.y;
    const bool shouldCreate = w * h > 0 && ( w != exampleWidth || h != exampleHeight );
    if( !shouldCreate )
      return;

    const char* debugName = "examplesview";
    const Render::TexSpec texSpecColor =
    {
      .mImage =
      {
        .mWidth = w,
        .mHeight = h,
        .mFormat =
        {
          .mElementCount = 4,
          .mPerElementByteCount = 1,
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mBinding = Render::Binding::ShaderResource | Render::Binding::RenderTarget,
    };
    const Render::TextureHandle textureHandleColor = Render::CreateTexture( texSpecColor, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( textureHandleColor, debugName );

    const Render::TexSpec texSpecDepth = 
    {
      .mImage =
      {
        .mWidth = w,
        .mHeight = h,
        .mFormat =
        {
          .mElementCount = 1,
          .mPerElementByteCount = sizeof( uint16_t ),
          .mPerElementDataType = Render::GraphicsType::unorm,
        },
      },
      .mBinding = Render::Binding::DepthStencil,
    };
    const Render::TextureHandle textureHandleDepth = Render::CreateTexture( texSpecDepth, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( textureHandleDepth, debugName );

    const Render::FramebufferHandle framebufferHandle = Render::CreateFramebufferForRenderToTexture(
      { textureHandleColor, textureHandleDepth }, TAC_STACK_FRAME );
    const Render::ViewHandle viewHandle = Render::CreateView();

    Render::DestroyView(exampleView);
    Render::DestroyTexture(exampleColor, TAC_STACK_FRAME);
    Render::DestroyTexture(exampleDepth, TAC_STACK_FRAME);
    Render::DestroyFramebuffer(exampleFramebuffer, TAC_STACK_FRAME);

    exampleView = viewHandle;
    exampleColor = textureHandleColor;
    exampleDepth = textureHandleDepth;
    exampleFramebuffer = framebufferHandle;
    exampleWidth = w;
    exampleHeight = h;
  }

  Render::TextureHandle ExamplesColor()
  {
    return exampleColor;
  }

  void ExamplesPresentationRender(World* world, const Camera* camera, const v2& drawSize )
  {
    ExamplesEnsureView( drawSize );
    const bool valid =  world && camera && exampleView.IsValid() ;
    if( !valid )
      return;

    Render::SetViewport( exampleView, Render::Viewport( drawSize.x, drawSize.y ) );
    Render::SetViewScissorRect( exampleView, Render::ScissorRect( drawSize.x, drawSize.y ) );
    Render::SetViewFramebuffer( exampleView, exampleFramebuffer );
    GamePresentationRender( world,
                            camera,
                            (int)drawSize.x,
                            (int)drawSize.y,
                            exampleView );
  }

} // namespace Tac

