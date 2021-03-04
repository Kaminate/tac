// This file is for things that you may want to use after including tacRenderer.h
#pragma once

#include "src/common/graphics/tacRenderer.h"
#include "src/common/math/tacVector4.h"
#include "src/common/math/tacMatrix4.h"
#include "src/common/tacPreprocessor.h"
#include <cstdint>


namespace Tac
{
  const v4 colorGrey = v4( v3( 1, 1, 1 ) * 95.0f, 255 ) / 255.0f;
  const v4 colorOrange = v4( 255, 200, 84, 255 ) / 255.0f;
  const v4 colorGreen = v4( 0, 255, 112, 255 ) / 255.0f;
  const v4 colorBlue = v4( 84, 255, 255, 255 ) / 255.0f;
  const v4 colorRed = v4( 255, 84, 84, 255 ) / 255.0f;
  const v4 colorMagenta = v4( 255, 84, 255, 255 ) / 255.0f;

  const Format formatv2 = { 2, sizeof( float ), GraphicsType::real };
  const Format formatv3 = { 3, sizeof( float ), GraphicsType::real };
  const Format formatu16 = { 1, sizeof( uint16_t ), GraphicsType::uint };

  struct DefaultCBufferPerFrame
  {
    static const char* name_view() { return "View"; };
    static const char* name_proj() { return "Projection"; };
    static const char* name_far() { return "far"; };
    static const char* name_near() { return "near"; };
    static const char* name_gbuffersize() { return "gbufferSize"; };
    static const int   shaderRegister = 0;
    m4                 mView;
    m4                 mProjection;
    float              mFar;
    float              mNear;
    v2                 mGbufferSize;
  };

  struct DefaultCBufferPerObject
  {
    static const char* name_world() { return "World"; };
    static const char* name_color() { return "Color"; };
    static const int   shaderRegister = 1;
    m4                 World;
    v4                 Color;
  };

  // maybe this should be in renderer idk
  v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated );
}

