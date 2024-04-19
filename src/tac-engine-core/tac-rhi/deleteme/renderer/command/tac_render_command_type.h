// 

#pragma once


namespace Tac::Render
{
  enum class CommandType
  {
    CreateBlendState,
    CreateConstantBuffer,
    CreateDepthState,
    CreateFramebuffer,
    CreateIndexBuffer,
    CreateRasterizerState,
    CreateSamplerState,
    CreateShader,
    CreateTexture,
    CreateMagicBuffer,
    CreateVertexBuffer,
    CreateVertexFormat,
    DestroyBlendState,
    DestroyConstantBuffer,
    DestroyDepthState,
    DestroyFramebuffer,
    DestroyIndexBuffer,
    DestroyRasterizerState,
    DestroySamplerState,
    DestroyShader,
    DestroyTexture,
    DestroyMagicBuffer,
    DestroyVertexBuffer,
    DestroyVertexFormat,
    UpdateIndexBuffer,
    UpdateTextureRegion,
    UpdateVertexBuffer,
    ResizeFramebuffer,
    SetRenderObjectDebugName,
    Count, // must be last
  };
} // namespace Tac::Render

