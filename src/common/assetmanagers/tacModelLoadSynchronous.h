#pragma once

#include "src/common/containers/tacVector.h"
#include "src/common/string/tacString.h"
#include "src/common/math/tacVector3.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"

namespace Tac
{
  struct Mesh;
  struct StringView;
  struct Errors;
  //Mesh LoadMeshSynchronous( const StringView& path,
  //                          const Render::VertexDeclarations&,
  //                          Errors& );

  Mesh LoadMeshIndexSynchronous( const StringView& path,
                                 int specifiedMeshIndex,
                                 const Render::VertexDeclarations&,
                                 Errors& );
}

