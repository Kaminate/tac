#include "src/common/graphics/tacRendererUtil.h"


namespace Tac
{
  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
  {
    return {
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w };
  }
}

