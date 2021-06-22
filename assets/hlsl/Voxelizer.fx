// inject direct lighting
// list of lights, with depth buffers?

#include "common.fx"
#include "VoxelCommon.fx"

//===----------------- vertex shader -----------------===//

//                          UAV requires a 'u' register
RWStructuredBuffer< Voxel > mySB : register( u0 );

Texture2D diffuseMaterialTexture : register( t0 );

// sampler   linearSampler          : register( s0 );

SamplerState linearSampler
{
  Filter = MIN_MAG_MIP_LINEAR;
  AddressU = Wrap;
  AddressV = Wrap;
};


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
  //                               Using underscores as filler identifiers
  //                               as each variable must have a semantic name,
  //                               but the name itself is unimportant
  float3 mWorldSpacePosition     : _;
  float3 mWorldSpaceNormal       : __;
  float2 mTexCoord               : ___;
};

VS_OUT_GS_IN VS( VS_INPUT input )
{
  VS_OUT_GS_IN result;
  result.mWorldSpacePosition = mul( World, float4( input.mPosition, 1.0 ) ).xyz;
  result.mWorldSpaceNormal = normalize( mul( ( float3x3 )World, input.mNormal ) );
  result.mTexCoord = input.mTexCoord;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float2 mTexCoord               : _;
  float3 mWorldSpaceNormal       : __;
  float3 mWorldSpacePosition     : ___;
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
  float3 l                     = float3( 0, 1, 0 );
  float3 n                     = input.mWorldSpaceNormal;
  float3 voxelNDC = ( input.mWorldSpacePosition - gVoxelGridCenter )
                  * ( 1.0f / gVoxelGridHalfWidth )
                  * ( 1.0f / gVoxelWidth );
  if( voxelNDC.x < -1 || voxelNDC.x > 1 ||
      voxelNDC.y < -1 || voxelNDC.y > 1 ||
      voxelNDC.z < -1 || voxelNDC.z > 1 )
    return;

  float3 voxelUVW = voxelNDC * 0.5f + 0.5f;
  bool   voxelUVWSaturated = voxelUVW.x > 0 && voxelUVW.x < 1 ||
                             voxelUVW.y > 0 && voxelUVW.y < 1 ||
                             voxelUVW.z > 0 && voxelUVW.z < 1;
  if( !voxelUVWSaturated )
    return;

  colorMaterialDiffuse  = diffuseMaterialTexture.Sample( linearSampler, input.mTexCoord ).xyz;
  colorLightDiffuse     = dot( n, l );
  color                 = colorMaterialDiffuse * colorLightDiffuse
                        + colorLightAmbient + 
                        + colorMaterialEmissive;

  uint  uint_voxelGridWidth = gVoxelGridHalfWidth * 2;
  uint  uint_voxelGridArea = uint_voxelGridWidth * uint_voxelGridWidth;
  uint3 iVoxel3 = floor( voxelUVW * ( gVoxelGridHalfWidth * 2 ) );
  uint  iVoxel = iVoxel3.x
               + iVoxel3.y * uint_voxelGridWidth
               + iVoxel3.z * uint_voxelGridArea;

  uint uint_color = VoxelEncodeHDRColor( color );
  uint uint_n     = VoxelEncodeUnitNormal( n );
  InterlockedMax( mySB[ iVoxel ].mColor, uint_color );
  InterlockedMax( mySB[ iVoxel ].mNormal, uint_n );
}

