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


  static ShaderFlags       shaderLightFlags;
  static ShaderFlags::Info shaderLightFlagType = shaderLightFlags.Add( 4 );
  static ShaderFlags::Info shaderLightFlagCastsShadows = shaderLightFlags.Add( 1 );
  const ShaderFlags::Info* GetShaderLightFlagType() { return &shaderLightFlagType; }
  const ShaderFlags::Info* GetShaderLightFlagCastsShadows() { return &shaderLightFlagCastsShadows; }

  bool             CBufferLights::TryAddLight( const ShaderLight& shaderLight )
  {
    const bool result = lightCount < TAC_MAX_SHADER_LIGHTS;
    if( result )
      lights[ lightCount++ ] = shaderLight;
    return result;
  }

  Render::ConstantBufferHandle CBufferLights::Handle;
  Render::ConstantBufferHandle DefaultCBufferPerFrame::Handle;
  Render::ConstantBufferHandle DefaultCBufferPerObject::Handle;

  void      DefaultCBufferPerFrame::Init()
  {
    Handle = Render::CreateConstantBuffer( "CBufferPerFrame",
                                           sizeof( DefaultCBufferPerFrame ),
                                           TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( Handle, "per-frame" );
  }

  void      DefaultCBufferPerObject::Init()
  {
    Handle = Render::CreateConstantBuffer( "CBufferPerObject",
                                           sizeof( DefaultCBufferPerObject ),
                                           TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( Handle, "per-obj" );
  }

  void      CBufferLights::Init()
  {
    Handle = Render::CreateConstantBuffer( "CBufferLights",
                                           sizeof( CBufferLights ),
                                           TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( Handle, "lights" );
  }

}

