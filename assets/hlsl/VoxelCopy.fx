

/*
void CS( uint3 id : SV_DispatchThreadID )
{
}

*/

can this be done with a vertex shader with blank vertexes?

/*
struct VS_OUTPUT
{
  float4 mClipSpacePosition  : SV_POSITION;
  float3 mColor              : _;
};
*/

// VS_OUTPUT 
void
VS( uint iVertex : SV_VERTEXID )
{
  // VS_OUTPUT output = ( VS_OUTPUT )0;
  // output.mClipSpacePosition = clipSpacePosition;
  // return output;
}

/*
struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};



PS_OUTPUT PS( VS_OUTPUT input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;

  // uhh this is actually NDC space
  const float2 screenSpacePosition
    = input.mScreenSpacePosition.xy
    / input.mScreenSpacePosition.w; // [-1,1]^2 
  float3 finalColor = float3( 0, 0, 0 );

  output.mColor = float4( finalColor, 1.0 );

  return output;
}

*/
