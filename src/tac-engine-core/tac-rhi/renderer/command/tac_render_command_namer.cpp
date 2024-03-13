#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/tac_renderer_backend.h"
#include "tac-rhi/renderer/command/tac_render_command_data.h"
#include "tac-rhi/renderer/command/tac_render_command_type.h"
#include "tac-rhi/renderer/tac_render_frame.h"

namespace Tac::Render
{


  template< typename THandle > Handle* SetTNameHelper( CommandDataSetRenderObjectDebugName* )            { TAC_ASSERT_INVALID_CODE_PATH; return nullptr;}
  template<> Handle* SetTNameHelper <BlendStateHandle>( CommandDataSetRenderObjectDebugName* data )      { return &data->mBlendStateHandle; }
  template<> Handle* SetTNameHelper <ConstantBufferHandle>( CommandDataSetRenderObjectDebugName* data )  { return &data->mConstantBufferHandle; }
  template<> Handle* SetTNameHelper <DepthStateHandle>( CommandDataSetRenderObjectDebugName* data )      { return &data->mDepthStateHandle; }
  template<> Handle* SetTNameHelper <FramebufferHandle>( CommandDataSetRenderObjectDebugName* data )     { return &data->mFramebufferHandle; }
  template<> Handle* SetTNameHelper <IndexBufferHandle>( CommandDataSetRenderObjectDebugName* data )     { return &data->mIndexBufferHandle; }
  template<> Handle* SetTNameHelper <RasterizerStateHandle>( CommandDataSetRenderObjectDebugName* data ) { return &data->mRasterizerStateHandle; }
  template<> Handle* SetTNameHelper <SamplerStateHandle>( CommandDataSetRenderObjectDebugName* data )    { return &data->mSamplerStateHandle; }
  template<> Handle* SetTNameHelper <ShaderHandle>( CommandDataSetRenderObjectDebugName* data )          {TAC_ASSERT_INVALID_CODE_PATH; return nullptr;}
  template<> Handle* SetTNameHelper <TextureHandle>( CommandDataSetRenderObjectDebugName* data )         { return &data->mTextureHandle; }
  template<> Handle* SetTNameHelper <MagicBufferHandle>( CommandDataSetRenderObjectDebugName* data )     { return &data->mMagicBufferHandle; }
  template<> Handle* SetTNameHelper <VertexBufferHandle>( CommandDataSetRenderObjectDebugName* data )    { return &data->mVertexBufferHandle; }
  template<> Handle* SetTNameHelper <VertexFormatHandle>( CommandDataSetRenderObjectDebugName* data )    { return &data->mVertexFormatHandle; }

  template<typename THandle> CommandDataSetRenderObjectDebugName GetCommandDataSetRenderObjectDebugName( THandle h, const StringView& name )
  {
    CommandDataSetRenderObjectDebugName commandData;
    Handle* pH = SetTNameHelper<THandle>( &commandData );
    *pH = h;
    commandData.mName = SubmitAlloc( name );
    return commandData;
  }

  template<typename THandle> void SetNameHelper( THandle h, const StringView& name)
  {
    const CommandDataSetRenderObjectDebugName commandData = GetCommandDataSetRenderObjectDebugName( h, name );
    Frame* submitFrame = GetSubmitFrame();
    submitFrame->mCommandBufferFrameBegin.PushCommand( CommandType::SetRenderObjectDebugName,
                                                        &commandData,
                                                        sizeof( commandData ) );
  }


  void SetRenderObjectDebugName( const BlendStateHandle h, const StringView& s )      { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const SamplerStateHandle h, const StringView& s )    { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const ConstantBufferHandle h, const StringView& s )  { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const VertexFormatHandle h, const StringView& s )    { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const DepthStateHandle h, const StringView& s )      { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const RasterizerStateHandle h, const StringView& s ) { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const IndexBufferHandle h, const StringView& s )     { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const VertexBufferHandle h, const StringView& s )    { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const TextureHandle h, const StringView& s )         { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const FramebufferHandle h, const StringView& s )     { SetNameHelper( h, s ); }
  void SetRenderObjectDebugName( const MagicBufferHandle h, const StringView& s )     { SetNameHelper( h, s ); }
} // namespace Tac::Render
