#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/tacIDCollection.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/tacOS.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/shell/tacDesktopApp.h"

#include <iostream>
#include <ctime>

static uint32_t gRaven = 0xcaacaaaa;
namespace Tac
{
  static bool gVerbose;

  struct CommandBufferIterator
  {
    bool IsValid()
    {
      return bufferPos < bufferEnd;
    }

    void* PopBytes( int n )
    {
      if( bufferPos + n > bufferEnd )
        return nullptr;
      void* result = ( void* )bufferPos;
      bufferPos += n;
      return result;
    }

    template< typename T >
    T* Pop()
    {
      return ( T* )PopBytes( sizeof( T ) );
    }

    template< typename T >
    T* PopCommandData()
    {
      auto result = Pop< T >();
      auto end = ( const char* )PopBytes( 3 );
      TAC_ASSERT( MemCmp( end, "end", 3 ) == 0 );
      return result;
    }

    const char* bufferBegin;
    const char* bufferEnd;
    const char* bufferPos;
  };

  namespace Render
  {

    static void ExecuteUniformCommands( const UniformBuffer* uniformBuffer,
                                        const int iUniformBegin,
                                        const int iUniformEnd,
                                        Errors& errors )
    {
      UniformBuffer::Iterator iter( uniformBuffer, iUniformBegin, iUniformEnd );
      while( iter.mCur < iter.mEnd )
      {
        const UniformBufferHeader header = iter.PopHeader();
        TAC_ASSERT( header.mCorruption == UniformBufferHeader().mCorruption );
        switch( header.mType )
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
          default: TAC_CRITICAL_ERROR_INVALID_CASE( header.mType ); return;
        }
      }
    }



    UniformBufferHeader::UniformBufferHeader( const UniformBufferEntryType type,
                                              const StackFrame stackFrame )
    {
      mType = type;
      mStackFrame = stackFrame;
    }

    UniformBuffer::Pusher* UniformBuffer::PushHeader( const UniformBufferHeader header )
    {
      static thread_local struct UniformBufferPusher : public UniformBuffer::Pusher
      {
        Pusher*        PushData( const void* bytes, const int byteCount ) override
        {
          MemCpy( mUniformBuffer->mBytes + mUniformBuffer->mByteCount, bytes, byteCount );
          mUniformBuffer->mByteCount += byteCount;
          return this;
        }
        Pusher*        PushString( const StringView s ) override
        {
          PushNumber( s.size() );
          PushData( s.c_str(), s.size() );
          PushData( "", 1 );
          return this;
        }
        Pusher*        PushNumber( const int i ) override
        {
          return PushData( &i, sizeof( i ) );
        }
        UniformBuffer* mUniformBuffer;
      } sPusher;
      sPusher.mUniformBuffer = this;
      return sPusher.PushData( &header, sizeof( header ) );
    }

    int                    UniformBuffer::size() const
    {
      return mByteCount;
    }

    void*                  UniformBuffer::data() const
    {
      return ( void* )mBytes;
    }

    void                   UniformBuffer::clear()
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

    UniformBufferHeader UniformBuffer::Iterator::PopHeader()
    {
      return *( UniformBufferHeader* )PopData( sizeof( UniformBufferHeader ) );
    }

    void*               UniformBuffer::Iterator::PopData( const int byteCount )
    {
      auto result = ( void* )mCur;
      mCur += byteCount;
      return result;
    }

    int                 UniformBuffer::Iterator::PopNumber()
    {
      return *( int* )PopData( sizeof( int ) );
    }

    StringView          UniformBuffer::Iterator::PopString()
    {
      const int len = PopNumber();
      const char* str = ( const char* )PopData( len );
      PopData( 1 );
      return StringView( str, len );
    }

    void        CommandBuffer::Push( const void* bytes,
                                     const int byteCount )
    {
      const int bufferSize = mBuffer.size();
      mBuffer.resize( mBuffer.size() + byteCount );
      MemCpy( mBuffer.data() + bufferSize, bytes, byteCount );
    }

    void        CommandBuffer::PushCommand( const CommandType type,
                                            const void* bytes,
                                            const int byteCount )
    {
      StringView cheep( "end" );
      Push( &type, sizeof( CommandType ) );
      Push( bytes, byteCount );
      Push( cheep.data(), cheep.size() );
    }

    void        CommandBuffer::Resize( int newSize ) { mBuffer.resize( newSize ); }
    void        CommandBuffer::Clear() { mBuffer.clear(); }
    const char* CommandBuffer::Data() const { return mBuffer.data(); }
    int         CommandBuffer::Size() const { return mBuffer.size(); }

    static Frame    gFrames[ 2 ];
    static Frame*   gRenderFrame = &gFrames[ 0 ];
    static Frame*   gSubmitFrame = &gFrames[ 1 ];
    static uint32_t gCrow = 0xcaacaaaa;
    //int i = gCrow + 1;

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
    static IdCollection mIdCollectionMagicBuffer( kMaxMagicBuffers );
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
      int                   mUniformBufferIndex = 0;
      DrawCall3             mDrawCall;
    };

    static thread_local Encoder gEncoder;

    static const int gSubmitRingBufferCapacity = 100 * 1024 * 1024;
    static char      gSubmitRingBufferBytes[ gSubmitRingBufferCapacity ];
    static int       gSubmitRingBufferPos;

    void        DebugPrintSubmitAllocInfo()
    {
      std::cout << "gSubmitRingBufferCapacity: " << gSubmitRingBufferCapacity << std::endl;
      std::cout << "gSubmitRingBufferBytes: " << ( void* )gSubmitRingBufferBytes << std::endl;
      std::cout << "gSubmitRingBufferPos : " << gSubmitRingBufferPos << std::endl;
    }

    bool        IsSubmitAllocated( const void* data )
    {
      const bool result =
        data >= gSubmitRingBufferBytes &&
        data < gSubmitRingBufferBytes + gSubmitRingBufferCapacity;
      return result;
    }

    void*       SubmitAlloc( const int byteCount )
    {
      if( !byteCount )
        return nullptr;
      const int beginPos = gSubmitRingBufferPos + byteCount > gSubmitRingBufferCapacity
        ? 0
        : gSubmitRingBufferPos;
      gSubmitRingBufferPos = beginPos + byteCount;
      return gSubmitRingBufferBytes + beginPos;
    }

    const void* SubmitAlloc( const void* bytes, const int byteCount )
    {
      if( !bytes )
        return nullptr;
      if( IsSubmitAllocated( bytes ) )
        return bytes;
      void* dst = SubmitAlloc( byteCount );
      MemCpy( dst, bytes, byteCount );
      return dst;
    }

    StringView  SubmitAlloc( const StringView stringView )
    {
      if( !stringView.mLen )
        return {};

      if( IsSubmitAllocated( stringView.data() ) )
        return stringView;

      void* resultData = SubmitAlloc( stringView.size() + 1 );
      MemCpy( resultData, stringView.c_str(), stringView.mLen );
      ( ( char* )resultData )[ stringView.mLen ] = '\0';
      return StringView( ( const char* )resultData, stringView.size() );
    }

    //void SubmitAllocBeginFrame()
    //{
    //}

    ViewHandle            CreateView()
    {
      return { mIdCollectionViewId.Alloc() };
    }

    ShaderHandle          CreateShader( const ShaderSource shaderSource,
                                        const ConstantBuffers constantBuffers,
                                        const StackFrame stackFrame )
    {
      TAC_ASSERT( constantBuffers.size() );
      const ShaderHandle shaderHandle = { mIdCollectionShader.Alloc() };
      CommandDataCreateShader commandData;
      commandData.mShaderSource.mStr = SubmitAlloc( shaderSource.mStr ).c_str();
      commandData.mShaderSource.mType = shaderSource.mType;
      commandData.mShaderHandle = shaderHandle;
      commandData.mConstantBuffers = constantBuffers;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateShader,
                                                          &commandData,
                                                          sizeof( CommandDataCreateShader ) );
      return shaderHandle;
    }

    VertexBufferHandle    CreateVertexBuffer( const int byteCount,
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
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateVertexBuffer,
                                                          &commandData,
                                                          sizeof( CommandDataCreateVertexBuffer ) );
      return vertexBufferHandle;
    }

    MagicBufferHandle                CreateMagicBuffer( const int byteCount,
                                                        //const void* mOptionalInitialBytes,
                                                        const int stride,
                                                        const Access access,
                                                        const StackFrame stackFrame )
    {
      TAC_UNUSED_PARAMETER( access );
      const MagicBufferHandle magicBufferHandle = { mIdCollectionMagicBuffer.Alloc() };
      CommandDataCreateMagicBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mStackFrame = stackFrame;
      commandData.mMagicBufferHandle = magicBufferHandle;
      commandData.mStride = stride;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateMagicBuffer,
                                                          &commandData,
                                                          sizeof( CommandDataCreateMagicBuffer ) );
      return magicBufferHandle;
    }

    ConstantBufferHandle  CreateConstantBuffer( const int byteCount,
                                                const int shaderRegister,
                                                const StackFrame stackFrame )
    {
      const ConstantBufferHandle constantBufferHandle = { mIdCollectionConstantBuffer.Alloc() };
      CommandDataCreateConstantBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mShaderRegister = shaderRegister;
      commandData.mConstantBufferHandle = constantBufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateConstantBuffer,
                                                          &commandData,
                                                          sizeof( CommandDataCreateConstantBuffer ) );
      return constantBufferHandle;
    }

    IndexBufferHandle     CreateIndexBuffer( const int byteCount,
                                             const void* optionalInitialBytes,
                                             const Access access,
                                             const Format format,
                                             const StackFrame stackFrame )
    {
      const IndexBufferHandle indexBufferHandle = { mIdCollectionIndexBuffer.Alloc() };
      CommandDataCreateIndexBuffer commandData;
      commandData.mByteCount = byteCount;
      commandData.mOptionalInitialBytes = SubmitAlloc( optionalInitialBytes, byteCount );
      commandData.mAccess = access;
      commandData.mFormat = format;
      commandData.mIndexBufferHandle = indexBufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateIndexBuffer,
                                                          &commandData,
                                                          sizeof( commandData ) );
      return indexBufferHandle;
    }

    TextureHandle         CreateTexture( TexSpec texSpec,
                                         const StackFrame stackFrame )
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
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateTexture,
                                                          &commandData,
                                                          sizeof( CommandDataCreateTexture ) );
      return textureHandle;
    }


    FramebufferHandle     CreateFramebufferForWindow( const void* nativeWindowHandle,
                                                      const int width,
                                                      const int height,
                                                      const StackFrame stackFrame )
    {
      const FramebufferHandle framebufferHandle = mIdCollectionFramebuffer.Alloc();
      CommandDataCreateFramebuffer commandData;
      commandData.mNativeWindowHandle = nativeWindowHandle;
      commandData.mWidth = width;
      commandData.mHeight = height;
      commandData.mFramebufferHandle = framebufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateFramebuffer,
                                                          &commandData,
                                                          sizeof( CommandDataCreateFramebuffer ) );
      return framebufferHandle;
    }

    FramebufferHandle     CreateFramebufferForRenderToTexture( FramebufferTextures framebufferTextures,
                                                               const StackFrame stackFrame )
    {
      const FramebufferHandle framebufferHandle = mIdCollectionFramebuffer.Alloc();
      CommandDataCreateFramebuffer commandData;
      commandData.mFramebufferHandle = framebufferHandle;
      commandData.mStackFrame = stackFrame;
      commandData.mFramebufferTextures = framebufferTextures;
      //commandData.mTextureCount = textureHandleCount;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateFramebuffer,
                                                          &commandData,
                                                          sizeof( CommandDataCreateFramebuffer ) );
      return framebufferHandle;
    }

    BlendStateHandle      CreateBlendState( const BlendState blendState,
                                            const StackFrame stackFrame )
    {
      const BlendStateHandle blendStateHandle = mIdCollectionBlendState.Alloc();
      CommandDataCreateBlendState commandData;
      commandData.mBlendState = blendState;
      commandData.mBlendStateHandle = blendStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateBlendState,
                                                          &commandData,
                                                          sizeof( CommandDataCreateBlendState ) );
      return blendStateHandle;
    }

    RasterizerStateHandle CreateRasterizerState( const RasterizerState rasterizerState,
                                                 const StackFrame stackFrame )
    {
      const RasterizerStateHandle rasterizerStateHandle = mIdCollectionRasterizerState.Alloc();
      CommandDataCreateRasterizerState commandData;
      commandData.mRasterizerState = rasterizerState;
      commandData.mRasterizerStateHandle = rasterizerStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateRasterizerState,
                                                          &commandData,
                                                          sizeof( commandData ) );
      return rasterizerStateHandle;
    }

    SamplerStateHandle    CreateSamplerState( const SamplerState samplerState,
                                              const StackFrame stackFrame )
    {
      const SamplerStateHandle samplerStateHandle = mIdCollectionSamplerState.Alloc();
      CommandDataCreateSamplerState commandData;
      commandData.mSamplerState = samplerState;
      commandData.mSamplerStateHandle = samplerStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateSamplerState,
                                                          &commandData,
                                                          sizeof( CommandDataCreateSamplerState ) );
      return samplerStateHandle;
    }

    DepthStateHandle      CreateDepthState( const DepthState depthState,
                                            const StackFrame stackFrame )
    {
      const DepthStateHandle depthStateHandle = mIdCollectionDepthState.Alloc();
      CommandDataCreateDepthState commandData;
      commandData.mDepthState = depthState;
      commandData.mDepthStateHandle = depthStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateDepthState,
                                                          &commandData,
                                                          sizeof( commandData ) );
      return depthStateHandle;
    }

    VertexFormatHandle    CreateVertexFormat( const VertexDeclarations vertexDeclarations,
                                              const ShaderHandle shaderHandle,
                                              const StackFrame stackFrame )
    {
      const VertexFormatHandle vertexFormatHandle = mIdCollectionVertexFormat.Alloc();
      CommandDataCreateVertexFormat commandData;
      commandData.mShaderHandle = shaderHandle;
      commandData.mVertexDeclarations = vertexDeclarations;
      commandData.mVertexFormatHandle = vertexFormatHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::CreateVertexFormat,
                                                          &commandData,
                                                          sizeof( CommandDataCreateVertexFormat ) );
      return vertexFormatHandle;
    }

    static void SetRenderObjectDebugName( CommandDataSetRenderObjectDebugName& commandData, const char* name )
    {
      commandData.mName = SubmitAlloc( name );
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::SetRenderObjectDebugName,
                                                          &commandData,
                                                          sizeof( commandData ) );
    }

    void SetRenderObjectDebugName( const IndexBufferHandle indexBufferHandle, const char* name )
    {
      CommandDataSetRenderObjectDebugName commandData;
      commandData.mIndexBufferHandle = indexBufferHandle;
      SetRenderObjectDebugName( commandData, name );
    }

    void SetRenderObjectDebugName( const VertexBufferHandle vertexBufferHandle, const char* name )
    {
      CommandDataSetRenderObjectDebugName commandData;
      commandData.mVertexBufferHandle = vertexBufferHandle;
      SetRenderObjectDebugName( commandData, name );
    }

    void SetRenderObjectDebugName( const TextureHandle textureHandle, const char* name )
    {
      CommandDataSetRenderObjectDebugName commandData;
      commandData.mTextureHandle = textureHandle;
      SetRenderObjectDebugName( commandData, name );
    }

    template< typename T, int N >
    static void FinishFreeingHandle( FixedVector< T, N >& freed, IdCollection& collection )
    {
      if( freed.empty() )
        return;
      for( auto handle : freed )
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

    void DestroyView( const ViewHandle viewHandle )
    {
      gSubmitFrame->mFreeDeferredHandles.mFreedViews.push_back( viewHandle );
    }

    void DestroyVertexBuffer( const VertexBufferHandle vertexBufferHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )vertexBufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedVertexBuffers.push_back( vertexBufferHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyVertexBuffer,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyIndexBuffer( const IndexBufferHandle indexBufferHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )indexBufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedIndexBuffers.push_back( indexBufferHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyIndexBuffer,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyTexture( const TextureHandle textureHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )textureHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedTextures.push_back( textureHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyTexture,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyFramebuffer( const FramebufferHandle framebufferHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )framebufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedFramebuffers.push_back( framebufferHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyFramebuffer,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyShader( const ShaderHandle shaderHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )shaderHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedShaders.push_back( shaderHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyShader,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyVertexFormat( const VertexFormatHandle vertexFormatHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )vertexFormatHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedVertexFormatInputLayouts.push_back( vertexFormatHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyVertexFormat,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyConstantBuffer( const ConstantBufferHandle constantBufferHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )constantBufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedConstantBuffers.push_back( constantBufferHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyConstantBuffer,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyDepthState( const DepthStateHandle depthStateHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )depthStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedDepthStencilStates.push_back( depthStateHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyDepthState,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyBlendState( const BlendStateHandle blendStateHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )blendStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedBlendStates.push_back( blendStateHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyBlendState,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroyRasterizerState( const RasterizerStateHandle rasterizerStateHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )rasterizerStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedRasterizerStates.push_back( rasterizerStateHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroyRasterizerState,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }

    void DestroySamplerState( const SamplerStateHandle samplerStateHandle, const StackFrame stackFrame )
    {
      CommandDataDestroy commandData;
      commandData.mIndex = ( int )samplerStateHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mFreeDeferredHandles.mFreedSamplerStates.push_back( samplerStateHandle );
      gSubmitFrame->mCommandBufferFrameEnd.PushCommand( CommandType::DestroySamplerState,
                                                        &commandData,
                                                        sizeof( commandData ) );
    }


    void                  ResizeFramebuffer( const FramebufferHandle framebufferHandle,
                                             const int w,
                                             const int h,
                                             const StackFrame stackFrame )
    {
      CommandDataResizeFramebuffer commandData;
      commandData.mWidth = w;
      commandData.mHeight = h;
      commandData.mFramebufferHandle = framebufferHandle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::ResizeFramebuffer,
                                                          &commandData,
                                                          sizeof( CommandDataResizeFramebuffer ) );
    }

    void UpdateTextureRegion( const TextureHandle handle,
                              const TexUpdate texUpdate,
                              const StackFrame stackFrame )
    {
      const int byteCount = texUpdate.mSrc.mHeight * texUpdate.mPitch;
      CommandDataUpdateTextureRegion commandData;
      commandData.mTexUpdate = texUpdate;
      commandData.mTexUpdate.mSrcBytes = Render::SubmitAlloc( texUpdate.mSrcBytes, byteCount );
      commandData.mTextureHandle = handle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::UpdateTextureRegion,
                                                          &commandData,
                                                          sizeof( CommandDataUpdateTextureRegion ) );
    }

    void UpdateVertexBuffer( const VertexBufferHandle handle,
                             const void* bytes,
                             const int byteCount,
                             const StackFrame stackFrame )
    {
      CommandDataUpdateVertexBuffer commandData;
      commandData.mBytes = Render::SubmitAlloc( bytes, byteCount );
      commandData.mByteCount = byteCount;
      commandData.mVertexBufferHandle = handle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::UpdateVertexBuffer,
                                                          &commandData,
                                                          sizeof( CommandDataUpdateVertexBuffer ) );
    }

    void UpdateIndexBuffer( const IndexBufferHandle handle,
                            const void* bytes,
                            const int byteCount,
                            const StackFrame stackFrame )
    {
      CommandDataUpdateIndexBuffer commandData;
      commandData.mBytes = Render::SubmitAlloc( bytes, byteCount );
      commandData.mByteCount = byteCount;
      commandData.mIndexBufferHandle = handle;
      commandData.mStackFrame = stackFrame;
      gSubmitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::UpdateIndexBuffer,
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
      updateConstantBufferData.mStackFrame = stackFrame;
      gEncoder.mDrawCall.mUpdateConstantBuffers.push_back( updateConstantBufferData );
    }

    void SetViewFramebuffer( const ViewHandle viewId, const FramebufferHandle framebufferHandle )
    {
      TAC_ASSERT( ( unsigned )viewId < ( unsigned )kMaxViews )
        View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mFrameBufferHandle = framebufferHandle;
    }

    void SetViewScissorRect( const ViewHandle viewId, const ScissorRect scissorRect )
    {
      TAC_ASSERT( ( unsigned )viewId < ( unsigned )kMaxViews )
        View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mScissorRect = scissorRect;
      view->mScissorSet = true;
    }

    void SetViewport( const ViewHandle viewId, const Viewport viewport )
    {
      TAC_ASSERT( ( unsigned )viewId < ( unsigned )kMaxViews )
        View* view = &gSubmitFrame->mViews[ ( int )viewId ];
      view->mViewport = viewport;
      view->mViewportSet = true;
    }

    void SetShader( const ShaderHandle shaderHandle )
    {
      gEncoder.mDrawCall.mShaderHandle = shaderHandle;
    }

    void SetVertexBuffer( const VertexBufferHandle vertexBufferHandle, const int startVertex, const int vertexCount )
    {
      gEncoder.mDrawCall.mVertexBufferHandle = vertexBufferHandle;
      gEncoder.mDrawCall.mStartVertex = startVertex;
      gEncoder.mDrawCall.mVertexCount = vertexCount;
    }

    void SetIndexBuffer( const IndexBufferHandle indexBufferHandle, const int startIndex, const int indexCount )
    {
      gEncoder.mDrawCall.mIndexBufferHandle = indexBufferHandle;
      gEncoder.mDrawCall.mStartIndex = startIndex;
      gEncoder.mDrawCall.mIndexCount = indexCount;
    }

    void SetBlendState( const BlendStateHandle blendStateHandle )
    {
      gEncoder.mDrawCall.mBlendStateHandle = blendStateHandle;
    }

    void SetRasterizerState( const RasterizerStateHandle rasterizerStateHandle )
    {
      gEncoder.mDrawCall.mRasterizerStateHandle = rasterizerStateHandle;
    }

    void SetSamplerState( const SamplerStateHandle samplerStateHandle )
    {
      gEncoder.mDrawCall.mSamplerStateHandle = samplerStateHandle;
    }

    void SetDepthState( const DepthStateHandle depthStateHandle )
    {
      gEncoder.mDrawCall.mDepthStateHandle = depthStateHandle;
    }

    void SetVertexFormat( const VertexFormatHandle vertexFormatHandle )
    {
      gEncoder.mDrawCall.mVertexFormatHandle = vertexFormatHandle;
    }

    void SetTexture( const DrawCallTextures textureHandle )
    {
      gEncoder.mDrawCall.mTextureHandle = textureHandle;
    }

    //void SetTexture( TextureHandle textureHandle )
    //{
    //  gEncoder.mTextureHandle = textureHandle;
    //}

    void BeginGroup( const StringView name, const StackFrame stackFrame )
    {
      TAC_ASSERT( !name.empty() );
      UniformBufferHeader header( UniformBufferEntryType::DebugGroupBegin, stackFrame );
      gSubmitFrame->mUniformBuffer.
        PushHeader( header )->
        PushString( name );
    }

    void EndGroup( const StackFrame stackFrame )
    {
      UniformBufferHeader header( UniformBufferEntryType::DebugGroupEnd, stackFrame );
      gSubmitFrame->mUniformBuffer.PushHeader( header );
    }

    void UpdateConstantBuffer2( const ConstantBufferHandle constantBufferHandle,
                                const void* bytes,
                                const int byteCount,
                                const StackFrame stackFrame )
    {
      UniformBufferHeader header( UniformBufferEntryType::UpdateConstantBuffer, stackFrame );
      gSubmitFrame->mUniformBuffer.
        PushHeader( header )->
        PushNumber( ( int )constantBufferHandle )->
        PushNumber( byteCount )->
        PushData( bytes, byteCount );
    }

    // need lots of comments pls

    // i think these 2 semaphores just ensure that RenderFrame() and SubmitFrame()
    // alternate calls
    static SemaphoreHandle gSubmitSemaphore;
    static SemaphoreHandle gRenderSemaphore;

    static void RenderDrawCalls( Errors& errors )
    {
      TAC_PROFILE_BLOCK;
      const DrawCall3* drawCallBegin = gRenderFrame->mDrawCalls.data();
      const int drawCallCount = gRenderFrame->mDrawCalls.size();
      for( int iDrawCall = 0; iDrawCall < drawCallCount; ++iDrawCall )
      {
        if( gRenderFrame->mBreakOnDrawCall == iDrawCall )
          OSDebugBreak();

        const DrawCall3* drawCall = drawCallBegin + iDrawCall;

        ExecuteUniformCommands( &gRenderFrame->mUniformBuffer,
                                drawCall->iUniformBegin,
                                drawCall->iUniformEnd,
                                errors );
        Renderer::Instance->RenderDrawCall( gRenderFrame, drawCall, errors );
      }

    }

    void RenderFrame( Errors& errors )
    {
      TAC_PROFILE_BLOCK;
      TAC_ASSERT( IsMainThread() );
      if( !Renderer::Instance )
        return;

      {
        TAC_PROFILE_BLOCK_NAMED( "wait submit" );
        OSSemaphoreDecrementWait( gSubmitSemaphore );
      }

      if( gRenderFrame->mBreakOnFrameRender )
        OSDebugBreak();

      Renderer::Instance->ExecuteCommands( &gRenderFrame->mCommandBufferFrameBegin, errors );
      TAC_HANDLE_ERROR( errors );

      Renderer::Instance->RenderBegin( gRenderFrame, errors );

      RenderDrawCalls( errors );
      TAC_HANDLE_ERROR( errors );

      Renderer::Instance->ExecuteCommands( &gRenderFrame->mCommandBufferFrameEnd, errors );
      TAC_HANDLE_ERROR( errors );

      Renderer::Instance->RenderEnd( gRenderFrame, errors );
      TAC_HANDLE_ERROR( errors );

      OSSemaphoreIncrementPost( gRenderSemaphore );

      {
        TAC_PROFILE_BLOCK_NAMED( "swap buffers" );
        Renderer::Instance->SwapBuffers();
      }
    }

    static int gFrameCount;

    Frame::Frame()
    {
      Clear();
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
    void SubmitFrame()
    {
      TAC_ASSERT( IsLogicThread() );

      // Finish submitting this frame
      {
        TAC_PROFILE_BLOCK_NAMED( "wait render" );
        OSSemaphoreDecrementWait( gRenderSemaphore );
      }

      gSubmitFrame->mFreeDeferredHandles.FinishFreeingHandles();

      if( gEncoder.mUniformBufferIndex != gSubmitFrame->mUniformBuffer.size() )
      {
        DrawCall3 drawCall;
        drawCall.mStackFrame = TAC_STACK_FRAME;
        drawCall.iUniformBegin = gEncoder.mUniformBufferIndex;
        drawCall.iUniformEnd = gSubmitFrame->mUniformBuffer.size();
        gSubmitFrame->mDrawCalls.push_back( drawCall );
      }

      Swap( gRenderFrame, gSubmitFrame );
      gSubmitFrame->Clear();
      gEncoder = Encoder();
      gFrameCount++;

      // Start submitting the next frame
      OSSemaphoreIncrementPost( gSubmitSemaphore );
    }

    void Init( Errors& errors )
    {
      gSubmitSemaphore = OSSemaphoreCreate();
      gRenderSemaphore = OSSemaphoreCreate();

      // i guess well make render frame go first
      OSSemaphoreIncrementPost( gSubmitSemaphore );

      Renderer::Instance->Init( errors );
    }

    void Encoder::Submit( const Render::ViewHandle viewHandle,
                          const StackFrame stackFrame )
    {
      if( gSubmitFrame->mDrawCalls.size() == kDrawCallCapacity )
      {
        OSDebugBreak();
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

      mDrawCall.mStackFrame = stackFrame;
      mDrawCall.iUniformBegin = iUniformBegin;
      mDrawCall.iUniformEnd = iUniformEnd;
      mDrawCall.mViewHandle = viewHandle;
      gSubmitFrame->mDrawCalls.push_back( mDrawCall );

      // Prepare the next draw
      mDrawCall = DrawCall3();
      mUniformBufferIndex = uniformBufferSize;
    }

    void Submit( const Render::ViewHandle viewHandle,
                 const StackFrame stackFrame )
    {
      gEncoder.Submit( viewHandle, stackFrame );
    }

    void GetPerspectiveProjectionAB( const float f, const float n, float& a, float& b )
    {
      Renderer::Instance->GetPerspectiveProjectionAB( f, n, a, b );
    }


    void Uninit()
    {
      delete Renderer::Instance;
    }


    void                             SetBreakpointWhenThisFrameIsRendered()
    {
      gSubmitFrame->mBreakOnFrameRender = true;

    }
    void                             SetBreakpointWhenNextDrawCallIsExecuted()
    {
      gSubmitFrame->mBreakOnDrawCall = gSubmitFrame->mDrawCalls.size();
    }


    static struct ShaderReloadInfo
    {
      String      mFullPath;
      std::time_t mFileModifyTime = 0;
    } sShaderReloadInfos[ kMaxPrograms ];

    static std::time_t ShaderReloadGetFileModifyTime( const char* path )
    {
      std::time_t fileModifyTime = 0;
      for( ;; )
      {
        Errors fileModifyErrors;
        OSGetFileLastModifiedTime( &fileModifyTime, path, fileModifyErrors );

        if( fileModifyErrors )
          OSDebugPopupBox( fileModifyErrors.ToString() );
        else
          break;
      }

      TAC_ASSERT( fileModifyTime );
      return fileModifyTime;
    }

    void               ShaderReloadHelperAdd( ShaderHandle shaderHandle, const char* fullPath )
    {
      if( !IsDebugMode() )
        return;
      ShaderReloadInfo* info = &sShaderReloadInfos[ ( int )shaderHandle ];
      info->mFullPath = fullPath;
      info->mFileModifyTime = ShaderReloadGetFileModifyTime( fullPath );
    }
    void               ShaderReloadHelperRemove( const Render::ShaderHandle shaderHandle )
    {
      if( !IsDebugMode() )
        return;
      sShaderReloadInfos[ ( int )shaderHandle ] = ShaderReloadInfo();
    }

    static void        ShaderReloadHelperUpdateAux( ShaderReloadInfo* shaderReloadInfo,
                                                    ShaderReloadFunction* shaderReloadFunction )
    {
      if( !shaderReloadInfo->mFileModifyTime )
        return;
      const std::time_t fileModifyTime = ShaderReloadGetFileModifyTime( shaderReloadInfo->mFullPath );
      if( fileModifyTime == shaderReloadInfo->mFileModifyTime )
        return;
      shaderReloadInfo->mFileModifyTime = fileModifyTime;
      shaderReloadFunction( Render::ShaderHandle( ( int )( shaderReloadInfo - sShaderReloadInfos ) ), shaderReloadInfo->mFullPath.c_str() );
    }

    void               ShaderReloadHelperUpdate( ShaderReloadFunction* shaderReloadFunction )
    {
      if( !IsDebugMode() )
        return;
      static double lastUpdateSeconds;

      const double curSec = ShellGetElapsedSeconds();
      if( curSec - lastUpdateSeconds < 0.5f )
        return;
      lastUpdateSeconds = curSec;
      for( ShaderReloadInfo& shaderReloadInfo : sShaderReloadInfos )
        ShaderReloadHelperUpdateAux( &shaderReloadInfo, shaderReloadFunction );
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

  struct CommandHandlerBase
  {
    virtual void Invoke( Renderer*, CommandBufferIterator*, Errors& ) = 0;
    const char* mName;
  };

  template< typename CommandData >
  struct CommandHandler : public CommandHandlerBase
  {
    CommandHandler( void ( Renderer::* callback )( CommandData*, Errors& ) ) : mCallback( callback ) {}

    static CommandHandler* Instance()
    {
      static CommandHandler sCommandHandler;
      return &sCommandHandler;
    }

    void Invoke( Renderer* renderer,
                 CommandBufferIterator* commandBufferIterator,
                 Errors& errors ) override
    {
      auto commandData = commandBufferIterator->PopCommandData< CommandData >();
      ( renderer->*mCallback )( commandData, errors );
      TAC_HANDLE_ERROR( errors );
    }

    void ( Renderer::* mCallback )( CommandData*, Errors& );
  };

  template< typename Handle >
  struct CommandHandlerDestroy : public CommandHandlerBase
  {

    CommandHandlerDestroy( void ( Renderer::* callback )( Handle, Errors& ) ) : mCallback( callback ){}
    void Invoke( Renderer* renderer,
                 CommandBufferIterator* commandBufferIterator,
                 Errors& errors ) override
    {
      auto commandData = commandBufferIterator->PopCommandData< Render::CommandDataDestroy >();
      ( renderer->*mCallback )( Handle( commandData->mIndex ), errors );
      TAC_HANDLE_ERROR( errors );
    }

    void ( Renderer::* mCallback )( Handle, Errors& );
  };

  static struct CommandHandlers
  {
    void Add( const Render::CommandType commandType, CommandHandlerBase* commandHandler, const char* name )
    {
      commandHandler->mName = name;
      mCommandHandlers[ ( int )commandType ] = commandHandler;
    }

    void Invoke( Renderer* renderer,
                 const Render::CommandType comandType,
                 CommandBufferIterator* commandBufferIterator,
                 Errors& errors )
    {
      TAC_ASSERT_INDEX( comandType, Render::CommandType::Count );
      CommandHandlerBase* commandHandler = mCommandHandlers[ ( int )comandType ];
      if( gVerbose )
        std::cout << commandHandler->mName << "::End\n";
      commandHandler->Invoke( renderer, commandBufferIterator, errors );
      if( gVerbose )
        std::cout << commandHandler->mName << "::Begin\n";
    }

    CommandHandlers()
    {
      static CommandHandler< Render::CommandDataCreateBlendState > createBlendState( &Renderer::AddBlendState );
      static CommandHandler< Render::CommandDataCreateConstantBuffer > createConstantBuffer( &Renderer::AddConstantBuffer );
      static CommandHandler< Render::CommandDataCreateDepthState > createDepthState( &Renderer::AddDepthState );
      static CommandHandler< Render::CommandDataCreateFramebuffer > createFramebuffer( &Renderer::AddFramebuffer );
      static CommandHandler< Render::CommandDataCreateIndexBuffer > createIndexBuffer( &Renderer::AddIndexBuffer );
      static CommandHandler< Render::CommandDataCreateMagicBuffer > createMagicBuffer( &Renderer::AddMagicBuffer );
      static CommandHandler< Render::CommandDataCreateRasterizerState > createRasterizerState( &Renderer::AddRasterizerState );
      static CommandHandler< Render::CommandDataCreateSamplerState > createSamplerState( &Renderer::AddSamplerState );
      static CommandHandler< Render::CommandDataCreateShader > createShader( &Renderer::AddShader );
      static CommandHandler< Render::CommandDataCreateTexture > createTexture( &Renderer::AddTexture );
      static CommandHandler< Render::CommandDataCreateVertexBuffer > createVertexBuffer( &Renderer::AddVertexBuffer );
      static CommandHandler< Render::CommandDataCreateVertexFormat> createVertexFormat( &Renderer::AddVertexFormat );
      static CommandHandler< Render::CommandDataResizeFramebuffer > resizeFramebuffer( &Renderer::ResizeFramebuffer );
      static CommandHandler< Render::CommandDataSetRenderObjectDebugName > setRenderObjectDebugName( &Renderer::SetRenderObjectDebugName );
      static CommandHandler< Render::CommandDataUpdateIndexBuffer >        updateIndexBuffer( &Renderer::UpdateIndexBuffer );
      static CommandHandler< Render::CommandDataUpdateTextureRegion > updateTextureRegion( &Renderer::UpdateTextureRegion );
      static CommandHandler< Render::CommandDataUpdateVertexBuffer > updateVertexBuffer( &Renderer::UpdateVertexBuffer );
      static CommandHandlerDestroy< Render::BlendStateHandle > destroyBlendState( &Renderer::RemoveBlendState );
      static CommandHandlerDestroy< Render::ConstantBufferHandle > destroyConstantBuffer( &Renderer::RemoveConstantBuffer );
      static CommandHandlerDestroy< Render::DepthStateHandle > destroyDepthState( &Renderer::RemoveDepthState );
      static CommandHandlerDestroy< Render::FramebufferHandle > destroyFramebuffer( &Renderer::RemoveFramebuffer );
      static CommandHandlerDestroy< Render::IndexBufferHandle > destroyIndexBuffer( &Renderer::RemoveIndexBuffer );
      static CommandHandlerDestroy< Render::RasterizerStateHandle > destroyRasterizerState( &Renderer::RemoveRasterizerState );
      static CommandHandlerDestroy< Render::SamplerStateHandle > destroySamplerState( &Renderer::RemoveSamplerState );
      static CommandHandlerDestroy< Render::ShaderHandle > destroyShader( &Renderer::RemoveShader );
      static CommandHandlerDestroy< Render::TextureHandle > destroyTexture( &Renderer::RemoveTexture );
      static CommandHandlerDestroy< Render::VertexBufferHandle > destroyVertexBuffer( &Renderer::RemoveVertexBuffer );
      static CommandHandlerDestroy< Render::VertexFormatHandle > destroyVertexFormat( &Renderer::RemoveVertexFormat );

      Add( Render::CommandType::CreateBlendState, &createBlendState, "createBlendState" );
      Add( Render::CommandType::CreateConstantBuffer, &createConstantBuffer, "createConstantBuffer" );
      Add( Render::CommandType::CreateDepthState, &createDepthState, "createDepthState" );
      Add( Render::CommandType::CreateFramebuffer, &createFramebuffer, "createFramebuffer" );
      Add( Render::CommandType::CreateIndexBuffer, &createIndexBuffer, "createIndexBuffer" );
      Add( Render::CommandType::CreateMagicBuffer, &createMagicBuffer, "create magic buffer" );
      Add( Render::CommandType::CreateRasterizerState, &createRasterizerState, "createRasterizerState" );
      Add( Render::CommandType::CreateSamplerState, &createSamplerState, "createSamplerState" );
      Add( Render::CommandType::CreateShader, &createShader, "createShader" );
      Add( Render::CommandType::CreateTexture, &createTexture, "createTexture" );
      Add( Render::CommandType::CreateVertexBuffer, &createVertexBuffer, "createVertexBuffer" );
      Add( Render::CommandType::CreateVertexFormat, &createVertexFormat, "createVertexFormat" );
      Add( Render::CommandType::DestroyBlendState, &destroyBlendState, "destroyBlendState" );
      Add( Render::CommandType::DestroyConstantBuffer, &destroyConstantBuffer, "destroyConstantBuffer" );
      Add( Render::CommandType::DestroyDepthState, &destroyDepthState, "destroyDepthState" );
      Add( Render::CommandType::DestroyFramebuffer, &destroyFramebuffer, "destroyFramebuffer" );
      Add( Render::CommandType::DestroyIndexBuffer, &destroyIndexBuffer, "destroyIndexBuffer" );
      Add( Render::CommandType::DestroyRasterizerState, &destroyRasterizerState, "destroyRasterizerState" );
      Add( Render::CommandType::DestroySamplerState, &destroySamplerState, "destroySamplerState" );
      Add( Render::CommandType::DestroyShader, &destroyShader, "destroyShader" );
      Add( Render::CommandType::DestroyTexture, &destroyTexture, "destroyTexture" );
      Add( Render::CommandType::DestroyVertexBuffer, &destroyVertexBuffer, "destroyVertexBuffer" );
      Add( Render::CommandType::DestroyVertexFormat, &destroyVertexFormat, "destroyVertexFormat" );
      Add( Render::CommandType::ResizeFramebuffer, &resizeFramebuffer, "resizeFramebuffer" );
      Add( Render::CommandType::SetRenderObjectDebugName, &setRenderObjectDebugName, "setRenderObjectDebugName" );
      Add( Render::CommandType::UpdateIndexBuffer, &updateIndexBuffer, "updateIndexBuffer" );
      Add( Render::CommandType::UpdateTextureRegion, &updateTextureRegion, "updateTextureRegion" );
      Add( Render::CommandType::UpdateVertexBuffer, &updateVertexBuffer, "updateVertexBuffer" );

    }

    CommandHandlerBase* mCommandHandlers[ ( int )Render::CommandType::Count ];
  } sCommandHandlers;



  void Renderer::ExecuteCommands( Render::CommandBuffer* commandBuffer, Errors& errors )
  {
    CommandBufferIterator iter;
    iter.bufferBegin = commandBuffer->Data();
    iter.bufferEnd = commandBuffer->Data() + commandBuffer->Size();
    iter.bufferPos = commandBuffer->Data();
    while( iter.IsValid() )
    {
      auto renderCommandType = iter.Pop< Render::CommandType >();
      sCommandHandlers.Invoke( this, *renderCommandType, &iter, errors );
    }
  }

}




