#include "tac_level_editor_mouse_picking.h" // self-inc
#include "tac_level_editor_light_widget.h"

#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/color/tac_color_util.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-ecs/presentation/game/tac_game_presentation.h"
#include "tac-ecs/presentation/mesh/tac_mesh_presentation.h"
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

  struct Ray
  {
    v3 mPos {};
    v3 mDir {};
  };

  struct RaycastResult
  {
    bool  mHit {};
    float mT   {};
  };


  static PickData pickData;

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
    const float lightWidgetSize{ LightWidget::sSize };
    const float t{ RaySphere( ray.mPos,
                              ray.mDir,
                              light->mEntity->mWorldPosition,
                              lightWidgetSize ) };
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

    const v3 modelSpaceMouseRayPos3 { ( transformInv * v4( ray.mPos, 1 ) ).xyz() };
    const v3 modelSpaceMouseRayDir3 { Normalize( ( transformInv * v4( ray.mDir, 0 ) ).xyz() ) };

    const MeshRaycast::Ray meshRay
    {
      .mPos{ modelSpaceMouseRayPos3 },
      .mDir{ modelSpaceMouseRayDir3 },
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
    const float dist { Distance( ray.mPos, worldSpaceHitPoint ) };
    return RaycastResult
    {
      .mHit { true },
      .mT   { dist },
    };
  }

  static void MousePickingEntityDebug( Ray ray,
                                       const World* world,
                                       Errors& errors )
  {
    Debug3DDrawData* drawData{ world->mDebug3DDrawData };

    MeshRaycast::SubMeshTriangle closestTri{};
    MeshRaycast::Result closestResult{};
    const MeshRaycast::Ray meshRay
    {
      .mPos{ ray.mPos },
      .mDir{ ray.mDir },
    };

    for( Entity* entity : world->mEntities )
    {
      Model* model{ Model::GetModel( entity ) };
      if( !model )
        continue;

      TAC_CALL( const Mesh* mesh { MeshPresentation::GetModelMesh( model, errors ) } );
      if( !mesh )
        continue;

      for( const MeshRaycast::SubMeshTriangle& subMeshTri : mesh->mMeshRaycast.mTris )
      {
        const v3 wsTriv0{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 0 ], 1.0f ) ).xyz() };
        const v3 wsTriv1{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 1 ], 1.0f ) ).xyz() };
        const v3 wsTriv2{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 2 ], 1.0f ) ).xyz() };


        const MeshRaycast::SubMeshTriangle meshTri
        {
          wsTriv0,
          wsTriv1,
          wsTriv2,
        };

        if( const MeshRaycast::Result meshResult{ MeshRaycast::RaycastTri( meshRay, meshTri ) };
            meshResult.mHit && ( !closestResult.mHit || meshResult.mT < closestResult.mT ) )
        {
          closestResult = meshResult;
          closestTri = meshTri;
        }

        const v3 color{ 1, 0, 0 };
        drawData->DebugDraw3DTriangle( wsTriv0, wsTriv1, wsTriv2 , color );

      }

    }

    if( closestResult.mHit )
    {

      const v3 triColor{ 0, 1, 0 };
      v3 v0{ closestTri[ 0 ] };
      v3 v1{ closestTri[ 1 ] };
      v3 v2{ closestTri[ 2 ] };
      const v3 centroid{ ( v0 + v1 + v2 ) / 3 };
      v0 += ( centroid - v0 ) * 0.01f;
      v1 += ( centroid - v1 ) * 0.01f;
      v2 += ( centroid - v2 ) * 0.01f;
      for( float t {}; t < 1; t += 0.1f )
      {
        drawData->DebugDraw3DTriangle( v0 + ( centroid - v0 ) * t,
                                       v1 + ( centroid - v1 ) * t,
                                       v2 + ( centroid - v2 ) * t,
                                       v3( 1 - t, 0, 0 ) );
      }
      //float radius = 0.3f / Distance( centroid, ray.mPos );
      //drawData->DebugDraw3DSphere( v0,radius, triColor );

    }
  }

  static RaycastResult MousePickingEntity( Ray ray, const Entity* entity, Errors& errors )
  {
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
    if( mSelectedEntities->empty() || !mGizmoMgr->mGizmosEnabled )
      return;

    const v3 selectionGizmoOrigin { mGizmoMgr->mGizmoOrigin };
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
      modelSpaceRayPos3 /= mGizmoMgr-> mArrowLen;

      const MeshRaycast::Ray meshRay
      { 
        .mPos{ modelSpaceRayPos3 },
        .mDir{ modelSpaceRayDir3 },
      };

      const MeshRaycast::Result meshRaycastResult{ mArrow->mMeshRaycast.Raycast( meshRay ) };
      if( !meshRaycastResult.mHit  )
        continue;

      const float dist{ meshRaycastResult.mT * mGizmoMgr->mArrowLen };
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
      .mPos{ camera->mPos },
      .mDir{ mWorldSpaceMouseDir },
    };

    for( Entity* entity : world->mEntities )
    {
      TAC_CALL( const RaycastResult raycastResult{ MousePickingEntity( ray, entity, errors ) } );
      if( !raycastResult.mHit || !pickData.IsNewClosest( raycastResult.mT ) )
        continue;

      pickData.closestDist = raycastResult.mT;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }

    if( sDrawRaycast)
    {
      TAC_CALL( MousePickingEntityDebug( ray, world, errors ) );
    }


  }

  void CreationMousePicking::MousePickingSelection( const Camera* camera )
  {
    
    if( !AppKeyboardApi::JustPressed( Key::MouseLeft ) )
      return;

    const v3 worldSpaceHitPoint { camera->mPos + pickData.closestDist * mWorldSpaceMouseDir };

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        const v3 gizmoOrigin{ mSelectedEntities->ComputeAveragePosition() };

        v3 arrowDir{};
        arrowDir[ pickData.arrowAxis ] = 1;

        mGizmoMgr->mSelectedGizmo = true;
        mGizmoMgr->mTranslationGizmoDir = arrowDir;
        mGizmoMgr->mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
        mGizmoMgr->mTranslationGizmoAxis = pickData.arrowAxis;
      } break;

      case PickedObject::Entity:
      {
        const v3 entityWorldOrigin {
          ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz() };
        mSelectedEntities->Select( pickData.closest );
      } break;

      case PickedObject::None:
      {
        mSelectedEntities->clear();
      } break;
    }
  }

  void CreationMousePicking::Init( SelectedEntities* selectedEntities,
                                   GizmoMgr* gizmoMgr,
                                   Errors& errors )
  {

    mSelectedEntities = selectedEntities;
    mGizmoMgr = gizmoMgr;

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
    const bool picked{ pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
                       pickData.arrowAxis == i };
    return picked;
  }

  void CreationMousePicking::Update( const World* world, const Camera* camera, Errors& errors )
  {
    pickData = {};

    if( !mWindowHovered )
      return;

    TAC_CALL( MousePickingEntities( world, camera, errors ) );

    MousePickingGizmos( camera );

    MousePickingSelection( camera );

    if( pickData.pickedObject != PickedObject::None )
    {
      const v3 worldSpaceHitPoint{
        camera->mPos + pickData.closestDist * mWorldSpaceMouseDir };

      Debug3DDrawData* debug3DDrawData{ world->mDebug3DDrawData };
      debug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }
  }

  void CreationMousePicking::BeginFrame( WindowHandle mWindowHandle, Camera* camera )
  {
    mWindowHovered = AppWindowApi::IsHovered( mWindowHandle );
    if( !mWindowHovered )
      return;

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

