// Texture2D terrainTexture : register( t0 );
// Texture2D noiseTexture : register( t1 );
// sampler linearSampler : register( s0 );

// inject direct lighting
// list of lights, with depth buffers?

#include "common.fx"

//===----------------- vertex shader -----------------===//

struct Voxel
{
  uint mColor;
  uint mNormal;
};

StructuredBuffer< Voxel > mySB : register( t0 );

Texture2D diffuseMaterialTexture : register( t0 );
sampler   linearSampler          : register( s0 );

cbuffer CBufferVoxelizer : register( b2 )
{
  float3 gVoxelGridCenter;
  float  gVoxelGridHalfWidth;
  float  gVoxelWidth; // Width of a single voxel
}

// Note 1: Variables are passed between shader stages through SEMANTICS, not variable names!
//         This allows you to have separate structs for VS_OUT and GS_IN
// Note 2: built in semantics ( POSITION[n], NORMAL[n] ) dont carry any special meaning
//         ( left over from d3d9? ), the only thing that really matters is user-supplied semantics
//         and SV_ ( system value ) semantics
// Note 3: The VS_IN semantic names must correspond to D3D11_INPUT_ELEMENT_DESC::SemanticName or else get
//         [ STATE_CREATION ERROR #163: CREATEINPUTLAYOUT_MISSINGELEMENT]

struct VS_INPUT
{
  float3 mPosition : POSITION;
  float3 mNormal   : NORMAL;
  float2 mTexCoord : TEXCOORD;
};

struct VS_OUT_GS_IN
{
  float3 mWorldSpacePosition     : POSITION_ASS;
  float3 mWorldSpaceNormal       : NORMAL_SEX;
  float2 mTexCoord               : TEXCOORD_FUCK;
};

VS_OUT_GS_IN VS( VS_INPUT input )
{
  VS_OUT_GS_IN result;
  result.mWorldSpacePosition = mul( World, float4( input.mPosition, 1.0 ) ).xyz;
  result.mWorldSpaceNormal = mul( ( float3x3 )World, input.mNormal );
  result.mTexCoord = input.mTexCoord;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float2 mTexCoord               : TEXCOORD_CUNT;
  float3 mWorldSpaceNormal       : NORMAL_BITCH;
  float3 mWorldSpacePosition     : POSITION_SLUT;
  float4 mClipSpacePosition      : SV_POSITION;
};

[ maxvertexcount( 3 ) ]
void GS( triangle VS_OUT_GS_IN input[ 3 ],
         inout TriangleStream< GS_OUT_PS_IN > outputStream )
{
  const float3 worldSpaceFaceNormal = input[ 0 ].mWorldSpaceNormal
                                    + input[ 1 ].mWorldSpaceNormal
                                    + input[ 2 ].mWorldSpaceNormal;
  const float dominantAxisValue = max( max( worldSpaceFaceNormal.x,
                                            worldSpaceFaceNormal.y ),
                                            worldSpaceFaceNormal.z );

  for( int i = 0; i < 3; ++i )
  {
    VS_OUT_GS_IN curInput = input[ i ];

    float4 clipSpacePosition = float4( 0, 0, 0, 0 );
    {
      clipSpacePosition.xyz = curInput.mWorldSpacePosition - gVoxelGridCenter;
      if( dominantAxisValue == worldSpaceFaceNormal.x )
        clipSpacePosition = float4( clipSpacePosition.z, clipSpacePosition.y, 1, 1 );
      else if( dominantAxisValue == worldSpaceFaceNormal.y )
        clipSpacePosition = float4( clipSpacePosition.x, clipSpacePosition.z, 1, 1 );
      clipSpacePosition.xy /= gVoxelWidth;
      clipSpacePosition.xy /= gVoxelGridHalfWidth;
    }

    GS_OUT_PS_IN output;
    output.mTexCoord           = curInput.mTexCoord;
    output.mWorldSpaceNormal   = curInput.mWorldSpaceNormal;
    output.mWorldSpacePosition = curInput.mWorldSpacePosition;
    output.mClipSpacePosition  = clipSpacePosition;
    outputStream.Append( output );
  }
}

//===----------------- pixel shader -----------------===//

void PS( GS_OUT_PS_IN input )
{
  float3 color                 = float3( 0, 0, 0 );
  float3 colorLightDiffuse     = float3( 0, 0, 0 );
  float3 colorLightAmbient     = float3( 0, 0, 0 );
  float3 colorMaterialDiffuse  = float3( 0, 0, 0 );
  float3 colorMaterialEmissive = float3( 0, 0, 0 );

  colorMaterialDiffuse  = diffuseMaterialTexture.Sample( linearSampler, input.mTexCoord ).xyz;
  colorLightDiffuse     = dot( input.mWorldSpaceNormal, float3( 0, 1, 0) );
  color = colorMaterialDiffuse * colorLightDiffuse
        + colorLightAmbient + 
        + colorMaterialEmissive;
}

