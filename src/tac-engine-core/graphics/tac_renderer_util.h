// This file is for things that you may want to use after including tac_renderer.h
#pragma once

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/tac_ints.h"



namespace Tac::Render
{
  
  struct ShaderFlags
  {
    struct Info
    {
      auto ShiftResult( u32 unshifted ) const -> u32;
      auto Extract( u32 flags ) const -> u32;
      int mOffset;
      int mBitCount;
    };

    Info Add( int bitCount );
    int mRunningBitCount {};
  };

  // this should really be called like per camera data
  struct DefaultCBufferPerFrame
  {
    static auto name_view()        { return "View";        };
    static auto name_proj()        { return "Projection";  };
    static auto name_far()         { return "far";         };
    static auto name_near()        { return "near";        };
    static auto name_gbuffersize() { return "gbufferSize"; };
    static void Init( Errors& );

    m4                                  mView;
    m4                                  mProjection;
    float                               mFar;
    float                               mNear;
    v2                                  mGbufferSize;
    float                               mSecModTau;
    float                               mSDFOnEdge; // [0, 1]
    float                               mSDFPixelDistScale; // [0, 1]
    static Render::BufferHandle         sHandle;
  };

  struct PremultipliedAlpha
  {
    static auto From_sRGB( const v3& ) -> PremultipliedAlpha;
    static auto From_sRGB_linearAlpha( const v3&, float ) -> PremultipliedAlpha;
    static auto From_sRGB_linearAlpha( const v4& ) -> PremultipliedAlpha;

    explicit PremultipliedAlpha( const v4& );
    PremultipliedAlpha() = default;

    v4 mColor {  1, 1, 1, 1 } ;
  };


  struct DefaultCBufferPerObject
  {
    static auto name_world()       { return "World";       };
    static auto name_color()       { return "Color";       };
    static void Init(Errors&);
    auto GetWorld() const -> const m4&;
    auto GetColor() const -> const v4&;

    static Render::BufferHandle         sHandle;
    m4                                  World { m4::Identity() };
    PremultipliedAlpha                  Color;
  };



  auto GetShaderLightFlagType() -> const ShaderFlags::Info*;
  auto GetShaderLightFlagCastsShadows() -> const ShaderFlags::Info*;

  // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
  struct ShaderLight
  {
    m4       mWorldToClip;
    v4       mWorldSpacePosition;
    v4       mWorldSpaceUnitDirection;
    v4       mColorRadiance;
    u32      mFlags    {};
    float    mProjA    {};
    float    mProjB    {};
    TAC_PAD_BYTES( 4 ) {};
  };

  struct CBufferLights
  {

    bool TryAddLight( const ShaderLight& );
    static void Init( Errors& );

    static const int TAC_MAX_SHADER_LIGHTS            { 4 };
    static const int sShaderRegister                  { 2 };
    ShaderLight      lights[ TAC_MAX_SHADER_LIGHTS ]  {};
    u32              lightCount                       {};
    u32              useLights                        { true };
    u32              testNumber                       { 1234567890 };

    static Render::BufferHandle sHandle;
  };

  // maybe this should be in renderer idk
  auto ToColorAlphaPremultiplied( const v4& colorAlphaUnassociated ) -> v4;


} // namespace Tac::Render

