#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::TextureAssetManager
{
  Render::TextureHandle                 GetTexture( AssetPathStringView, Errors& );
  Render::IBindlessArray::Binding GetBindlessTextureIndex( AssetPathStringView, Errors& );
  Render::TextureHandle                 GetTextureCube( AssetPathStringView, Errors& );
  Render::IBindlessArray*         GetBindlessArray();
}

