#pragma once
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"


struct TacLoadingMesh;
struct TacJobQueue;
struct TacRenderer;
struct TacVertexFormat;
struct TacVertexBuffer;
struct TacIndexBuffer;

struct TacSubMesh
{
  TacVertexBuffer* mVertexBuffer = nullptr;
  TacIndexBuffer* mIndexBuffer = nullptr;
};

struct TacMesh
{
  TacVector< TacSubMesh > mSubMeshes;
  TacVertexFormat* mVertexFormat = nullptr;
};

struct TacModelAssetManager
{
  void GetMesh( TacMesh** mesh, const TacString& path, TacVertexFormat* vertexFormat, TacErrors& errors );
  TacJobQueue* mJobQueue = nullptr;
  TacRenderer* mRenderer = nullptr;
};


