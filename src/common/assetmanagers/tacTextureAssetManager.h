#pragma once

#include "common/tacString.h"
#include "common/tacErrorHandling.h"

#include <map>
#include <set>

struct TacRenderer;
struct TacJobQueue;
struct TacTexture;
struct TacAsyncTexture;

struct TacTextureAssetManager
{
  static TacTextureAssetManager* Instance;
  TacTextureAssetManager();
  ~TacTextureAssetManager();
  void GetTexture( TacTexture** ppTexture, const TacString& textureFilepath, TacErrors& errors );
  void GetTextureCube( TacTexture** ppTexture, const TacString& textureDir, TacErrors& errors );
  void UpdateAsyncTexture(
    TacTexture** ppTexture,
    const TacString& key,
    TacAsyncTexture* asyncTexture,
    TacErrors& errors );

  TacTexture* FindLoadedTexture( const TacString& key );
  TacAsyncTexture* FindLoadingTexture( const TacString& key );

  std::map< TacString, TacAsyncTexture* > mLoadingTextures;
  std::map< TacString, TacTexture* > mLoadedTextures;
};
