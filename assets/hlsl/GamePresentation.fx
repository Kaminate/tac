#include "common.fx"

struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
};

Texture2D shadowMaps[ 4 ] : register( t0 );

sampler linearSampler : register( s0 );


#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPOT        1

struct Light
{
  //               Transforms a point from worldspace to the shadowmap's clipspace
  row_major matrix mWorldToClip;
  float3           mWorldSpacePosition;
  float3           mWorldSpaceUnitDirection;
  float4           mColor;
  uint             mType;
  //float            mNear;
  //float            mFar;
  float            mProjA;
  float            mProjB;
  TAC_PAD_BYTES( 4 );
};

cbuffer CBufferLights  : register( b2 )
{
  Light            lights[ 4 ];
  uint             lightCount;
  bool             useLights;
  uint             MagicNumber;
}


struct VS_OUTPUT
{
  float4 mClipSpacePosition  : SV_POSITION;
  float4 mWorldSpacePosition : SV_AUTO_SEMANTIC;
  float3 mWorldSpaceNormal   : SV_AUTO_SEMANTIC;
  // float3 m_debug_ndc         : SV_AUTO_SEMANTIC;
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
  // output.m_debug_ndc = clipSpacePosition.xyz / clipSpacePosition.w;
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


float3 ApplyLight( Texture2D shadowMap, Light light, VS_OUTPUT input )
{



  float3 colorDiffuse = float3( 0, 0, 0 );

  // temp, use matrix values so its not optimized
  //for( int i = 0; i < 4; ++i )
  //{
  //  for( int j = 0; j < 4; ++j )
  //  {
  //    colorDiffuse.x += light.mWorldToClip[ i ][ j ] / 10000.0;
  //  }
  //}

  const float4 pixelLightClipSpacePosition = mul( light.mWorldToClip, input.mWorldSpacePosition );
  // TESTING
  //const float4 pixelLightClipSpacePosition = mul( light.mWorldToClip, float4( 0, 0, 0, 1 ) );
  const float3 pixelLightNDCSpacePosition = pixelLightClipSpacePosition.xyz / pixelLightClipSpacePosition.w;
  if( pixelLightNDCSpacePosition.x < -1 ||
      pixelLightNDCSpacePosition.x > 1 ||
      pixelLightNDCSpacePosition.y < -1 ||
      pixelLightNDCSpacePosition.y > 1 )
    //continue;
    return float3( 0, 0, 0 );

  const float2 pixelLightTexel = pixelLightNDCSpacePosition.xy * 0.5 + 0.5;

  //Texture2D shadowMap = shadowMaps[ i ];

  // loop with offsets for fuzziness
  const float shadowMapSample = shadowMap.Sample( linearSampler, pixelLightTexel ).x;

  //const float shadowMapCameraDist = GetViewSpaceDistFromNDC( shadowMapSample, light.mNear, light.mFar );
  //const float pixelLightCameraDist = GetViewSpaceDistFromNDC( pixelLightNDCSpacePosition.z, light.mNear, light.mFar );
  //const float shadowMapCameraDist = -UnprojectClipToView( shadowMapSample, light.mProjA, light.mProjB );
  //const float pixelLightCameraDist = -UnprojectClipToView( pixelLightClipSpacePosition.z, light.mProjA, light.mProjB );
  const float shadowMapCameraDist = -UnprojectNDCToView( shadowMapSample, light.mProjA, light.mProjB );
  const float pixelLightCameraDist = -UnprojectNDCToView( pixelLightNDCSpacePosition.z, light.mProjA, light.mProjB );

  // this 0.1 is in linear space, but the sample is nonlinear
  //const bool occluded = pixelLightNDCSpacePosition.z > shadowMapSample + 0.1;
  const bool occluded = pixelLightCameraDist > shadowMapCameraDist + 0.1;

  if( !occluded )
  {
    if( light.mType == LIGHT_TYPE_SPOT )
    {
      float3 n = normalize( input.mWorldSpaceNormal );
      float3 l = -light.mWorldSpaceUnitDirection.xyz;
      const float ndotl = dot( n, l );

      // dont need to check if ndotl < 0 because of the ndc check?

      colorDiffuse += ndotl * light.mColor.xyz * Color.xyz;
    }
  }

  return colorDiffuse;
}

PS_OUTPUT PS( VS_OUTPUT input )
{
  float3 colorDiffuse = float3( 0, 0, 0 );
  if( useLights )
  {
    if( 0 < lightCount )
      colorDiffuse += ApplyLight( shadowMaps[ 0 ], lights[ 0 ], input );
    if( 1 < lightCount )
      colorDiffuse += ApplyLight( shadowMaps[ 1 ], lights[ 1 ], input );
    if( 2 < lightCount )
      colorDiffuse += ApplyLight( shadowMaps[ 2 ], lights[ 2 ], input );
    if( 3 < lightCount )
      colorDiffuse += ApplyLight( shadowMaps[ 3 ], lights[ 3 ], input );
  }
  else
  {
    colorDiffuse = Color.xyz;
  }

  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor.xyz = pow( colorDiffuse, 1.0 / 2.2 );
  output.mColor.w = Color.w;

  //if( MagicNumber == 1234567890 )
  //{
  //  output.mColor = float4( 1, 1, 0, 1 );
  //}

  return output;
}

