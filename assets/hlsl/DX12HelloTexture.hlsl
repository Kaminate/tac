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
struct LinearColor3       { float3 mValue; };
struct LinearColor4       { float4 mValue; };
struct TextureCoordinate2 { float2 mValue; };

ClipSpacePosition4 ClipSpacePosition3to4(const ClipSpacePosition3 pos)
{
  ClipSpacePosition4 result;
  result.mValue = float4(pos.mValue, 1.0f);
  return result;
}

LinearColor4 LinearColor3to4(const LinearColor3 col)
{
  LinearColor4 result;
  result.mValue = float4(col.mValue, 1.0f);
  return result;
}

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

//cbuffer MyCBuf : register( b0 )
//{
//  uint mVertexBuffer;
//  uint mTexture;
//};

static uint mVertexBuffer = 0;
static uint mTexture = 0;

typedef VSOutput PSInput;

ByteAddressBuffer BufferTable[] : register( t0, space0 );
Texture2D         Textures[]    : register( t0, space1 );
SamplerState      Sampler       : register( s0 );

VSOutput VSMain(uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof( Vertex ) * iVtx;
  const ByteAddressBuffer vertexBytes = BufferTable[ mVertexBuffer ];
  const Vertex input = vertexBytes.Load< Vertex >( byteOffset );

  VSOutput result;
  result.mPosition = ClipSpacePosition3to4(input.mPosition);
  result.mColor = input.mColor;
  result.mUVs = input.mUVs;
  return result;
}

LinearColor4 PSMain(PSInput input) : SV_TARGET
{
  Texture2D texture = Textures[mTexture];
  float4 sample = texture.Sample(Sampler, input.mUVs.mValue);

  LinearColor4 result;
  result.mValue = float4(0,0,0,0);
  result.mValue.rgb += input.mColor.mValue;
  result.mValue.rg += sample.rg;
  return result;
}
