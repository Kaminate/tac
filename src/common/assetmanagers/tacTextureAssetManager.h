
#pragma once

#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"

#include <map>
#include <set>
namespace Tac
{

struct Renderer;
struct JobQueue;
struct Texture;
struct AsyncTexture;

struct TextureAssetManager
{
  static TextureAssetManager* Instance;
  TextureAssetManager();
  ~TextureAssetManager();
  void GetTexture( Texture** ppTexture, const String& textureFilepath, Errors& errors );
  void GetTextureCube( Texture** ppTexture, const String& textureDir, Errors& errors );
  void UpdateAsyncTexture(
    Texture** ppTexture,
    const String& key,
    AsyncTexture* asyncTexture,
    Errors& errors );

  Texture* FindLoadedTexture( const String& key );
  AsyncTexture* FindLoadingTexture( const String& key );

  std::map< String, AsyncTexture* > mLoadingTextures;
  std::map< String, Texture* > mLoadedTextures;
};

}

