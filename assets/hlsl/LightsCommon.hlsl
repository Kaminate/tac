#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_SPOT        1

struct Light
{
  //               Transforms a point from worldspace to the shadowmap's clipspace
  row_major matrix mWorldToClip;

  float3           mWorldSpacePosition;
  TAC_PAD_BYTES( 4 );

  float3           mWorldSpaceUnitDirection;
  TAC_PAD_BYTES( 4 );

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

cbuffer CBufferLights : TAC_AUTO_REGISTER
{
  Light            lights[ 4 ];
  uint             lightCount;
  bool             useLights;
  uint             MagicNumber;
}
