#include "Common.fx"
#include "LightsCommon.fx"

struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
};

Texture2D shadowMaps[ 4 ] : register( t0 );
sampler linearSampler     : register( s0 );
sampler shadowMapSampler  : register( s1 );

struct VS_OUTPUT
{
  float4 mClipSpacePosition  : SV_POSITION;
  float4 mWorldSpacePosition : SV_AUTO_SEMANTIC;
  float3 mWorldSpaceNormal   : SV_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  float4 worldSpacePosition = mul( World, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( View, worldSpacePosition );
  float4 clipSpacePosition = mul( Projection, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mWorldSpacePosition = worldSpacePosition;
  output.mWorldSpaceNormal = mul( World, float4( input.Normal, 0 ) ).xyz;
  return output;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

//float UnprojectClipToViewSpace( float zClip, float n, float f )
//{
//  // Camera forward vector is in -z direction, this function returns a positive value
//  float result = ( n * f ) / ( f + ( n - f ) * zNdc );
//  return result;
//}

//float UnprojectClipToView( float zClip, float a, float b )
//{
//  // Camera forward vector is in -z direction, this function returns a positive value
//  return ( zClip - b ) / a;
//}

float UnprojectNDCToView( float zNDC, float a, float b )
{
  return -b / ( a + zNDC );
}


float3 ApplyLight( int iLight, Texture2D shadowMap, Light light, VS_OUTPUT input )
{
  float3 colorDiffuse = float3( 0, 0, 0 );

  bool isValid = iLight < lightCount;
  if( !isValid )
    return colorDiffuse;

  bool occluded = false;
  const bool lightCastsShadows = LightGetCastsShadows( light.mFlags );
  if( lightCastsShadows )
  {

    const float4 pixelLightClipSpacePosition = mul( light.mWorldToClip, input.mWorldSpacePosition );
    const float3 pixelLightNDCSpacePosition = pixelLightClipSpacePosition.xyz / pixelLightClipSpacePosition.w;
    if( pixelLightNDCSpacePosition.x < -1 ||
        pixelLightNDCSpacePosition.x > 1 ||
        pixelLightNDCSpacePosition.y < -1 ||
        pixelLightNDCSpacePosition.y > 1 )
      return colorDiffuse;

    // negative because dx vs ogl texel coords
    const float2 pixelLightTexel = float2( pixelLightNDCSpacePosition.x * 0.5 + 0.5,
                                           pixelLightNDCSpacePosition.y * -0.5 + 0.5 );

    const float shadowMapSample = shadowMap.Sample( shadowMapSampler, pixelLightTexel ).x;

    //const float shadowMapCameraDist = GetViewSpaceDistFromNDC( shadowMapSample, light.mNear, light.mFar );
    //const float pixelLightCameraDist = GetViewSpaceDistFromNDC( pixelLightNDCSpacePosition.z, light.mNear, light.mFar );
    //const float shadowMapCameraDist = -UnprojectClipToView( shadowMapSample, light.mProjA, light.mProjB );
    //const float pixelLightCameraDist = -UnprojectClipToView( pixelLightClipSpacePosition.z, light.mProjA, light.mProjB );
    const float shadowMapCameraDist = -UnprojectNDCToView( shadowMapSample, light.mProjA, light.mProjB );
    const float pixelLightCameraDist = -UnprojectNDCToView( pixelLightNDCSpacePosition.z, light.mProjA, light.mProjB );




    // this 0.1 is in linear space, but the sample is nonlinear
    //const bool occluded = pixelLightNDCSpacePosition.z > shadowMapSample + 0.1;
    const bool occluded_view = pixelLightCameraDist > shadowMapCameraDist + 0.7;
    const bool occluded_ndc = ( pixelLightNDCSpacePosition.z > shadowMapSample + 0.00001 );
    //const bool occluded = occluded_ndc;
    occluded = occluded_view;

  }

  if( !occluded )
  {
    uint lightType = LightGetType( light.mFlags );
    if( lightType == LIGHT_TYPE_SPOT )
    {
      float3 n = normalize( input.mWorldSpaceNormal );
      float3 l = -light.mWorldSpaceUnitDirection.xyz;
      const float ndotl = dot( n, l );

      // dont need to check if ndotl < 0 because of the ndc check?
      colorDiffuse += ndotl * light.mColorRadiance.xyz * Color.xyz;
    }
  }

  return colorDiffuse;
}

PS_OUTPUT PS( VS_OUTPUT input )
{
  float3 colorDiffuse = float3( 0, 0, 0 );
  if( useLights )
  {
    colorDiffuse += ApplyLight( 0, shadowMaps[ 0 ], lights[ 0 ], input );
    colorDiffuse += ApplyLight( 1, shadowMaps[ 1 ], lights[ 1 ], input );
    colorDiffuse += ApplyLight( 2, shadowMaps[ 2 ], lights[ 2 ], input );
    colorDiffuse += ApplyLight( 3, shadowMaps[ 3 ], lights[ 3 ], input );
  }
  else
  {
    colorDiffuse = Color.xyz;
  }

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor.xyz = pow( colorDiffuse, 1.0 / 2.2 );
  output.mColor.w = Color.w;

  return output;
}

