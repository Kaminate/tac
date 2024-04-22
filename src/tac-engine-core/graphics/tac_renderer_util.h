// This file is for things that you may want to use after including tac_renderer.h
#pragma once

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_matrix4.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/tac_ints.h"


#define TAC_PAD_BYTES( byteCount ) char TAC_CONCAT( mPadding, __COUNTER__ )[ byteCount ]

namespace Tac::Render
{
  
  struct ShaderFlags
  {
    struct Info
    {
      u32 ShiftResult( u32 unshifted ) const;
      u32 Extract( u32 flags ) const;
      int      mOffset;
      int      mBitCount;
    };

    Info       Add( int bitCount );
    int        mRunningBitCount = 0;
  };

  // this should really be called like per camera data
  struct DefaultCBufferPerFrame
  {
    static const char*                  name_view()        { return "View";        };
    static const char*                  name_proj()        { return "Projection";  };
    static const char*                  name_far()         { return "far";         };
    static const char*                  name_near()        { return "near";        };
    static const char*                  name_gbuffersize() { return "gbufferSize"; };
    static void                         Init(Errors&);

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
    static PremultipliedAlpha From_sRGB( const v3& );
    static PremultipliedAlpha From_sRGB_linearAlpha( const v3&, float );
    static PremultipliedAlpha From_sRGB_linearAlpha( const v4& );

    explicit PremultipliedAlpha( const v4& );
    PremultipliedAlpha() = default;
    v4 Color = { 1, 1, 1, 1 };
  };


  struct DefaultCBufferPerObject
  {

    //DefaultCBufferPerObject() = default;
    //DefaultCBufferPerObject( const m4&, const PremultipliedAlpha& );

    static const char*                  name_world()       { return "World";       };
    static const char*                  name_color()       { return "Color";       };
    static void                         Init(Errors&);
    static Render::BufferHandle         sHandle;

    const m4&                           GetWorld() const;
    const v4&                           GetColor() const;

  //private:
    m4                                  World = m4::Identity();
    PremultipliedAlpha                  Color;
  };



  const ShaderFlags::Info* GetShaderLightFlagType();
  const ShaderFlags::Info* GetShaderLightFlagCastsShadows();

  // https://docs.microsoft.com/en-us/windows/win32/direct3dhlsl/dx-graphics-hlsl-packing-rules
  struct ShaderLight
  {
    m4       mWorldToClip;
    v4       mWorldSpacePosition;
    v4       mWorldSpaceUnitDirection;
    v4       mColorRadiance;
    u32      mFlags;
    //float    mProjA;
    //float    mProjB;
    //TAC_PAD_BYTES( 4 );
    TAC_PAD_BYTES( 12 );
  };

  struct CBufferLights
  {
    static const int TAC_MAX_SHADER_LIGHTS = 4;
    static const int sShaderRegister = 2;

    ShaderLight      lights[ TAC_MAX_SHADER_LIGHTS ] = {};
    u32              lightCount = 0;
    u32              useLights = true;
    u32              testNumber = 1234567890;
    bool             TryAddLight( const ShaderLight& );

    static void      Init( Errors& );

    static Render::BufferHandle sHandle;
  };

  // maybe this should be in renderer idk
  v4 ToColorAlphaPremultiplied( const v4& colorAlphaUnassociated );


} // namespace Tac::Render

