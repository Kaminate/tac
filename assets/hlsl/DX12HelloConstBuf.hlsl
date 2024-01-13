
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

// -------------------------------------------------------------------------------------------------

//const int verA = __DXC_VERSION_MAJOR;
//const int verB = __DXC_VERSION_MINOR;
//const int verC = __DXC_VERSION_RELEASE;
//const int verD = __DXC_VERSION_COMMITS;
// version is a.b.c.d

//template< typename T, uint N>
//uint StrLen(T str[N])
//{
//    // Includes the null terminator
//  return N;
//}

// -------------------------------------------------------------------------------------------------

#include "dx12_math_types.hlsli"

struct Vertex
{
  ClipSpacePosition3 mPosition;
  LinearColor3       mColor;
  TextureCoordinate2 mUVs;
};

struct VSOutput
{
  ClipSpacePosition4 mPosition : SV_POSITION;
  LinearColor3       mColor    : TAC_AUTO_SEMANTIC;
  TextureCoordinate2 mUVs      : TAC_AUTO_SEMANTIC;
};

struct MyCBufType
{
  uint mVertexBuffer;
  uint mTexture;
};

static uint mVertexBuffer = 0;
static uint mTexture = 0;

typedef VSOutput PSInput;

ByteAddressBuffer            BufferTable[] : register( t0, space0 );
Texture2D                    Textures[]    : register( t0, space1 );
SamplerState                 Sampler       : register( s0 );
ConstantBuffer< MyCBufType > MyCBuf        : register( b0 ); // unused

VSOutput VSMain(uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof( Vertex ) * iVtx;
  const ByteAddressBuffer vertexBytes = BufferTable[ mVertexBuffer ];
  const Vertex input = vertexBytes.Load< Vertex >( byteOffset );

  VSOutput result;
  //result.mPosition = ClipSpacePosition4::Ctor(input.mPosition  //  ClipSpacePosition3to4(input.mPosition);
  //result.mPosition = input.mPosition;
  result.mPosition.Set(input.mPosition);
  result.mColor = input.mColor;
  result.mUVs = input.mUVs;
  return result;
}

LinearColor4 PSMain(PSInput input) : SV_TARGET
{
  Texture2D texture = Textures[mTexture];

  const float sample = texture.Sample(Sampler, input.mUVs.mFloat2).x;
  const float3 rgb = lerp(input.mColor.mFloat3, float3(1, 1, 1), sample);

  LinearColor4 result;
  result.mFloat4 = float4(rgb, 1);
  return result;
}
