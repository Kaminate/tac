#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/debug/tac_depth_buffer_visualizer.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
//#include "tac-std-lib/memory/tac_frame_memory.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

//#include <cmath> // std::tan

namespace Tac
{
  static struct LightTypeNames
  {
    LightTypeNames()
    {
      mNames[ Light::Type::kSpot ] = "Spot";
      mNames[ Light::Type::kDirectional ] = "Directional";
      for( int i{}; i < Light::Type::kCount; ++i )
        TAC_ASSERT( mNames[ i ] );
    }
    const char* GetName( const Light::Type type ) { return mNames[ type ]; }
    const char* mNames[ Light::Type::kCount ]     {};
  } sLightTypeNames;

  static const char* LightTypeToName( const Light::Type type ) { return sLightTypeNames.GetName( type ); }
  //static Light::Type LightTypeFromName( const char* name )     { return sLightTypeNames.GetType( name ); }


  static void LightDebugImguiType( Light* light )
  {
    const char* lightTypeStr = LightTypeToName( light->mType );
    ImGuiText( String() + "Light type: " + ToString( lightTypeStr ) );
    ImGuiText( "Change light type: " );
    for( Light::Type type { ( Light::Type )0 };
         type < Light::Type::kCount;
         type = ( Light::Type )( type + 1 ) )
    {
      if( type == light->mType )
        continue;
      ImGuiSameLine();
      const char* name { LightTypeToName( type ) };
      if( ImGuiButton( name ) )
        light->mType = type;
    }

    if( light->mType == Light::kSpot
        && ImGuiCollapsingHeader( String() + lightTypeStr + " light parameters" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      float fovDeg { light->mSpotHalfFOVRadians * ( 180.0f / 3.14f ) };
      if( ImGuiDragFloat( "half fov deg", &fovDeg ) )
      {
        const float eps { 1.0f };
        fovDeg = Max( fovDeg, eps );
        fovDeg = Min( fovDeg, 90.0f - eps );
        light->mSpotHalfFOVRadians = fovDeg * ( 3.14f / 180.0f );
      }
    }

  }

  static void LightDebugImguiShadowResolution( Light* light )
  {
    int newDim { light->mShadowResolution };
    if( ImGuiButton( "-" ) )
      newDim /= 2;
    ImGuiSameLine();
    if( ImGuiButton( "+" ) )
      newDim *= 2;
    ImGuiSameLine();
    newDim = Max( newDim, 64 );
    newDim = Min( newDim, 1024 );
    ImGuiText( String() + "Shadow Resolution "
               + ToString( light->mShadowResolution )
               + "x"
               + ToString( light->mShadowResolution ) );
  }

  static void Camera3DDraw( const Camera& camera, Debug3DDrawData* drawData )
  {
    const float nearPlaneHalfSize { camera.mNearPlane * Tan( camera.mFovyrad / 2 ) };
    const float farPlaneHalfSize { camera.mFarPlane * Tan( camera.mFovyrad / 2 ) };

    v3 nearPoints[ 4 ];
    v3 farPoints[ 4 ];
    for( int i{}; i < 4; ++i )
    {
      const v2 offsets[]  { v2( -1,-1 ), v2( 1, -1 ), v2( 1, 1 ), v2( -1,1 ) };
      const v2 offset { offsets[ i ] };

      auto GetPlanePoint = [ & ]( float planeDist, float halfSize )
        {
          return
            camera.mPos
            + camera.mForwards * planeDist
            + camera.mRight * offset.x * halfSize
            + camera.mUp * offset.y * halfSize;
        };

      nearPoints[ i ] = GetPlanePoint( camera.mNearPlane, nearPlaneHalfSize );
      farPoints[ i ] = GetPlanePoint( camera.mFarPlane, farPlaneHalfSize );
    }

    const v4 color( 1, 1, 1, 1 );
    for( int i{}; i < 4; ++i )
    {
      const int j { ( i + 1 ) % 4 };
      drawData->DebugDraw3DLine( nearPoints[ i ], farPoints[ i ], color );
      drawData->DebugDraw3DLine( nearPoints[ i ], nearPoints[ j ], color );
      drawData->DebugDraw3DLine( farPoints[ i ], farPoints[ j ], color );
    }
  }

  static void LightDebug3DDraw( Light* light )
  {
    Entity* entity { light->mEntity };
    World* world { entity->mWorld };
    Debug3DDrawData* drawData { world->mDebug3DDrawData };
    const Camera camera { light->GetCamera() };
    const v3 p { entity->mWorldPosition };
    drawData->DebugDraw3DArrow( p, p + camera.mForwards * 10.0f );

    const v3 axes[]  { camera.mRight,camera.mUp,-camera.mForwards };
    for( int i{}; i < 3; ++i )
      drawData->DebugDraw3DLine( p, p + axes[ i ] * 5.0f, v4( float( 0 == i ),
                                 float( 1 == i ),
                                 float( 2 == i ),
                                 1 ) );

    Camera3DDraw( camera, drawData );
  }
}

void Tac::LightDebugImgui( Light* light )
{
  LightDebug3DDraw( light );

  const int oldShadowMapResolution { light->mShadowResolution };
  LightDebugImguiType( light );
  ImGuiCheckbox( "Casts shadows", &light->mCastsShadows );
  LightDebugImguiShadowResolution( light );

  static Errors errors;

  TAC_CALL( const Render::TextureHandle viz{
    DepthBufferLinearVisualizationRender(
      light->mShadowMapDepth,
      light->mShadowResolution,
      light->mShadowResolution,
      errors ) }  );

  if( errors )
  {
    ImGuiText( "Errors: " + errors.ToString() );
  }

  const v2 shadowMapSize { v2( 1, 1 ) * 256 };
  ImGuiImage( viz.GetIndex(), shadowMapSize );


  Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  renderDevice->DestroyTexture( viz );


  if( light->mShadowResolution != oldShadowMapResolution )
  {
    light->FreeRenderResources();
  }

  const m4 world_xform { light->mEntity->mWorldTransform };

  const Camera camera = light->GetCamera();
  v3 x { camera.mRight };
  v3 y { camera.mUp };
  v3 z { -camera.mForwards };
  ImGuiDragFloat3( "Local x", x.data() );
  ImGuiDragFloat3( "Local y", y.data() );
  ImGuiDragFloat3( "Local z", z.data() );

  //ImGuiText( "world xform" );
  //const char* r0 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m00, world_xform.m01, world_xform.m02 );
  //const char* r1 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m10, world_xform.m11, world_xform.m12 );
  //const char* r2 = FrameMemoryPrintf( "%.2f %.2f %.2f", world_xform.m20, world_xform.m21, world_xform.m22 );
  //ImGuiText( r0  );
  //ImGuiText( r1  );
  //ImGuiText( r2  );

  ImGuiCheckbox( "override clip planes", &light->mOverrideClipPlanes );
  if( light->mOverrideClipPlanes )
  {
    ImGuiDragFloat( "near", &light->mNearPlaneOverride );
    ImGuiDragFloat( "far", &light->mFarPlaneOverride );
    light->mNearPlaneOverride = Max( light->mNearPlaneOverride, 0.01f );
    light->mFarPlaneOverride = Max( light->mFarPlaneOverride, light->mNearPlaneOverride + 1.0f );
  }

}

