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

    void CommandBuffer::Push( const void* bytes, int byteCount )
    {
      const int bufferSize = mBuffer.size();
      mBuffer.resize( mBuffer.size() + byteCount );
      MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
    }

    void CommandBuffer::PushCommand(
      CommandType type,
      StackFrame stackFrame,
      const void* bytes,
      int byteCount )
    {
      StringView cheep( "end" );
      Push( &type, sizeof( CommandType ) );
      Push( &stackFrame, sizeof( stackFrame ) );
      Push( bytes, byteCount );
      Push( cheep.data(), cheep.size() );
    }

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

    void UpdateConstantBuffers::Push( ConstantBufferHandle constantBufferHandle,
                                      const void* bytes,
                                      int byteCount )
    {
      TAC_ASSERT( mUpdateConstantBufferDataCount < kCapacity );
      UpdateConstantBuffer* updateConstantBuffer = &mUpdateConstantBufferDatas[ mUpdateConstantBufferDataCount++ ];
      updateConstantBuffer->mConstantBufferHandle = constantBufferHandle;
      updateConstantBuffer->mBytes = bytes;
      updateConstantBuffer->mByteCount = byteCount;
    }


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
      UpdateConstantBuffers mUpdateConstantBuffers;
      ShaderHandle mShaderHandle;
      TextureHandle mTextureHandle;
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
                               ShaderSource shaderSource,
                               ConstantBuffers constantBuffers,
                               StackFrame stackFrame )
    {
      TAC_ASSERT( constantBuffers.mConstantBufferCount );
      const ShaderHandle shaderHandle = { mIdCollectionShader.Alloc( name, stackFrame ) };
      CommandDataCreateShader commandData;
      commandData.mShaderSource.mShaderPath = SubmitAlloc( shaderSource.mShaderPath );
      commandData.mShaderSource.mShaderStr = SubmitAlloc( shaderSource.mShaderStr );
      commandData.mShaderHandle = shaderHandle;
      commandData.mConstantBuffers = constantBuffers;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateShader,
                                                stackFrame,
                                                &commandData,
                                                sizeof( CommandDataCreateShader ) );
      return shaderHandle;
    }
    VertexBufferHandle CreateVertexBuffer( StringView name,
                                           int byteCount,
                                           void* optionalInitialBytes,
                                           int stride,
                                           Access access,
                                           StackFrame stackFrame )
    {

      const VertexBufferHandle vertexBufferHandle = { mIdCollectionVertexBuffer.Alloc( name, stackFrame ) };
      CommandDataCreateVertexBuffer commandData;
      commandData.mAccess = access;
      commandData.mByteCount = byteCount;
      commandData.mOptionalInitialBytes = optionalInitialBytes;
      commandData.mStride = stride;
      commandData.mVertexBufferHandle = vertexBufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateVertexBuffer,
                                                stackFrame,
                                                &commandData, sizeof( CommandDataCreateVertexBuffer ) );
      return vertexBufferHandle;
    }

    ConstantBufferHandle CreateConstantBuffer( StringView name,
                                               int byteCount,
                                               int shaderRegister,
                                               StackFrame frame )
    {
      const ConstantBufferHandle constantBufferHandle = { mIdCollectionConstantBuffer.Alloc( name, frame ) };
      CommandDataCreateConstantBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mShaderRegister = shaderRegister;
      commandData.mConstantBufferHandle = constantBufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateConstantBuffer,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateConstantBuffer ) );
      return constantBufferHandle;
    }
    IndexBufferHandle CreateIndexBuffer( StringView name,
                                         int byteCount,
                                         void* optionalInitialBytes,
                                         Access access,
                                         Format format,
                                         StackFrame frame )
    {
      const IndexBufferHandle indexBufferHandle = { mIdCollectionIndexBuffer.Alloc( name, frame ) };
      CommandDataCreateIndexBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mOptionalInitialBytes = optionalInitialBytes;
      commandData.mAccess = access;
      commandData.mFormat = format;
      commandData.mIndexBufferHandle = indexBufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateIndexBuffer, frame, &commandData, sizeof( commandData ) );
      return indexBufferHandle;
    }

    TextureHandle CreateTexture( StringView name,
                                 CommandDataCreateTexture commandData,
                                 StackFrame frame )
    {
      const ResourceId resourceId = mIdCollectionTexture.Alloc( name, frame );

      const int imageByteCount = commandData.mImage.mFormat.CalculateTotalByteCount() *
        commandData.mImage.mWidth *
        commandData.mImage.mHeight;
      if( commandData.mImageBytes )
        commandData.mImageBytes = SubmitAlloc( commandData.mImageBytes, imageByteCount );
      for( int i = 0; i < 6; ++i )
        if( commandData.mImageBytesCubemap[ i ] )
          commandData.mImageBytesCubemap[ i ] = SubmitAlloc( commandData.mImageBytesCubemap[ i ],
                                                             imageByteCount );

      gSubmitFrame->mCommandBuffer.Push( CommandType::CreateTexture );
      gSubmitFrame->mCommandBuffer.Push( &frame, sizeof( frame ) );
      gSubmitFrame->mCommandBuffer.Push( &resourceId, sizeof( resourceId ) );
      gSubmitFrame->mCommandBuffer.Push( &commandData, sizeof( commandData ) );
      gSubmitFrame->mCommandBuffer.PushCommandEnd();
      return { resourceId };
    }

    void ResizeFramebuffer( FramebufferHandle framebufferHandle,
                            int w,
                            int h,
                            StackFrame frame )
    {
      CommandDataResizeFramebuffer commandData;
      commandData.mWidth = w;
      commandData.mHeight = h;
      commandData.mFramebufferHandle = framebufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::ResizeFramebuffer,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataResizeFramebuffer ) );
    }

    FramebufferHandle CreateFramebuffer( StringView name,
                                         DesktopWindowHandle desktopWindowHandle,
                                         int width,
                                         int height,
                                         StackFrame frame )
    {
      const FramebufferHandle framebufferHandle = mIdCollectionFramebuffer.Alloc( name, frame );
      CommandDataCreateFramebuffer commandData;
      commandData.mDesktopWindowHandle = desktopWindowHandle;
      commandData.mWidth = width;
      commandData.mHeight = height;
      commandData.mFramebufferHandle = framebufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateFramebuffer,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateFramebuffer ) );
      return framebufferHandle;
    }


    BlendStateHandle CreateBlendState( StringView name,
                                       BlendState blendState,
                                       StackFrame frame )
    {
      const BlendStateHandle blendStateHandle = mIdCollectionBlendState.Alloc( name, frame );
      CommandDataCreateBlendState commandData;
      commandData.mBlendState = blendState;
      commandData.mBlendStateHandle = blendStateHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateBlendState,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateBlendState ) );
      return blendStateHandle;
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
                                           VertexDeclarations vertexDeclarations,
                                           ShaderHandle shaderHandle,
                                           StackFrame frame )
    {
      const VertexFormatHandle vertexFormatHandle = mIdCollectionVertexFormat.Alloc( name, frame );
      CommandDataCreateVertexFormat commandData;
      commandData.mShaderHandle = shaderHandle;
      commandData.mVertexDeclarations = vertexDeclarations;
      commandData.mVertexFormatHandle = vertexFormatHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateVertexFormat,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateVertexFormat ) );
      return vertexFormatHandle;
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
                              TexUpdate texUpdate,
                              StackFrame frame )
    {
      const int byteCount = texUpdate.mSrc.mHeight * texUpdate.mPitch;
      texUpdate.mSrcBytes = Render::SubmitAlloc( texUpdate.mSrcBytes, byteCount );
      CommandDataUpdateTextureRegion commandData;
      commandData.mTexUpdate = texUpdate;
      commandData.mTextureHandle = handle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::UpdateTextureRegion,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataUpdateTextureRegion ) );
    }

    void UpdateVertexBuffer( VertexBufferHandle handle,
                             const void* bytes,
                             const int byteCount,
                             StackFrame frame )
    {
      CommandDataUpdateVertexBuffer commandData;
      commandData.mBytes = Render::SubmitAlloc( bytes, byteCount );
      commandData.mByteCount = byteCount;
      commandData.mVertexBufferHandle = handle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::UpdateVertexBuffer,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataUpdateVertexBuffer ) );
    }

    void UpdateIndexBuffer( IndexBufferHandle handle,
                            const void* bytes,
                            const int byteCount,
                            StackFrame frame )
    {
      CommandDataUpdateIndexBuffer commandData;
      commandData.mBytes = Render::SubmitAlloc( bytes, byteCount );
      commandData.mByteCount = byteCount;
      commandData.mIndexBufferHandle = handle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::UpdateIndexBuffer,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataUpdateIndexBuffer ) );
    }

    void UpdateConstantBuffer( ConstantBufferHandle handle,
                               const void* bytes,
                               const int byteCount,
                               StackFrame stackFrame )
    {
      TAC_UNUSED_PARAMETER( stackFrame );
      gEncoder.mUpdateConstantBuffers.Push( handle,
                                            Render::SubmitAlloc( bytes, byteCount ),
                                            byteCount );
    }

    void SetViewFramebuffer( ViewId viewId, FramebufferHandle framebufferHandle )
    {
      View* view = &gSubmitFrame->mViews[ viewId ];
      view->mFrameBufferHandle = framebufferHandle;
    }

    void SetViewScissorRect( ViewId viewId, ScissorRect scissorRect )
    {
      View* view = &gSubmitFrame->mViews[ viewId ];
      view->mScissorRect = scissorRect;
    }

    void SetViewport( ViewId viewId, Viewport viewport )
    {
      View* view = &gSubmitFrame->mViews[ viewId ];
      view->mViewport = viewport;
    }

    void SetShader( ShaderHandle shaderHandle )
    {
      gEncoder.mShaderHandle = shaderHandle;
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

    void SetTexture( TextureHandle textureHandle )
    {
      gEncoder.mTextureHandle = textureHandle;
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

      const int iDrawCall = gSubmitFrame->mDrawCallCount++;
      DrawCall3* drawCall = &gSubmitFrame->mDrawCalls[ iDrawCall ];
      drawCall->mIndexBufferHandle = gEncoder.mIndexBufferHandle;
      drawCall->mVertexBufferHandle = gEncoder.mVertexBufferHandle;
      drawCall->mBlendStateHandle = gEncoder.mBlendStateHandle;
      drawCall->mRasterizerStateHandle = gEncoder.mRasterizerStateHandle;
      drawCall->mSamplerStateHandle = gEncoder.mSamplerStateHandle;
      drawCall->mDepthStateHandle = gEncoder.mDepthStateHandle;
      drawCall->mVertexFormatHandle = gEncoder.mVertexFormatHandle;
      drawCall->mUpdateConstantBuffers = gEncoder.mUpdateConstantBuffers;
      drawCall->mStartIndex = gEncoder.mStartIndex;
      drawCall->mStartVertex = gEncoder.mStartVertex;
      drawCall->mIndexCount = gEncoder.mIndexCount;
      drawCall->mVertexCount = gEncoder.mVertexCount;
      drawCall->mShaderHandle = gEncoder.mShaderHandle;
      drawCall->mViewId = viewId;
      drawCall->mTextureHandle = gEncoder.mTextureHandle;
      gEncoder.mIndexBufferHandle = IndexBufferHandle();
      gEncoder.mVertexBufferHandle = VertexBufferHandle();
      gEncoder.mBlendStateHandle = BlendStateHandle();
      gEncoder.mRasterizerStateHandle = RasterizerStateHandle();
      gEncoder.mSamplerStateHandle = SamplerStateHandle();
      gEncoder.mDepthStateHandle = DepthStateHandle();
      gEncoder.mVertexFormatHandle = VertexFormatHandle();
      gEncoder.mUpdateConstantBuffers.mUpdateConstantBufferDataCount = 0;
      gEncoder.mShaderHandle = ShaderHandle();
      gEncoder.mTextureHandle = TextureHandle();
      gEncoder.mVertexCount = 0;
      gEncoder.mIndexCount = 0;
    }

    void GetPerspectiveProjectionAB( float f, float n, float& a, float& b )
    {
      Renderer::Instance->GetPerspectiveProjectionAB( f, n, a, b );
    }

    void AddDrawCall( const DrawCall2& drawCall )
    {
      Renderer::Instance->mDrawCall2s.push_back( drawCall );
    }

    void Init( Errors& errors )
    {
      Renderer::Instance->Init( errors );
    }

    void Uninit()
    {
      delete Renderer::Instance;
    }
  }



  Renderer* Renderer::Instance = nullptr;

  Renderer::Renderer()
  {
    Instance = this;
  }

  Renderer::~Renderer()
  {

  }

  String RendererTypeToString( const Renderer::Type rendererType )
  {
    switch( rendererType )
    {
      case Renderer::Type::Vulkan: return RendererNameVulkan;
      case Renderer::Type::OpenGL4: return RendererNameOpenGL4;
      case Renderer::Type::DirectX11: return RendererNameDirectX11;
      case Renderer::Type::DirectX12: return RendererNameDirectX12;
        TAC_INVALID_DEFAULT_CASE( rendererType );
    }
    TAC_INVALID_CODE_PATH;
    return "";
  }

  void RendererFactory::CreateRendererOuter()
  {
    CreateRenderer();
    Renderer::Instance->mName = mRendererName;
  }

  RendererRegistry& RendererRegistry::Instance()
  {
    // This variable must be inside this function or else the
    // renderers will add themselves too early or something
    // and then be stomped with an empty registry
    static RendererRegistry RendererRegistryInstance;
    return RendererRegistryInstance;
  }

  RendererFactory* RendererRegistry::FindFactory( StringView name )
  {
    for( RendererFactory* factory : mFactories )
      if( factory->mRendererName == name )
        return factory;
    return nullptr;
  }

}
