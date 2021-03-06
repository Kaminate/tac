#include "src/common/graphics/tacRendererBackend.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacOS.h"
#include "src/common/tacIDCollection.h"
#include "src/shell/tacDesktopApp.h"

#include <iostream>

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
          default: TAC_ASSERT_INVALID_CASE( header.mType ); return;
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
        Pusher*        PushData( const void* bytes, const int byteCount )
        {
          MemCpy( mUniformBuffer->mBytes + mUniformBuffer->mByteCount, bytes, byteCount );
          mUniformBuffer->mByteCount += byteCount;
          return this;
        }
        Pusher*        PushString( const StringView s )
        {
          PushNumber( s.size() );
          PushData( s.c_str(), s.size() );
          PushData( "", 1 );
          return this;
        }
        Pusher*        PushNumber( const int i )
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
      int                   mUniformBufferIndex = 0;
      DrawCall3             mDrawCall;
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

    StringView SubmitAlloc( const StringView stringView )
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

    ViewHandle CreateView()
    {
      return { mIdCollectionViewId.Alloc() };
    }



    ShaderHandle CreateShader( const ShaderSource shaderSource,
                               const ConstantBuffers constantBuffers,
                               const StackFrame stackFrame )
    {
      TAC_ASSERT( constantBuffers.mConstantBufferCount );
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

    VertexBufferHandle CreateVertexBuffer( const int byteCount,
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

    ConstantBufferHandle CreateConstantBuffer( const int byteCount,
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

    IndexBufferHandle CreateIndexBuffer( const int byteCount,
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

    TextureHandle CreateTexture( TexSpec texSpec,
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

    void ResizeFramebuffer( const FramebufferHandle framebufferHandle,
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

    FramebufferHandle CreateFramebuffer( const void* nativeWindowHandle,
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


    BlendStateHandle CreateBlendState( const BlendState blendState,
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

    SamplerStateHandle CreateSamplerState( const SamplerState samplerState,
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

    DepthStateHandle CreateDepthState( const DepthState depthState,
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

    VertexFormatHandle CreateVertexFormat( const VertexDeclarations vertexDeclarations,
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

    void FreeDeferredHandles::FinishFreeingHandles()
    {
      if( mFreedBlendStates.size() )
      {
        for( auto handle : mFreedBlendStates )
          mIdCollectionBlendState.Free( ( int )handle );
        mFreedBlendStates.clear();
      }

      if( mFreedConstantBuffers.size() )
      {
        for( auto handle : mFreedConstantBuffers )
          mIdCollectionConstantBuffer.Free( ( int )handle );
        mFreedConstantBuffers.clear();
      }

      if( mFreedDepthStencilStates.size() )
      {
        for( auto handle : mFreedDepthStencilStates )
          mIdCollectionDepthState.Free( ( int )handle );
        mFreedDepthStencilStates.clear();
      }

      if( mFreedFramebuffers.size() )
      {
        for( auto handle : mFreedFramebuffers )
          mIdCollectionFramebuffer.Free( ( int )handle );
        mFreedFramebuffers.clear();
      }

      if( mFreedIndexBuffers.size() )
      {
        for( auto handle : mFreedIndexBuffers )
          mIdCollectionIndexBuffer.Free( ( int )handle );
        mFreedIndexBuffers.clear();
      }

      if( mFreedRasterizerStates.size() )
      {
        for( auto handle : mFreedRasterizerStates )
          mIdCollectionRasterizerState.Free( ( int )handle );
        mFreedRasterizerStates.clear();
      }

      if( mFreedSamplerStates.size() )
      {
        for( auto handle : mFreedSamplerStates )
          mIdCollectionSamplerState.Free( ( int )handle );
        mFreedSamplerStates.clear();
      }

      if( mFreedShaders.size() )
      {
        for( auto handle : mFreedShaders )
          mIdCollectionShader.Free( ( int )handle );
        mFreedShaders.clear();
      }

      if( mFreedTextures.size() )
      {
        for( auto handle : mFreedTextures )
          mIdCollectionTexture.Free( ( int )handle );
        mFreedTextures.clear();
      }

      if( mFreedVertexBuffers.size() )
      {
        for( auto handle : mFreedVertexBuffers )
          mIdCollectionVertexBuffer.Free( ( int )handle );
        mFreedVertexBuffers.clear();
      }

      if( mFreedVertexFormatInputLayouts.size() )
      {
        for( auto handle : mFreedVertexFormatInputLayouts )
          mIdCollectionVertexFormat.Free( ( int )handle );
        mFreedVertexFormatInputLayouts.clear();
      }

      if( mFreedViews.size() )
      {
        for( auto handle : mFreedViews )
          mIdCollectionViewId.Free( ( int )handle );
        mFreedViews.clear();
      }
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
                                                        &vertexFormatHandle,
                                                        sizeof( vertexFormatHandle ) );
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

    void RenderFrame( Errors& errors )
    {
      TAC_ASSERT( IsMainThread() );
      if( !Renderer::Instance )
        return;

      SemaphoreDecrementWait( gSubmitSemaphore );


      if( gRenderFrame->mBreakOnFrameRender )
        OS::DebugBreak();


      Renderer::Instance->ExecuteCommands( &gRenderFrame->mCommandBufferFrameBegin, errors );
      TAC_ASSERT( errors.empty() );

      Renderer::Instance->RenderBegin( gRenderFrame, errors );
      const DrawCall3* drawCallBegin = gRenderFrame->mDrawCalls.data();
      const int drawCallCount = gRenderFrame->mDrawCalls.size();
      for( int iDrawCall = 0; iDrawCall < drawCallCount; ++iDrawCall )
      {
        if( gRenderFrame->mBreakOnDrawCall == iDrawCall )
          OS::DebugBreak();

        const DrawCall3* drawCall = drawCallBegin + iDrawCall;

        ExecuteUniformCommands( &gRenderFrame->mUniformBuffer,
                                drawCall->iUniformBegin,
                                drawCall->iUniformEnd,
                                errors );
        Renderer::Instance->RenderDrawCall( gRenderFrame, drawCall, errors );
      }

      Renderer::Instance->ExecuteCommands( &gRenderFrame->mCommandBufferFrameEnd, errors );
      TAC_ASSERT( errors.empty() );

      Renderer::Instance->RenderEnd( gRenderFrame, errors );

      TAC_ASSERT( errors.empty() );

      SemaphoreIncrementPost( gRenderSemaphore );

      Renderer::Instance->SwapBuffers();
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
      SemaphoreDecrementWait( gRenderSemaphore );

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
      SemaphoreIncrementPost( gSubmitSemaphore );
    }

    void Init( Errors& errors )
    {
      gSubmitSemaphore = SemaphoreCreate();
      gRenderSemaphore = SemaphoreCreate();

      // i guess well make render frame go first
      SemaphoreIncrementPost( gSubmitSemaphore );

      Renderer::Instance->Init( errors );
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
      auto renderCommandType = Pop< Render::CommandType >( bufferPos );
      //auto renderCommandType = ( Render::CommandType* )bufferPos;
      //bufferPos += sizeof( Render::CommandType );

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


        case Render::CommandType::DestroyVertexBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyVertexBuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveVertexBuffer( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyVertexBuffer::End\n";
        } break;

        case Render::CommandType::DestroyIndexBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyIndexBuffer::Begin\n";
          const auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveIndexBuffer( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyIndexBuffer::End\n";
        } break;

        case Render::CommandType::DestroyTexture:
        {
          if( gVerbose )
            std::cout << "DestroyTexture::Begin\n";
          const auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveTexture( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyTexture::End\n";
        } break;

        case Render::CommandType::DestroyFramebuffer:
        {
          if( gVerbose )
            std::cout << "DestroyFramebuffer::Begin\n";
          const auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveFramebuffer( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyFramebuffer::End\n";
        } break;

        case Render::CommandType::DestroyConstantBuffer:
        {
          if( gVerbose )
            std::cout << "DestroyConstantBuffer::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveConstantBuffer( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyConstantBuffer::End\n";
        } break;

        case Render::CommandType::DestroyDepthState:
        {
          if( gVerbose )
            std::cout << "DestroyDepthState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveDepthState( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyDepthState::End\n";
        } break;

        case Render::CommandType::DestroyRasterizerState:
        {
          if( gVerbose )
            std::cout << "DestroyRasterizerState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveRasterizerState( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyRasterizerState::End\n";
        } break;

        case Render::CommandType::DestroySamplerState:
        {
          if( gVerbose )
            std::cout << "DestroySamplerState::Begin\n";

          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveSamplerState( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );

          if( gVerbose )
            std::cout << "DestroySamplerState::End\n";
        } break;

        case Render::CommandType::DestroyShader:
        {
          if( gVerbose )
            std::cout << "DestroyShader::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveShader( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyShader::End\n";
        } break;

        case Render::CommandType::DestroyVertexFormat:
        {
          if( gVerbose )
            std::cout << "DestroyVertexFormat::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveVertexFormat( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors );
          if( gVerbose )
            std::cout << "DestroyVertexFormat::End\n";
        } break;

        case Render::CommandType::DestroyBlendState:
        {
          if( gVerbose )
            std::cout << "DestroyBlendState::Begin\n";
          auto commandData = PopCommandData< Render::CommandDataDestroy >( bufferPos );
          RemoveBlendState( commandData->mIndex, errors );
          TAC_HANDLE_ERROR( errors )
          if( gVerbose )
            std::cout << "DestroyBlendState::End\n";
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

        default: TAC_ASSERT_INVALID_CASE( *renderCommandType ); return;
      }
    }
  }

  //String RendererTypeToString( const Renderer::Type rendererType )
  //{
  //  switch( rendererType )
  //  {
  //    case Renderer::Type::Vulkan: return RendererNameVulkan;
  //    case Renderer::Type::OpenGL4: return RendererNameOpenGL4;
  //    case Renderer::Type::DirectX11: return RendererNameDirectX11;
  //    case Renderer::Type::DirectX12: return RendererNameDirectX12;
  //    default: TAC_ASSERT_INVALID_CASE( rendererType ); return "";
  //  }
  //}
}
