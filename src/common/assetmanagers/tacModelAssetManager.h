#pragma once
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include <map>

namespace Tac
{


struct LoadingMesh;
struct JobQueue;
struct Renderer;
struct VertexFormat;
struct VertexBuffer;
struct IndexBuffer;

typedef Array< v3, 3 > SubMeshTriangle;
v3 GetNormal( const SubMeshTriangle& tri );

struct SubMesh
{
  VertexBuffer* mVertexBuffer = nullptr;
  IndexBuffer* mIndexBuffer = nullptr;
  Vector< SubMeshTriangle > mTris;
  void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist );
};

struct Mesh
{
  Vector< SubMesh > mSubMeshes;
  VertexFormat* mVertexFormat = nullptr;
  void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist );
  m4 mTransform = m4::Identity();
  m4 mTransformInv = m4::Identity();
};

struct ModelAssetManager
{
  static ModelAssetManager* Instance;
  ModelAssetManager();
  ~ModelAssetManager();
  void GetMesh(
    Mesh** mesh,
    const String& path,
    VertexFormat* vertexFormat,
    Errors& errors );
  std::map< String, Mesh* > mMeshes;
};


}
