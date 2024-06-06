#pragma once

#include "tac-rhi/render3/tac_render_api.h"

//namespace Tac::Render { struct TextureHandle; }
namespace Tac { struct AssetPathStringView; struct Errors; }

namespace Tac::TextureAssetManager
{
  Render::TextureHandle GetTexture( const AssetPathStringView&, Errors& );
  Render::TextureHandle GetTextureCube( const AssetPathStringView&, Errors& );
}

