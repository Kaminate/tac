#include "tac_example_dx12_checkerboard.h"


namespace Tac
{
  Vector<u8> Checkerboard::GenerateCheckerboardTextureData()
  {
    const int rowPitch = TextureWidth * TexturePixelSize;
    const int cellPitch = rowPitch >> 3;        // The width of a cell in the checkboard texture.
    const int cellHeight = TextureWidth >> 3;    // The height of a cell in the checkerboard texture.
    const int textureSize = rowPitch * TextureHeight;

    Vector<u8> data( textureSize );
    u8* pData = &data[ 0 ];

    for( int n = 0; n < textureSize; n += TexturePixelSize )
    {
      int x = n % rowPitch;
      int y = n / rowPitch;
      int i = x / cellPitch;
      int j = y / cellHeight;

      if( i % 2 == j % 2 )
      {
        pData[ n + 0 ] = 0x00;    // R
        pData[ n + 1 ] = 0x00;    // G
        pData[ n + 2 ] = 0x00;    // B
        pData[ n + 3 ] = 0xff;    // A
      }
      else
      {
        pData[ n + 0 ] = 0xff;    // R
        pData[ n + 1 ] = 0xff;    // G
        pData[ n + 2 ] = 0xff;    // B
        pData[ n + 3 ] = 0xff;    // A
      }
    }

    return data;
  }
}
