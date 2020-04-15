
#pragma once

namespace Tac
{
  struct StringView;
  struct Errors;
  namespace Render
  {
    struct TextureHandle;
  }

  namespace TextureAssetManager
  {
    Render::TextureHandle GetTexture( const StringView& textureFilepath,
                                      Tac::Errors& errors );
    Render::TextureHandle GetTextureCube( const StringView& textureDir,
                                          Errors& errors );
  };
}

