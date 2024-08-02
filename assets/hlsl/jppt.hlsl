
RWTexture2D<float4> sOutputTexture : register( u0 );

[numthreads(8,8,1)]
void CSMain( uint3 id : SV_DispatchThreadID )
{
  sOutputTexture[ id.xy ] = float4( 1, 0, 0, 1 );
}
