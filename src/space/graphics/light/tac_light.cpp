#include "space/graphics/light/tac_light.h" // self-inc

#include "src/common/tac_ints.h"

#include "src/common/math/tac_matrix3.h"
#include "src/common/graphics/tac_camera.h"
#include "src/common/dataprocess/tac_json.h"
#include "src/common/math/tac_math.h"
#include "src/common/graphics/tac_renderer_util.h"
#include "space/graphics/tac_graphics.h"
#include "space/ecs/tac_entity.h"
#include "space/ecs/tac_component_registry.h"

namespace Tac
{
  static ComponentRegistryEntry* sComponentRegistryEntry;




  static Component* CreateLightComponent( World* world )
  {
    return GetGraphics( world )->CreateLightComponent();
  }

  static void       DestroyLightComponent( World* world, Component* component )
  {
    GetGraphics( world )->DestroyLightComponent( ( Light* )component );
  }

  static void       SaveLightComponent( Json& lightJson, Component* component )
  {
    auto light = ( Light* )component;
    lightJson[ TAC_MEMBER_NAME( Light, mSpotHalfFOVRadians ) ].SetNumber( light->mSpotHalfFOVRadians );
    lightJson[ TAC_MEMBER_NAME( Light, mSpotHalfFOVRadians ) ].SetNumber( light->mSpotHalfFOVRadians );
    lightJson[ TAC_MEMBER_NAME( Light, mType ) ].SetString( LightTypeToString( light->mType ) );
    lightJson[ TAC_MEMBER_NAME( Light, mShadowResolution ) ].SetNumber( light->mShadowResolution );
    lightJson[ TAC_MEMBER_NAME( Light, mOverrideClipPlanes ) ].SetBool( light->mOverrideClipPlanes );
    lightJson[ TAC_MEMBER_NAME( Light, mFarPlaneOverride ) ].SetNumber( light->mFarPlaneOverride );
    lightJson[ TAC_MEMBER_NAME( Light, mNearPlaneOverride ) ].SetNumber( light->mNearPlaneOverride );
  }

  static void       LoadLightComponent( Json& lightJson, Component* component )
  {
    auto light = ( Light* )component;

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mSpotHalfFOVRadians ) ) )
      light->mSpotHalfFOVRadians = ( float )lightJson[ TAC_MEMBER_NAME( Light, mSpotHalfFOVRadians ) ].mNumber;

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mType ) ) )
      light->mType = LightTypeFromString( lightJson[ TAC_MEMBER_NAME( Light, mType ) ].mString );

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mShadowResolution ) ) )
      light->mShadowResolution = ( int )lightJson[ TAC_MEMBER_NAME( Light, mShadowResolution ) ].mNumber;

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mOverrideClipPlanes ) ) )
      light->mOverrideClipPlanes = ( bool )lightJson[ TAC_MEMBER_NAME( Light, mOverrideClipPlanes ) ].mBoolean;

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mFarPlaneOverride ) ) )
      light->mFarPlaneOverride = ( float )lightJson[ TAC_MEMBER_NAME( Light, mFarPlaneOverride ) ].mNumber;

    if( lightJson.HasChild( TAC_MEMBER_NAME( Light, mNearPlaneOverride ) ) )
      light->mNearPlaneOverride = ( float )lightJson[ TAC_MEMBER_NAME( Light, mNearPlaneOverride ) ].mNumber;
  }


  Light*                        Light::GetLight( Entity* entity )
  {
    return ( Light* )entity->GetComponent( sComponentRegistryEntry );
  }

  const Light*                  Light::GetLight( const Entity* entity )
  {
    return ( Light* )entity->GetComponent( sComponentRegistryEntry );
  }

  const ComponentRegistryEntry* Light::GetEntry() const
  {
    return sComponentRegistryEntry;
  }

  v3                            Light::GetUnitDirection() const
  {
    v3 z = mEntity->mWorldTransform.GetColumn( 2 ).xyz();
    z = Normalize( z );
    return -z;
  }

  Camera                        Light::GetCamera() const
  {
    v3 z = -GetUnitDirection();
    v3 x, y;
    GetFrameRH( z, x, y );

    v3 up( 0, 1, 0 );
    float rads = Acos( Dot( up, y ) );

    m3 m = m3::RotRadAngleAxis( rads, z );
    x = m * x;
    y = m * y;

    // Ideally, we want:
    // 
    // - near value as big as possible
    // - far value as small as possible
    // - fov as small as possible ( if not spot light )
    //
    // to cover the scene

    // Try round up to nearest integer
    {
      auto round3 = []( v3 v )
      {
        return v3( Round( v.x ),
                   Round( v.y ),
                   Round( v.z ) ); };
      const v3 roundedZ = round3( z );
      const v3 roundedX = round3( x );
      const v3 roundedY = round3( y );
      const float roundEps = 0.01f;
      const bool canRound =
        Distance( roundedZ, z ) < roundEps &&
        Distance( roundedX, x ) < roundEps &&
        Distance( roundedY, y ) < roundEps;
      x = canRound ? roundedX : x;
      y = canRound ? roundedY : y;
      z = canRound ? roundedZ : z;
    }


    Camera camera = {};
    camera.mForwards = -z;
    camera.mRight = x;
    camera.mUp = y;
    camera.mFovyrad = mSpotHalfFOVRadians * 2;
    camera.mPos = mEntity->mWorldPosition;

    if( mOverrideClipPlanes )
    {
      camera.mNearPlane = mNearPlaneOverride;
      camera.mFarPlane = mFarPlaneOverride;
    }

    return camera;
  }

  void                          Light::FreeRenderResources()
  {
    mCreatedRenderResources = false;

    Render::DestroyTexture( mShadowMapDepth, TAC_STACK_FRAME );
    mShadowMapDepth = Render::TextureHandle();

    //Render::DestroyTexture( mShadowMapColor, TAC_STACK_FRAME );
    //mShadowMapColor = Render::TextureHandle();

    Render::DestroyFramebuffer( mShadowFramebuffer, TAC_STACK_FRAME );
    mShadowFramebuffer = Render::FramebufferHandle();

    Render::DestroyView( mShadowView );
    mShadowView = Render::ViewHandle();
  }

  void LightDebugImgui( Light* );

  void                                   RegisterLightComponent()
  {
    sComponentRegistryEntry = ComponentRegistry_RegisterComponent();
    sComponentRegistryEntry->mName = "Light";
    //sComponentRegistryEntry->mNetworkBits = ComponentLightBits;
    sComponentRegistryEntry->mCreateFn = CreateLightComponent;
    sComponentRegistryEntry->mDestroyFn = DestroyLightComponent;
    sComponentRegistryEntry->mDebugImguiFn = []( Component* component ){ LightDebugImgui( ( Light* )component ); };
    sComponentRegistryEntry->mSaveFn = SaveLightComponent;
    sComponentRegistryEntry->mLoadFn = LoadLightComponent;
  }

  const char*                            LightTypeToString( Light::Type type )
  {
    switch( type )
    {
      case Light::kDirectional: return "Directional";
      case Light::kSpot: return "Spot";
      default: TAC_ASSERT_INVALID_CASE( type ); return nullptr;
    }
  }

  Light::Type                            LightTypeFromString( const char* str )
  {
    for( int i = 0; i < Light::Type::kCount; ++i )
    {
      auto curType = ( Light::Type )i;
      const char* curTypeStr = LightTypeToString( curType );
      if( 0 == StrCmp( curTypeStr, str ) )
        return curType;
    }
    return Light::Type::kCount;
  }

  Render::ShaderLight                            LightToShaderLight( const Light* light )
  {
    const Camera camera = light->GetCamera();
    const Render::InProj inProj = { .mNear = camera.mNearPlane, .mFar = camera.mFarPlane };
    const Render::OutProj outProj = Render::GetPerspectiveProjectionAB( inProj );
    const float a = outProj.mA;
    const float b = outProj.mB;
    const float w = ( float )light->mShadowResolution;
    const float h = ( float )light->mShadowResolution;
    const float aspect = w / h;
    const m4 view = camera.View();
    const m4 proj = camera.Proj( a, b, aspect );


    const u32 flags = 0
      | Render::GetShaderLightFlagType()->ShiftResult( light->mType )
      | Render::GetShaderLightFlagCastsShadows()->ShiftResult( light->mCastsShadows );

    Render::ShaderLight shaderLight = {};
    shaderLight.mColorRadiance.xyz() = light->mColor;
    shaderLight.mColorRadiance.w = light->mRadiance;
    shaderLight.mFlags = flags;
    shaderLight.mWorldSpaceUnitDirection.xyz() = light->GetUnitDirection();
    shaderLight.mWorldSpacePosition.xyz() = light->mEntity->mWorldPosition;
    shaderLight.mWorldToClip = proj * view;
    shaderLight.mProjA = a;
    shaderLight.mProjB = b;
    return shaderLight;

  }

} // namespace Tac

