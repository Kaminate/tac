#pragma once

#include "src/common/containers/tac_vector.h"
#include "src/common/tac_ints.h"

namespace Tac
{
  struct Checkerboard
  {
    static const int TextureWidth = 256;
    static const int TextureHeight = 256;

    // The number of bytes used to represent a pixel in the texture.
    static const int TexturePixelSize = 4;

    static Vector<u8> GenerateCheckerboardTextureData();
  };

}
