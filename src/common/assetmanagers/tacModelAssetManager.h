#pragma once

//#include "src/common/tacString.h"
//#include "src/common/tacErrorHandling.h"
//#include "src/common/containers/tacVector.h"
//#include "src/common/containers/tacArray.h"
//#include "src/common/math/tacVector3.h"
//#include "src/common/math/tacVector4.h"
//#include "src/common/math/tacMatrix4.h"
#include "src/common/graphics/tacRenderer.h"

//#include <map>
//#include <iosfwd>

//struct VertexDeclaration;
//template<typename T, int N> struct FixedVector;
//typedef FixedVector<VertexDeclaration, 10>VertexDeclarations;

//template<VertexDeclaration, 10> struct VertexDeclarations;

//template<class T = VertexDeclaration, int N = 10 > struct VertexDeclarations;

//template< ,
//	class _Traits = char_traits<_Elem>,
//	class _Alloc = allocator<_Elem>>
//	struct VertexDeclarations;

namespace Tac
{
  struct Mesh;
  struct StringView;
  //struct VertexDeclarations;
  struct Errors;
  void                       ModelAssetManagerUninit();

  //                         the mesh will be loaded into the vertex format specified by vertex declarations.
  void                       ModelAssetManagerGetMesh( Mesh** mesh,
                                                       StringView path,
                                                       const Render::VertexDeclarations&,
                                                       Errors& );
}
