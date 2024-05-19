#pragma once

#include "tac_dx12_texture.h"
//#include "tac-win32/dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/containers/tac_array.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render { struct DX12Context; struct DX12ContextManager; }

namespace Tac::Render
{

  struct DX12TextureMgr
  {
    struct Params
    {
      ID3D12Device*       mDevice                       {};
      DX12DescriptorHeap* mCpuDescriptorHeapRTV         {};
      DX12DescriptorHeap* mCpuDescriptorHeapDSV         {};
      DX12DescriptorHeap* mCpuDescriptorHeapCBV_SRV_UAV {};
      DX12ContextManager* mContextManager               {};
    };
    void          Init( Params );
    void          CreateTexture( TextureHandle, CreateTextureParams, Errors& );
    void          CreateRenderTargetColor( TextureHandle, PCom< ID3D12Resource >, Errors& );
    void          UpdateTexture( TextureHandle, UpdateTextureParams, DX12Context*, Errors& );
    void          DestroyTexture( TextureHandle );
    DX12Texture*  FindTexture( TextureHandle );

  private:
    using DX12Textures = Array< DX12Texture, 100 >;

    struct Bindings
    {
      Optional< DX12Descriptor > mRTV;
      Optional< DX12Descriptor > mDSV;
      Optional< DX12Descriptor > mSRV;
    };

    Bindings CreateBindings( ID3D12Resource*, Binding );

    void     UploadTextureData( ID3D12Resource*,
                                D3D12_RESOURCE_STATES*,
                                UpdateTextureParams,
                                DX12Context*,
                                Errors& );

    DX12Textures        mTextures                     {};
    ID3D12Device*       mDevice                       {};
    DX12DescriptorHeap* mCpuDescriptorHeapRTV         {};
    DX12DescriptorHeap* mCpuDescriptorHeapDSV         {};
    DX12DescriptorHeap* mCpuDescriptorHeapCBV_SRV_UAV {};
    DX12ContextManager* mContextManager               {};
  };
} // namespace Tac::Render
