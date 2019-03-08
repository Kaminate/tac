#pragma once

#include "common/tacString.h"
#include "common/tacErrorHandling.h"

#include <map>
#include <set>

struct TacRenderer;
struct TacJobQueue;
struct TacTexture;
struct TacAsyncTextureData;

struct TacTextureAssetManager
{
  void GetTexture( TacTexture** ppTexture, const TacString& textureFilepath, TacErrors& errors );
  std::set< TacAsyncTextureData* > mLoadingTextures;
  TacJobQueue* mJobQueue = nullptr;
  TacRenderer* mRenderer = nullptr;
  std::map< TacString, TacTexture* > mLoadedTextureMap;
};


