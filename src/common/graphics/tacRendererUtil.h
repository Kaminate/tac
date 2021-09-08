// This file is for things that you may want to use after including tacRenderer.h
#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacPreprocessor.h"

#include <cinttypes>

#define TAC_PAD_BYTES( byteCount ) char TAC_CONCAT( mPadding, __COUNTER__ )[ byteCount ]

namespace Tac
{
  struct ShaderFlags
  {
    struct Info
    {
      uint32_t ShiftResult( uint32_t unshifted ) const;
      uint32_t Extract( uint32_t flags ) const;
      int      mOffset;
      int      mBitCount;
    };
    Info       Add( int bitCount );
    int        mRunningBitCount = 0;
  };

  // this should really be called like per camera data
  struct DefaultCBufferPerFrame
  {
    static const char* name_view()        { return "View";        };
    static const char* name_proj()        { return "Projection";  };
    static const char* name_far()         { return "far";         };
    static const char* name_near()        { return "near";        };
    static const char* name_gbuffersize() { return "gbufferSize"; };
    m4                 mView;
    m4                 mProjection;
    float              mFar;
    float              mNear;
    v2                 mGbufferSize;
    float              mSecModTau;
    static void        Init();
    static Render::ConstantBufferHandle Handle;
  };

  struct DefaultCBufferPerObject
  {
    static const char* name_world()       { return "World";       };
    static const char* name_color()       { return "Color";       };
    m4                 World;
    v4                 Color;
    static void      Init();
    static Render::ConstantBufferHandle Handle;
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
    uint32_t mFlags;
    float    mProjA;
    float    mProjB;
    TAC_PAD_BYTES( 4 );
  };

  struct CBufferLights
  {
    static const int TAC_MAX_SHADER_LIGHTS = 4;
    static const int shaderRegister = 2;

    ShaderLight      lights[ TAC_MAX_SHADER_LIGHTS ] = {};
    uint32_t         lightCount = 0;
    uint32_t         useLights = true;
    uint32_t         testNumber = 1234567890;
    bool             TryAddLight( const ShaderLight& );

    static void      Init();
    static Render::ConstantBufferHandle Handle;
  };

  // maybe this should be in renderer idk
  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );

}

