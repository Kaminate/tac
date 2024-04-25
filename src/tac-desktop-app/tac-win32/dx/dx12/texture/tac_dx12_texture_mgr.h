#pragma once

#include "tac_dx12_texture.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12TextureMgr
  {
    struct Params
    {
      ID3D12Device*       mDevice{};
      DX12DescriptorHeap* mCpuDescriptorHeapRTV{};
    };
    void          Init( Params );
    void          CreateTexture( TextureHandle, CreateTextureParams, Errors& );
    void          CreateRenderTargetTexture( TextureHandle, PCom<ID3D12Resource>, Errors& );
    void          UpdateTexture( TextureHandle, UpdateTextureParams );
    void          DestroyTexture( TextureHandle );
    DX12Texture*  FindTexture( TextureHandle );

    DX12Texture         mTextures[ 100 ];
    ID3D12Device*       mDevice{};
    DX12DescriptorHeap* mCpuDescriptorHeapRTV{};
  };
} // namespace Tac::Render
