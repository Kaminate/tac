#include "VoxelCommon.fx"

RWStructuredBuffer< Voxel > mySB              : register( u0 );
RWTexture3D< float3 >       myRadianceTexture : register( u1 );

void VS( const uint iVertex : SV_VERTEXID )
{
  const Voxel voxel = mySB[ iVertex ];
  const uint3 iVoxel = ExtractVoxelIndex( iVertex );
  const float3 color = VoxelDecodeHDRColor( voxel.mColor );

  myRadianceTexture[ iVoxel ] = color;
}

