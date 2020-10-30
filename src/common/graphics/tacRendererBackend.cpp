#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/common/tacIDCollection.h"
#include "src/shell/tacDesktopApp.h"

static uint32_t gRaven = 0xcaacaaaa;
namespace Tac
{
  static bool gVerbose;

  template< typename T >
  static T* Pop( const char*& bufferPos )
  {
    auto result = ( T* )bufferPos;
    bufferPos += sizeof( T );
    return result;
  }

  template< typename T >
  static T* PopCommandData( const char*& bufferPos )
  {
    auto result = ( T* )bufferPos;
    bufferPos += sizeof( T );

    TAC_ASSERT( bufferPos[ 0 ] == 'e' );
    TAC_ASSERT( bufferPos[ 1 ] == 'n' );
    TAC_ASSERT( bufferPos[ 2 ] == 'd' );
    bufferPos += 3;

    return result;
  }

  namespace Render
  {


    void UniformBuffer::PushData( const void* bytes,
                                  const int byteCount )
    {
      memcpy( mBytes + mByteCount, bytes, byteCount );
      mByteCount += byteCount;
    }

    void UniformBuffer::PushType( const UniformBufferEntryType type )
    {
      PushData( &type, sizeof( type ) );
    }

    void UniformBuffer::PushString( StringView s )
    {
      PushNumber( s.size() );
      PushData( s.c_str(), s.size() );
      PushData( "", 1 );
    }

    void UniformBuffer::PushNumber( int i )
    {
      PushData( &i, sizeof( i ) );
    }

    int UniformBuffer::size()const
    {
      return mByteCount;
    }

    void* UniformBuffer::data()const
    {
      return ( void* )mBytes;
    }

    void UniformBuffer::clear()
    {
      mByteCount = 0;
    }


    UniformBuffer::Iterator::Iterator( const UniformBuffer* uniformBuffer,
                                       const int iBegin,
                                       const int iEnd )
    {
      const char* uniformBufferData = ( char* )uniformBuffer->data();
      mCur = uniformBufferData + iBegin;
      mEnd = uniformBufferData + iEnd;
    }

    UniformBufferEntryType UniformBuffer::Iterator::PopType()
    {
      return *( UniformBufferEntryType* )PopData( sizeof( UniformBufferEntryType ) );
    }

    void* UniformBuffer::Iterator::PopData( int byteCount )
    {
      auto result = ( void* )mCur;
      mCur += byteCount;
      return result;
    }

    int UniformBuffer::Iterator::PopNumber()
    {
      return *( int* )PopData( sizeof( int ) );
    }

    StringView UniformBuffer::Iterator::PopString()
    {
      const int len = PopNumber();
      const char* str = ( const char* )PopData( len );
      PopData( 1 );
      return StringView( str, len );
    }

    //template< int N >
    //struct IdCollection
    //{
    //  index      Alloc( StringView name, Tac::StackFrame frame );
    //  void            Free( index id );
    //private:
    //  index      AllocFreeId( StringView name, Tac::StackFrame frame );
    //  index      AllocNewId( StringView name, Tac::StackFrame frame );
    //  index      mFree[ N ];
    //  int             mFreeCount = 0;
    //  int             mAllocCounter = 0;
    //  String          mNames[ N ];
    //  Tac::StackFrame mFrames[ N ];
    //};

    //template< int N > index IdCollection<N>::Alloc( StringView name, Tac::StackFrame frame )
    //{
    //  return mFreeCount ? AllocFreeId( name, frame ) : AllocNewId( name, frame );
    //}

    //template< int N > void IdCollection<N>::Free( index id )
    //{
    //  TAC_ASSERT( ( unsigned )id < ( unsigned )mAllocCounter );
    //  TAC_ASSERT( !Contains( mFree, mFree + mFreeCount, id ) );
    //  mFree[ mFreeCount++ ] = id;
    //  TAC_ASSERT( mFreeCount <= N );
    //}

    //template< int N > index IdCollection<N>::AllocFreeId( StringView name, Tac::StackFrame frame )
    //{
    //  const index result = mFree[ --mFreeCount ];
    //  mNames[ result ] = name;
    //  mFrames[ result ] = frame;
    //  return result;
    //}

    //template< int N > index IdCollection<N>::AllocNewId( StringView name, Tac::StackFrame frame )
    //{
    //  TAC_ASSERT( mAllocCounter < N );
    //  mNames[ mAllocCounter ] = name;
    //  mFrames[ mAllocCounter ] = frame;
    //  return mAllocCounter++;
    //}


    void CommandBuffer::Push( const void* bytes,
                              int byteCount )
    {
      const int bufferSize = mBuffer.size();
      mBuffer.resize( mBuffer.size() + byteCount );
      MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
    }

    void CommandBuffer::PushCommand( CommandType type,
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

    void CommandBuffer::Resize( int newSize )
    {
      mBuffer.resize( newSize );
    }
    void CommandBuffer::Clear()
    {
      mBuffer.clear();
    }
    const char* CommandBuffer::Data() const
    {
      return mBuffer.data();

    }
    int CommandBuffer::Size() const
    {

      return mBuffer.size();
    }

    static Frame gFrames[ 2 ];
    static Frame* gRenderFrame = &gFrames[ 0 ];
    static Frame* gSubmitFrame = &gFrames[ 1 ];
    static uint32_t gCrow = 0xcaacaaaa;
    int i = gCrow + 1;

    static IdCollection mIdCollectionBlendState( kMaxBlendStates );
    static IdCollection mIdCollectionConstantBuffer( kMaxConstantBuffers );
    static IdCollection mIdCollectionDepthState( kMaxDepthStencilStates );
    static IdCollection mIdCollectionFramebuffer( kMaxFramebuffers );
    static IdCollection mIdCollectionIndexBuffer( kMaxIndexBuffers );
    static IdCollection mIdCollectionRasterizerState( kMaxRasterizerStates );
    static IdCollection mIdCollectionSamplerState( kMaxSamplerStates );
    static IdCollection mIdCollectionShader( kMaxPrograms );
    static IdCollection mIdCollectionTexture( kMaxTextures );
    static IdCollection mIdCollectionVertexBuffer( kMaxVertexBuffers );
    static IdCollection mIdCollectionVertexFormat( kMaxInputLayouts );
    static IdCollection mIdCollectionViewId( kMaxViews );

    //void UpdateConstantBuffers::Push( ConstantBufferHandle constantBufferHandle,
    //                                  const void* bytes,
    //                                  int byteCount )
    //{
    //  TAC_ASSERT( mUpdateConstantBufferDataCount < kCapacity );
    //  UpdateConstantBuffer* updateConstantBuffer = &mUpdateConstantBufferDatas[ mUpdateConstantBufferDataCount++ ];
    //  updateConstantBuffer->mConstantBufferHandle = constantBufferHandle;
    //  updateConstantBuffer->mBytes = bytes;
    //  updateConstantBuffer->mByteCount = byteCount;
    //}

    struct Encoder
    {
      void                  Submit( Render::ViewHandle viewHandle,
                                    StackFrame stackFrame );
      VertexBufferHandle    mVertexBufferHandle;
      IndexBufferHandle     mIndexBufferHandle;
      int                   mStartIndex;
      int                   mStartVertex;
      int                   mIndexCount;
      int                   mVertexCount;
      BlendStateHandle      mBlendStateHandle;
      RasterizerStateHandle mRasterizerStateHandle;
      SamplerStateHandle    mSamplerStateHandle;
      DepthStateHandle      mDepthStateHandle;
      VertexFormatHandle    mVertexFormatHandle;
      UpdateConstantBuffers mUpdateConstantBuffers;
      ShaderHandle          mShaderHandle;
      DrawCallTextures      mTextureHandle;
      int                   mUniformBufferIndex = 0;
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
      if( !byteCount )
        return nullptr;
      const int beginPos = gSubmitRingBufferPos + byteCount > gSubmitRingBufferCapacity
        ? 0
        : gSubmitRingBufferPos;
      gSubmitRingBufferPos = beginPos + byteCount;
      return gSubmitRingBufferBytes + beginPos;
    }

    const void* SubmitAlloc( const void* bytes, int byteCount )
    {
      if( !bytes )
        return nullptr;
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

    ViewHandle CreateView()
    {
      return { mIdCollectionViewId.Alloc() };
    }

    ShaderHandle CreateShader( StringView name,
                               ShaderSource shaderSource,
                               ConstantBuffers constantBuffers,
                               StackFrame stackFrame )
    {
      TAC_ASSERT( constantBuffers.mConstantBufferCount );
      const ShaderHandle shaderHandle = { mIdCollectionShader.Alloc() };
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
    VertexBufferHandle CreateVertexBuffer( const StringView name,
                                           const int byteCount,
                                           const void* optionalInitialBytes,
                                           const int stride,
                                           const Access access,
                                           const StackFrame stackFrame )
    {

      const VertexBufferHandle vertexBufferHandle = { mIdCollectionVertexBuffer.Alloc() };
      CommandDataCreateVertexBuffer commandData;
      commandData.mAccess = access;
      commandData.mByteCount = byteCount;
      commandData.mOptionalInitialBytes = SubmitAlloc( optionalInitialBytes, byteCount );
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
      const ConstantBufferHandle constantBufferHandle = { mIdCollectionConstantBuffer.Alloc() };
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
    IndexBufferHandle CreateIndexBuffer( const StringView name,
                                         const int byteCount,
                                         const void* optionalInitialBytes,
                                         const Access access,
                                         const Format format,
                                         const StackFrame frame )
    {
      const IndexBufferHandle indexBufferHandle = { mIdCollectionIndexBuffer.Alloc() };
      CommandDataCreateIndexBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mOptionalInitialBytes = SubmitAlloc( optionalInitialBytes, byteCount );
      commandData.mAccess = access;
      commandData.mFormat = format;
      commandData.mIndexBufferHandle = indexBufferHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateIndexBuffer, frame, &commandData, sizeof( commandData ) );
      return indexBufferHandle;
    }

    TextureHandle CreateTexture( StringView name,
                                 TexSpec texSpec,
                                 StackFrame frame )
    {
      const int imageByteCount =
        texSpec.mImage.mFormat.CalculateTotalByteCount() *
        texSpec.mImage.mWidth *
        texSpec.mImage.mHeight;
      if( texSpec.mImageBytes )
        texSpec.mImageBytes = SubmitAlloc( texSpec.mImageBytes, imageByteCount );
      for( int i = 0; i < 6; ++i )
        if( texSpec.mImageBytesCubemap[ i ] )
          texSpec.mImageBytesCubemap[ i ] = SubmitAlloc( texSpec.mImageBytesCubemap[ i ],
                                                         imageByteCount );
      const TextureHandle textureHandle = mIdCollectionTexture.Alloc();
      CommandDataCreateTexture commandData;
      commandData.mTexSpec = texSpec;
      commandData.mTextureHandle = textureHandle;


      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateTexture,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateTexture ) );
      return textureHandle;
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
                                         const void* nativeWindowHandle,
                                         int width,
                                         int height,
                                         StackFrame frame )
    {
      const FramebufferHandle framebufferHandle = mIdCollectionFramebuffer.Alloc();
      CommandDataCreateFramebuffer commandData;
      //commandData.mDesktopWindowHandle = desktopWindowHandle;
      commandData.mNativeWindowHandle = nativeWindowHandle;
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
      const BlendStateHandle blendStateHandle = mIdCollectionBlendState.Alloc();
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
                                                 RasterizerState rasterizerState,
                                                 StackFrame frame )
    {
      const RasterizerStateHandle rasterizerStateHandle = mIdCollectionRasterizerState.Alloc();
      CommandDataCreateRasterizerState commandData;
      commandData.mRasterizerState = rasterizerState;
      commandData.mRasterizerStateHandle = rasterizerStateHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateRasterizerState,
                                                frame,
                                                &commandData,
                                                sizeof( commandData ) );
      return rasterizerStateHandle;
    }
    SamplerStateHandle CreateSamplerState( StringView name,
                                           SamplerState samplerState,
                                           StackFrame frame )
    {
      const SamplerStateHandle samplerStateHandle = mIdCollectionSamplerState.Alloc();
      CommandDataCreateSamplerState commandData;
      commandData.mSamplerState = samplerState;
      commandData.mSamplerStateHandle = samplerStateHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateSamplerState,
                                                frame,
                                                &commandData,
                                                sizeof( CommandDataCreateSamplerState ) );
      return samplerStateHandle;
    }
    DepthStateHandle CreateDepthState( StringView name,
                                       DepthState depthState,
                                       StackFrame frame )
    {
      const DepthStateHandle depthStateHandle = mIdCollectionDepthState.Alloc();
      CommandDataCreateDepthState commandData;
      commandData.mDepthState = depthState;
      commandData.mDepthStateHandle = depthStateHandle;
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::CreateDepthState,
                                                frame,
                                                &commandData, sizeof( commandData ) );
      return depthStateHandle;
    }
    VertexFormatHandle CreateVertexFormat( StringView name,
                                           VertexDeclarations vertexDeclarations,
                                           ShaderHandle shaderHandle,
                                           StackFrame frame )
    {
      const VertexFormatHandle vertexFormatHandle = mIdCollectionVertexFormat.Alloc();
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


    void DestroyVertexBuffer( const VertexBufferHandle vertexBufferHandle, const StackFrame frame )
    {
      mIdCollectionVertexBuffer.Free( ( int )vertexBufferHandle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyVertexBuffer,
                                                frame,
                                                &vertexBufferHandle,
                                                sizeof( vertexBufferHandle ) );
    }
    void DestroyIndexBuffer( const IndexBufferHandle indexBufferHandle, const StackFrame frame )
    {
      mIdCollectionIndexBuffer.Free( ( int )indexBufferHandle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyIndexBuffer,
                                                frame,
                                                &indexBufferHandle, sizeof( indexBufferHandle ) );
    }
    void DestroyTexture( TextureHandle handle, StackFrame frame )
    {
      mIdCollectionTexture.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyTexture,
                                                frame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyFramebuffer( FramebufferHandle handle, StackFrame frame )
    {
      mIdCollectionFramebuffer.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyFramebuffer,
                                                frame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyShader( ShaderHandle handle, StackFrame stackFrame )
    {
      mIdCollectionShader.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyShader,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyVertexFormat( VertexFormatHandle handle, StackFrame stackFrame )
    {
      mIdCollectionVertexFormat.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyVertexFormat,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyConstantBuffer( ConstantBufferHandle handle, StackFrame stackFrame )
    {
      mIdCollectionConstantBuffer.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyConstantBuffer,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyDepthState( DepthStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionDepthState.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyDepthState,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyBlendState( BlendStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionBlendState.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyBlendState,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroyRasterizerState( RasterizerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionRasterizerState.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroyRasterizerState,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }
    void DestroySamplerState( SamplerStateHandle handle, StackFrame stackFrame )
    {
      mIdCollectionSamplerState.Free( ( int )handle );
      gSubmitFrame->mCommandBuffer.PushCommand( CommandType::DestroySamplerState,
                                                stackFrame,
                                                &handle,
                                                sizeof( handle ) );
    }



    void UpdateTextureRegion( const TextureHandle handle,
                              const TexUpdate texUpdate,
                              const StackFrame frame )
    {
      const int byteCount = texUpdate.mSrc.mHeight * texUpdate.mPitch;
      CommandDataUpdateTextureRegion commandData;
      commandData.mTexUpdate = texUpdate;
      commandData.mTexUpdate.mSrcBytes = Render::SubmitAlloc( texUpdate.mSrcBytes, byteCount );
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

    void UpdateIndexBuffer( const IndexBufferHandle handle,
                            const void* bytes,
                            const int byteCount,
                            const StackFrame frame )
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

    void UpdateConstantBuffer( const ConstantBufferHandle handle,
                               const void* bytes,
                               const int byteCount,
                               const StackFrame stackFrame )
    {
      TAC_UNUSED_PARAMETER( stackFrame );
      UpdateConstantBufferData updateConstantBufferData;
      updateConstantBufferData.mByteCount = byteCount;
      updateConstantBufferData.mBytes = Render::SubmitAlloc( bytes, byteCount );
      updateConstantBufferData.mConstantBufferHandle = handle;
      gEncoder.mUpdateConstantBuffers.push_back( updateConstantBufferData );
    }

    void SetViewFramebuffer( ViewHandle viewId, FramebufferHandle framebufferHandle )
    {
      View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mFrameBufferHandle = framebufferHandle;
    }

    void SetViewScissorRect( ViewHandle viewId, ScissorRect scissorRect )
    {
      View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mScissorRect = scissorRect;
      view->mScissorSet = true;
    }

    void SetViewport( ViewHandle viewId, Viewport viewport )
    {
      View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mViewport = viewport;
      view->mViewportSet = true;
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

    void SetTexture( DrawCallTextures textureHandle )
    {
      gEncoder.mTextureHandle = textureHandle;
    }

    void BeginGroup( StringView name )
    {
      gSubmitFrame->mUniformBuffer.PushType( UniformBufferEntryType::DebugGroupBegin );
      gSubmitFrame->mUniformBuffer.PushString( name );
    }

    void EndGroup()
    {
      gSubmitFrame->mUniformBuffer.PushType( UniformBufferEntryType::DebugGroupEnd );
    }

    void UpdateConstantBuffer2( ConstantBufferHandle constantBufferHandle,
                                const void* bytes,
                                int byteCount,
                                StackFrame )
    {
      gSubmitFrame->mUniformBuffer.PushType( UniformBufferEntryType::UpdateConstantBuffer );
      gSubmitFrame->mUniformBuffer.PushNumber( ( int )constantBufferHandle );
      gSubmitFrame->mUniformBuffer.PushNumber( byteCount );
      gSubmitFrame->mUniformBuffer.PushData( bytes, byteCount );
    }

    // need lots of comments pls

    // i think these 2 semaphores just ensure that RenderFrame() and SubmitFrame()
    // alternate calls
    static SemaphoreHandle gSubmitSemaphore;
    static SemaphoreHandle gRenderSemaphore;

    void RenderFrame( Errors& errors )
    {
      TAC_ASSERT( gThreadType == ThreadType::Main );
      if( !Renderer::Instance )
        return;

      SemaphoreDecrementWait( gSubmitSemaphore );

      Renderer::Instance->ExecuteCommands( &gRenderFrame->mCommandBuffer, errors );
      TAC_ASSERT( errors.empty() );

      Renderer::Instance->Render2( gRenderFrame, errors );
      TAC_ASSERT( errors.empty() );

      SemaphoreIncrementPost( gRenderSemaphore );

      Renderer::Instance->SwapBuffers();
    }

    static int gFrameCount;
    void SubmitFrame()
    {
      TAC_ASSERT( gThreadType == ThreadType::Stuff );
      SemaphoreDecrementWait( gRenderSemaphore );

      // submit finish
      if( gEncoder.mUniformBufferIndex != gSubmitFrame->mUniformBuffer.size() )
      {
        DrawCall3 drawCall;
        drawCall.mStackFrame = TAC_STACK_FRAME;
        drawCall.iUniformBegin = gEncoder.mUniformBufferIndex;
        drawCall.iUniformEnd = gSubmitFrame->mUniformBuffer.size();
        gSubmitFrame->mDrawCalls.push_back( drawCall );
      }

      Swap( gRenderFrame, gSubmitFrame );
      gSubmitFrame->mCommandBuffer.Resize( 0 );
      gSubmitFrame->mDrawCalls.clear();
      gSubmitFrame->mUniformBuffer.clear();
      gEncoder = Encoder();
      //gSubmitFrame->mDrawCallCount = 0;
      gFrameCount++;

      // submit start

      SemaphoreIncrementPost( gSubmitSemaphore );
    }

    void Init()
    {
      //std::cout << "Render::Init begin" << std::endl;
      gSubmitSemaphore = SemaphoreCreate();
      gRenderSemaphore = SemaphoreCreate();

      // i guess well make render frame go first
      SemaphoreIncrementPost( gSubmitSemaphore );

      //DebugPrintSubmitAllocInfo();
      //std::cout << "Render::Init end" << std::endl;
    }

    void Encoder::Submit( const Render::ViewHandle viewHandle,
                          const StackFrame stackFrame )
    {
      if( gSubmitFrame->mDrawCalls.size() == kDrawCallCapacity )
      {
        OS::DebugBreak();
        return;
      }

      int iUniformBegin = 0;
      int iUniformEnd = 0;
      const int uniformBufferSize = gSubmitFrame->mUniformBuffer.size();
      if( mUniformBufferIndex != uniformBufferSize )
      {
        iUniformBegin = mUniformBufferIndex;
        iUniformEnd = uniformBufferSize;
      }

      DrawCall3 drawCall;
      drawCall.mStackFrame = stackFrame;
      drawCall.mIndexBufferHandle = mIndexBufferHandle;
      drawCall.mVertexBufferHandle = mVertexBufferHandle;
      drawCall.mBlendStateHandle = mBlendStateHandle;
      drawCall.mRasterizerStateHandle = mRasterizerStateHandle;
      drawCall.mSamplerStateHandle = mSamplerStateHandle;
      drawCall.mDepthStateHandle = mDepthStateHandle;
      drawCall.mVertexFormatHandle = mVertexFormatHandle;
      drawCall.mUpdateConstantBuffers = mUpdateConstantBuffers;
      drawCall.mStartIndex = mStartIndex;
      drawCall.mStartVertex = mStartVertex;
      drawCall.mIndexCount = mIndexCount;
      drawCall.mVertexCount = mVertexCount;
      drawCall.mShaderHandle = mShaderHandle;
      drawCall.mTextureHandle = mTextureHandle;
      drawCall.iUniformBegin = iUniformBegin;
      drawCall.iUniformEnd = iUniformEnd;
      drawCall.mViewHandle = viewHandle;
      gSubmitFrame->mDrawCalls.push_back( drawCall );

      mIndexBufferHandle = IndexBufferHandle();
      mVertexBufferHandle = VertexBufferHandle();
      mBlendStateHandle = BlendStateHandle();
      mRasterizerStateHandle = RasterizerStateHandle();
      mSamplerStateHandle = SamplerStateHandle();
      mDepthStateHandle = DepthStateHandle();
      mVertexFormatHandle = VertexFormatHandle();
      mUpdateConstantBuffers.clear();
      mStartIndex = 0;
      mStartVertex = 0;
      mIndexCount = 0;
      mVertexCount = 0;
      mShaderHandle = ShaderHandle();
      mTextureHandle = TextureHandle();
      mUniformBufferIndex = uniformBufferSize;
    }

    void Submit( const Render::ViewHandle viewHandle,
                 const StackFrame stackFrame )
    {
      gEncoder.Submit( viewHandle, stackFrame );
    }

    void GetPerspectiveProjectionAB( float f, float n, float& a, float& b )
    {
      Renderer::Instance->GetPerspectiveProjectionAB( f, n, a, b );
    }

    //void AddDrawCall( const DrawCall2& drawCall )
    //{
    //  Renderer::Instance->mDrawCall2s.push_back( drawCall );
    //}

    void Init( Errors& errors )
    {
      Renderer::Instance->Init( errors );

      //mIdCollectionBlendState.Init( kMaxBlendStates );
      //mIdCollectionConstantBuffer.Init( kMaxConstantBuffers );
      //mIdCollectionDepthState.Init( kMaxDepthStencilStates );
      //mIdCollectionFramebuffer.Init( kMaxFramebuffers );
      //mIdCollectionIndexBuffer.Init( kMaxIndexBuffers );
      //mIdCollectionRasterizerState.Init( kMaxRasterizerStates );
      //mIdCollectionShader.Init( kMaxPrograms );
      //mIdCollectionVertexBuffer.Init( kMaxVertexBuffers );
      //mIdCollectionSamplerState.Init( kMaxSamplerStates );
      //mIdCollectionTexture.Init( kMaxTextures );
      //mIdCollectionVertexFormat.Init( kMaxInputLayouts );
      //mIdCollectionViewId.Init( kMaxViews );
    }

    void Uninit()
    {
      delete Renderer::Instance;
    }

    void ExecuteUniformCommands( const UniformBuffer* uniformBuffer,
                                 const int iUniformBegin,
                                 const int iUniformEnd,
                                 Errors& errors )
    {
      UniformBuffer::Iterator iter( uniformBuffer, iUniformBegin, iUniformEnd );
      while( iter.mCur < iter.mEnd )
      {
        const UniformBufferEntryType type = iter.PopType();
        switch( type )
        {
          case UniformBufferEntryType::DebugGroupBegin:
          {
            const StringView desc = iter.PopString();
            Renderer::Instance->DebugGroupBegin( desc );
          } break;
          case UniformBufferEntryType::DebugMarker:
          {
            const StringView desc = iter.PopString();
            Renderer::Instance->DebugMarker( desc );
          } break;
          case UniformBufferEntryType::DebugGroupEnd:
          {
            Renderer::Instance->DebugGroupEnd();
          } break;
          case UniformBufferEntryType::UpdateConstantBuffer:
          {
            const ConstantBufferHandle constantBufferHandle = { iter.PopNumber() };
            const int byteCount = iter.PopNumber();
            const char* bytes = ( const char* )iter.PopData( byteCount );
            CommandDataUpdateConstantBuffer commandData;
            commandData.mBytes = bytes;
            commandData.mByteCount = byteCount;
            commandData.mConstantBufferHandle = constantBufferHandle;
            Renderer::Instance->UpdateConstantBuffer( &commandData, errors );
          } break;
          TAC_INVALID_DEFAULT_CASE( type )
        }
      }
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

  void Renderer::ExecuteCommands( Render::CommandBuffer* commandBuffer, Errors& errors )
  {

    // factor out this while loop out of rendererDX11 and rendererOGL
    const char* bufferBegin = commandBuffer->Data();
    const char* bufferEnd = bufferBegin + commandBuffer->Size();
    const char* bufferPos = bufferBegin;

    while( bufferPos < bufferEnd )
    {
      auto renderCommandType = Pop<  Render::CommandType >( bufferPos );
      //auto renderCommandType = ( Render::CommandType* )bufferPos;
      //bufferPos += sizeof( Render::CommandType );

      auto stackFrame = Pop<  StackFrame >( bufferPos );

      //auto stackFrame = ( StackFrame* )bufferPos;
      //bufferPos += sizeof( StackFrame );
      TAC_UNUSED_PARAMETER( stackFrame );


      switch( *renderCommandType )
      {
        case Render::CommandType::CreateVertexBuffer:
        {
          if( gVerbose )
            std::cout << "CreateVertexBuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateVertexBuffer >( bufferPos );
          AddVertexBuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateVertexBuffer::End\n";
        } break;

        case Render::CommandType::CreateIndexBuffer:
        {
          if( gVerbose )
            std::cout << "CreateIndexBuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateIndexBuffer >( bufferPos );
          AddIndexBuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateIndexBuffer::End\n";
        } break;

        case Render::CommandType::CreateTexture:
        {
          if( gVerbose )
            std::cout << "CreateTexture::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateTexture >( bufferPos );
          AddTexture( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateTexture::End\n";
        } break;

        case Render::CommandType::CreateFramebuffer:
        {
          if( gVerbose )
            std::cout << "CreateFramebuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateFramebuffer >( bufferPos );
          AddFramebuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateFramebuffer::End\n";
        } break;

        case Render::CommandType::DestroyVertexBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyVertexBuffer::Begin\n";
          auto vertexBufferHandle = PopCommandData< Render::VertexBufferHandle >( bufferPos );
          RemoveVertexBuffer( *vertexBufferHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyVertexBuffer::End\n";
        } break;

        case Render::CommandType::DestroyIndexBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyIndexBuffer::Begin\n";
          auto indexBufferHandle = PopCommandData< Render::IndexBufferHandle >( bufferPos );
          RemoveIndexBuffer( *indexBufferHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyIndexBuffer::End\n";
        } break;

        case Render::CommandType::DestroyTexture:
        {
          if( gVerbose )
            std::cout << "DestroyTexture::Begin\n";
          auto textureHandle = PopCommandData< Render::TextureHandle >( bufferPos );
          RemoveTexture( *textureHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyTexture::End\n";
        } break;

        case Render::CommandType::DestroyFramebuffer:
        {
          if( gVerbose )
            std::cout << "DestroyFramebuffer::Begin\n";
          auto framebufferHandle = PopCommandData< Render::FramebufferHandle >( bufferPos );
          RemoveFramebuffer( *framebufferHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyFramebuffer::End\n";
        } break;

        case Render::CommandType::UpdateTextureRegion:
        {
          if( gVerbose )
            std::cout << "UpdateTextureRegion::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataUpdateTextureRegion >( bufferPos );
          UpdateTextureRegion( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "UpdateTextureRegion::End\n";
        } break;

        case Render::CommandType::UpdateVertexBuffer:
        {
          if( gVerbose )
            std::cout << "UpdateVertexBuffer::Begin\n";


          auto commandData = PopCommandData< Render::CommandDataUpdateVertexBuffer >( bufferPos );


          static bool tryCatchGarbageData = true;
          if( tryCatchGarbageData )
          {
            const float f0 = *( float* )commandData->mBytes;
            const float bound = 10000.0f;
            const bool probablyOk = f0 > -bound && f0 < bound;
            if( !probablyOk )
              OS::DebugBreak();
          }

          UpdateVertexBuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "UpdateVertexBuffer::End\n";
        } break;

        case Render::CommandType::UpdateIndexBuffer:
        {
          if( gVerbose )
            std::cout << "UpdateIndexBuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataUpdateIndexBuffer >( bufferPos );
          UpdateIndexBuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "UpdateIndexBuffer::End\n";
        } break;

        //case Render::CommandType::UpdateConstantBuffer:
        //{
        //  auto index = ( Render::ConstantBufferHandle* )bufferPos;
        //  bufferPos += sizeof( Render::ConstantBufferHandle );
        //  auto commandData = ( Render::CommandDataUpdateBuffer* )bufferPos;
        //  bufferPos += sizeof( Render::CommandDataUpdateBuffer );
        //  PopCheep( bufferPos );
        //  UpdateConstantBuffer( *index, commandData, errors );
        //} break;

        case Render::CommandType::CreateBlendState:
        {
          if( gVerbose )
            std::cout << "CreateBlendState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateBlendState >( bufferPos );
          AddBlendState( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateBlendState::End\n";
        } break;

        case Render::CommandType::CreateConstantBuffer:
        {
          if( gVerbose )
            std::cout << "CreateConstantBuffer::Begin\n";

          auto commandData = PopCommandData< Render::CommandDataCreateConstantBuffer >( bufferPos );
          AddConstantBuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateConstantBuffer::End\n";
        } break;

        case Render::CommandType::CreateDepthState:
        {
          if( gVerbose )
            std::cout << "CreateDepthState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateDepthState >( bufferPos );
          AddDepthState( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateDepthState::End\n";
        } break;

        case Render::CommandType::CreateRasterizerState:
        {
          if( gVerbose )
            std::cout << "CreateRasterizerState::Begin\n";

          auto commandData = PopCommandData< Render::CommandDataCreateRasterizerState >( bufferPos );
          AddRasterizerState( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateRasterizerState::End\n";
        } break;

        case Render::CommandType::CreateSamplerState:
        {
          if( gVerbose )
            std::cout << "CreateSamplerState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateSamplerState >( bufferPos );
          AddSamplerState( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateSamplerState::End\n";
        } break;

        case Render::CommandType::CreateShader:
        {
          if( gVerbose )
            std::cout << "CreateShader::Begin\n";

          auto commandData = PopCommandData< Render::CommandDataCreateShader >( bufferPos );
          AddShader( commandData, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "CreateShader::End\n";
        } break;

        case Render::CommandType::CreateVertexFormat:
        {
          if( gVerbose )
            std::cout << "CreateVertexFormat::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataCreateVertexFormat >( bufferPos );
          AddVertexFormat( commandData, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "CreateVertexFormat::End\n";
        } break;

        case Render::CommandType::DestroyBlendState:
        {
          if( gVerbose )
            std::cout << "DestroyBlendState::Begin\n";
          auto blendStateHandle = PopCommandData< Render::BlendStateHandle >( bufferPos );
          RemoveBlendState( *blendStateHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyBlendState::End\n";
        } break;

        case Render::CommandType::DestroyConstantBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyConstantBuffer::Begin\n";
          auto constantBufferHandle = PopCommandData< Render::ConstantBufferHandle >( bufferPos );
          RemoveConstantBuffer( *constantBufferHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyConstantBuffer::End\n";
        } break;

        case Render::CommandType::DestroyDepthState:
        {
          if( gVerbose )
            std::cout << "DestroyDepthState::Begin\n";
          auto depthStateHandle = PopCommandData< Render::DepthStateHandle >( bufferPos );
          RemoveDepthState( *depthStateHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyDepthState::End\n";
        } break;

        case Render::CommandType::DestroyRasterizerState:
        {
          if( gVerbose )
            std::cout << "DestroyRasterizerState::Begin\n";
          auto rasterizerStateHandle = PopCommandData< Render::RasterizerStateHandle >( bufferPos );
          RemoveRasterizerState( *rasterizerStateHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyRasterizerState::End\n";
        } break;

        case Render::CommandType::DestroySamplerState:
        {
          if( gVerbose )
            std::cout << "DestroySamplerState::Begin\n";

          auto samplerStateHandle = PopCommandData< Render::SamplerStateHandle >( bufferPos );
          RemoveSamplerState( *samplerStateHandle, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "DestroySamplerState::End\n";
        } break;

        case Render::CommandType::DestroyShader:
        {
          if( gVerbose )
            std::cout << "DestroyShader::Begin\n";
          auto shaderHandle = PopCommandData< Render::ShaderHandle >( bufferPos );
          RemoveShader( *shaderHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyShader::End\n";
        } break;

        case Render::CommandType::DestroyVertexFormat:
        {
          if( gVerbose )
            std::cout << "DestroyVertexFormat::Begin\n";
          auto vertexFormatHandle = PopCommandData< Render::VertexFormatHandle >( bufferPos );
          RemoveVertexFormat( *vertexFormatHandle, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyVertexFormat::End\n";
        } break;

        case Render::CommandType::ResizeFramebuffer:
        {
          if( gVerbose )
            std::cout << "ResizeFramebuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataResizeFramebuffer >( bufferPos );
          ResizeFramebuffer( commandData, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "ResizeFramebuffer::End\n";
        } break;

        TAC_INVALID_DEFAULT_CASE( *renderCommandType );
      }
    }
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
