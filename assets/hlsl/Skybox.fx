TextureCube cubemap : register( t0 );
sampler linearSampler : register( s0 );

struct VS_INPUT
{
  float3 Position : POSITION;
};

struct VS_OUTPUT
{
  float4 mClipSpacePosition : SV_POSITION;
  float3 mViewSpacePosition : TEXCOORD;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 viewSpacePosition = mul( View, float4( input.Position, 1 ) );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition );

  // Shader optimization trick which causes the ndc position to be
  // on the far plane, thus allowing the skybox to be rendered after
  // all other objects, and can skip out on the pixel shader where it
  // fails the depth test.
  clipSpacePosition.z = clipSpacePosition.w;

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mViewSpacePosition = viewSpacePosition.xyz;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( VS_OUTPUT input )
{
  float3 sampled = cubemap.Sample( linearSampler, input.mViewSpacePosition ).xyz;

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor.xyz = sampled.xyz; // gamma?
  output.mColor.w = 1;

  return output;
}

