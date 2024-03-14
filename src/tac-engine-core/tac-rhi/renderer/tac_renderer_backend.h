#pragma once

#include "tac-rhi/renderer/tac_renderer.h"
#include "tac-rhi/renderer/command/tac_render_command_data.h"
#include "tac-rhi/renderer/tac_shader_reload_helper.h"

namespace Tac { struct AssetPathStringView; }
namespace Tac::Render
{
  struct Frame;
  struct DrawCall;

  bool                  IsSubmitAllocated( const void* );
  Frame*                GetRenderFrame();
  Frame*                GetSubmitFrame();
  AssetPathStringView   GetShaderAssetPath( const ShaderNameStringView& );
  AssetPathStringView   GetShaderAssetDir();

  struct Renderer
  {
    static Renderer* Instance;
    virtual      ~Renderer() = default;
    virtual void Init( Errors& ) {};

    virtual void RenderBegin( const Frame*, Errors& ) = 0;
    virtual void RenderDrawCall( const Frame*, const DrawCall*, Errors& ) = 0;
    virtual void RenderEnd( float dt, const Frame*, Errors& ) = 0;
    virtual void SwapBuffers() = 0;

    virtual void DebugGroupBegin( StringView ) = 0;
    virtual void DebugGroupEnd() = 0;
    virtual void DebugMarker( StringView ) = 0;
    virtual void SetRenderObjectDebugName( const CommandDataSetRenderObjectDebugName*, Errors& ) = 0;

    virtual void AddBlendState( const CommandDataCreateBlendState*, Errors& ) = 0;
    virtual void AddConstantBuffer( const CommandDataCreateConstantBuffer*, Errors& ) = 0;
    virtual void AddDepthState( const CommandDataCreateDepthState*, Errors& ) = 0;
    virtual void AddFramebuffer( const CommandDataCreateFramebuffer*, Errors& ) = 0;
    virtual void AddIndexBuffer( const CommandDataCreateIndexBuffer*, Errors& ) = 0;
    virtual void AddRasterizerState( const CommandDataCreateRasterizerState*, Errors& ) = 0;
    virtual void AddSamplerState( const CommandDataCreateSamplerState*, Errors& ) = 0;
    virtual void AddShader( const CommandDataCreateShader*, Errors& ) = 0;
    virtual void AddTexture( const CommandDataCreateTexture*, Errors& ) = 0;
    virtual void AddMagicBuffer( const CommandDataCreateMagicBuffer*, Errors& ) = 0;
    virtual void AddVertexBuffer( const CommandDataCreateVertexBuffer*, Errors& ) = 0;
    virtual void AddVertexFormat( const CommandDataCreateVertexFormat*, Errors& ) = 0;

    virtual void RemoveBlendState( BlendStateHandle, Errors& ) = 0;
    virtual void RemoveConstantBuffer( ConstantBufferHandle, Errors& ) = 0;
    virtual void RemoveDepthState( DepthStateHandle, Errors& ) = 0;
    virtual void RemoveFramebuffer( FramebufferHandle, Errors& ) = 0;
    virtual void RemoveIndexBuffer( IndexBufferHandle, Errors& ) = 0;
    virtual void RemoveRasterizerState( RasterizerStateHandle, Errors& ) = 0;
    virtual void RemoveSamplerState( SamplerStateHandle, Errors& ) = 0;
    virtual void RemoveShader( ShaderHandle, Errors& ) = 0;
    virtual void RemoveTexture( TextureHandle, Errors& ) = 0;
    virtual void RemoveMagicBuffer( MagicBufferHandle, Errors& ) = 0;
    virtual void RemoveVertexBuffer( VertexBufferHandle, Errors& ) = 0;
    virtual void RemoveVertexFormat( VertexFormatHandle, Errors& ) = 0;

    virtual void UpdateConstantBuffer( const CommandDataUpdateConstantBuffer*, Errors& ) = 0;
    virtual void UpdateIndexBuffer( const CommandDataUpdateIndexBuffer*, Errors& ) = 0;
    virtual void UpdateTextureRegion( const CommandDataUpdateTextureRegion*, Errors& ) = 0;
    virtual void UpdateVertexBuffer( const CommandDataUpdateVertexBuffer*, Errors& ) = 0;
    virtual void ResizeFramebuffer( const CommandDataResizeFramebuffer*, Errors& ) = 0;

    virtual OutProj             GetPerspectiveProjectionAB(InProj) = 0;
    virtual AssetPathStringView GetShaderPath( const ShaderNameStringView& ) const = 0;
    virtual AssetPathStringView GetShaderDir() const = 0;
  };

} // namespace Tac::Render
