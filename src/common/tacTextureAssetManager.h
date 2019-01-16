#pragma once

#include "tacAssetManager.h"
#include "tacRenderer.h"

#include <map>

struct TacAssetManager;
struct TacRenderer;
struct TacJobQueue;

struct TacAsyncTextureData;

struct TacTextureAssetManager
{
  void GetTexture( TacTexture** ppTexture, const TacString& textureFilepath, TacErrors& errors );
  std::set< TacAsyncTextureData* > mLoadingTextures;
  //TacAssetManager* mAssetManager = nullptr;
  TacJobQueue* mJobQueue = nullptr;
  TacRenderer* mRenderer = nullptr;
  std::map< TacString, TacTexture* > mLoadedTextureMap;
};


