#pragma once

#include "src/common/math/tac_vector3.h"
#include "src/common/containers/tac_optional.h"

namespace Tac
{

  struct BarycentricTriInput
  {
    v3 mP;
    v3 mTri0;
    v3 mTri1;
    v3 mTri2;
  };

  struct BarycentricTriOutput
  {
    float mBary0;
    float mBary1;
    float mBary2;
  };

  Optional< BarycentricTriOutput > BarycentricTriangle( BarycentricTriInput );

  // ----------------------------------------------------------------------------------------------

  struct BarycentricTetInput
  {
    v3 mP;
    v3 mTet0;
    v3 mTet1;
    v3 mTet2;
    v3 mTet3;
  };

  struct BarycentricTetOutput
  {
    float mBary0;
    float mBary1;
    float mBary2;
    float mBary3;
  };

  Optional< BarycentricTetOutput > BarycentricTetrahedron( BarycentricTetInput );

} // namespace Tac

