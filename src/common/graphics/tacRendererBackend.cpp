#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/shell/tacDesktopApp.h"

static uint32_t gRaven = 0xcaacaaaa;
namespace Tac
{
  namespace Render
  {


    template< int N >
    struct IdCollection
    {
      ResourceId      Alloc( StringView name, Tac::StackFrame frame )
      {
        return mFreeCount ? AllocFreeId( name, frame ) : AllocNewId( name, frame );
      }

      void            Free( ResourceId id )
      {
        TAC_ASSERT( ( unsigned )id < ( unsigned )mAllocCounter );
        TAC_ASSERT( !Contains( mFree, mFree + mFreeCount, id ) );
        mFree[ mFreeCount++ ] = id;
        TAC_ASSERT( mFreeCount <= N );
      }

    private:
      ResourceId      AllocFreeId( StringView name, Tac::StackFrame frame )
      {
        const ResourceId result = mFree[ --mFreeCount ];
        mNames[ result ] = name;
        mFrames[ result ] = frame;
        return result;
      }

      ResourceId      AllocNewId( StringView name, Tac::StackFrame frame )
      {
        TAC_ASSERT( mAllocCounter < N );
        mNames[ mAllocCounter ] = name;
        mFrames[ mAllocCounter ] = frame;
        return mAllocCounter++;
      }

      ResourceId      mFree[ N ];
      int             mFreeCount = 0;
      int             mAllocCounter = 0;
      String          mNames[ N ];
      Tac::StackFrame mFrames[ N ];
    };

    void CommandBuffer::PushCommandEnd()
    {
      Push( "end", 3 );
    }
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
    static uint32_t gCrow = 0xcaacaaaa;
    int i = gCrow + 1;

    static IdCollection<kMaxBlendStates> mIdCollectionBlendState;
    static IdCollection<kMaxConstantBuffers> mIdCollectionConstantBuffer;
    static IdCollection<kMaxDepthStencilStates> mIdCollectionDepthState;
    static IdCollection<kMaxFramebuffers> mIdCollectionFramebuffer;
    static IdCollection<kMaxIndexBuffers> mIdCollectionIndexBuffer;
    static IdCollection<kMaxRasterizerStates> mIdCollectionRasterizerState;
    static IdCollection<kMaxPrograms> mIdCollectionShader;
    static IdCollection<kMaxVertexBuffers> mIdCollectionVertexBuffer;
    static IdCollection<kMaxSamplerStates> mIdCollectionSamplerState;
    static IdCollection<kMaxTextures> mIdCollectionTexture;
    static IdCollection<kMaxInputLayouts> mIdCollectionVertexFormat;


    struct Encoder
    {
      VertexBufferHandle mVertexBufferHandle;
      IndexBufferHandle mIndexBufferHandle;
      int mStartIndex;
      int mStartVertex;
      int mIndexCount;
      int mVertexCount;
      BlendStateHandle mBlendStateHandle;
      RasterizerStateHandle mRasterizerStateHandle;
      SamplerStateHandle mSamplerStateHandle;
      DepthStateHandle mDepthStateHandle;
      VertexFormatHandle mVertexFormatHandle;
    };

    static thread_local Encoder gEncoder;

    static const int gSubmitRingBufferCapacity = 100 * 1024 * 1024;
    static char gSubmitRingBufferBytes[ gSubmitRingBufferCapacity ];
    static int gSubmitRingBufferPos;

    void DebugPrintSubmitAllocInfo()
    {
      std::cout << "gSubmitRingBufferCapacity: " << gSubmitRingBufferCapacity << std::endl;
      std::cout << "gSubmitRingBufferBytes: " << ( void* )gSubmitRingBufferBytes << std::endl;
      std::cout << "gSubmitRingBufferPos : " << gSubmitRingBufferPos << std::endl;
    }

    bool IsSubmitAllocated( const void* data )
    {
      const bool result =
        data >= gSubmitRingBufferBytes &&
        data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity;
      return result;
    }

    //static void AssertSubmitAllocated( void* data )
    //{
    //  TAC_ASSERT( data >= gSubmitRingBufferBytes );
    //  TAC_ASSERT( data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity );
    //}

    void* SubmitAlloc( const int byteCount )
    {
      const int beginPos = gSubmitRingBufferPos + byteCount > gSubmitRingBufferCapacity
        ? 0
        : gSubmitRingBufferPos;
      gSubmitRingBufferPos = beginPos + byteCount;
      return gSubmitRingBufferBytes + beginPos;
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
      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateShader );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
      return { resourceId };
    }


    void DestroyVertexBuffer( VertexBufferHandle handle, StackFrame frame )
    {
      mIdCollectionVertexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyIndexBuffer( IndexBufferHandle handle, StackFrame frame )
    {
      mIdCollectionIndexBuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyIndexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyTexture( TextureHandle handle, StackFrame frame )
    {
      mIdCollectionTexture.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyFramebuffer( FramebufferHandle handle, StackFrame frame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyFramebuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyShader( ShaderHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyShader );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyVertexFormat( VertexFormatHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyVertexFormat );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyConstantBuffer( ConstantBufferHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyConstantBuffer );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyDepthState( DepthStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyDepthState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyBlendState( BlendStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyBlendState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroyRasterizerState( RasterizerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroyRasterizerState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }
    void DestroySamplerState( SamplerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionFramebuffer.Free( handle.mResourceId );
      gSubmitFrame->mCommandBuffer.Push( CommandType::DestroySamplerState );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( stackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }



    void UpdateTextureRegion( TextureHandle handle,
                              CommandDataUpdateTextureRegion commandData,
                              StackFrame frame )
    {
      const int byteCount = commandData.mSrc.mHeight * commandData.mPitch;
      commandData.mSrcBytes = Render::SubmitAlloc( commandData.mSrcBytes, byteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateTextureRegion );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }

    void UpdateVertexBuffer( VertexBufferHandle handle,
                             CommandDataUpdateBuffer commandData,
                             StackFrame frame )

    {
      //std::cout << "UpdateVertexBuffer begin " << std::endl;;
      commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      //std::cout <<  "commandData.mBytes: " <<  (void*)commandData.mBytes << std::endl;;

      ( ( char* )commandData.mBytes )[ 0 ] = 'h';
      ( ( char* )commandData.mBytes )[ 1 ] = 'e';
      ( ( char* )commandData.mBytes )[ 2 ] = 'l';
      ( ( char* )commandData.mBytes )[ 3 ] = 'l';
      ( ( char* )commandData.mBytes )[ 4 ] = 'o';
      ( ( char* )commandData.mBytes )[ 5 ] = '\0';

      //DebugPrintSubmitAllocInfo();
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateVertexBuffer );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
      //std::cout <<  "UpdateVertexBuffer end " << std::endl;;
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
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
    }

    void UpdateConstantBuffer( ConstantBufferHandle handle,
                               CommandDataUpdateBuffer commandData,
                               StackFrame stackFrame )
    {
      commandData.mBytes = Render::SubmitAlloc( commandData.mBytes, commandData.mByteCount );
      gSubmitFrame->mCommandBuffer.Push( CommandType::UpdateConstantBuffer );
      gSubmitFrame->mCommandBuffer.Push( &stackFrame, sizeof( StackFrame ) );
      gSubmitFrame->mCommandBuffer.Push( &handle, sizeof( handle ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
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

    void SetBlendState( BlendStateHandle blendStateHandle )
    {
      gEncoder.mBlendStateHandle = blendStateHandle;
    }

    void SetRasterizerState( RasterizerStateHandle rasterizerStateHandle )
    {
      gEncoder.mRasterizerStateHandle = rasterizerStateHandle;
    }

    void SetSamplerState( SamplerStateHandle samplerStateHandle )
    {
      gEncoder.mSamplerStateHandle = samplerStateHandle;
    }

    void SetDepthState( DepthStateHandle depthStateHandle )
    {
      gEncoder.mDepthStateHandle = depthStateHandle;
    }

    void SetVertexFormat( VertexFormatHandle vertexFormatHandle )
    {
      gEncoder.mVertexFormatHandle = vertexFormatHandle;
    }

    static const void* AllocateUniform( const void* bytes, int byteCount )
    {
      void* submitBytes = gSubmitFrame->mUniformBuffer.mBytes;
      gSubmitFrame->mUniformBuffer.mByteCount += byteCount;
      MemCpy( submitBytes, bytes, byteCount );
      return submitBytes;
    }

    //void SetUniform( ConstantBufferHandle constantBufferHandle, const void* bytes, int byteCount )
    //{
    //  const void* uniformBytes = AllocateUniform( bytes, byteCount );
    //  gEncoder.

    //}


    // need lots of comments pls

    // i think these 2 semaphores just ensure that RenderFrame() and SubmitFrame()
    // alternate calls
    static Semaphore::Handle gSubmitSemaphore;
    static Semaphore::Handle gRenderSemaphore;

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
      gSubmitFrame->mDrawCallCount = 0;
      gFrameCount++;

      // submit start

      Semaphore::Increment( gSubmitSemaphore );
    }

    void Init()
    {
      //std::cout << "Render::Init begin" << std::endl;
      gSubmitSemaphore = Semaphore::Create();
      gRenderSemaphore = Semaphore::Create();

      // i guess well make render frame go first
      Semaphore::Increment( gSubmitSemaphore );

      //DebugPrintSubmitAllocInfo();
      //std::cout << "Render::Init end" << std::endl;
    }

    void Submit( ViewId viewId )
    {
      if( gSubmitFrame->mDrawCallCount == kDrawCallCapacity )
      {
        OS::DebugBreak();
        return;
      }

      int iDrawCall = gSubmitFrame->mDrawCallCount++;
      DrawCall3* drawCall = &gSubmitFrame->mDrawCalls[ iDrawCall ];
      drawCall->mIndexBufferHandle = gEncoder.mIndexBufferHandle;
      drawCall->mVertexBufferHandle = gEncoder.mVertexBufferHandle;
      drawCall->mBlendStateHandle = gEncoder.mBlendStateHandle;
      drawCall->mRasterizerStateHandle = gEncoder.mRasterizerStateHandle;
      drawCall->mSamplerStateHandle = gEncoder.mSamplerStateHandle;
      drawCall->mDepthStateHandle = gEncoder.mDepthStateHandle;
      drawCall->mVertexFormatHandle = gEncoder.mVertexFormatHandle;
      gEncoder.mIndexBufferHandle = IndexBufferHandle();
      gEncoder.mVertexBufferHandle = VertexBufferHandle();
      gEncoder.mBlendStateHandle = BlendStateHandle();
      gEncoder.mRasterizerStateHandle = RasterizerStateHandle();
      gEncoder.mSamplerStateHandle = SamplerStateHandle();
      gEncoder.mDepthStateHandle = DepthStateHandle();
      gEncoder.mVertexFormatHandle = VertexFormatHandle();
    }
  }
}
