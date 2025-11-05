// i think this file should be broken into 2 parts
// 1) load a .obj into ram
// 2) convert ram into mesh
#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector3.h"

namespace Tac
{
  struct WavefrontObj
  {
    struct Vertex
    {
      // 0-based indexes
      int miPosition {};
      int miNormal   {};
      int miTexCoord {};
    };

    struct Face
    {
      Vertex mVertexes[ 3 ]{};
    };

    static void Init();
    static auto Load( const void*, int ) -> WavefrontObj;

    Vector< v3 >   mNormals;
    Vector< v2 >   mTexCoords;
    Vector< v3 >   mPositions;
    Vector< Face > mFaces;
  };

} // namespace Tac

