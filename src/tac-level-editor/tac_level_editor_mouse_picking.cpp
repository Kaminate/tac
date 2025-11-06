#include "tac_level_editor_mouse_picking.h" // self-inc
#include "tac_level_editor_light_widget.h"


#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-ecs/renderpass/mesh/tac_mesh_presentation.h"
#include "tac-ecs/graphics/material/tac_material.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"


namespace Tac
{
  enum class PickedObject
  {
    None = 0,
    Entity,
    WidgetTranslationArrow,
  };

  struct PickData
  {
    bool IsNewClosest( float dist ) const
    {
      return pickedObject == PickedObject::None || dist < closestDist;
    }

    PickedObject pickedObject {};
    float        closestDist  {};
    Entity*      closest      {};
    int          arrowAxis    {};
  };

  struct RaycastResult
  {
    bool  mHit {};
    float mT   {};
  };


  static PickData pickData;

  CreationMousePicking CreationMousePicking::sInstance;

  // -----------------------------------------------------------------------------------------------

  // Only need the position for picking
  static Render::VertexDeclarations GetPosOnlyVtxDecls()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
    };
    Render::VertexDeclarations m3DvertexFormatDecls;
    m3DvertexFormatDecls.push_back( posDecl );
    return m3DvertexFormatDecls;
  }

  static RaycastResult MousePickingEntityLight( Ray ray, const Light* light )
  {
    const Sphere sphere{ .mOrigin{light->mEntity->mWorldPosition}, .mRadius{LightWidget::sSize} };
    const float t{ RaySphere( ray ,sphere ) };
    return RaycastResult
    {
      .mHit { t > 0 },
      .mT   { t },
    };
  }

  static RaycastResult MousePickingEntityModel( Ray ray, const Model* model, Errors& errors )
  {
    const Entity* entity { model->mEntity };
    TAC_CALL_RET( const Mesh* mesh { MeshPresentation::GetModelMesh( model, errors ) } );
    if( !mesh )
      return {};

    bool transformInvExists;
    const m4 transformInv { m4::Inverse( entity->mWorldTransform, &transformInvExists ) };
    if( !transformInvExists )
      return {};

    const v3 modelSpaceMouseRayPos3 { ( transformInv * v4( ray.mOrigin, 1 ) ).xyz() };
    const v3 modelSpaceMouseRayDir3 { Normalize( ( transformInv * v4( ray.mDirection, 0 ) ).xyz() ) };

    const Ray meshRay
    {
      .mOrigin{ modelSpaceMouseRayPos3 },
      .mDirection{ modelSpaceMouseRayDir3 },
    };

    const MeshRaycast::Result meshRaycastResult { mesh->mMeshRaycast.Raycast( meshRay ) };
    if ( !meshRaycastResult.mHit )
      return {};

    // Recompute the distance by transforming the model space hit point into world space in order to
    // account for non-uniform scaling
    const v3 modelSpaceHitPoint {
      modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * meshRaycastResult.mT };
    const v3 worldSpaceHitPoint {
      ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz() };
    const float dist { Distance( ray.mOrigin, worldSpaceHitPoint ) };
    return RaycastResult
    {
      .mHit { true },
      .mT   { dist },
    };
  }

  static void MousePickingEntityDebug( Ray ray,
                                       const World* world,
                                       const Camera* ,
                                       Errors& errors )
  {
    Debug3DDrawData* drawData{ world->mDebug3DDrawData };

    Triangle closestTri{};
    MeshRaycast::Result closestResult{};
    const Ray meshRay = ray;

    for( Entity* entity : world->mEntities )
    {
      bool isPicked = pickData.pickedObject == PickedObject::Entity && pickData.closest == entity ; 
      bool isSelected = SelectedEntities::IsSelected(entity);
      if( !(isPicked || isSelected))
        continue;

      if( Material * material{ Material::GetMaterial( entity ) } )
        if( !material->mRenderEnabled )
          continue;

      Model* model{ Model::GetModel( entity ) };
      if( !model )
        continue;

      TAC_CALL( const Mesh* mesh { MeshPresentation::GetModelMesh( model, errors ) } );
      if( !mesh )
        continue;

      for( const Triangle& subMeshTri : mesh->mMeshRaycast.mTris )
      {
        const v3 wsTriv0{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 0 ], 1.0f ) ).xyz() };
        const v3 wsTriv1{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 1 ], 1.0f ) ).xyz() };
        const v3 wsTriv2{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 2 ], 1.0f ) ).xyz() };
        const Triangle meshTri { wsTriv0, wsTriv1, wsTriv2, };
        if( const MeshRaycast::Result meshResult{ MeshRaycast::RaycastTri( meshRay, meshTri ) };
            meshResult.mHit && ( !closestResult.mHit || meshResult.mT < closestResult.mT ) )
        {
          closestResult = meshResult;
          closestTri = meshTri;
        }

        // TODO(N8):
        //
        // Using debug draw to draw triangle wireframe is an abuse of debug draw.
        // Debug draw uses lines primitives, which rasterizes differently from triangle primitives,
        // leading to z-fighting.
        // 
        // Instead, I should use a real mesh triangle wireframe for picked/selected objects,
        // and use debug draw for just 
        const v3 color{ 1, 0, 0 };
        drawData->DebugDraw3DTriangle( wsTriv0, wsTriv1, wsTriv2, color );

      }

    }

    if( closestResult.mHit )
    {

      const v3 triColor{ 0, 1, 0 };
      v3 v0{ closestTri[ 0 ] };
      v3 v1{ closestTri[ 1 ] };
      v3 v2{ closestTri[ 2 ] };

      v3 zfighting = 0.001f * Normalize( Cross( v1 - v0, v2 - v0 ) );
      v0 += zfighting;
      v1 += zfighting;
      v2 += zfighting;

      const v3 centroid{ ( v0 + v1 + v2 ) / 3 };
      v0 += ( centroid - v0 ) * 0.01f;
      v1 += ( centroid - v1 ) * 0.01f;
      v2 += ( centroid - v2 ) * 0.01f;
      for( float t {}; t < 1; t += 0.1f )
      {
        v3 color = v3( 1 - t, 0, 0 );
        drawData->DebugDraw3DTriangle( v0 + ( centroid - v0 ) * t,
                                       v1 + ( centroid - v1 ) * t,
                                       v2 + ( centroid - v2 ) * t,
                                       color );
      }
      //float radius = 0.3f / Distance( centroid, ray.mPos );
      //drawData->DebugDraw3DSphere( v0,radius, triColor );

    }
  }

  static RaycastResult MousePickingEntity( Ray ray, const Entity* entity, Errors& errors )
  {
    if( const Material * material{ Material::GetMaterial( entity ) } )
      if( !material->mRenderEnabled )
        return {};

    if( const Model * model{ Model::GetModel( entity ) } )
    {
      TAC_CALL_RET( const RaycastResult raycastResult{
        MousePickingEntityModel( ray, model, errors ) } );
      if( raycastResult.mHit )
        return raycastResult;
    }

    if( const Light * light{ Light::GetLight( entity ) } )
      if( const RaycastResult raycastResult{ MousePickingEntityLight( ray, light ) };
          raycastResult.mHit )
        return raycastResult;

    return {};
  }

  // -----------------------------------------------------------------------------------------------

  void CreationMousePicking::MousePickingGizmos( const Camera* camera )
  {
    GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
    if( SelectedEntities::empty() || !gizmoMgr->mGizmosEnabled || !mWindowHovered )
      return;

    const v3 selectionGizmoOrigin { gizmoMgr->mGizmoOrigin };
    const m4 invArrowRots[]{ m4::RotRadZ( 3.14f / 2.0f ),
                             m4::Identity(),
                             m4::RotRadX( -3.14f / 2.0f ), };
    for( int i{}; i < 3; ++i )
    {
      // 1/3: inverse transform
      v3 modelSpaceRayPos3 { camera->mPos - selectionGizmoOrigin };
      v4 modelSpaceRayPos4 { v4( modelSpaceRayPos3, 1 ) };
      v3 modelSpaceRayDir3 { mWorldSpaceMouseDir };
      v4 modelSpaceRayDir4 { v4( mWorldSpaceMouseDir, 0 ) };

      // 2/3: inverse rotate
      const m4& invArrowRot { invArrowRots[ i ] };
      modelSpaceRayPos4 = invArrowRot * modelSpaceRayPos4;
      modelSpaceRayPos3 = modelSpaceRayPos4.xyz();
      modelSpaceRayDir4 = invArrowRot * modelSpaceRayDir4;
      modelSpaceRayDir3 = modelSpaceRayDir4.xyz();

      // 3/3: inverse scale
      modelSpaceRayPos3 /= gizmoMgr-> mArrowLen;

      const Ray meshRay
      { 
        .mOrigin{ modelSpaceRayPos3 },
        .mDirection{ modelSpaceRayDir3 },
      };

      const MeshRaycast::Result meshRaycastResult{ mArrow->mMeshRaycast.Raycast( meshRay ) };
      if( !meshRaycastResult.mHit  )
        continue;

      const float dist{ meshRaycastResult.mT * gizmoMgr->mArrowLen };
      if( !pickData.IsNewClosest( dist ) )
        continue;

      pickData.arrowAxis = i;
      pickData.closestDist = dist;
      pickData.pickedObject = PickedObject::WidgetTranslationArrow;
    }
  }

  void CreationMousePicking::MousePickingEntities( const World* world,
                                                   const Camera* camera,
                                                   Errors& errors )
  {
    const Ray ray
    {
      .mOrigin{ camera->mPos },
      .mDirection{ mWorldSpaceMouseDir },
    };

    if(mWindowHovered)
    {
      for( Entity* entity : world->mEntities )
      {
        TAC_CALL( const RaycastResult raycastResult{ MousePickingEntity( ray, entity, errors ) } );
        if( !raycastResult.mHit || !pickData.IsNewClosest( raycastResult.mT ) )
          continue;

        pickData.closestDist = raycastResult.mT;
        pickData.closest = entity;
        pickData.pickedObject = PickedObject::Entity;
      }
    }

    if( sDrawRaycast)
    {
      TAC_CALL( MousePickingEntityDebug( ray, world, camera, errors ) );
    }
  }

  void CreationMousePicking::MousePickingSelection( const Camera* camera )
  {
    if( !AppKeyboardApi::JustPressed( Key::MouseLeft ) || !mWindowHovered )
      return;

    const v3 worldSpaceHitPoint { camera->mPos + pickData.closestDist * mWorldSpaceMouseDir };
    GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        const v3 gizmoOrigin{ SelectedEntities::ComputeAveragePosition() };

        v3 arrowDir{};
        arrowDir[ pickData.arrowAxis ] = 1;

        gizmoMgr->mSelectedGizmo = true;
        gizmoMgr->mTranslationGizmoDir = arrowDir;
        gizmoMgr->mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
        gizmoMgr->mTranslationGizmoAxis = pickData.arrowAxis;
      } break;

      case PickedObject::Entity:
      {
        const v3 entityWorldOrigin {
          ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz() };
        SelectedEntities::Select( pickData.closest );
      } break;

      case PickedObject::None:
      {
        SelectedEntities::clear();
      } break;
    }
  }

  void CreationMousePicking::Init( Errors& errors )
  {
    const Render::VertexDeclarations m3DvertexFormatDecls{ GetPosOnlyVtxDecls() };
    const ModelAssetManager::Params meshParams
    {
      .mPath        { "assets/editor/arrow.gltf" },
      .mOptVtxDecls { m3DvertexFormatDecls },
    };
    TAC_CALL( mArrow = ModelAssetManager::GetMesh( meshParams, errors ) );
  }

  v3   CreationMousePicking::GetWorldspaceMouseDir() const { return mWorldSpaceMouseDir; }

  bool CreationMousePicking::IsTranslationWidgetPicked( int i )
  {
    return
      pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
      pickData.arrowAxis == i;
  }

  void CreationMousePicking::Update( const World* world, const Camera* camera, Errors& errors )
  {
    pickData = {};
    TAC_CALL( MousePickingEntities( world, camera, errors ) );
    MousePickingGizmos( camera );
    MousePickingSelection( camera );
    if( pickData.pickedObject != PickedObject::None )
    {
      const v3 worldSpaceHitPoint{ camera->mPos + pickData.closestDist * mWorldSpaceMouseDir };
      world->mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }
  }

  void CreationMousePicking::BeginFrame( WindowHandle mWindowHandle, Camera* camera )
  {
    mWindowHovered = AppWindowApi::IsHovered( mWindowHandle );

    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };
    const v2i windowPos{ AppWindowApi::GetPos( mWindowHandle ) };
    const float w{ ( float )windowSize.x };
    const float h{ ( float )windowSize.y };
    const float x{ ( float )windowPos.x };
    const float y{ ( float )windowPos.y };
    const v2 screenspaceCursorPos { AppKeyboardApi::GetMousePosScreenspace() };
    dynmc float xNDC { ( ( screenspaceCursorPos.x - x ) / w ) };
    dynmc float yNDC { ( ( screenspaceCursorPos.y - y ) / h ) };
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    const float aspect { w / h };
    const float theta { camera->mFovyrad / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / aspect };
    const float sY { cotTheta };
    const m4 viewInv{ m4::ViewInv( camera->mPos,
                                   camera->mForwards,
                                   camera->mRight,
                                   camera->mUp ) };
    const v3 viewSpaceMousePosNearPlane{ xNDC / sX,
                                         yNDC / sY,
                                         -1 };
    const v3 viewSpaceMouseDir { Normalize( viewSpaceMousePosNearPlane ) };
    const v4 viewSpaceMouseDir4 { v4( viewSpaceMouseDir, 0 ) };
    const v4 worldSpaceMouseDir4 { viewInv * viewSpaceMouseDir4 };
    mWorldSpaceMouseDir = worldSpaceMouseDir4.xyz();
    mViewSpaceUnitMouseDir = viewSpaceMouseDir;
  }


} // namespace Tac

