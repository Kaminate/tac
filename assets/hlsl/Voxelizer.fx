// Texture2D terrainTexture : register( t0 );
// Texture2D noiseTexture : register( t1 );
// sampler linearSampler : register( s0 );

// inject direct lighting
// list of lights, with depth buffers?

#include "common.fx"

//===----------------- vertex shader -----------------===//

struct Voxel
{
  uint color;
  uint normal;
};

StructuredBuffer< Voxel > mySB : register( t0 );


struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
  float2 TexCoord : TEXCOORD;
};

struct VS_OUT_GS_IN
{
  float3 objectSpacePosition;
  float3 objectSpaceNormal;
  float2 TexCoord;

  // float3 mWorldSpacePosition  : HI;
  // float3 mWorldSpaceNormal    : NORMAL;
  // float2 mTexCoord            : TEXCOORD0;
  // float4 mClipSpacePosition   : SV_POSITION;
  // float4 mScreenSpacePosition : TEXCOORD1;
};

VS_OUT_GS_IN VS( VS_INPUT input )
{
  VS_OUT_GS_IN result;
  result.WorldSpacePosition = input.Position;
  result.WorldSpaceNormal = input.Normal;
  result.TexCoord = input.TexCoord;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
};

[ maxvertexcount( 3 ) ]
void GS( triangle VS_OUT_GS_IN input[ 3 ],
         inout TriangleStream< GS_OUT_PS_IN > outputStream )
{
  float3 faceNormal = input[0].

  for( int i = 0; i < 3; ++i )
  {
    GS_OUT_PS_IN output;
    outputStream.Append( output );
  }
}

//===----------------- pixel shader -----------------===//

void PS( GS_OUT_PS_IN input )
{
}

