// inject direct lighting
// list of lights, with depth buffers?

#include "common.fx"
#include "VoxelCommon.fx"

//===----------------- vertex shader -----------------===//

//                          UAV requires a 'u' register
RWStructuredBuffer< Voxel > mySB : register( u1 );

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
  //
  //                               ^ update: these semantics names will show up in renderdoc,
  //                                         so it would be more useful to name them
  float3 mWorldSpacePosition     : mWorldSpacePosition;
  float3 mWorldSpaceUnitNormal   : mWorldSpaceUnitNormal;
  float2 mTexCoord               : mTexCoord;
};

VS_OUT_GS_IN VS( VS_INPUT input )
{
  VS_OUT_GS_IN result;
  result.mWorldSpacePosition = mul( World, float4( input.mPosition, 1.0 ) ).xyz;
  result.mWorldSpaceUnitNormal = normalize( mul( ( float3x3 )World, input.mNormal ) );
  result.mTexCoord = input.mTexCoord;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float2 mTexCoord               : mTexCoord;
  float3 mWorldSpaceUnitNormal   : mWorldSpaceUnitNormal;
  float3 mWorldSpacePosition     : mWorldSpacePosition;
  float4 mClipSpacePosition      : SV_POSITION;

  float3 debug_worldSpaceAbsFaceNormal : WS_ABS_face_NORMAL;
  float3 debug_gVoxelGridCenter        : voxel_grid_center;
  float debug_gVoxelWidth              : voxel_width;
  float debug_gVoxelGridHalfWidth      : voxel_grid_half_width;
};

[ maxvertexcount( 3 ) ]
void GS( triangle VS_OUT_GS_IN input[ 3 ],
         inout TriangleStream< GS_OUT_PS_IN > outputStream )
{
  // non normalized
  const float3 worldSpaceAbsFaceNormal = abs( input[ 0 ].mWorldSpaceUnitNormal +
                                              input[ 1 ].mWorldSpaceUnitNormal +
                                              input[ 2 ].mWorldSpaceUnitNormal );
  const float dominantAxisValue = max( max( worldSpaceAbsFaceNormal.x,
                                            worldSpaceAbsFaceNormal.y ),
                                            worldSpaceAbsFaceNormal.z );

  for( int i = 0; i < 3; ++i )
  {
    VS_OUT_GS_IN curInput = input[ i ];

    float4 clipSpacePosition = float4( 0, 0, 0, 0 );
    {
      clipSpacePosition.xyz = curInput.mWorldSpacePosition - gVoxelGridCenter;
      if( dominantAxisValue == worldSpaceAbsFaceNormal.x )
        clipSpacePosition = float4( clipSpacePosition.z, clipSpacePosition.y, 1, 1 );
      else if( dominantAxisValue == worldSpaceAbsFaceNormal.y )
        clipSpacePosition = float4( clipSpacePosition.x, clipSpacePosition.z, 1, 1 );
      clipSpacePosition.xy /= gVoxelWidth;
      clipSpacePosition.xy /= gVoxelGridHalfWidth;
    }

    GS_OUT_PS_IN output;
    output.mTexCoord             = curInput.mTexCoord;
    output.mWorldSpaceUnitNormal = curInput.mWorldSpaceUnitNormal;
    output.mWorldSpacePosition   = curInput.mWorldSpacePosition;
    output.mClipSpacePosition    = clipSpacePosition;

    output.debug_worldSpaceAbsFaceNormal = worldSpaceAbsFaceNormal;
    output.debug_gVoxelGridCenter = gVoxelGridCenter;
    output.debug_gVoxelWidth = gVoxelWidth;
    output.debug_gVoxelGridHalfWidth = gVoxelGridHalfWidth;

    outputStream.Append( output );
  }
}

//===----------------- pixel shader -----------------===//


/*
struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
*/

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( GS_OUT_PS_IN input )
{
  float3 colorLightAmbient     = float3( 0, 0, 0 );
  float3 colorMaterialEmissive = float3( 0, 0, 0 );
  float3 l                     = float3( 0, 1, 0 );
  float3 n                     = input.mWorldSpaceUnitNormal;
  float3 voxelNDC
    = ( input.mWorldSpacePosition - gVoxelGridCenter )
    / gVoxelGridHalfWidth;
    // think we dont divide by gVoxelWidth here
    // * ( 1.0f / gVoxelWidth );
  if( voxelNDC.x < -1 || voxelNDC.x > 1 ||
      voxelNDC.y < -1 || voxelNDC.y > 1 ||
      voxelNDC.z < -1 || voxelNDC.z > 1 )
    return ( PS_OUTPUT )0;

  float3 voxelUVW = voxelNDC * 0.5f + 0.5f;

  // | Commenting out because
  // v this is just a copy of the previous early out
  //bool   voxelUVWSaturated = voxelUVW.x > 0 && voxelUVW.x < 1 ||
  //                           voxelUVW.y > 0 && voxelUVW.y < 1 ||
  //                           voxelUVW.z > 0 && voxelUVW.z < 1;
  //if( !voxelUVWSaturated )
  //  return ( PS_OUTPUT )0;

  float3 colorMaterialDiffuse = diffuseMaterialTexture.Sample( linearSampler, input.mTexCoord ).xyz;
  float3 colorLightDiffuse = dot( n, l );
  float4 color
    //= colorMaterialDiffuse * colorLightDiffuse
    //+ colorLightAmbient
    //+ colorMaterialEmissive;
    = float4( colorMaterialDiffuse, 1 );

  //float voxelGridWidth = gVoxelGridHalfWidth * 2;
  //uint  uint_voxelGridWidth = voxelGridWidth;
  //uint  uint_voxelGridArea = uint_voxelGridWidth * uint_voxelGridWidth;
  uint3 iVoxel3 = floor( voxelUVW * gVoxelGridSize );
  uint  iVoxel
    = iVoxel3.x
    + iVoxel3.y * gVoxelGridSize
    + iVoxel3.z * gVoxelGridSize * gVoxelGridSize;

  // temp debugging begin
  //iVoxel = 7;
  //color = float4( 1, 1, 0, 1 );
  // temp debugging end

  uint uint_color = VoxelEncodeHDRColor( color );
  uint uint_n     = VoxelEncodeUnitNormal( n );
  InterlockedMax( mySB[ iVoxel ].mColor, uint_color );
  InterlockedMax( mySB[ iVoxel ].mNormal, uint_n );

  PS_OUTPUT output = ( PS_OUTPUT )0;
  return output;
}

