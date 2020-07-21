#pragma once
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/containers/tacArray.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"
#include <map>

namespace Tac
{


  struct LoadingMesh;
  struct JobQueue;
  struct Renderer;

  typedef Array< v3, 3 > SubMeshTriangle;
  typedef Vector< SubMeshTriangle > SubMeshTriangles;

  v3 GetNormal( const SubMeshTriangle& tri );

  struct SubMesh
  {
    Render::VertexBufferHandle mVertexBuffer;
    Render::IndexBufferHandle mIndexBuffer;
    SubMeshTriangles mTris;
    void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist );
    int mIndexCount = 0;
  };

  struct Mesh
  {
    Vector< SubMesh > mSubMeshes;
    Render::VertexFormatHandle mVertexFormat;
    void Raycast( v3 inRayPos, v3 inRayDir, bool* outHit, float* outDist );
    m4 mTransform = m4::Identity();
    m4 mTransformInv = m4::Identity();
  };

  struct ModelAssetManager
  {
    static ModelAssetManager* Instance;
    ModelAssetManager();
    ~ModelAssetManager();
    void GetMesh( Mesh** mesh,
                  StringView path,
                  Render::VertexFormatHandle vertexFormat,
                  VertexDeclaration* vertexDeclarations,
                  int vertexDeclarationCount,
                  Errors& errors );
    std::map< StringID, Mesh* > mMeshes;
  };


}
