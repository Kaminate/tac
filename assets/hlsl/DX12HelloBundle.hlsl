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

struct ClipSpacePosition3 { float3 mFloat3; };
struct ClipSpacePosition4 { float4 mFloat4; };
struct LinearColor3       { float3 mFloat3; };
struct LinearColor4       { float4 mFloat4; };
struct TextureCoordinate2 { float2 mFloat2; };

ClipSpacePosition4 ClipSpacePosition3to4(const ClipSpacePosition3 pos)
{
  ClipSpacePosition4 result;
  result.mFloat4 = float4(pos.mFloat3, 1.0f);
  return result;
}

LinearColor4 LinearColor3to4(const LinearColor3 col)
{
  LinearColor4 result;
  result.mFloat4 = float4(col.mFloat3, 1.0f);
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
  result.mPosition = ClipSpacePosition3to4(input.mPosition);
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

