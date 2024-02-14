
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

struct MyCBufType
{
  uint             mVertexBuffer;
  uint             mTexture;
  matrix           mWorld;
};

static uint mVertexBuffer = 0;
static uint mTexture = 0;

typedef VSOutput PSInput;

ByteAddressBuffer            BufferTable[] : register( t0, space0 );
Texture2D                    Textures[]    : register( t0, space1 );
SamplerState                 Sampler       : register( s0 );
ConstantBuffer< MyCBufType > MyCBuf        : register( b0 ); // unused

struct Foo
{
  float fooI;
};

struct Bar
{
  float2 barI;
};

VSOutput VSMain(uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof( Vertex ) * iVtx;
  const ByteAddressBuffer vertexBytes = BufferTable[ mVertexBuffer ];
  const Vertex input = vertexBytes.Load< Vertex >( byteOffset );


  //MyCBuf.mTexture;
  //MyCBuf.mWorld;
  //MyCBuf.mVertexBuffer;

  VSOutput result;
  result.mPosition = ClipSpacePosition4(float4(input.mPosition, 1));
  //result.mPosition = input.mPosition; // operator =
  result.mColor = input.mColor;
  result.mUVs = input.mUVs;
  return result;
}

LinearColor4 PSMain(PSInput input) : SV_TARGET
{
  Texture2D texture = Textures[mTexture];

  const float sample = texture.Sample(Sampler, input.mUVs.mFloat2).x;
  const float3 rgb = lerp(input.mColor.mFloat3, float3(1, 1, 1), sample);


  
  //LinearColor4 result = LinearColor4(float4(rgb, 1));

  //LinearColor4 result = LinearColor4(float4(0, 1, 0, 1));

  //LinearColor4 result = LinearColor4::Ctor(LinearColor3(rgb), 1);
  //LinearColor4 result = LinearColor4(float4(rgb, 1));

  //Foo foo = Foo(0.5);
  //Bar bar = Bar(float2(foo.fooI, 3));

  //result.mFloat4 = float4(0, 0, 0.5, 1);
  //result.mFloat4 = float4(0, 0, foo.fooI.x, 1);
  //result.mFloat4 = float4(0, 0, bar.barI.x, 1);

  //result.mFloat4 = float4(rgb, 1);
  return LinearColor4(float4(rgb, 1));
}
