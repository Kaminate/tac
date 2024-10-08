//#include "Common.hlsl"
#include "LightsCommon.hlsl"

struct PerFrameStruct
{
  row_major matrix mView;
  row_major matrix mProj;
  // todo: add editor params, ie v4 addColor, bool unlit show albedo
  float4 mAmbient;
};


struct PerObjectStruct
{
  row_major matrix mWorld;

  // Is this premultiplied alpha?
  // Is this sRGB?
  float4           mColor;
};

typedef ConstantBuffer< PerFrameStruct > PerFrameBuf;
typedef ConstantBuffer< PerObjectStruct > PerObjectBuf;



struct VS_INPUT
{
  float3 Position : POSITION;
  float3 Normal   : NORMAL;
};

Texture2D    shadowMaps[ 4 ]   : TAC_AUTO_REGISTER;
SamplerState shadowMapSampler  : TAC_AUTO_REGISTER;
PerFrameBuf  sPerFrame         : TAC_AUTO_REGISTER( b );
PerObjectBuf sPerObj           : TAC_AUTO_REGISTER( b );

struct VS_OUTPUT
{
  float4 mClipSpacePosition  : SV_POSITION;
  float4 mWorldSpacePosition : TAC_AUTO_SEMANTIC;
  float3 mWorldSpaceNormal   : TAC_AUTO_SEMANTIC;
};

VS_OUTPUT VS( VS_INPUT input )
{
  matrix world = sPerObj.mWorld;
  matrix view = sPerFrame.mView;
  matrix proj = sPerFrame.mProj;

  float4 worldSpacePosition = mul( world, float4( input.Position, 1 ) );
  float4 viewSpacePosition = mul( view, worldSpacePosition );
  float4 clipSpacePosition = mul( proj, viewSpacePosition);

  VS_OUTPUT output = ( VS_OUTPUT )0;
  output.mClipSpacePosition = clipSpacePosition;
  output.mWorldSpacePosition = worldSpacePosition;
  output.mWorldSpaceNormal = mul( world, float4( input.Normal, 0 ) ).xyz;
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


float3 ApplyLight( int iLight,
                   VS_OUTPUT input )
{
  Texture2D shadowMap = shadowMaps[ iLight ];
  Light light = lights[ iLight ];
  float4 color = sPerObj.mColor;

  const bool lightCastsShadows = LightGetCastsShadows( light.mFlags );
  if( lightCastsShadows )
  {

    const float4 pixelLightClipSpacePosition = mul( light.mWorldToClip, input.mWorldSpacePosition );
    const float3 pixelLightNDCSpacePosition = pixelLightClipSpacePosition.xyz / pixelLightClipSpacePosition.w;
    if( pixelLightNDCSpacePosition.x < -1 ||
        pixelLightNDCSpacePosition.x > 1 ||
        pixelLightNDCSpacePosition.y < -1 ||
        pixelLightNDCSpacePosition.y > 1 )
      return float3( 0, 0, 0 );

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
    bool occluded = occluded_view;

    if( occluded )
      return float3( 0, 0, 0 );
  }

  uint lightType = LightGetType( light.mFlags );
  if( lightType == LIGHT_TYPE_SPOT )
  {
    float3 n = normalize( input.mWorldSpaceNormal );
    float3 l = -light.mWorldSpaceUnitDirection.xyz;
    const float ndotl = dot( n, l );

    // dont need to check if ndotl < 0 because of the ndc check?
    return ndotl * light.mColorRadiance.xyz * color;
  }

  return float3( 0, 0, 0 );
}

PS_OUTPUT PS( VS_OUTPUT input )
{
  // ambient so that when you draw something, you see something without light
  float ambient = 0.1; // 0.001 // 0
  float3 colorDiffuse = float3( ambient, ambient, ambient );

  float4 color = sPerObj.mColor;

  if( useLights )
  {
    for( int iLight = 0; iLight < lightCount; ++iLight )
    {
      colorDiffuse += ApplyLight( iLight, input );
    }
  }
  else
  {
    colorDiffuse = color.xyz;
  }


  PS_OUTPUT output = ( PS_OUTPUT )0;
  output.mColor.xyz = pow( colorDiffuse, 1.0 / 2.2 );
  output.mColor.w = color.w;


  // temp begin
  output.mColor.xyz /= 1000.0;
  output.mColor.xyz += color.xyz;
  output.mColor.x += input.mClipSpacePosition.x / 1000.0;
  output.mColor.x += input.mClipSpacePosition.y / 1000.0;
  output.mColor.x += input.mClipSpacePosition.z / 1000.0;
  output.mColor.x += input.mClipSpacePosition.w / 1000.0;
  
  float d = input.mClipSpacePosition.z / input.mClipSpacePosition.w;
  output.mColor.x += d / 1000.0f;
  // temp end

  output.mColor.xyz += sPerFrame.mAmbient.xyz;

  return output;
}

