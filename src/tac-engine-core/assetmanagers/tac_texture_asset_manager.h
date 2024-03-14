#pragma once

namespace Tac::Render { struct TextureHandle; }
namespace Tac { struct AssetPathStringView; struct Errors; }

namespace Tac::TextureAssetManager
{
  Render::TextureHandle GetTexture( const AssetPathStringView&, Errors& );
  Render::TextureHandle GetTextureCube( const AssetPathStringView&, Errors& );
}

