#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/common/assetmanagers/tacMesh.h"

namespace Tac
{
  typedef Mesh MeshLoadFunction( const char* path,
                                 const int iModel,
                                 const Render::VertexDeclarations&,
                                 Errors& );

  void              ModelLoadFunctionRegister( MeshLoadFunction*, const char* ext );
  MeshLoadFunction* ModelLoadFunctionFind( const char* ext );
}

