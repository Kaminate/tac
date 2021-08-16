#define vec2 float2
#define vec3 float3
#define vec4 float4
#define v2   float2
#define v3   float3
#define v4   float4

cbuffer CBufferPerFrame : register( b0 )
{
  row_major matrix View;
  row_major matrix Projection;
  float            far;
  float            near;
  float2           gbufferSize;

  // float            truncSecs;
  // double           elapsedAppSeconds;
  float            secModTau;
}

cbuffer CBufferPerObject : register( b1 )
{
  row_major matrix World;
  // Is this premultiplied alpha?
  float4           Color;
}

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPOT        1

struct Light
{
  //               Transforms a point from worldspace to the shadowmap's clipspace
  row_major matrix mWorldToClip;
  float3           mWorldSpacePosition;
  float3           mWorldSpaceUnitDirection;

  // this could be a float3 with the radiance multiplied with the color?
  float4           mColorRadiance; // rgb: color, a: radiance

  uint             mFlags;
  float            mProjA;
  float            mProjB;
  TAC_PAD_BYTES( 4 );
};

TAC_DEFINE_BITFIELD_BEGIN;
TAC_DEFINE_BITFIELD_ELEMENT( LightGetType, 4 );
TAC_DEFINE_BITFIELD_ELEMENT( LightGetCastsShadows, 1 );
TAC_DEFINE_BITFIELD_END;

cbuffer CBufferLights  : register( b2 )
{
  Light            lights[ 4 ];
  uint             lightCount;
  bool             useLights;
  uint             MagicNumber;
}
