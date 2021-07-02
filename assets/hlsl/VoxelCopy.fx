#include "VoxelCommon.fx"

RWStructuredBuffer< Voxel > mySB              : register( u0 );
RWTexture3D< float4 >       myRadianceTexture : register( u1 );

void VS( const uint iVertex : SV_VERTEXID )
{
  const Voxel voxel = mySB[ iVertex ];
  const uint3 iVoxel = ExtractVoxelIndex( iVertex );
  const float4 color = VoxelDecodeHDRColor( voxel.mColor );
  myRadianceTexture[ iVoxel ] = color;

  // Finished with structured buffer.
  // Clear it for the next time's use
  mySB[ iVertex ] = ( Voxel )0;
}

