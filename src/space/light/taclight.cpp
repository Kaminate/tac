#include "src/space/light/tacLight.h"
#include "src/space/tacentity.h"
#include "src/common/tacCamera.h"
#include "src/common/tacJson.h"
#include "src/space/graphics/tacgraphics.h"

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
    lightJson[ "mSpotHalfFOVRadians" ].SetNumber( light->mSpotHalfFOVRadians );
    lightJson[ "mType" ].SetString( LightTypeToString( light->mType ) );
    lightJson[ "mShadowResolution" ].SetNumber( light->mShadowResolution );
  }

  static void       LoadLightComponent( Json& lightJson, Component* component )
  {
    auto light = ( Light* )component;
    if( lightJson.HasChild( "mSpotHalfFOVRadians" ) )
      light->mSpotHalfFOVRadians = ( float )lightJson[ "mSpotHalfFOVRadians" ].mNumber;
    if( lightJson.HasChild( "mType" ) )
      light->mType = LightTypeFromString( lightJson[ "mType" ].mString );
    if( lightJson.HasChild( "mShadowResolution" ) )
      light->mShadowResolution = ( int )lightJson[ "mShadowResolution" ].mNumber;
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
    const v3 unitDir = GetUnitDirection();
    v3 x, y;
    GetFrameRH( -unitDir, x, y );

    // Ideally, we want:
    // 
    // - near value as big as possible
    // - far value as small as possible
    // - fov as small as possible ( if not spot light )
    //
    // to cover the scene


    Camera camera = {};
    camera.mForwards = unitDir;
    camera.mRight = x;
    camera.mUp = y;
    camera.mFovyrad = mSpotHalfFOVRadians * 2;
    camera.mPos = mEntity->mWorldPosition;
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
      default: TAC_CRITICAL_ERROR_INVALID_CASE( type ); return nullptr;
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

}

