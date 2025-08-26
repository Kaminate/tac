#pragma once

#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/string/tac_string_identifier.h"
#include "tac-std-lib/containers/tac_map.h"

namespace Tac
{
  struct TextureAssetManager
  {
    static auto GetTextureSize( AssetPathStringView, Errors& ) -> v3i;
    static auto GetTexture( AssetPathStringView, Errors& ) -> Render::TextureHandle;
    static auto GetBindlessIndex( AssetPathStringView, Errors& ) -> Render::IBindlessArray::Binding;
    static auto GetBindlessArray() -> Render::IBindlessArray*;
  };
}

