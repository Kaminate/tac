#include "common.fx"
#include "VoxelCommon.fx"

Texture3D voxels : register( t0 );

#define CUBE_STRIP_VTX_COUNT 14
#define DEBUG_ENABLE_RANDOM_VOXEL_COLORS 0

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
  float4 mColor                 : AUTO_SEMANTIC;
  float3 mWorldSpaceVoxelCenter : AUTO_SEMANTIC;
};


VS_OUT_GS_IN VS( const uint iVertex : SV_VERTEXID )
{
  uint3 iVoxel = ExtractVoxelIndex( iVertex );

  float4 color = voxels.Load( int4( iVoxel.x, iVoxel.y, iVoxel.z, 0 ) );

  float3 worldSpaceVoxelMinCorner = gVoxelGridCenter - gVoxelGridHalfWidth;
  float3 worldSpaceVoxelCenter
    = worldSpaceVoxelMinCorner
    + gVoxelWidth * ( float3( iVoxel.x, iVoxel.y, iVoxel.z ) + 0.5 );

#if DEBUG_ENABLE_RANDOM_VOXEL_COLORS
  float colorspeed = 1.345;
  color.x = ( sin( worldSpaceVoxelCenter.x * colorspeed + 5.) * 0.5 + 0.5 );
  color.y = ( sin( worldSpaceVoxelCenter.y * colorspeed + 6.) * 0.5 + 0.5 );
  color.z = ( sin( worldSpaceVoxelCenter.z * colorspeed + 7.) * 0.5 + 0.5 );
#endif

  VS_OUT_GS_IN result;
  result.mColor = color;
  result.mWorldSpaceVoxelCenter = worldSpaceVoxelCenter;

  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float4 mClipSpacePosition      : SV_POSITION;
  float4 mColor                  : AUTO_SEMANTIC;
};

[maxvertexcount( CUBE_STRIP_VTX_COUNT )]
void GS( point VS_OUT_GS_IN inputs[ 1 ],
         inout TriangleStream< GS_OUT_PS_IN > outputs )
{
  VS_OUT_GS_IN input = inputs[ 0 ];
  if( input.mColor.w == 0 )
    return;

  for( int iVtx = 0; iVtx < CUBE_STRIP_VTX_COUNT; ++iVtx )
  {
    float3 modelSpacePosition = GenerateCubeVertex( iVtx ) * 2.0f - 1.0f;
    float3 worldSpacePosition = input.mWorldSpaceVoxelCenter + modelSpacePosition * ( gVoxelWidth / 2 );
    float4 viewSpacePosition = mul( View, float4( worldSpacePosition, 1 ) );
    float4 clipSpacePosition = mul( Projection, viewSpacePosition );

    GS_OUT_PS_IN output;
    output.mClipSpacePosition = clipSpacePosition;
    output.mColor = input.mColor;
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
  output.mColor = input.mColor;
  return output;
}

