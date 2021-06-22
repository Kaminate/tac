#include "VoxelCommon.fx"

/*
void CS( uint3 id : SV_DispatchThreadID )
{
}

*/


RWStructuredBuffer< Voxel > mySB        : register( u0 );
RWTexture3D< float3 > myRadianceTexture : register( u1 );

void VS( uint iVertex : SV_VERTEXID )
{
  Voxel voxel = mySB[ iVertex ];

  uint i = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint j = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint k = iVertex % gVoxelGridSize;

  float3 color = VoxelDecodeHDRColor( voxel.mColor );

  myRadianceTexture[ uint3( i, j, k ) ] = color;
}

