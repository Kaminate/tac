#include "tac_light.h" // self-inc

#include "tac-ecs/component/tac_component_registry.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/tac_renderer_util.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/tac_ints.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta.h"

namespace Tac
{
  static ComponentInfo* sComponentInfo;

  TAC_META_REGISTER_COMPOSITE_BEGIN( Light );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mSpotHalfFOVRadians );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mType );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mShadowResolution );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mOverrideClipPlanes );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mFarPlaneOverride );
  TAC_META_REGISTER_COMPOSITE_MEMBER( Light, mNearPlaneOverride );
  TAC_META_REGISTER_COMPOSITE_END( Light );

  static Component* CreateLightComponent( World* world )
  {
    return Graphics::From( world )->CreateLightComponent();
  }

  static void       DestroyLightComponent( World* world, Component* component )
  {
    Graphics::From( world )->DestroyLightComponent( ( Light* )component );
  }

  static void       SaveLightComponent( Json& lightJson, Component* component )
  {
    auto light { ( Light* )component };
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
    return ( Light* )entity->GetComponent( sComponentInfo );
  }

  const Light*                  Light::GetLight( const Entity* entity )
  {
    return ( Light* )entity->GetComponent( sComponentInfo );
  }

  const ComponentInfo* Light::GetEntry() const
  {
    return sComponentInfo;
  }

  v3                            Light::GetUnitDirection() const
  {
    v3 z { mEntity->mWorldTransform.GetColumn( 2 ).xyz() };
    z = Normalize( z );
    return -z;
  }

  Camera                        Light::GetCamera() const
  {
    v3 z { -GetUnitDirection() };
    v3 x, y;
    GetFrameRH( z, x, y );

    v3 up( 0, 1, 0 );
    const float rads { Acos( Dot( up, y ) ) };

    m3 m { m3::RotRadAngleAxis( rads, z ) };
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
                   Round( v.z ) );
        };

      const v3 roundedZ { round3( z ) };
      const v3 roundedX { round3( x ) };
      const v3 roundedY { round3( y ) };
      const float roundEps { 0.01f };
      const bool canRound{
        Distance( roundedZ, z ) < roundEps &&
        Distance( roundedX, x ) < roundEps &&
        Distance( roundedY, y ) < roundEps };
      x = canRound ? roundedX : x;
      y = canRound ? roundedY : y;
      z = canRound ? roundedZ : z;
    }


    Camera camera  {};
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

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    renderDevice->DestroyTexture( mShadowMapDepth );
    mShadowMapDepth = {};
  }

  void                                   Light::RegisterComponent()
  {
    *( sComponentInfo = ComponentInfo::Register() ) = ComponentInfo
    {
      .mName         { "Light" },
      //sComponentInfo->mNetVars = ComponentLightBits;
      .mCreateFn     { CreateLightComponent },
      .mDestroyFn    { DestroyLightComponent },
      .mDebugImguiFn { []( Component* c ) { LightDebugImgui( ( Light* )c ); } },
      //sComponentInfo->mSaveFn = SaveLightComponent;
      //sComponentInfo->mLoadFn = LoadLightComponent;
    };
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

  Light::Type                            LightTypeFromString( const StringView& str )
  {
    for( int i{}; i < Light::Type::kCount; ++i )
    {
      auto curType { ( Light::Type )i };
      const char* curTypeStr { LightTypeToString( curType ) };
      if( ( StringView )curTypeStr == str )
        return curType;
    }
    return Light::Type::kCount;
  }

  Render::ShaderLight                            LightToShaderLight( const Light* light )
  {
    const Camera camera { light->GetCamera() };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera.mNearPlane },
      .mViewSpaceFar  { camera.mFarPlane },
      .mAspectRatio   { 1 },
      .mFOVYRadians   { camera.mFovyrad },
    };

    const m4 view { camera.View() };
    const m4 proj { m4::ProjPerspective( projParams ) };

    const u32 flags{ 0
      | Render::GetShaderLightFlagType()->ShiftResult( light->mType )
      | Render::GetShaderLightFlagCastsShadows()->ShiftResult( light->mCastsShadows ) };

    const float a{ proj.m22 };
    const float b{ proj.m23 };

    const Render::ShaderLight shaderLight
    {
      .mWorldToClip             { proj * view },
      .mWorldSpacePosition      { light->mEntity->mWorldPosition, 1 },
      .mWorldSpaceUnitDirection { light->GetUnitDirection(), 0 },
      .mColorRadiance           { light->mColor, light->mRadiance },
      .mFlags                   { flags },
      .mProjA                   { a },
      .mProjB                   { b },
    };
    return shaderLight;

  }

} // namespace Tac

