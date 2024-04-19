#include "tac_dx12_texture_mgr.h" // self-inc
#include "tac-win32/dx/dx12/tac_dx12_enum_helper.h"
#include "tac-win32/dx/dx12/tac_dx12_helper.h"

namespace Tac::Render
{
  void DX12TextureMgr::Init( ID3D12Device* device )
  {
    mDevice = device;
  }

  void DX12TextureMgr::CreateTexture( TextureHandle h, CreateTextureParams params , Errors& errors)
  {
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12TextureMgr::UpdateTexture( TextureHandle h, UpdateTextureParams params )
  {
    DX12Texture& dynTex = mTextures[ h.GetIndex() ];
    TAC_ASSERT_UNIMPLEMENTED;
  }

  void DX12TextureMgr::DestroyTexture( TextureHandle h )
  {
    if( h.IsValid() )
      mTextures[ h.GetIndex() ] = {};
  }
} // namespace Tac::Render
