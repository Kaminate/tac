#include "tac-rhi/renderer/command/tac_render_command_type.h"
#include "tac-rhi/renderer/command/tac_render_command_data.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_render_frame.h"

namespace Tac::Render
{

  template< typename THandle > CommandType GetDestroyCommandType()      { TAC_ASSERT_INVALID_CODE_PATH; return CommandType::Count; };
  template<> CommandType GetDestroyCommandType<BlendStateHandle>()      { return  CommandType::DestroyBlendState; }
  template<> CommandType GetDestroyCommandType<ConstantBufferHandle>()  { return  CommandType::DestroyConstantBuffer; }
  template<> CommandType GetDestroyCommandType<DepthStateHandle>()      { return  CommandType::DestroyDepthState; }
  template<> CommandType GetDestroyCommandType<FramebufferHandle>()     { return  CommandType::DestroyFramebuffer; }
  template<> CommandType GetDestroyCommandType<IndexBufferHandle>()     { return  CommandType::DestroyIndexBuffer; }
  template<> CommandType GetDestroyCommandType<RasterizerStateHandle>() { return  CommandType::DestroyRasterizerState; }
  template<> CommandType GetDestroyCommandType<SamplerStateHandle>()    { return  CommandType::DestroySamplerState; }
  template<> CommandType GetDestroyCommandType<ShaderHandle>()          { return  CommandType::DestroyShader; }
  template<> CommandType GetDestroyCommandType<TextureHandle>()         { return  CommandType::DestroyTexture; }
  template<> CommandType GetDestroyCommandType<MagicBufferHandle>()     { return  CommandType::DestroyMagicBuffer; }
  template<> CommandType GetDestroyCommandType<VertexBufferHandle>()    { return  CommandType::DestroyVertexBuffer; }
  template<> CommandType GetDestroyCommandType<VertexFormatHandle>()    { return  CommandType::DestroyVertexFormat; }

  template< typename THandle > void DeferredFreeHandle( THandle )       { TAC_ASSERT_INVALID_CODE_PATH; };
  template<> void DeferredFreeHandle <>( BlendStateHandle h )           { GetSubmitFrame()->mFreeDeferredHandles.mFreedBlendStates.push_back( h ); }
  template<> void DeferredFreeHandle <>( ConstantBufferHandle h )       { GetSubmitFrame()->mFreeDeferredHandles.mFreedConstantBuffers.push_back( h ); }
  template<> void DeferredFreeHandle <>( DepthStateHandle h )           { GetSubmitFrame()->mFreeDeferredHandles.mFreedDepthStencilStates.push_back( h ); }
  template<> void DeferredFreeHandle <>( FramebufferHandle h )          { GetSubmitFrame()->mFreeDeferredHandles.mFreedFramebuffers.push_back( h ); }
  template<> void DeferredFreeHandle <>( IndexBufferHandle h )          { GetSubmitFrame()->mFreeDeferredHandles.mFreedIndexBuffers.push_back( h ); }
  template<> void DeferredFreeHandle <>( RasterizerStateHandle h )      { GetSubmitFrame()->mFreeDeferredHandles.mFreedRasterizerStates.push_back( h ); }
  template<> void DeferredFreeHandle <>( SamplerStateHandle h )         { GetSubmitFrame()->mFreeDeferredHandles.mFreedSamplerStates.push_back( h ); }
  template<> void DeferredFreeHandle <>( ShaderHandle h )               { GetSubmitFrame()->mFreeDeferredHandles.mFreedShaders.push_back( h ); }
  template<> void DeferredFreeHandle <>( TextureHandle h )              { GetSubmitFrame()->mFreeDeferredHandles.mFreedTextures.push_back( h ); }
  template<> void DeferredFreeHandle <>( MagicBufferHandle h )          { GetSubmitFrame()->mFreeDeferredHandles.mFreedMagicBuffers.push_back( h ); }
  template<> void DeferredFreeHandle <>( VertexBufferHandle h )         { GetSubmitFrame()->mFreeDeferredHandles.mFreedVertexBuffers.push_back( h ); }
  template<> void DeferredFreeHandle <>( VertexFormatHandle h )         { GetSubmitFrame()->mFreeDeferredHandles.mFreedVertexFormatInputLayouts.push_back( h ); }

  static void DestroyHelperAux( const Handle h, const StackFrame& sf, const CommandType cmdType )
  {
    if( !h.IsValid() )
      return;

    const CommandDataDestroy commandData
    {
      .mStackFrame = sf,
      .mIndex = h.GetIndex(),
    };

    const int n = sizeof( commandData );
    Frame* submitFrame = GetSubmitFrame();
    submitFrame->mCommandBufferFrameEnd.PushCommand( cmdType, &commandData, n );
  };

  template< typename THandle > void DestroyHelper( THandle h, const StackFrame& sf )
  {
    const CommandType cmdType = GetDestroyCommandType< THandle >();
    DeferredFreeHandle( h );
    DestroyHelperAux( h, sf, cmdType );
  }

  void DestroyVertexBuffer( const VertexBufferHandle h, const StackFrame& sf )        { DestroyHelper( h, sf ); }
  void DestroyIndexBuffer( const IndexBufferHandle h, const StackFrame& sf )          { DestroyHelper( h, sf ); }
  void DestroyMagicBuffer( const MagicBufferHandle h, const StackFrame& sf )          { DestroyHelper( h, sf ); }
  void DestroyTexture( const TextureHandle h, const StackFrame& sf )                  { DestroyHelper( h, sf ); }
  void DestroyFramebuffer( const FramebufferHandle h, const StackFrame& sf )          { DestroyHelper( h, sf ); }
  void DestroyShader( const ShaderHandle h, const StackFrame& sf )                    { DestroyHelper( h, sf ); }
  void DestroyVertexFormat( const VertexFormatHandle h, const StackFrame& sf )        { DestroyHelper( h, sf ); }
  void DestroyConstantBuffer( const ConstantBufferHandle h, const StackFrame& sf )    { DestroyHelper( h, sf ); }
  void DestroyDepthState( const DepthStateHandle h, const StackFrame& sf )            { DestroyHelper( h, sf ); }
  void DestroyBlendState( const BlendStateHandle h, const StackFrame& sf )            { DestroyHelper( h, sf ); }
  void DestroyRasterizerState( const RasterizerStateHandle h, const StackFrame& sf )  { DestroyHelper( h, sf ); }
  void DestroySamplerState( const SamplerStateHandle h, const StackFrame& sf )        { DestroyHelper( h, sf ); }


} // namespace Tac::Render
