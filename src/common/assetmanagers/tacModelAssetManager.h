#pragma once

//#include "src/common/tacString.h"
//#include "src/common/tacErrorHandling.h"
//#include "src/common/containers/tacVector.h"
//#include "src/common/containers/tacArray.h"
//#include "src/common/math/tacVector3.h"
//#include "src/common/math/tacVector4.h"
//#include "src/common/math/tacMatrix4.h"
//#include "src/common/graphics/tacRenderer.h"

//#include <map>

namespace Tac
{
  struct Mesh;
  struct StringView;
  struct VertexDeclarations;
  struct Errors;
  void                       ModelAssetManagerUninit();

  //                         the mesh will be loaded into the vertex format specified by vertex declarations.
  void                       ModelAssetManagerGetMesh( Mesh** mesh,
                                                       StringView path,
                                                       const VertexDeclarations&,
                                                       Errors& );
}
