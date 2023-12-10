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


struct Vertex
{
  float4 position : POSITION;
  float4 color    : COLOR;
};


struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : TAC_AUTO_SEMANTIC;
};

PSInput VSMain(Vertex input)
{
  PSInput result;
  result.position = input.position;
  result.color = input.color;
  return result;
}

float4 sRGBToLinear(float4 color)
{
  return pow(color, 2.2);
}

struct PSOutput
{
  float4 mLinearColor;
};

PSOutput PSMain(PSInput input) : SV_TARGET
{
  //return pow(input.color, 1.0 / 2.0);
  //return input.color;
  //return pow(input.color,  2.0);
  //return sRGBToLinear(input.color);


  // a note on gamma:
  //
  //   the triangle vertexes are red, green, and blue, which have the same values in
  //   linear and sRGB space.
  //
  //   What I think is happening is that first the vertex shader receives these linear values.
  //
  //   Then in the rasterizer, these linear values are interpolated, and fed to the pixel shader.
  //   
  //   Then in the pixel shader, these linear values are output to the backbuffer,
  //   which is DXGI_FORMAT_R16G16B16A16_FLOAT.
  //
  //   This float display formats receive linear-valued data, which afterwards windows will convert
  //   to sRGB when displaying on the monitor.
  //
  // So, if we wanted the vertexes to have a sRGB color value, then we should first convert it to
  // linear in the vertex shader.

  PSOutput output;
  output.mLinearColor = input.color;
  return output;
}
