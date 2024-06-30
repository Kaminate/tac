#include "tac_level_editor_mouse_picking.h" // self-inc
#include "tac_level_editor_light_widget.h"

#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/presentation/tac_mesh_presentation.h"
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

    PickedObject pickedObject;
    float        closestDist;
    Entity*      closest;
    int          arrowAxis;
  };

  static PickData pickData;

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

  // -----------------------------------------------------------------------------------------------

  void CreationMousePicking::MousePickingGizmos( const Camera* camera )
  {
    if( mSelectedEntities->empty() || !mGizmoMgr->mGizmosEnabled )
      return;

    const v3 selectionGizmoOrigin { mGizmoMgr->mGizmoOrigin };

    const m4 invArrowRots[]  {
      m4::RotRadZ( 3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( -3.14f / 2.0f ), };

    for( int i { 0 }; i < 3; ++i )
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

      bool hit {};
      float dist {};
      mArrow->MeshModelSpaceRaycast( modelSpaceRayPos3, modelSpaceRayDir3, &hit, &dist );
      dist *= mGizmoMgr->mArrowLen;
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;

      pickData.arrowAxis = i;
      pickData.closestDist = dist;
      pickData.pickedObject = PickedObject::WidgetTranslationArrow;
    }
  }

  void CreationMousePicking::MousePickingEntities( const World* world, const Camera* camera )
  {
    for( Entity* entity : world->mEntities )
    {
      bool hit { false };
      float dist { 0 };
      MousePickingEntity( camera, entity, &hit, &dist );
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;

      pickData.closestDist = dist;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }
  }

  void CreationMousePicking::MousePickingSelection( const Camera* camera )
  {
    SimKeyboardApi keyboardApi{};
    if( !keyboardApi.JustPressed( Key::MouseLeft ) )
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
    TAC_CALL( mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                               0,
                                                               m3DvertexFormatDecls,
                                                               errors ) );
  }

  v3 CreationMousePicking::GetWorldspaceMouseDir() const { return mWorldSpaceMouseDir; }

  bool CreationMousePicking::IsTranslationWidgetPicked( int i )
  {
    const bool picked{
      pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
      pickData.arrowAxis == i };
    return picked;
  }

  void CreationMousePicking::Update( const World* world, const Camera* camera )
  {
    pickData = {};

    SimWindowApi windowApi{};

    if( !mWindowHovered )
      return;

    MousePickingEntities( world, camera );

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
    SimWindowApi windowApi{};

    mWindowHovered = windowApi.IsHovered( mWindowHandle );
    if( !mWindowHovered )
      return;

    const v2i windowSize{ windowApi.GetSize( mWindowHandle ) };
    const v2i windowPos{ windowApi.GetPos( mWindowHandle ) };

    SimKeyboardApi keyboardApi{};

    const float w{ ( float )windowSize.x };
    const float h{ ( float )windowSize.y };
    const float x{ ( float )windowPos.x };
    const float y{ ( float )windowPos.y };
    const v2 screenspaceCursorPos { keyboardApi.GetMousePosScreenspace() };
    float xNDC { ( ( screenspaceCursorPos.x - x ) / w ) };
    float yNDC { ( ( screenspaceCursorPos.y - y ) / h ) };
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
    const v3 viewSpaceMousePosNearPlane 
    {
      xNDC / sX,
      yNDC / sY,
      -1,
    };

    const v3 viewSpaceMouseDir { Normalize( viewSpaceMousePosNearPlane ) };
    const v4 viewSpaceMouseDir4 { v4( viewSpaceMouseDir, 0 ) };
    const v4 worldSpaceMouseDir4 { viewInv * viewSpaceMouseDir4 };
    mWorldSpaceMouseDir = worldSpaceMouseDir4.xyz();
    mViewSpaceUnitMouseDir = viewSpaceMouseDir;
  }

  void CreationMousePicking::MousePickingEntityLight( const Camera* camera,
                                                      const Light* light,
                                                      bool* hit,
                                                      float* dist )
  {
    const float lightWidgetSize{ LightWidget::sSize };
    const float t{ RaySphere( camera->mPos,
                         mWorldSpaceMouseDir,
                         light->mEntity->mWorldPosition,
                         lightWidgetSize ) };
    if( t > 0 )
    {
      *hit = true;
      *dist = t;
    }
  }

  void CreationMousePicking::MousePickingEntityModel( const Camera* camera,
                                                      const Model* model,
                                                      bool* hit,
                                                      float* dist )
  {
    const Entity* entity { model->mEntity };
    const Mesh* mesh { MeshPresentationGetModelMesh( model ) };
    if( !mesh )
    {
      *hit = false;
      return;
    }

    bool transformInvExists;
    const m4 transformInv { m4::Inverse( entity->mWorldTransform, &transformInvExists ) };
    if( !transformInvExists )
    {
      *hit = false;
      return;
    }

    const v3 modelSpaceMouseRayPos3 { ( transformInv * v4( camera->mPos, 1 ) ).xyz() };
    const v3 modelSpaceMouseRayDir3 { Normalize( ( transformInv * v4( mWorldSpaceMouseDir, 0 ) ).xyz() ) };
    float modelSpaceDist;
    mesh->MeshModelSpaceRaycast( modelSpaceMouseRayPos3, modelSpaceMouseRayDir3, hit, &modelSpaceDist );

    // Recompute the distance by transforming the model space hit point into world space in order to
    // account for non-uniform scaling
    if( *hit )
    {
      const v3 modelSpaceHitPoint { modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * modelSpaceDist };
      const v3 worldSpaceHitPoint { ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz() };
      *dist = Distance( camera->mPos, worldSpaceHitPoint );
    }
  }

  void CreationMousePicking::MousePickingEntity( const Camera* camera,
                                                 const Entity* entity,
                                                 bool* hit,
                                                 float* dist )
  {
    if( const Model * model{ Model::GetModel( entity ) } )
    {
      MousePickingEntityModel( camera, model, hit, dist );
      if( hit )
        return;
    }

    if( const Light * light{ Light::GetLight( entity ) } )
    {
      MousePickingEntityLight( camera, light, hit, dist );
      if( hit )
        return;
    }
  }



} // namespace Tac

