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
    static Render::TextureHandle           GetTexture( AssetPathStringView, Errors& );
    static Render::IBindlessArray::Binding GetBindlessIndex( AssetPathStringView, Errors& );
    static Render::IBindlessArray*         GetBindlessArray();
  };
}

