#include "common.fx"
#include "VoxelCommon.fx"

Texture3D voxels : register( t0 );

#define CUBE_STRIP_VTX_COUNT 14

float3 GenerateCubeVertex( uint i )
{
  uint b = 1 << i;
  float x = ( 0x287a & b ) != 0;
  float y = ( 0x02af & b ) != 0;
  float z = ( 0x31e3 & b ) != 0;
  return float3( x, y, z );
}

//===----------------- vertex shader -----------------===//

struct VS_OUT_GS_IN
{
  float3 mColor                 : _;
  float3 mWorldSpaceVoxelCenter : __;
};

VS_OUT_GS_IN VS( uint iVertex : SV_VERTEXID )
{
  uint i = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint j = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint k = iVertex % gVoxelGridSize;

  float3 color = voxels.Load(int4(i,j,k,0)).xyz;
  float3 worldSpaceVoxelCenter = float3( i, j, k );
  worldSpaceVoxelCenter /= ( float )gVoxelGridSize;
  worldSpaceVoxelCenter -= gVoxelGridHalfWidth; 
  worldSpaceVoxelCenter += gVoxelWidth / 2;

  VS_OUT_GS_IN result;
  result.mColor = color;
  result.mWorldSpaceVoxelCenter = worldSpaceVoxelCenter;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float4 mClipSpacePosition      : SV_POSITION;
  float3 mColor                  : _;
};

[ maxvertexcount( CUBE_STRIP_VTX_COUNT ) ]
void GS( point VS_OUT_GS_IN input[ 1 ],
         inout TriangleStream< GS_OUT_PS_IN > outputs )
{
  for( int iVtx = 0; iVtx < CUBE_STRIP_VTX_COUNT; ++iVtx )
  {
    float3 modelSpacePosition = GenerateCubeVertex( iVtx );
    float3 worldSpacePosition = input[ 0 ].mWorldSpaceVoxelCenter + modelSpacePosition * ( gVoxelWidth / 2 );

    GS_OUT_PS_IN output;
    output.mClipSpacePosition = float4( worldSpacePosition, 1 );
    output.mColor = input[0].mColor;;
    outputs.Append( output );

  }
}



//===----------------- pixel shader -----------------===//

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( GS_OUT_PS_IN input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor = float4( input.mColor, 1.0 );
  return output;
}

