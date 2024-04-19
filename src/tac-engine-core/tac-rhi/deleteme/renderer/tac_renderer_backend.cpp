#include "tac_renderer_backend.h" // self-inc

#include "tac-rhi/renderer/tac_render_frame.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_render_frame.h"
#include "tac-rhi/renderer/command/tac_render_command_handler.h"
#include "tac-rhi/renderer/tac_render_submit_ring_buffer.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/error/tac_error_handling.h"
//#include "tac-engine-core/profile/tac_profile.h"
#include "tac-std-lib/os/tac_os.h"
//#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/filesystem/tac_asset.h"
//#include "tac-desktop-app/tac_desktop_app.h"
//#include "tac-desktop-app/tac_desktop_app_threads.h"

namespace Tac::Render
{
  // -----------------------------------------------------------------------------------------------

  struct Encoder
  {
    void                  Submit( ViewHandle, const StackFrame& );

    int                   mUniformBufferIndex = 0;
    DrawCall              mDrawCall;
  };

  // -----------------------------------------------------------------------------------------------



  // -----------------------------------------------------------------------------------------------

  static thread_local Encoder gEncoder;
  static SubmitRingBuffer     gSubmitRingBuffer;
  static OS::ISemaphore*      gSubmitSemaphore;
  static OS::ISemaphore*      gRenderSemaphore;
  static int                  gFrameCount;
  static Frame                gFrames[ 2 ];
  static Frame* gRenderFrame = &gFrames[ 0 ];
  static Frame* gSubmitFrame = &gFrames[ 1 ];
  static u32                  gCrow = 0xcaacaaaa;
  Renderer* Renderer::Instance = nullptr;


  static void RenderDrawCalls( Errors& errors )
  {
    //TAC_PROFILE_BLOCK;
    Renderer* renderer = Renderer::Instance;
    const DrawCall* drawCallBegin = gRenderFrame->mDrawCalls.data();
    const int drawCallCount = gRenderFrame->mDrawCalls.size();
    for( int iDrawCall = 0; iDrawCall < drawCallCount; ++iDrawCall )
    {
      if( gRenderFrame->mBreakOnDrawCall == iDrawCall )
        OS::OSDebugBreak();

      const DrawCall* drawCall = drawCallBegin + iDrawCall;
      ExecuteUniformCommands( &gRenderFrame->mUniformBuffer,
                              drawCall->iUniformBegin,
                              drawCall->iUniformEnd,
                              errors );
      renderer->RenderDrawCall( gRenderFrame, drawCall, errors );
    }
  }

  static void ExecuteCommands( CommandBuffer* commandBuffer, Errors& errors )
  {
    static const CommandHandlers sCommandHandlers;
    Renderer* renderer = Renderer::Instance;

    CommandBufferIterator iter = commandBuffer->GetIterator();
    while( iter.IsValid() )
    {
      const auto renderCommandType = *iter.Pop< CommandType >();
      sCommandHandlers.Invoke( renderer, renderCommandType, &iter, errors );
    }
  }

  void Encoder::Submit( const ViewHandle viewHandle,
                        const StackFrame& stackFrame )
  {
    if( gSubmitFrame->mDrawCalls.size() == kDrawCallCapacity )
    {
      OS::OSDebugBreak();
      return;
    }

    int iUniformBegin = 0;
    int iUniformEnd = 0;
    const int uniformBufferSize = gSubmitFrame->mUniformBuffer.size();
    if( mUniformBufferIndex != uniformBufferSize )
    {
      iUniformBegin = mUniformBufferIndex;
      iUniformEnd = uniformBufferSize;
      mUniformBufferIndex = uniformBufferSize; // Now update after its been used
    }

    mDrawCall.mStackFrame = stackFrame;
    mDrawCall.iUniformBegin = iUniformBegin;
    mDrawCall.iUniformEnd = iUniformEnd;
    mDrawCall.mViewHandle = viewHandle;

    gSubmitFrame->mDrawCalls.push_back( mDrawCall );

    // Prepare the next draw
    mDrawCall = DrawCall();
  }

}

namespace Tac
{

  Render::Frame* Render::GetRenderFrame() { return gRenderFrame; }
  Render::Frame* Render::GetSubmitFrame() { return gSubmitFrame; }

  bool           Render::IsSubmitAllocated( const void* p )        { return gSubmitRingBuffer.IsSubmitAllocated( p ); }
  void*          Render::SubmitAlloc( const int n )                { return gSubmitRingBuffer.SubmitAlloc( n ); }
  const void*    Render::SubmitAlloc( const void* p, const int n ) { return gSubmitRingBuffer.SubmitAlloc( p, n ); }
  StringView     Render::SubmitAlloc( const StringView& sv )       { return gSubmitRingBuffer.SubmitAlloc(sv); }

  void Render::DestroyView( const ViewHandle viewHandle )
  {
    if(viewHandle.IsValid())
      gSubmitFrame->mFreeDeferredHandles.mFreedViews.push_back( viewHandle );
  }

  void Render::ResizeFramebuffer( const FramebufferHandle framebufferHandle,
                          const int w,
                          const int h,
                          const StackFrame& stackFrame )
  {
    const CommandDataResizeFramebuffer commandData
    {
      .mStackFrame = stackFrame,
      .mWidth = w,
      .mHeight = h,
      .mFramebufferHandle = framebufferHandle
    };
    gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::ResizeFramebuffer,
                                                        &commandData,
                                                        sizeof( CommandDataResizeFramebuffer ) );
  }

  void Render::UpdateTextureRegion( const TextureHandle handle,
                            const TexUpdate& texUpdate,
                            const StackFrame& stackFrame )
  {
    const int byteCount = texUpdate.mSrc.mHeight * texUpdate.mPitch;

    TexUpdate allocTexUpdate = texUpdate;
    allocTexUpdate.mSrcBytes = SubmitAlloc( texUpdate.mSrcBytes, byteCount );

    const CommandDataUpdateTextureRegion commandData
    {
      .mStackFrame = stackFrame,
      .mTextureHandle = handle,
      .mTexUpdate = allocTexUpdate
    };
    gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::UpdateTextureRegion,
                                                        &commandData,
                                                        sizeof( CommandDataUpdateTextureRegion ) );
  }

  void Render::UpdateVertexBuffer( const VertexBufferHandle handle,
                           const void* bytes,
                           const int byteCount,
                           const StackFrame& stackFrame )
  {
    const CommandDataUpdateVertexBuffer commandData
    {
      .mStackFrame = stackFrame,
      .mVertexBufferHandle = handle,
      .mBytes = SubmitAlloc( bytes, byteCount ),
      .mByteCount = byteCount
    };
    gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::UpdateVertexBuffer,
                                                        &commandData,
                                                        sizeof( CommandDataUpdateVertexBuffer ) );
  }

  void Render::UpdateIndexBuffer( const IndexBufferHandle handle,
                          const void* bytes,
                          const int byteCount,
                          const StackFrame& stackFrame )
  {
    const CommandDataUpdateIndexBuffer commandData
    {
      .mStackFrame = stackFrame,
      .mIndexBufferHandle = handle,
      .mBytes = SubmitAlloc( bytes, byteCount ),
      .mByteCount = byteCount
    };
    const CommandType type = CommandType::UpdateIndexBuffer;
    const int n = sizeof( CommandDataUpdateIndexBuffer );
    gSubmitFrame->mCommandBufferFrameBegin.PushCommand( type, &commandData, n );
  }

  void Render::SetViewFramebuffer( const ViewHandle viewId, const FramebufferHandle framebufferHandle )
  {
     gSubmitFrame->mViews.FindView( viewId )->mFrameBufferHandle = framebufferHandle;
  }

  void Render::SetViewScissorRect( const ViewHandle viewId, const ScissorRect& scissorRect )
  {
     gSubmitFrame->mViews.FindView( viewId )->SetScissorRect( scissorRect );
  }

  void Render::SetViewport( const ViewHandle viewId, const Viewport& viewport )
  {
     gSubmitFrame->mViews.FindView( viewId )->SetViewport( viewport );
  }

  void Render::SetVertexBuffer( const VertexBufferHandle vertexBufferHandle, const int startVertex, const int vertexCount )
  {
    gEncoder.mDrawCall.mVertexBufferHandle = vertexBufferHandle;
    gEncoder.mDrawCall.mStartVertex = startVertex;
    gEncoder.mDrawCall.mVertexCount = vertexCount;
  }

  void Render::SetIndexBuffer( const IndexBufferHandle indexBufferHandle, const int startIndex, const int indexCount )
  {
    gEncoder.mDrawCall.mIndexBufferHandle = indexBufferHandle;
    gEncoder.mDrawCall.mStartIndex = startIndex;
    gEncoder.mDrawCall.mIndexCount = indexCount;
  }

  void Render::SetShader( const ShaderHandle h )                   { gEncoder.mDrawCall.mShaderHandle = h; }
  void Render::SetBlendState( const BlendStateHandle h )           { gEncoder.mDrawCall.mBlendStateHandle = h; }
  void Render::SetRasterizerState( const RasterizerStateHandle h ) { gEncoder.mDrawCall.mRasterizerStateHandle = h; }
  void Render::SetSamplerState( const DrawCallSamplers& h )        { gEncoder.mDrawCall.mSamplerStateHandle = h; }
  void Render::SetDepthState( const DepthStateHandle h )           { gEncoder.mDrawCall.mDepthStateHandle = h; }
  void Render::SetVertexFormat( const VertexFormatHandle h )       { gEncoder.mDrawCall.mVertexFormatHandle = h; }
  void Render::SetTexture( const DrawCallTextures& t )             { gEncoder.mDrawCall.mDrawCallTextures = t; }
  void Render::SetPrimitiveTopology( const PrimitiveTopology pt )  { gEncoder.mDrawCall.mPrimitiveTopology = pt; }

  void Render::SetPixelShaderUnorderedAccessView( TextureHandle textureHandle, int i )
  {
    gEncoder.mDrawCall.mDrawCallUAVs.mUAVTextures[ i ] = textureHandle;
  }

  void Render::SetPixelShaderUnorderedAccessView( MagicBufferHandle magicBufferHandle, int i )
  {
    gEncoder.mDrawCall.mDrawCallUAVs.mUAVMagicBuffers[ i ] = magicBufferHandle;
  }

  void Render::BeginGroup( const StringView& name, const StackFrame& stackFrame )
  {
    TAC_ASSERT( !name.empty() );
    const UniformBufferHeader header( UniformBufferEntryType::DebugGroupBegin, stackFrame );
    gSubmitFrame->mUniformBuffer.PushHeader( header );
    gSubmitFrame->mUniformBuffer.PushString( name );
  }

  void Render::EndGroup( const StackFrame& stackFrame )
  {
    const UniformBufferHeader header( UniformBufferEntryType::DebugGroupEnd, stackFrame );
    gSubmitFrame->mUniformBuffer.PushHeader( header );
  }

  void Render::UpdateConstantBuffer( const ConstantBufferHandle constantBufferHandle,
                             const void* bytes,
                             const int byteCount,
                             const StackFrame& stackFrame )
  {
    const void* allocd = SubmitAlloc( bytes, byteCount );
    const UniformBufferHeader header( UniformBufferEntryType::UpdateConstantBuffer, stackFrame );
    gSubmitFrame->mUniformBuffer.PushHeader( header );
    gSubmitFrame->mUniformBuffer.PushNumber( ( int )constantBufferHandle );
    gSubmitFrame->mUniformBuffer.PushNumber( byteCount );
    gSubmitFrame->mUniformBuffer.PushPointer( allocd );
  }

  void Render::RenderFinish()
  {
    //TAC_ASSERT( DesktopAppThreads::IsMainThread() );
    gRenderSemaphore->IncrementPost(  );
  }


  void Render::RenderFrame( Errors& errors )
  {
    //TAC_PROFILE_BLOCK;

    //TAC_ASSERT( DesktopAppThreads::IsMainThread() );

    Renderer* renderer = Renderer::Instance;
    if( !renderer )
      return;

    {
      //TAC_PROFILE_BLOCK_NAMED( "wait submit" );
      gSubmitSemaphore->DecrementWait();
    }

    if( gRenderFrame->mBreakOnFrameRender )
      OS::OSDebugBreak();

    TAC_CALL( ExecuteCommands( &gRenderFrame->mCommandBufferFrameBegin, errors ) );

    renderer->RenderBegin( gRenderFrame, errors );

    TAC_CALL( RenderDrawCalls( errors ) );

    TAC_CALL( ExecuteCommands( &gRenderFrame->mCommandBufferFrameEnd, errors ) );

    TAC_CALL( renderer->RenderEnd( 1.0f / 60.0f, gRenderFrame, errors ) ); // hackkk

    gRenderSemaphore->IncrementPost();

    {
      //TAC_PROFILE_BLOCK_NAMED( "swap buffers" );
      renderer->SwapBuffers();
    }
  }


  void Render::SubmitFinish()
  {
    //TAC_ASSERT( DesktopAppThreads::IsLogicThread() );
    gSubmitSemaphore->IncrementPost();
  }

  void Render::SubmitFrame()
  {
    //TAC_ASSERT( DesktopAppThreads::IsLogicThread() );

    // Finish submitting this frame
    {
      //TAC_PROFILE_BLOCK_NAMED( "wait render" );
      gRenderSemaphore->DecrementWait(  );
    }

    gSubmitFrame->mFreeDeferredHandles.FinishFreeingHandles();

    if( gEncoder.mUniformBufferIndex != gSubmitFrame->mUniformBuffer.size() )
    {
      DrawCall drawCall;
      drawCall.mStackFrame = TAC_STACK_FRAME;
      drawCall.iUniformBegin = gEncoder.mUniformBufferIndex;
      drawCall.iUniformEnd = gSubmitFrame->mUniformBuffer.size();

      gSubmitFrame->mDrawCalls.push_back( drawCall );
    }

    // dont clear the views? because like we can technically call submitframe() several times
    // and like we dont want them to be cleared if that happens
    const Views views = gSubmitFrame->mViews;

    Swap( gRenderFrame, gSubmitFrame );
    gSubmitFrame->Clear();
    gSubmitFrame->mViews = views;
    gEncoder = Encoder();
    gFrameCount++;

    // Start submitting the next frame
    gSubmitSemaphore->IncrementPost();
  }

  void Render::Init( Errors& errors )
  {
    gSubmitSemaphore = OS::OSSemaphoreCreate();
    gRenderSemaphore = OS::OSSemaphoreCreate();

    // i guess well make render frame go first
    gSubmitSemaphore->IncrementPost();

    Renderer::Instance->Init( errors );
  }


  void Render::Submit( const ViewHandle viewHandle, const StackFrame& stackFrame )
  {
    gEncoder.Submit( viewHandle, stackFrame );
  }

  Render::OutProj Render::GetPerspectiveProjectionAB(InProj in)
  {
    return Renderer::Instance->GetPerspectiveProjectionAB(in);
  }

  void Render::Uninit()
  {
    delete Renderer::Instance;
  }

  void Render::SetBreakpointWhenThisFrameIsRendered()
  {
    gSubmitFrame->mBreakOnFrameRender = true;
  }

  void Render::SetBreakpointWhenNextDrawCallIsExecuted()
  {
    gSubmitFrame->mBreakOnDrawCall = gSubmitFrame->mDrawCalls.size();
  }

  AssetPathStringView Render::GetShaderAssetPath( const ShaderNameStringView& s )
  {
    return Renderer::Instance->GetShaderPath( s );
  }

  AssetPathStringView Render::GetShaderAssetDir()
  {
    return Renderer::Instance->GetShaderDir();
  }

} // namespace Tac

