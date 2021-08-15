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

  uint32_t ShaderFlags::Info::ShiftResult( uint32_t unshifted ) const
  {
    return unshifted << mOffset;
  }

  uint32_t ShaderFlags::Info::Extract( uint32_t flags ) const
  {
    return ( flags >> mOffset ) & ( ( 1 << mBitCount ) - 1 );
  }

  ShaderFlags::Info ShaderFlags::Add( int bitCount )
  {
    Info shaderFlag;
    shaderFlag.mOffset = mRunningBitCount;
    shaderFlag.mBitCount = bitCount;
    mRunningBitCount += bitCount;
    return shaderFlag;
  }

}

