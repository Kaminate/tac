struct Voxel
{
  uint mColor;
  uint mNormal;
};

cbuffer CBufferVoxelizer         : TAC_AUTO_REGISTER
{
  //     Position of the voxel grid in worldspace.
  //     It's not rotated, aligned to world axis.
  float3 gVoxelGridCenter;

  //     Half width of the entire voxel grid in worldspace
  float  gVoxelGridHalfWidth;
  
  //     Width of a single voxel
  float  gVoxelWidth; 

  //     Number of voxels x per side, where the total voxel count is x * x * x
  uint   gVoxelGridSize;
}

uint VoxelEncodeHDRColor( float4 color )
{
  uint result = 0;
  result |= uint( color.x * 255 ) << ( 8 * 0 );
  result |= uint( color.y * 255 ) << ( 8 * 1 );
  result |= uint( color.z * 255 ) << ( 8 * 2 );
  result |= uint( color.w * 255 ) << ( 8 * 3 );
  return result;
}

float4 VoxelDecodeHDRColor( uint color )
{
  uint x = ( color >> ( 8 * 0 ) ) & 255;
  uint y = ( color >> ( 8 * 1 ) ) & 255;
  uint z = ( color >> ( 8 * 2 ) ) & 255;
  uint w = ( color >> ( 8 * 3 ) ) & 255;
  float4 result = float4( x, y, z, w ) / 255;
  //float4 result( x / 255,
  //               y / 255,
  //               z / 255,
  //               w / 255 );
  return result;
}

uint VoxelEncodeUnitNormal( float3 unitNormal )
{
  unitNormal = ( unitNormal + 1 ) / 2; // map to [0,1]
  uint3 unormal = uint3( unitNormal * ( ( 1 << 10 ) - 1 ) );
  uint result = ( unormal.x << ( 10 * 0 ) )
              | ( unormal.y << ( 10 * 1 ) )
              | ( unormal.z << ( 10 * 2 ) );
  return result;
}

float3 VoxelDecodeUnitNormal( uint unitNormal )
{
  uint x = ( unitNormal >> ( 10 * 0 ) ) & ( ( 1 << 10 ) - 1 );
  uint y = ( unitNormal >> ( 10 * 1 ) ) & ( ( 1 << 10 ) - 1 );
  uint z = ( unitNormal >> ( 10 * 2 ) ) & ( ( 1 << 10 ) - 1 );
  float3 result = float3( x / ( ( 1 << 10 ) - 1 ),
                          y / ( ( 1 << 10 ) - 1 ), 
                          z / ( ( 1 << 10 ) - 1 ));
  result = result * 2 - 1;
  return result;
}


uint3 ExtractVoxelIndex( uint iVertex )
{
  uint i = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint j = iVertex % gVoxelGridSize;
  iVertex /= gVoxelGridSize;
  uint k = iVertex % gVoxelGridSize;
  return uint3( i, j, k );
}
