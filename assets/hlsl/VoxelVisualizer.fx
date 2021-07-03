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
  float4 mColor                 : mColor;
  float3 mWorldSpaceVoxelCenter : mWorldSpaceVoxelCenter;
};


VS_OUT_GS_IN VS( const uint iVertex : SV_VERTEXID )
{
  uint3 iVoxel = ExtractVoxelIndex( iVertex );

  float4 color = voxels.Load( int4( iVoxel.x, iVoxel.y, iVoxel.z, 0 ) );// .xyz;
  float3 worldSpaceVoxelMinCorner = gVoxelGridCenter - gVoxelGridHalfWidth;
  float3 worldSpaceVoxelCenter
    = worldSpaceVoxelMinCorner
    + gVoxelWidth * ( float3( iVoxel.x, iVoxel.y, iVoxel.z ) + 0.5 );
  VS_OUT_GS_IN result;
  result.mColor = color;
  result.mWorldSpaceVoxelCenter = worldSpaceVoxelCenter;

   //if( iVertex != ( int ) secModTau % 6 )
   // result.mWorldSpaceVoxelCenter = float3( 10, 10, 10 );

  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float4 mClipSpacePosition      : SV_POSITION;
  float4 mColor                  : mColor;
};

[maxvertexcount( CUBE_STRIP_VTX_COUNT )]
void GS( point VS_OUT_GS_IN input[ 1 ],
         inout TriangleStream< GS_OUT_PS_IN > outputs )
{
  for( int iVtx = 0; iVtx < CUBE_STRIP_VTX_COUNT; ++iVtx )
  {
    float worldSpaceEpsilon = 0.02; // Shrink the voxels so the outlines draw on top
    float3 modelSpacePosition = GenerateCubeVertex( iVtx ) * 2.0f - 1.0f;
    float3 worldSpacePosition
      = input[ 0 ].mWorldSpaceVoxelCenter
      + modelSpacePosition * ( ( gVoxelWidth - worldSpaceEpsilon ) / 2 );
    float4 viewSpacePosition = mul( View, float4( worldSpacePosition, 1 ) );
    float4 clipSpacePosition = mul( Projection, viewSpacePosition );

    GS_OUT_PS_IN output;
    output.mClipSpacePosition = clipSpacePosition;
    output.mColor = input[ 0 ].mColor;
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
  output.mColor = input.mColor; // float4( input.mColor, 1.0 );
  if( input.mColor.w == 0 )
    discard;
  return output;
}

