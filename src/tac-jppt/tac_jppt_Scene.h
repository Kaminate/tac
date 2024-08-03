#pragma once

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector3i.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-rhi/render3/tac_render_api.h"

namespace Tac::gpupt
{
  inline constexpr int InvalidID{ -1 };

  struct Camera
  {
    m4    mFrame{ 1 };
    float mLens{ 0.05f }; // ?
    float mFilm{ 0.036f }; // ?
    float mAspect{ 1.5f };
    float mFocus{ 1000 }; // ?
    v3    mPad0{};
    float mAperture{};
    i32   mOrthographics{};
    v3    mPad1{};
  };
  // ^ wat is with this weird padding

  static_assert( sizeof( Camera ) % 16 == 0 );

  // ???
  struct Instance
  {
    m4  GetModelMatrix() const;

    v3  mPosition;
    v3  mRotation; // is this euler radians?
    v3  mScale{ 1 };
    int mShape{ InvalidID };
  };


  // ???
  struct Shape
  {
    void CalculateTangents();

    Vector< v3 >  mPositions;
    Vector< v3 >  mNormals;
    Vector< v2 >  mTexCoords;
    Vector< v4 >  mColours;
    Vector< v4 >  mTangents;
    Vector< v3i > mTriangles;
  };

  // ???
  struct Scene
  {
    static Scene* CreateCornellBox();

    Vector< Camera >     mCameras;
    Vector< Instance >   mInstances;
    Vector< Shape >      mShapes;
    Vector< String >     mCameraNames;
    Vector< String >     mInstanceNames;
    Vector< String >     mShapeNames;
    Render::BufferHandle mCamerasBuffer;
  };

} // namespace Tac::gpupt
