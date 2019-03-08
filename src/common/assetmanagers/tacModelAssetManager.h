#pragma once
#include "common/tacString.h"
#include "common/tacErrorHandling.h"

#include <map>
#include <set>

struct TacLoadingMesh;
struct TacJobQueue;
struct TacRenderer;

struct TacMesh
{
};

struct TacModelAssetManager
{
  void GetMesh( TacMesh** mesh, const TacString& path, TacErrors& errors );
  std::map< TacString, TacMesh* > mLoadedMeshes;
  std::set< TacLoadingMesh* > mLoadingMeshes;
  TacJobQueue* mJobQueue = nullptr;
  TacRenderer* mRenderer = nullptr;
};


