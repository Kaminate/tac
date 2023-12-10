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
  float4 position;
  float4 color;
};


struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : TAC_AUTO_SEMANTIC;
};

#define bufferSpace space0

ByteAddressBuffer BufferTable[] : register(t0, bufferSpace);

PSInput VSMain(uint iVtx : SV_VertexID )
{
  Vertex input = BufferTable[0].Load<Vertex>( iVtx );

  PSInput result;
  result.position = input.position;
  result.color = input.color;
  return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
