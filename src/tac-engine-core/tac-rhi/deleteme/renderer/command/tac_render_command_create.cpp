#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/tac_render_id.h"
#include "tac-rhi/renderer/command/tac_render_command_type.h"
#include "tac-rhi/renderer/tac_render_frame.h"

namespace Tac::Render
{

  ViewHandle            CreateView() { return { mIdCollectionViewId.Alloc() }; }

  template< typename THandle > CommandType GetCreateCommandType()      { TAC_ASSERT_INVALID_CODE_PATH; return CommandType::Count; };
  template<> CommandType GetCreateCommandType<CommandDataCreateBlendState>()      { return CommandType::CreateBlendState; }
  template<> CommandType GetCreateCommandType<CommandDataCreateConstantBuffer>()  { return CommandType::CreateConstantBuffer; }
  template<> CommandType GetCreateCommandType<CommandDataCreateDepthState>()      { return CommandType::CreateDepthState; }
  template<> CommandType GetCreateCommandType<CommandDataCreateFramebuffer>()     { return CommandType::CreateFramebuffer; }
  template<> CommandType GetCreateCommandType<CommandDataCreateIndexBuffer>()     { return CommandType::CreateIndexBuffer; }
  template<> CommandType GetCreateCommandType<CommandDataCreateRasterizerState>() { return CommandType::CreateRasterizerState; }
  template<> CommandType GetCreateCommandType<CommandDataCreateSamplerState>()    { return CommandType::CreateSamplerState; }
  template<> CommandType GetCreateCommandType<CommandDataCreateShader>()          { return CommandType::CreateShader; }
  template<> CommandType GetCreateCommandType<CommandDataCreateTexture>()         { return CommandType::CreateTexture; }
  template<> CommandType GetCreateCommandType<CommandDataCreateMagicBuffer>()     { return CommandType::CreateMagicBuffer; }
  template<> CommandType GetCreateCommandType<CommandDataCreateVertexBuffer>()    { return CommandType::CreateVertexBuffer; }
  template<> CommandType GetCreateCommandType<CommandDataCreateVertexFormat>()    { return CommandType::CreateVertexFormat; }

  template < typename CommandData >
  const CommandData& AddCreateCommand( const CommandData& commandData )
  {
    Frame* submitFrame { GetSubmitFrame() };
    const CommandType cmdType { GetCreateCommandType< CommandData >() };
    const int n { sizeof( CommandData ) };
    submitFrame->mCommandBufferFrameBegin.PushCommand( cmdType, &commandData, n );
    return commandData;
  }

  ShaderHandle          CreateShader( const ShaderNameStringView& shaderName, const StackFrame& stackFrame )
  {
    const CommandDataCreateShader commandData
    {
      .mStackFrame     { stackFrame },
      .mNameStringView { ShaderNameStringView( SubmitAlloc( shaderName ) ) },
      .mShaderHandle   { mIdCollectionShader.Alloc() },
    };
    return AddCreateCommand(commandData).mShaderHandle;
  }

  VertexBufferHandle    CreateVertexBuffer( const int byteCount,
                                            const void* optionalInitialBytes,
                                            const int stride,
                                            const Access access,
                                            const StackFrame& stackFrame )
  {
    const CommandDataCreateVertexBuffer commandData
    {
      .mStackFrame           { stackFrame },
      .mVertexBufferHandle   { mIdCollectionVertexBuffer.Alloc() },
      .mByteCount            { byteCount },
      .mOptionalInitialBytes { SubmitAlloc( optionalInitialBytes, byteCount ) },
      .mStride               { stride },
      .mAccess               { access },
    };
    return AddCreateCommand( commandData ).mVertexBufferHandle;
  }

  MagicBufferHandle     CreateMagicBuffer( const int byteCount,
                                           const void* optionalInitialBytes,
                                           const int stride,
                                           const Binding binding,
                                           const Access access,
                                           const StackFrame& stackFrame )
  {
    const CommandDataCreateMagicBuffer commandData
    {
      .mStackFrame { stackFrame },
      .mMagicBufferHandle { mIdCollectionMagicBuffer.Alloc() },
      .mByteCount { byteCount },
      .mOptionalInitialBytes { optionalInitialBytes },
      .mStride { stride },
      .mBinding { binding },
      .mAccess { access },
    };
    return AddCreateCommand( commandData ).mMagicBufferHandle;
  }

  ConstantBufferHandle  CreateConstantBuffer( const char* name, 
                                              const int byteCount,
                                              const StackFrame& stackFrame )
  {
    const CommandDataCreateConstantBuffer commandData
    {
      .mStackFrame { stackFrame },
      .mConstantBufferHandle  { mIdCollectionConstantBuffer.Alloc() },
      .mByteCount { byteCount },
      .mName { SubmitAlloc( name ) },
    };
    return AddCreateCommand( commandData ).mConstantBufferHandle;
  }

  IndexBufferHandle     CreateIndexBuffer( const int byteCount,
                                           const void* optionalInitialBytes,
                                           const Access access,
                                           const Format& format,
                                           const StackFrame& stackFrame )
  {
    const CommandDataCreateIndexBuffer commandData
    {
      .mStackFrame = stackFrame,
      .mIndexBufferHandle = { mIdCollectionIndexBuffer.Alloc() },
      .mByteCount = byteCount,
      .mOptionalInitialBytes = SubmitAlloc( optionalInitialBytes, byteCount ),
      .mAccess = access,
      .mFormat = format
    };
    return AddCreateCommand( commandData ).mIndexBufferHandle;
  }

  TextureHandle         CreateTexture( const TexSpec& texSpec,
                                       const StackFrame& stackFrame )
  {
    TexSpec commandDataTexSpec = texSpec;

    const int imageByteCount =
      texSpec.mImage.mFormat.CalculateTotalByteCount() *
      texSpec.mImage.mWidth *
      texSpec.mImage.mHeight;

    if( texSpec.mImageBytes )
      commandDataTexSpec.mImageBytes = SubmitAlloc( texSpec.mImageBytes, imageByteCount );

    for( int i = 0; i < 6; ++i )
      if( const void* bytes = texSpec.mImageBytesCubemap[ i ] )
        commandDataTexSpec.mImageBytesCubemap[ i ] = SubmitAlloc( bytes, imageByteCount );

    const CommandDataCreateTexture commandData
    {
      .mStackFrame = stackFrame,
      .mTextureHandle = mIdCollectionTexture.Alloc(),
      .mTexSpec = commandDataTexSpec
    };
    return AddCreateCommand( commandData ).mTextureHandle;
  }


  FramebufferHandle     CreateFramebufferForWindow( const void* nativeWindowHandle,
                                                    const int width,
                                                    const int height,
                                                    const StackFrame& stackFrame )
  {
    const CommandDataCreateFramebuffer commandData
    {
      .mStackFrame = stackFrame,
      .mFramebufferHandle = mIdCollectionFramebuffer.Alloc(),
      .mNativeWindowHandle = nativeWindowHandle,
      .mWidth = width,
      .mHeight = height,
    };
    return AddCreateCommand( commandData ).mFramebufferHandle;
  }

  FramebufferHandle     CreateFramebufferForRenderToTexture( const FramebufferTextures& framebufferTextures,
                                                             const StackFrame& stackFrame )
  {
    const CommandDataCreateFramebuffer commandData
    {
      .mStackFrame = stackFrame,
      .mFramebufferHandle = mIdCollectionFramebuffer.Alloc(),
      .mFramebufferTextures = framebufferTextures
    };
    return AddCreateCommand( commandData ).mFramebufferHandle;
  }

  BlendStateHandle      CreateBlendState( const BlendState& blendState,
                                          const StackFrame& stackFrame )
  {
    const CommandDataCreateBlendState commandData
    {
      .mStackFrame = stackFrame,
      .mBlendStateHandle = mIdCollectionBlendState.Alloc(),
      .mBlendState = blendState
    };
    return AddCreateCommand( commandData ).mBlendStateHandle;
  }

  RasterizerStateHandle CreateRasterizerState( const RasterizerState& rasterizerState,
                                               const StackFrame& stackFrame )
  {
    const CommandDataCreateRasterizerState commandData
    {
      .mStackFrame = stackFrame,
      .mRasterizerStateHandle = mIdCollectionRasterizerState.Alloc(),
      .mRasterizerState = rasterizerState
    };
    return AddCreateCommand( commandData ).mRasterizerStateHandle;
  }

  SamplerStateHandle    CreateSamplerState( const SamplerState& samplerState,
                                            const StackFrame& stackFrame )
  {
    const CommandDataCreateSamplerState commandData
    {
      .mStackFrame = stackFrame,
      .mSamplerState = samplerState,
      .mSamplerStateHandle = mIdCollectionSamplerState.Alloc(),
    };
    return AddCreateCommand( commandData ).mSamplerStateHandle;
  }

  DepthStateHandle      CreateDepthState( const DepthState& depthState,
                                          const StackFrame& stackFrame )
  {
    const CommandDataCreateDepthState commandData
    {
      .mStackFrame = stackFrame,
      .mDepthStateHandle = mIdCollectionDepthState.Alloc(),
      .mDepthState = depthState
    };
    
    return AddCreateCommand( commandData ).mDepthStateHandle;
  }

  VertexFormatHandle    CreateVertexFormat( const VertexDeclarations& vertexDeclarations,
                                            const ShaderHandle shaderHandle,
                                            const StackFrame& stackFrame )
  {
    const CommandDataCreateVertexFormat commandData
    {
      .mStackFrame = stackFrame ,
      .mVertexFormatHandle = mIdCollectionVertexFormat.Alloc(),
      .mVertexDeclarations = vertexDeclarations ,
      .mShaderHandle = shaderHandle,
    };
    
    return AddCreateCommand( commandData ).mVertexFormatHandle;
  }


} // namespace Tac::Render
