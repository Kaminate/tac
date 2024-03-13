#pragma once

#include "tac-std-lib/tac_core.h"



namespace Tac::TextureAssetManager
{
  Render::TextureHandle GetTexture( const AssetPathStringView&, Errors& );
  Render::TextureHandle GetTextureCube( const AssetPathStringView&, Errors& );
}

