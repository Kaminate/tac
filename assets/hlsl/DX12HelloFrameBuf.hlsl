
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

#if __DXC_VERSION_MAJOR <= 1 && __DXC_VERSION_MINOR < 7
  #error dxc ver is less than 1.7
#endif

#if __HLSL_VERSION < 2021
  #error hlsl version is less than 2021
#endif

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

#pragma pack_matrix( row_major )

// must match MyCBufType in c++
struct MyCBufType
{
  matrix           mWorld;
  uint             mVertexBuffer;
  uint             mTexture;
};


typedef VSOutput PSInput;

ByteAddressBuffer            BufferTable[] : register( t0, space0 );
Texture2D                    Textures[]    : register( t0, space1 );
SamplerState                 Sampler       : register( s0 );
ConstantBuffer< MyCBufType > MyCBuf        : register( b0 );


VSOutput VSMain( uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof( Vertex ) * iVtx;
  const ByteAddressBuffer vertexBytes = BufferTable[ MyCBuf.mVertexBuffer ];
  const Vertex input = vertexBytes.Load< Vertex >( byteOffset );

  VSOutput result;
  result.mPosition = ClipSpacePosition4(mul(MyCBuf.mWorld, float4(input.mPosition, 1)));
  result.mColor = input.mColor;
  result.mUVs = input.mUVs;
  return result;
}

LinearColor4 PSMain( PSInput input) : SV_TARGET
{
  Texture2D texture = Textures[MyCBuf.mTexture];

  const float sample = texture.Sample(Sampler, input.mUVs.mFloat2).x;
  const float3 rgb = lerp(input.mColor.mFloat3, float3(1, 1, 1), sample);

  return LinearColor4(float4(rgb, 1));
}

// TODO: save below to hlsl test?
//
//  struct Foo { float fooI; };
//  struct Bar { float2 barI; };
//  Foo foo = Foo(0.5);
//  Bar bar = Bar(float2(foo.fooI, 3));

