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
};

struct VSOutput
{
  ClipSpacePosition4 mPosition : SV_POSITION;
  LinearColor3       mColor    : TAC_AUTO_SEMANTIC;
};

typedef VSOutput PSInput;

ByteAddressBuffer BufferTable[] : register(t0, space0);

VSOutput VSMain(uint iVtx : SV_VertexID )
{
  const uint byteOffset = sizeof(Vertex) * iVtx;
  const Vertex input = BufferTable[0].Load < Vertex > (byteOffset);

  VSOutput result;
  result.mPosition = ClipSpacePosition3to4(input.mPosition);
  result.mColor = input.mColor;
  return result;
}

LinearColor4 PSMain(PSInput input) : SV_TARGET
{
  return LinearColor3to4(input.mColor);
}
