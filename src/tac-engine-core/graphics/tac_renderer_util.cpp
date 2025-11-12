#include "tac_renderer_util.h" // self-inc
#include "tac-std-lib/error/tac_error_handling.h"


namespace Tac::Render
{
  auto ShaderFlags::Info::ShiftResult( u32 unshifted ) const -> u32
  {
    return unshifted << mOffset;
  }

  auto ShaderFlags::Info::Extract( u32 flags ) const -> u32
  {
    return ( flags >> mOffset ) & ( ( 1 << mBitCount ) - 1 );
  }

  auto ShaderFlags::Add( int bitCount ) -> ShaderFlags::Info
  {
    const Info shaderFlag
    {
      .mOffset   { mRunningBitCount },
      .mBitCount { bitCount },
    };
    mRunningBitCount += bitCount;
    return shaderFlag;
  }


  static ShaderFlags       shaderLightFlags;
  static ShaderFlags::Info shaderLightFlagType              { shaderLightFlags.Add( 4 ) };
  static ShaderFlags::Info shaderLightFlagCastsShadows      { shaderLightFlags.Add( 1 ) };
  Render::BufferHandle     CBufferLights::sHandle;
  Render::BufferHandle     DefaultCBufferPerFrame::sHandle;
  Render::BufferHandle     DefaultCBufferPerObject::sHandle;

  auto CBufferLights::TryAddLight( const ShaderLight& shaderLight ) -> bool
  {
    const bool result { lightCount < TAC_MAX_SHADER_LIGHTS };
    if( result )
      lights[ lightCount++ ] = shaderLight;
    return result;
  }

  void DefaultCBufferPerFrame::Init( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::CreateBufferParams params
    {
      .mByteCount    { sizeof( DefaultCBufferPerFrame ) },
      .mUsage        { Render::Usage::Dynamic },
      .mBinding      { Render::Binding::ConstantBuffer },
      .mOptionalName { "CBufferPerFrame" },
    };
    sHandle = TAC_CALL( renderDevice->CreateBuffer( params, errors ) );
  }

  void DefaultCBufferPerObject::Init(Errors& errors)
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::CreateBufferParams params
    {
      .mByteCount    { sizeof( DefaultCBufferPerObject ) },
      .mUsage        { Render::Usage::Dynamic },
      .mBinding      { Render::Binding::ConstantBuffer },
      .mOptionalName {  "CBufferPerObject" },
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

  PremultipliedAlpha::PremultipliedAlpha( const v4& c ) : mColor( c ) {}

  auto PremultipliedAlpha::From_sRGB( const v3& sRGB ) -> PremultipliedAlpha
  {
    return PremultipliedAlpha( { sRGB, 1.0f } );
  }

  auto PremultipliedAlpha::From_sRGB_linearAlpha( const v3& sRGB, float linearAlpha ) -> PremultipliedAlpha
  {
    return PremultipliedAlpha( { sRGB * linearAlpha, linearAlpha } );
  }

  auto PremultipliedAlpha::From_sRGB_linearAlpha( const v4& sRGB_linearAlpha ) -> PremultipliedAlpha
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

  auto DefaultCBufferPerObject::GetWorld() const -> const m4&{ return World; }

  auto DefaultCBufferPerObject::GetColor() const -> const v4&{ return Color.mColor; }

  void CBufferLights::Init(Errors& errors)
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    Render::CreateBufferParams params
    {
      .mByteCount    { sizeof( CBufferLights ) },
      .mUsage        { Render::Usage::Dynamic },
      .mBinding      { Render::Binding::ConstantBuffer },
      .mOptionalName { "CBufferLights" },
    };
    sHandle = TAC_CALL( renderDevice->CreateBuffer( params, errors ) );
  }

} // namespace Tac::Render

namespace Tac
{
  auto Render::ToColorAlphaPremultiplied( const v4& colorAlphaUnassociated ) -> v4
  {
    return {
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w };
  }
  auto Render::GetShaderLightFlagType() -> const ShaderFlags::Info*          { return &shaderLightFlagType; }
  auto Render::GetShaderLightFlagCastsShadows() -> const ShaderFlags::Info*  { return &shaderLightFlagCastsShadows; }
}

