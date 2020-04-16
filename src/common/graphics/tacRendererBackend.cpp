#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/shell/tacDesktopApp.h"

namespace Tac
{
  namespace Render
  {
    struct IdCollection
    {
      ResourceId                       Alloc( StringView name, Tac::StackFrame frame );
      void                             Free( ResourceId );
      Vector< ResourceId >             mFree;
      int                              mAllocCounter = 0;
      Vector< String >                 mNames;
      Vector< Tac::StackFrame >        mFrames;
    };

    void CommandBuffer::Push( CommandType type )
    {
      Push( &type, sizeof( CommandType ) );
    }
    void CommandBuffer::Push( const void* bytes, int byteCount )
    {
      const int bufferSize = mBuffer.size();
      mBuffer.resize( mBuffer.size() + byteCount );
      MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
    }

    struct View
    {
      FramebufferHandle mFramebufferHandle;
    };

    static View gViews[ 10 ];


    static Frame gFrames[ 2 ];
    static Frame* gRenderFrame = &gFrames[ 0 ];
    static Frame* gSubmitFrame = &gFrames[ 1 ];

    static IdCollection                     mIdCollectionShader;
    static IdCollection                     mIdCollectionVertexBuffer;
    static IdCollection                     mIdCollectionIndexBuffer;
    static IdCollection                     mIdCollectionTexture;
    static IdCollection                     mIdCollectionFramebuffer;
    static IdCollection                     mIdCollectionVertexFormat;
    static IdCollection                     mIdCollectionDepthState;
    static IdCollection                     mIdCollectionSamplerState;
    static IdCollection                     mIdCollectionRasterizerState;
    static IdCollection                     mIdCollectionBlendState;
    static IdCollection                     mIdCollectionConstantBuffer;

    ResourceId IdCollection::Alloc( StringView name, Tac::StackFrame frame )
    {
      if( mFree.empty() )
      {
        mNames.push_back( name );
        mFrames.push_back( frame );
        return mAllocCounter++;
      }
      const ResourceId result = mFree.back();
      mNames[ result ] = name;
      mFrames[ result ] = frame;
      mFree.pop_back();
      return result;
    }

    void IdCollection::Free( ResourceId id )
    {
      TAC_ASSERT( ( unsigned )id < ( unsigned )mAllocCounter );
      TAC_ASSERT( !Contains( mFree, id ) );
      mFree.push_back( id );
    }

    struct Encoder
    {
      VertexBufferHandle mVertexBufferHandle;
      IndexBufferHandle mIndexBufferHandle;
      int mStartIndex;
      int mStartVertex;
      int mIndexCount;
      int mVertexCount;
    };

    static thread_local Encoder gEncoder;

    static char* gSubmitRingBufferBytes;
    static int gSubmitRingBufferCapacity;
    static int gSubmitRingBufferPos;

    static bool IsSubmitAllocated( const void* data )
    {
      const bool result =
        data >= gSubmitRingBufferBytes &&
        data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity;
      return data;
    }

    //static void AssertSubmitAllocated( void* data )
    //{
    //  TAC_ASSERT( data >= gSubmitRingBufferBytes );
    //  TAC_ASSERT( data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity );
    //}

    static void SubmitAllocInit( const int ringBufferByteCount )
    {
      gSubmitRingBufferCapacity = ringBufferByteCount;
      gSubmitRingBufferBytes = new char[ ringBufferByteCount ];
    }


    void* SubmitAlloc( const int byteCount )
    {
      if( gSubmitRingBufferPos + byteCount >= gSubmitRingBufferCapacity )
      {
        gSubmitRingBufferPos = byteCount;
        return gSubmitRingBufferBytes;
      }

      void* result = ( void* )( gSubmitRingBufferBytes + gSubmitRingBufferPos );
      gSubmitRingBufferPos += byteCount;
      return result;
    }

    const void* SubmitAlloc( const void* bytes, int byteCount )
    {
      if( IsSubmitAllocated( bytes ) )
        return bytes;
      void* dst = SubmitAlloc( byteCount );
      MemCpy( dst, bytes, byteCount );
      return dst;
    }

    StringView SubmitAlloc( StringView stringView )
    {
      if( !stringView.mLen )
        return {};

      if( IsSubmitAllocated( stringView.data() ) )
        return stringView;

      const void* resultData = SubmitAlloc( stringView.data(), stringView.size() );
      StringView result( ( const char* )resultData, stringView.size() );
      return result;
    }

    //void SubmitAllocBeginFrame()
    //{
    //}

    ShaderHandle CreateShader( StringView name,
                               CommandDataCreateShader commandData,
                               StackFrame stackFrame )
    {
      commandData.mShaderPath = SubmitAlloc( commandData.mShaderPath );
      commandData.mShaderStr = SubmitAlloc( commandData.mShaderStr );
      const ResourceId resourceId = mIdCollectionShader.Alloc( name, stackFrame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    VertexBufferHandle CreateVertexBuffer( StringView name,
                                           CommandDataCreateBuffer commandData,
                                           StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionVertexBuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    ConstantBufferHandle CreateConstantBuffer( StringView name,
                                               CommandDataCreateConstantBuffer commandData,
                                               StackFrame frame )
    {

      const ResourceId resourceId = mIdCollectionConstantBuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateConstantBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    IndexBufferHandle CreateIndexBuffer( StringView name,
                                         CommandDataCreateBuffer commandData,
                                         StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionIndexBuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    TextureHandle CreateTexture( StringView name,
                                 CommandDataCreateTexture commandData,
                                 StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionTexture.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }

    FramebufferHandle CreateFramebuffer( StringView name,
                                         CommandDataCreateFramebuffer commandData,
                                         StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionFramebuffer.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateFramebuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }


    BlendStateHandle CreateBlendState( StringView name,
                                       CommandDataCreateBlendState commandData,
                                       StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionBlendState.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateBlendState );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    RasterizerStateHandle CreateRasterizerState( StringView name,
                                                 CommandDataCreateRasterizerState commandData,
                                                 StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionRasterizerState.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateRasterizerState );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    SamplerStateHandle CreateSamplerState( StringView name,
                                           CommandDataCreateSamplerState commandData,
                                           StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionSamplerState.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateSamplerState );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    DepthStateHandle CreateDepthState( StringView name,
                                       CommandDataCreateDepthState commandData,
                                       StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionDepthState.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateDepthState );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }
    VertexFormatHandle CreateVertexFormat( StringView name,
                                           CommandDataCreateVertexFormat commandData,
                                           StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionVertexFormat.Alloc( name, frame );
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateVertexFormat );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      return { resourceId };
    }


    void DestroyVertexBuffer( VertexBufferHandle handle, StackFrame frame )
    {
      mIdCollectionVertexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyIndexBuffer( IndexBufferHandle handle, StackFrame frame )
    {
      mIdCollectionIndexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyTexture( TextureHandle handle, StackFrame frame )
    {
      mIdCollectionTexture.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyFramebuffer( FramebufferHandle handle, StackFrame frame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyFramebuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyShader( ShaderHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyShader );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyVertexFormat( VertexFormatHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyVertexFormat );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyConstantBuffer( ConstantBufferHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyConstantBuffer );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyDepthState( DepthStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyDepthState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyBlendState( BlendStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyBlendState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroyRasterizerState( RasterizerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyRasterizerState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }
    void DestroySamplerState( SamplerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroySamplerState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
    }


    void UpdateTextureRegion(
      TextureHandle handle,
      CommandDataUpdateTextureRegion commandData,
      StackFrame frame )
    {
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateTextureRegion );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateVertexBuffer( VertexBufferHandle handle,
                             CommandDataUpdateBuffer commandData,
                             StackFrame frame )

    {
      commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void UpdateIndexBuffer( IndexBufferHandle handle,
                            CommandDataUpdateBuffer commandData,
                            StackFrame frame )
    {
      commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
    }

    void SetViewFramebuffer( ViewId viewId, FramebufferHandle framebufferHandle )
    {
      gViews[ viewId ].mFramebufferHandle = framebufferHandle;
    }

    void SetVertexBuffer( VertexBufferHandle vertexBufferHandle, int startVertex, int vertexCount )
    {
      gEncoder.mVertexBufferHandle = vertexBufferHandle;
      gEncoder.mStartVertex = startVertex;
      gEncoder.mVertexCount = vertexCount;
    }
    void SetIndexBuffer( IndexBufferHandle indexBufferHandle, int startIndex, int indexCount )
    {
      gEncoder.mIndexBufferHandle = indexBufferHandle;
      gEncoder.mStartIndex = startIndex;
      gEncoder.mIndexCount = indexCount;
    }

    // need lots of comments pls

    // i think these 2 semaphores just ensure that RenderFrame() and SubmitFrame()
    // alternate calls
    Semaphore::Handle gSubmitSemaphore;
    Semaphore::Handle gRenderSemaphore;

    void RenderFrame()
    {
      TAC_ASSERT( gThreadType == ThreadType::Main );

      Semaphore::WaitAndDecrement( gSubmitSemaphore );

      Errors errors;
      Renderer::Instance->Render2( gRenderFrame, errors );
      TAC_ASSERT( errors.empty() );

      Semaphore::Increment( gRenderSemaphore );

      Renderer::Instance->SwapBuffers();
    }

    static int gFrameCount;
    void SubmitFrame()
    {
      TAC_ASSERT( gThreadType == ThreadType::Stuff );
      Semaphore::WaitAndDecrement( gRenderSemaphore );

      // submit finish

      Swap( gRenderFrame, gSubmitFrame );
      gSubmitFrame->mCommandBuffer.mBuffer.clear();
      gFrameCount++;

      // submit start

      Semaphore::Increment( gSubmitSemaphore );
    }

    void Init( int ringBufferByteCount )
    {
      SubmitAllocInit( ringBufferByteCount );
      gSubmitSemaphore = Semaphore::Create();
      gRenderSemaphore = Semaphore::Create();

      // i guess well make render frame go first
      Semaphore::Increment( gSubmitSemaphore );
    }

    void Submit( ViewId viewId )
    {
      int iDrawCall = gSubmitFrame->mDrawCallCount++;
      DrawCall3* drawCall = &gSubmitFrame->mDrawCalls[ iDrawCall ];
      drawCall->mIndexBufferHandle = gEncoder.mIndexBufferHandle;
      drawCall->mVertexBufferHandle = gEncoder.mVertexBufferHandle;
      gEncoder.mIndexBufferHandle = IndexBufferHandle();
      gEncoder.mVertexBufferHandle = VertexBufferHandle();
    }
  }
}
