#include "tac_renderer_util.h" // self-inc
#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac::Render
{
  v4 ToColorAlphaPremultiplied( const v4& colorAlphaUnassociated )
  {
    return {
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w };
  }

  u32 ShaderFlags::Info::ShiftResult( u32 unshifted ) const
  {
    return unshifted << mOffset;
  }

  u32 ShaderFlags::Info::Extract( u32 flags ) const
  {
    return ( flags >> mOffset ) & ( ( 1 << mBitCount ) - 1 );
  }

  ShaderFlags::Info ShaderFlags::Add( int bitCount )
  {
    const Info shaderFlag
    {
      .mOffset = mRunningBitCount,
      .mBitCount = bitCount
    };
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

  Render::BufferHandle CBufferLights::sHandle;
  Render::BufferHandle DefaultCBufferPerFrame::sHandle;
  Render::BufferHandle DefaultCBufferPerObject::sHandle;

  void      DefaultCBufferPerFrame::Init( Errors& errors )
  {
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::CreateBufferParams params
    {
      .mByteCount = sizeof( DefaultCBufferPerFrame ),
      .mAccess = Render::Usage::Dynamic,
      .mOptionalName =  "CBufferPerFrame",
      .mStackFrame = TAC_STACK_FRAME,
    };
    sHandle = TAC_CALL( renderDevice->CreateBuffer( params, errors ) );
  }

  void      DefaultCBufferPerObject::Init(Errors& errors)
  {
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::CreateBufferParams params
    {
      .mByteCount = sizeof( DefaultCBufferPerObject ),
      .mAccess = Render::Usage::Dynamic,
      .mOptionalName =  "CBufferPerObject",
      .mStackFrame = TAC_STACK_FRAME,
    };
    sHandle = TAC_CALL( renderDevice->CreateBuffer( params, errors ) );
  }

  //void      DefaultCBufferPerObject::SetColor( const v3& sRGB, float linearAlpha )
  //{
  //  Color = GetColor( sRGB, linearAlpha );
  //}

  //v4        DefaultCBufferPerObject::GetColor( const v3& sRGB, float linearAlpha )
  //{
  //  // premultiplied alpha
  //  v4 Color;
  //  Color.xyz() = sRGB * linearAlpha;
  //  Color.w = linearAlpha;
  //  return Color;
  //}

   PremultipliedAlpha::PremultipliedAlpha( const v4& c ) : Color( c ) {}

  PremultipliedAlpha PremultipliedAlpha::From_sRGB( const v3& sRGB )
  {
    return PremultipliedAlpha( { sRGB, 1.0f } );
  }

  PremultipliedAlpha PremultipliedAlpha::From_sRGB_linearAlpha( const v3& sRGB, float linearAlpha )
  {
    return PremultipliedAlpha( { sRGB * linearAlpha, linearAlpha } );
  }

  PremultipliedAlpha PremultipliedAlpha::From_sRGB_linearAlpha( const v4& sRGB_linearAlpha )
  {
    return From_sRGB_linearAlpha( sRGB_linearAlpha.xyz(), sRGB_linearAlpha.w );
  }

  //DefaultCBufferPerObject::DefaultCBufferPerObject( const m4& world, const Premu  & sRGB )
  //  : World( world ), Color( sRGB, 1 )
  //{
  //}

  //DefaultCBufferPerObject::DefaultCBufferPerObject( const m4& world, const v3& sRGB, float linearAlpha )
  //  : World( world ),
  //  Color( sRGB* linearAlpha, linearAlpha )
  //{
  //}

  //DefaultCBufferPerObject::DefaultCBufferPerObject( const v3& sRGB )
  //  : Color( sRGB, 1 ) {}
  //DefaultCBufferPerObject::DefaultCBufferPerObject( const v3& sRGB, float linearAlpha )
  //  : Color( sRGB* linearAlpha, linearAlpha )
  //{
  //}

  const m4& DefaultCBufferPerObject::GetWorld() const { return World; }

  const v4& DefaultCBufferPerObject::GetColor() const { return Color.Color; }

  void      CBufferLights::Init(Errors& errors)
  {
    Render::IDevice* renderDevice = Render::RenderApi::GetRenderDevice();
    Render::CreateBufferParams params
    {
      .mByteCount = sizeof( CBufferLights ),
      .mAccess = Render::Usage::Dynamic,
      .mOptionalName =  "CBufferLights",
      .mStackFrame = TAC_STACK_FRAME,
    };
    sHandle = TAC_CALL( renderDevice->CreateBuffer( params, errors ) );
  }

} // namespace Tac::Render

