#include "src/space/light/tacLight.h"
#include "src/space/tacentity.h"
#include "src/common/tacCamera.h"
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
  }

  static void       LoadLightComponent( Json& lightJson, Component* component )
  {
    auto light = ( Light* )component;
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

  Camera                        Light::GetCamera() const
  {
    if( debugCamera )
    {
      Camera c  = *debugCamera;
      //c.mNearPlane = 1.0f;
      //c.mFarPlane = c.mPos.Length() * 1.1f;
      //if( c.mFarPlane < c.mNearPlane )
      //  c.mFarPlane = c.mNearPlane + 1.0f;

      return c;
    }
    // normalize?
    // ortho-normalize?
    v3 x = mEntity->mWorldTransform.GetColumn( 0 ).xyz();
    v3 y = mEntity->mWorldTransform.GetColumn( 1 ).xyz();
    v3 z = mEntity->mWorldTransform.GetColumn( 2 ).xyz();
    Camera camera = {};
    camera.mForwards = -z;
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

}

