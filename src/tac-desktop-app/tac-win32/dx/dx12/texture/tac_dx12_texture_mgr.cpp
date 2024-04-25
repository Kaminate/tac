#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"
#include "tac-win32/dx/dx12/descriptor/tac_dx12_descriptor_heap.h"

namespace Tac::Render
{
  void DX12TextureMgr::Init( Params params )
  {
    mDevice = params.mDevice;
    mCpuDescriptorHeapRTV = params.mCpuDescriptorHeapRTV;
  }

  void DX12TextureMgr::CreateTexture( TextureHandle h, CreateTextureParams params , Errors& errors)
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12TextureMgr::CreateRenderTargetTexture( TextureHandle h,
                                                  PCom<ID3D12Resource> resource,
                                                  Errors& errors)
  {
    DX12Texture* texture = FindTexture( h );
    TAC_ASSERT( texture );

    const DX12DescriptorHeapAllocation allocation  { mCpuDescriptorHeapRTV->Allocate() };
    const D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle { allocation.GetCPUHandle() };

    ID3D12Resource* pResource = resource.Get();
    mDevice->CreateRenderTargetView( pResource, nullptr, cpuHandle );

    *texture = DX12Texture
    {
      .mResource { resource },
      .mDesc     { resource->GetDesc() },
      .mState    { D3D12_RESOURCE_STATE_PRESENT }, // Render targets are created in present state
      .mRTV      { allocation },
    };
  }

  void DX12TextureMgr::UpdateTexture( TextureHandle h, UpdateTextureParams params )
  {
    DX12Texture& dynTex { mTextures[ h.GetIndex() ] };
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12TextureMgr::DestroyTexture( TextureHandle h )
  {
    if( h.IsValid() )
      mTextures[ h.GetIndex() ] = {};
  }

  DX12Texture* DX12TextureMgr::FindTexture( TextureHandle h )
  {
    return h.IsValid() ? &mTextures[ h.GetIndex() ] : nullptr;
  }
} // namespace Tac::Render
