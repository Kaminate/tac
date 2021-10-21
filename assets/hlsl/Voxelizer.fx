// inject direct lighting
// list of lights, with depth buffers?

#include "Common.fx"
#include "LightsCommon.fx"
#include "VoxelCommon.fx"

#define DEBUG_MAKE_EVERY_VOXEL_YELLOW 0
#define DEBUG_INPUT_VARIABLES         0


//===----------------- vertex shader -----------------===//

//                          UAV requires a 'u' register
// umm... so i think like
// these share the same buffer as rtvs and start after the render target views,
// so atm we have 1 rtv occupying slot 0, so the uav starts at index 1
//
//  so we definitely dont want to use an auto register macro here
RWStructuredBuffer< Voxel > mySB : register( u1 );

Texture2D diffuseMaterialTexture : TAC_AUTO_REGISTER;

Texture2D shadowMaps[ 4 ]        : TAC_AUTO_REGISTER;

sampler linearSampler            : TAC_AUTO_REGISTER;
sampler shadowMapSampler         : TAC_AUTO_REGISTER;



// Note 1: Variables are passed between shader stages through SEMANTICS, not variable names!
//         This allows you to have separate structs for VS_OUT and GS_IN
// Note 2: built in semantics ( POSITION[n], NORMAL[n] ) dont carry any special meaning
//         ( left over from d3d9? ), the only thing that really matters is user-supplied semantics
//         and SV_ ( system value ) semantics
// Note 3: The VS_IN semantic names must correspond to D3D11_INPUT_ELEMENT_DESC::SemanticName or else get
//         [ STATE_CREATION ERROR #163: CREATEINPUTLAYOUT_MISSINGELEMENT]

struct VS_INPUT
{
  float3 mPosition : POSITION;
  float3 mNormal   : NORMAL;
  float2 mTexCoord : TEXCOORD;
};

struct VS_OUT_GS_IN
{
  //                               Using underscores as filler identifiers
  //                               as each variable must have a semantic name,
  //                               but the name itself is unimportant
  //
  //                               ^ update: these semantics names will show up in renderdoc,
  //                                         so it would be more useful to name them
  //
  //                               ^ update: i have preprocessor'd my problems away
  //
  float3 mWorldSpacePosition     : SV_AUTO_SEMANTIC;
  float3 mWorldSpaceUnitNormal   : SV_AUTO_SEMANTIC;
  float2 mTexCoord               : SV_AUTO_SEMANTIC;
};

VS_OUT_GS_IN VS( VS_INPUT input )
{
  VS_OUT_GS_IN result;
  result.mWorldSpacePosition = mul( World, float4( input.mPosition, 1.0 ) ).xyz;
  result.mWorldSpaceUnitNormal = normalize( mul( ( float3x3 )World, input.mNormal ) );
  result.mTexCoord = input.mTexCoord;
  return result;
}

//===----------------- geometry shader -----------------===//

struct GS_OUT_PS_IN
{
  float2 mTexCoord                     : SV_AUTO_SEMANTIC;
  float3 mWorldSpaceUnitNormal         : SV_AUTO_SEMANTIC;
  float3 mWorldSpacePosition           : SV_AUTO_SEMANTIC;
  float4 mClipSpacePosition            : SV_POSITION;

#if DEBUG_INPUT_VARIABLES
  float3 debug_worldspaceabsfacenormal : SV_AUTO_SEMANTIC;
  float3 debug_gvoxelgridcenter        : SV_AUTO_SEMANTIC;
  float  debug_gvoxelwidth             : SV_AUTO_SEMANTIC;
  float  debug_gvoxelgridhalfwidth     : SV_AUTO_SEMANTIC;
#endif
};

[ maxvertexcount( 3 ) ]
void GS( triangle VS_OUT_GS_IN input[ 3 ],
         inout TriangleStream< GS_OUT_PS_IN > outputStream )
{
  // non normalized
  const float3 worldSpaceAbsFaceNormal = abs( input[ 0 ].mWorldSpaceUnitNormal +
                                              input[ 1 ].mWorldSpaceUnitNormal +
                                              input[ 2 ].mWorldSpaceUnitNormal );
  const float dominantAxisValue = max( max( worldSpaceAbsFaceNormal.x,
                                            worldSpaceAbsFaceNormal.y ),
                                            worldSpaceAbsFaceNormal.z );

  for( int i = 0; i < 3; ++i )
  {
    VS_OUT_GS_IN curInput = input[ i ];

    float4 clipSpacePosition;
    {
      float3 ndc = ( curInput.mWorldSpacePosition - gVoxelGridCenter )
                 / gVoxelGridHalfWidth;
      if( dominantAxisValue == worldSpaceAbsFaceNormal.x )
        clipSpacePosition = float4( ndc.zy, 0, 1 );
      else if( dominantAxisValue == worldSpaceAbsFaceNormal.y )
        clipSpacePosition = float4( ndc.xz, 0, 1 );
      else
        clipSpacePosition = float4( ndc.xy, 0, 1 );
    }

    GS_OUT_PS_IN output;
    output.mTexCoord             = curInput.mTexCoord;
    output.mWorldSpaceUnitNormal = curInput.mWorldSpaceUnitNormal;
    output.mWorldSpacePosition   = curInput.mWorldSpacePosition;
    output.mClipSpacePosition    = clipSpacePosition;

#if DEBUG_INPUT_VARIABLES
    output.debug_worldspaceabsfacenormal = worldSpaceAbsFaceNormal;
    output.debug_gvoxelgridcenter = gVoxelGridCenter;
    output.debug_gvoxelwidth = gVoxelWidth;
    output.debug_gvoxelgridhalfwidth = gVoxelGridHalfWidth;
#endif

    outputStream.Append( output );
  }
}

//===----------------- pixel shader -----------------===//

// We are computing the radiance for this voxel
// ( each pixel shader corresponds to one voxel )

// return value: diffuse radiance
float3 ApplyLight( int iLight, GS_OUT_PS_IN input )
{
  Light light = lights[ iLight ];

  const bool lightCastsShadows = LightGetCastsShadows( light.mFlags );
  if( lightCastsShadows )
  {
    // TODO:
    // this code is in both Voxelizer.fx( this file )
    // and GamePresentation.fx.
    //
    // Move to LightsCommon.fx?


    const float4 pixelLightClipSpacePosition = mul( light.mWorldToClip, float4( input.mWorldSpacePosition, 1 ) );
    const float3 pixelLightNDCSpacePosition = pixelLightClipSpacePosition.xyz / pixelLightClipSpacePosition.w;
    if( pixelLightNDCSpacePosition.x < -1 ||
        pixelLightNDCSpacePosition.x > 1 ||
        pixelLightNDCSpacePosition.y < -1 ||
        pixelLightNDCSpacePosition.y > 1 )
      return float3( 0, 0, 0 );

    // negative because dx vs ogl texel coords
    const float2 pixelLightTexel = float2( pixelLightNDCSpacePosition.x * 0.5 + 0.5,
                                           pixelLightNDCSpacePosition.y * -0.5 + 0.5 );


    Texture2D shadowMap = shadowMaps[ iLight ];
    const float shadowMapSample = shadowMap.Sample( shadowMapSampler, pixelLightTexel ).x;
    // const float shadowMapCameraDist = -UnprojectNDCToView( shadowMapSample, light.mProjA, light.mProjB );

    const bool occluded = pixelLightNDCSpacePosition.z - 0.005 > shadowMapSample;

    if( occluded )
      // TODO:
      // we shouldn't return 0,0,0 here,
      // we should still add the light's ambient
      return float3( 0, 0, 0 );
  }


  float3 l = -light.mWorldSpaceUnitDirection;
  float3 n = input.mWorldSpaceUnitNormal;

  // TODO:
  // this shouldnt be computed each
  // time this function is called ( for each light )

  float3 colorMaterialDiffuse
    = diffuseMaterialTexture.Sample( linearSampler, input.mTexCoord ).xyz
    * Color.xyz;
  float ndotl = dot( n, l );
  float3 color
    = ndotl * colorMaterialDiffuse * light.mColorRadiance;
    //+ colorLightAmbient
    //+ colorMaterialEmissive;

  return color;
}

struct PS_OUTPUT
{
  float4 mColor : SV_Target0;
};

PS_OUTPUT PS( GS_OUT_PS_IN input )
{
  PS_OUTPUT output = ( PS_OUTPUT )0;
  float3 colorLightAmbient     = float3( 0, 0, 0 );
  float3 colorMaterialEmissive = float3( 0, 0, 0 );
  float3 l                     = float3( 0, 1, 0 );
  float3 n                     = input.mWorldSpaceUnitNormal;
  float3 voxelNDC              = ( input.mWorldSpacePosition - gVoxelGridCenter )
                               / gVoxelGridHalfWidth;

  // Using <= >= instead of < > to prevent iVoxel3 from having invalid values
  if( voxelNDC.x <= -1 || voxelNDC.x >= 1 ||
      voxelNDC.y <= -1 || voxelNDC.y >= 1 ||
      voxelNDC.z <= -1 || voxelNDC.z >= 1 )
    return ( PS_OUTPUT )0;

  float3 voxelUVW = voxelNDC * 0.5f + 0.5f;
  float3 colorMaterialDiffuse
    = diffuseMaterialTexture.Sample( linearSampler, input.mTexCoord ).xyz
    * Color.xyz;
  float3 colorLightDiffuse = dot( n, l );
  float4 color
    //= colorMaterialDiffuse * colorLightDiffuse
    //+ colorLightAmbient
    //+ colorMaterialEmissive;
    = float4( colorMaterialDiffuse, 1 );

if( useLights )
{
  color = float4( 0, 0, 0, 1 );
#if 1
  for( int i = 0; i < lightCount; ++i )
    color.xyz += ApplyLight( i, input );
#else
  ApplyLight( 0 );
  ApplyLight( 1 );
  ApplyLight( 2 );
  ApplyLight( 3 );
#endif
}
else
{
  // ???
}

  uint3 iVoxel3 = floor( voxelUVW * gVoxelGridSize );
  uint  iVoxel
    = iVoxel3.x
    + iVoxel3.y * gVoxelGridSize
    + iVoxel3.z * gVoxelGridSize * gVoxelGridSize;

#if DEBUG_MAKE_EVERY_VOXEL_YELLOW
  for( int i = 0; i < gVoxelGridSize * gVoxelGridSize * gVoxelGridSize; ++i )
    InterlockedMax( mySB[ i ].mColor,
                    VoxelEncodeHDRColor( float4( 1, 1, 0, 1 ) ) );
  return output;
#endif

  uint uint_color = VoxelEncodeHDRColor( color );
  InterlockedMax( mySB[ iVoxel ].mColor, uint_color );

  uint uint_n     = VoxelEncodeUnitNormal( n );
  InterlockedMax( mySB[ iVoxel ].mNormal, uint_n );

  return output;
}

