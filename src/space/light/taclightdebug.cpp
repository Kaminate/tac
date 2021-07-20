#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/string/tacString.h"
#include "src/common/tacFrameMemory.h"
#include "src/common/tacCamera.h"
#include "src/common/math/tacMath.h"
#include "src/space/light/tacLight.h"
#include "src/space/tacworld.h"
#include "src/space/tacentity.h"
#include "src/common/graphics/tacDebug3D.h"

namespace Tac
{
  static struct LightTypeNames
  {
    friend static const char* LightTypeToName( Light::Type );
    //friend static Light::Type LightTypeFromName( const char* );
    LightTypeNames()
    {
      mNames[ Light::Type::kSpot ] = "Spot";
      mNames[ Light::Type::kDirectional ] = "Directional";
      for( int i = 0; i < Light::Type::kCount; ++i )
        TAC_ASSERT( mNames[ i ] );
    }
  private:

    const char* GetName( const Light::Type type )
    {
      return mNames[ type ];
    }

    //Light::Type GetType( const char* name )
    //{
    //  for( int i = 0; i < Light::Type::kCount; ++i )
    //    if( !StrCmp( mNames[ i ], name ) )
    //      return ( Light::Type )i;
    //  return Light::Type::kCount;
    //}

    const char* mNames[ Light::Type::kCount ] = {};
  } sLightTypeNames;

  static const char* LightTypeToName( const Light::Type type ) { return sLightTypeNames.GetName( type ); }
  //static Light::Type LightTypeFromName( const char* name )     { return sLightTypeNames.GetType( name ); }


  static void LightDebugImguiType( Light* light )
  {
    ImGuiText( FrameMemoryPrintf( "Light type: %s", LightTypeToName( light->mType ) ) );
    ImGuiText( "Change light type: " );
    for( Light::Type type = ( Light::Type )0; type < Light::Type::kCount; type = ( Light::Type )( type + 1 ) )
    {
      if( type == light->mType )
        continue;
      ImGuiSameLine();
      const char* name = LightTypeToName( type );
      if( ImGuiButton( name ) )
        light->mType = type;
    }

    if( light->mType == Light::kSpot
        && ImGuiCollapsingHeader( FrameMemoryPrintf( "%s light parameters", LightTypeToName( light->mType ) ) ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      float fovDeg = light->mSpotHalfFOVRadians * ( 180.0f / 3.14f );
      if( ImGuiDragFloat( "half fov deg", &fovDeg ) )
      {
        const float eps = 1.0f;
        fovDeg = Max( fovDeg, eps );
        fovDeg = Min( fovDeg, 90.0f - eps );
        light->mSpotHalfFOVRadians = fovDeg * ( 3.14f / 180.0f );
      }
    }

  }

  static void LightDebugImguiShadowResolution( Light* light )
  {
    int newDim = light->mShadowResolution;
    if( ImGuiButton( "-" ) )
      newDim /= 2;
    ImGuiSameLine();
    if( ImGuiButton( "+" ) )
      newDim *= 2;
    ImGuiSameLine();
    newDim = Max( newDim, 64 );
    newDim = Min( newDim, 1024 );
    ImGuiText( FrameMemoryPrintf( "Shadow Resolution %ix%i",
                                  light->mShadowResolution,
                                  light->mShadowResolution ) );
  }

  static void LightDebug3DDraw( Light* light )
  {
    Entity* entity = light->mEntity;
    World* world = entity->mWorld;
    Debug3DDrawData* drawData = world->mDebug3DDrawData;
    Camera camera = light->GetCamera();
    const v3 p = entity->mWorldPosition;
    drawData->DebugDraw3DArrow( p, p + camera.mForwards * 10.0f );

    const v3 axes[] = { camera.mRight,camera.mUp,-camera.mForwards };
    for( int i = 0; i < 3; ++i )
      drawData->DebugDraw3DLine( p, p + axes[ i ] * 5.0f, v4( float( 0 == i ),
                                                              float( 1 == i ),
                                                              float( 2 == i ),
                                                              1 ) );
  }

  void LightDebugImgui( Light* light )
  {
    LightDebug3DDraw( light );

    const int oldShadowMapResolution = light->mShadowResolution;
    LightDebugImguiType( light );
    ImGuiCheckbox( "Casts shadows", &light->mCastsShadows );
    //ImGuiImage( ( int )light->mShadowMapColor, { 100, 100 } );
    LightDebugImguiShadowResolution( light );

    ImGuiText( "note you cant really see shit because nonlinear depth" );
    ImGuiImage( ( int )light->mShadowMapDepth, { 100, 100 } );

    if( light->mShadowResolution != oldShadowMapResolution )
    {
      light->FreeRenderResources();
    }

    m4 world_xform = light->mEntity->mWorldTransform;

    ImGuiText( "world xform" );
    //const char* r0 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m00, world_xform.m01, world_xform.m02 );
    //const char* r1 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m10, world_xform.m11, world_xform.m12 );
    //const char* r2 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m20, world_xform.m21, world_xform.m22 );
    //ImGuiText( r0  );
    //ImGuiText( r1  );
    //ImGuiText( r2  );

  }
}

