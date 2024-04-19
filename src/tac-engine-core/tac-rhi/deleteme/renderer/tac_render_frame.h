#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/command/tac_render_command_buffer.h"
#include "tac-rhi/renderer/tac_render_uniform_buffer.h"

namespace Tac::Render
{
  const int kDrawCallCapacity = 1000;
  const int kMaxTextures = 1000;
  const int kMaxVertexBuffers = 1000;
  const int kMaxMagicBuffers = 1000;
  const int kMaxIndexBuffers = 1000;
  const int kMaxFramebuffers = 100;
  const int kMaxRasterizerStates = 100;
  const int kMaxSamplerStates = 100;
  const int kMaxDepthStencilStates = 100;
  const int kMaxInputLayouts = 100;
  const int kMaxBlendStates = 100;
  const int kMaxConstantBuffers = 100;
  const int kMaxPrograms = 100;
  const int kMaxViews = 100;

  struct DrawCallUAVs
  {
    bool                  HasValidHandle() const;

    TextureHandle         mUAVTextures[ 2 ];
    MagicBufferHandle     mUAVMagicBuffers[ 2 ];
  };

  // Self-contained all the shit you would ever need to draw a thing
  struct DrawCall
  {
    StackFrame            mStackFrame;
    VertexBufferHandle    mVertexBufferHandle;
    IndexBufferHandle     mIndexBufferHandle;
    BlendStateHandle      mBlendStateHandle;
    RasterizerStateHandle mRasterizerStateHandle;
    DrawCallSamplers      mSamplerStateHandle;
    DepthStateHandle      mDepthStateHandle;
    VertexFormatHandle    mVertexFormatHandle;
    ShaderHandle          mShaderHandle;
    DrawCallTextures      mDrawCallTextures;
    ViewHandle            mViewHandle;
    PrimitiveTopology     mPrimitiveTopology = PrimitiveTopology::Unknown;
    DrawCallUAVs          mDrawCallUAVs;
    int                   mStartIndex = 0;
    int                   mStartVertex = 0;
    int                   mIndexCount = 0;
    int                   mVertexCount = 0;
    int                   iUniformBegin = 0;
    int                   iUniformEnd = 0;
  };


  typedef FixedVector< DrawCall, kDrawCallCapacity > DrawCalls;


  struct View
  {
    FramebufferHandle mFrameBufferHandle;
    Viewport          mViewport;
    ScissorRect       mScissorRect;
    bool              mViewportSet = false;
    bool              mScissorSet = false;

    void SetScissorRect( const ScissorRect& );
    void SetViewport(  const Viewport& );
  };

  // This struct holds handles deleted this frame
  struct FreeDeferredHandles
  {
    // Let's say you do the following:
    //   Render::DestroyVertexBuffer( foo.vertexbuffer );
    //   bar.vertexBuffer = Render::CreateVertexBuffer();
    // We don't want to reuse the same vertex buffer handle, because it will be destroyed
    // at the end of the frame n during ExecuteCommands( mCommandBufferFrameEnd ),
    // causing problems for frame n+1 which is still trying to use the resource.
    // so instead defer deletion of the handle ( not the resource ) until we are done issuing commands.
    void FinishFreeingHandles();

    FixedVector< BlendStateHandle, kMaxBlendStates >           mFreedBlendStates;
    FixedVector< ConstantBufferHandle, kMaxConstantBuffers >   mFreedConstantBuffers;
    FixedVector< DepthStateHandle, kMaxDepthStencilStates >    mFreedDepthStencilStates;
    FixedVector< FramebufferHandle, kMaxFramebuffers >         mFreedFramebuffers;
    FixedVector< IndexBufferHandle, kMaxIndexBuffers >         mFreedIndexBuffers;
    FixedVector< MagicBufferHandle, kMaxMagicBuffers >         mFreedMagicBuffers;
    FixedVector< RasterizerStateHandle, kMaxRasterizerStates > mFreedRasterizerStates;
    FixedVector< SamplerStateHandle, kMaxSamplerStates >       mFreedSamplerStates;
    FixedVector< ShaderHandle, kMaxPrograms >                  mFreedShaders;
    FixedVector< TextureHandle, kMaxTextures >                 mFreedTextures;
    FixedVector< VertexBufferHandle, kMaxVertexBuffers >       mFreedVertexBuffers;
    FixedVector< VertexFormatHandle, kMaxInputLayouts >        mFreedVertexFormatInputLayouts;
    FixedVector< ViewHandle, kMaxViews >                       mFreedViews;
  };

  struct Views
  {
    View*               FindView( ViewHandle );
    const View*         FindView( ViewHandle ) const;

    View                mViews[ kMaxViews ];
  };

  struct Frame
  {
    void                Clear();

    CommandBuffer       mCommandBufferFrameBegin;
    CommandBuffer       mCommandBufferFrameEnd;
    DrawCalls           mDrawCalls;
    UniformBuffer       mUniformBuffer;
    Views               mViews;
    bool                mBreakOnFrameRender = false;
    int                 mBreakOnDrawCall = -1;
    FreeDeferredHandles mFreeDeferredHandles;
  };


} // namespace Tac::Render
