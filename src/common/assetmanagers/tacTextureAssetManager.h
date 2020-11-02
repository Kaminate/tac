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
    Render::TextureHandle GetTexture( StringView textureFilepath,
                                      Errors& );
    Render::TextureHandle GetTextureCube( StringView textureDir,
                                          Errors& );
  };
}

