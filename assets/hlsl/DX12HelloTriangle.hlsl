//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

struct ClipSpacePosition3 { float3 mValue; };
struct ClipSpacePosition4 { float4 mValue; };
struct LinearColor3 { float3 mValue; };
struct LinearColor4 { float4 mValue; };

struct Vertex
{
  ClipSpacePosition4 mPosition : POSITION;
  LinearColor4       mColor    : COLOR;
};

struct VSOutput
{
    ClipSpacePosition4  mPosition : SV_POSITION;
    LinearColor4        mColor    : TAC_AUTO_SEMANTIC;
};

typedef VSOutput PSInput;

VSOutput VSMain(Vertex input)
{
  VSOutput result;
  result.mPosition = input.mPosition;
  result.mColor    = input.mColor;
  return result;
}

LinearColor4 PSMain(PSInput input) : SV_TARGET
{
  // NOTE (on gamma)
  //
  //   The triangle vertexes are red, green, and blue, which have the same values in
  //   linear and sRGB space (1,0,0), (0,1,0), (0,0,1).
  //
  //   1) Vertex Shader - First the vertex shader receives these values, and we can
  //      treat them as linear values here.
  //   2) Rasterizer - These linear values are interpolated, and fed to the pixel shader.
  //   3) Pixel shader - These linear values are output to the backbuffer, which is
  //      DXGI_FORMAT_R16G16B16A16_FLOAT. This float display format receives linear-valued data,
  //      which afterwards windows will convert to sRGB when displaying on the monitor.
  //
  // So, if we wanted the vertexes to have a specific sRGB color value, we should first convert it
  // to linear in the vertex shader.

  return input.mColor;
}
