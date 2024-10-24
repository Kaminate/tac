#pragma once

#include "tac_dx12_texture.h"
//#include "tac-dx/dx12/tac_renderer_dx12_ver3.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-dx/dx12/resource/tac_dx12_resource.h"
#include "tac-std-lib/containers/tac_array.h"

#include <d3d12.h>

namespace Tac { struct Errors; }

namespace Tac::Render
{
  struct DX12Context;
  struct DX12ContextManager;
  struct DX12TransitionHelper;
}

namespace Tac::Render
{

  struct DX12TextureMgr
  {
    TextureHandle CreateTexture( CreateTextureParams, Errors& );
    void          CreateRenderTargetColor( TextureHandle, PCom< ID3D12Resource >, Errors& );
    void          UpdateTexture( TextureHandle, UpdateTextureParams, DX12Context*, Errors& );
    void          DestroyTexture( TextureHandle );
    DX12Texture*  FindTexture( TextureHandle );
    void          TransitionTexture( TextureHandle, DX12TransitionHelper* );
    void          SetName( TextureHandle, StringView );
    void          TransitionResource( DX12Resource*,
                                      Binding,
                                      DX12TransitionHelper* );

  private:
    using DX12Textures = Array< DX12Texture, 100 >;

    struct Bindings
    {
      Bindings() = default;
      Bindings(  ID3D12Resource*, Binding );

      Optional< DX12Descriptor > mRTV;
      Optional< DX12Descriptor > mDSV;
      Optional< DX12Descriptor > mSRV;
      Optional< DX12Descriptor > mUAV;
    };

    void     UploadTextureData( DX12Resource*,
                                UpdateTextureParams,
                                DX12Context*,
                                Errors& );

    DX12Textures        mTextures                     {};
  };
} // namespace Tac::Render
