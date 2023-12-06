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

struct TAC_VS_INPUT
{
  float4 position : TAC_AUTO_SEMANTIC;
  float4 color    : TAC_AUTO_SEMANTIC;
};

struct PSInput
{
    float4 position : SV_POSITION;
    float4 color    : TAC_AUTO_SEMANTIC;
};

#define bufferSpace space0

ByteAddressBuffer BufferTable[] : register(t0, bufferSpace);

PSInput VSMain(TAC_VS_INPUT input)
{
    PSInput result;
    result.position = input.position;
    result.color = input.color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET
{
    return input.color;
}
