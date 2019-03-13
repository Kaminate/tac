#pragma once
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include "common/containers/tacArray.h"
#include "common/math/tacVector3.h"


struct TacLoadingMesh;
struct TacJobQueue;
struct TacRenderer;
struct TacVertexFormat;
struct TacVertexBuffer;
struct TacIndexBuffer;

typedef TacArray< v3, 3 > TacSubMeshTriangle;
v3 TacGetNormal( const TacSubMeshTriangle& tri );

struct TacSubMesh
{
  TacVertexBuffer* mVertexBuffer = nullptr;
  TacIndexBuffer* mIndexBuffer = nullptr;
  TacVector< TacSubMeshTriangle > mTris;
  void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, v3* outHitPoint);
};

struct TacMesh
{
  TacVector< TacSubMesh > mSubMeshes;
  TacVertexFormat* mVertexFormat = nullptr;
  void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, v3* outHitPoint);
};

struct TacModelAssetManager
{
  void GetMesh( TacMesh** mesh, const TacString& path, TacVertexFormat* vertexFormat, TacErrors& errors );
  TacJobQueue* mJobQueue = nullptr;
  TacRenderer* mRenderer = nullptr;
};


