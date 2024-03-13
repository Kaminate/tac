#include "tac-rhi/renderer/tac_render_frame.h" // self-inc

#include "tac-rhi/renderer/tac_render_id.h"

namespace Tac::Render
{

  void View::SetScissorRect( const ScissorRect& scissorRect )
  {
    mScissorRect = scissorRect;
    mScissorSet = true;
  }

  void View::SetViewport(  const Viewport& viewport )
  {
    mViewport = viewport;
    mViewportSet = true;
  }

  template< typename T, int N >
  static void FinishFreeingHandle( FixedVector< T, N >& freed, IdCollection& collection )
  {
    if( freed.empty() )
      return;

    for( const T& handle : freed )
      collection.Free( ( int )handle );

    freed.clear();
  }

  void FreeDeferredHandles::FinishFreeingHandles()
  {
    FinishFreeingHandle( mFreedBlendStates, mIdCollectionBlendState );
    FinishFreeingHandle( mFreedConstantBuffers, mIdCollectionConstantBuffer );
    FinishFreeingHandle( mFreedDepthStencilStates, mIdCollectionDepthState );
    FinishFreeingHandle( mFreedFramebuffers, mIdCollectionFramebuffer );
    FinishFreeingHandle( mFreedIndexBuffers, mIdCollectionIndexBuffer );
    FinishFreeingHandle( mFreedRasterizerStates, mIdCollectionRasterizerState );
    FinishFreeingHandle( mFreedSamplerStates, mIdCollectionSamplerState );
    FinishFreeingHandle( mFreedShaders, mIdCollectionShader );
    FinishFreeingHandle( mFreedMagicBuffers, mIdCollectionMagicBuffer );
    FinishFreeingHandle( mFreedTextures, mIdCollectionTexture );
    FinishFreeingHandle( mFreedVertexBuffers, mIdCollectionVertexBuffer );
    FinishFreeingHandle( mFreedVertexFormatInputLayouts, mIdCollectionVertexFormat );
    FinishFreeingHandle( mFreedViews, mIdCollectionViewId );
  }


  bool               DrawCallUAVs::HasValidHandle() const
  {
    for( const MagicBufferHandle& uav : mUAVMagicBuffers )
      if( uav.IsValid() )
        return true;

    for( const TextureHandle& uav : mUAVTextures )
      if( uav.IsValid() )
        return true;

    return false;
  }

  View*               Views::FindView( const ViewHandle viewHandle )
  {
    const int i = viewHandle.GetIndex();
    TAC_ASSERT_INDEX( i, kMaxViews );
    return  viewHandle.IsValid() ? &mViews[ i ] : nullptr;
  }

  const View*         Views::FindView( const ViewHandle viewHandle ) const
  {
    const int i = viewHandle.GetIndex();
    TAC_ASSERT_INDEX( i, kMaxViews );
    return  viewHandle.IsValid() ? &mViews[ i ] : nullptr;
  }

  void Frame::Clear()
  {
    // calling Resize prevents us of using curFrame = Frame();
    mCommandBufferFrameBegin.Resize( 0 );
    mCommandBufferFrameEnd.Resize( 0 );
    mDrawCalls.clear();
    mUniformBuffer.clear();
    mBreakOnFrameRender = false;
    mBreakOnDrawCall = -1;
  }

} // namespace Tac::Render

