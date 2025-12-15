#include "tac_level_editor_mouse_picking.h" // self-inc
#include "tac_level_editor.h"

#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-level-editor/gizmo/tac_level_editor_light_widget.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/gizmo/tac_level_editor_gizmo_mgr.h"
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
  static Ray      sMouseRay_worldspace      {};
  static Mesh*    mArrow                    {};
  static bool     mWindowHovered            {};
  bool            CreationMousePicking::sDrawRaycast { true };

  // -----------------------------------------------------------------------------------------------

  // Only need the position for picking
  static auto GetPosOnlyVtxDecls() -> Render::VertexDeclarations
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

  static auto MousePickingEntityLight( const Light* light ) -> RaycastResult
  {
    const Ray ray_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    const Sphere sphere{ .mOrigin{light->mEntity->mWorldPosition}, .mRadius{LightWidget::sSize} };
    const float t{ RaySphere( ray_worldspace, sphere ) };
    return
      RaycastResult
      {
        .mHit { t > 0 },
        .mT   { t },
      };
  }

  static auto MousePickingEntityModel( const Model* model, Errors& errors ) -> RaycastResult
  {
    const Ray ray_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
    TAC_CALL_RET( const Mesh * mesh{ MeshPresentation::GetModelMesh( model, errors ) } );
    if( mesh )
      if( const MeshRaycast::Result meshRaycastResult_worldspace{
        mesh->mMeshRaycast.Raycast_worldspace( ray_worldspace, model->mEntity->mWorldTransform ) };
        meshRaycastResult_worldspace.IsValid() )
        return RaycastResult{ .mHit { true }, .mT { meshRaycastResult_worldspace.mT }, };
    return {};
  }

  static auto MousePickingEntity( const Entity* entity, Errors& errors ) -> RaycastResult
  {
    if( const Material * material{ Material::GetMaterial( entity ) } )
      if( !material->mRenderEnabled )
        return {};

    if( const Model * model{ Model::GetModel( entity ) } )
    {
      TAC_CALL_RET( const RaycastResult raycastResult{ MousePickingEntityModel( model, errors ) } );
      if( raycastResult.mHit )
        return raycastResult;
    }

    if( const Light * light{ Light::GetLight( entity ) } )
      if( const RaycastResult raycastResult{ MousePickingEntityLight( light ) };
          raycastResult.mHit )
        return raycastResult;

    return {};
  }

  static void MousePickingGizmos()
  {
    const Camera* camera{ Creation::GetCamera() };
    GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
    if( SelectedEntities::empty()
        || !gizmoMgr->mGizmosEnabled
        || !gizmoMgr->mTranslationGizmoVisible
        || !mWindowHovered )
      return;

    const v3 selectionGizmoOrigin { gizmoMgr->mGizmoOrigin };
    const m4 invArrowRots[]{ m4::RotRadZ( 3.14f / 2.0f ),
                             m4::Identity(),
                             m4::RotRadX( -3.14f / 2.0f ), };
    for( int i{}; i < 3; ++i )
    {
      const Ray mouseRay_worldspace{ CreationMousePicking::GetWorldspaceMouseRay() };
      const m4 gizmoWorldMatrix{ GizmoMgr::WidgetRendererGetAxisWorld( i ) };
      if( const MeshRaycast::Result meshRaycastResult_worldspace{
        mArrow->mMeshRaycast.Raycast_worldspace( mouseRay_worldspace, gizmoWorldMatrix ) };
        meshRaycastResult_worldspace.IsValid() )
      {
        if( pickData.IsNewClosest( meshRaycastResult_worldspace.mT ) )
        {
          pickData.arrowAxis = i;
          pickData.closestDist = meshRaycastResult_worldspace.mT;
          pickData.pickedObject = PickedObject::WidgetTranslationArrow;
        }
      }
    }
  }

  static void MousePickingEntities( Errors& errors )
  {
    if( mWindowHovered )
    {
      for( World* world{ Creation::GetWorld() };
           Entity* entity : world->mEntities )
      {
        TAC_CALL( const RaycastResult raycastResult{ MousePickingEntity( entity, errors ) } );
        if( raycastResult.mHit && pickData.IsNewClosest( raycastResult.mT ) )
        {
          pickData.closestDist = raycastResult.mT;
          pickData.closest = entity;
          pickData.pickedObject = PickedObject::Entity;
        }
      }
    }

  }

  static void MousePickingSelection()
  {
    if( !AppKeyboardApi::JustPressed( Key::MouseLeft ) || !mWindowHovered )
      return;

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        const v3 worldSpaceHitPoint{ sMouseRay_worldspace.mOrigin + pickData.closestDist * sMouseRay_worldspace.mDirection };
        const v3 gizmoOrigin{ SelectedEntities::ComputeAveragePosition() };
        const v3 arrowDirs[]{ v3( 1,0,0 ), v3( 0,1,0 ), v3( 0,0,1 ) };
        const v3 arrowDir{ arrowDirs[pickData.arrowAxis] };
        GizmoMgr* gizmoMgr{ &GizmoMgr::sInstance };
        gizmoMgr->mSelectedGizmo = true;
        gizmoMgr->mTranslationGizmoDir = arrowDir;
        gizmoMgr->mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
        gizmoMgr->mTranslationGizmoAxis = pickData.arrowAxis;
      } break;

      case PickedObject::Entity:
      {
        // Commenting out so that this is handled in the new tac_level_editor_selection_tool.h
        //SelectedEntities::Select( pickData.closest );
      } break;

      case PickedObject::None:
      {
        // Commenting out so that this is handled in the new tac_level_editor_selection_tool.h
        //SelectedEntities::clear();
      } break;
    }
  }

  auto CreationMousePicking::GetPickedEntity() -> Entity*
  {
    return pickData.pickedObject == PickedObject::Entity ? pickData.closest : nullptr;
  }

  auto CreationMousePicking::GetPickedPos() -> v3
  {
    Ray ray{ GetWorldspaceMouseRay() };
    return ray.mOrigin + ray.mDirection * pickData.closestDist;
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

  bool CreationMousePicking::IsTranslationWidgetPicked( int i )
  {
    return
      pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
      pickData.arrowAxis == i;
  }

  void CreationMousePicking::Update( Errors& errors )
  {
    pickData = {};
    TAC_CALL( MousePickingEntities( errors ) );
    MousePickingGizmos();
    MousePickingSelection();
    if( pickData.pickedObject != PickedObject::None )
    {
      dynmc World* world{ Creation::GetWorld() };
      const Camera* camera{ Creation::GetCamera() };
      const Ray ray_worldspace{ GetWorldspaceMouseRay() };
      const v3 worldSpaceHitPoint{ ray_worldspace.mOrigin + pickData.closestDist * ray_worldspace.mDirection };
      world->mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }
  }

  static auto ComputeOrthoMouseRay_worldspace( WindowHandle mWindowHandle ) -> Ray
  {
    const Camera* camera{ Creation::GetCamera() };
    TAC_ASSERT(camera->mType == Camera::Type::kOrthographic);

    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };
    const v2 mousePos_screenspace { AppKeyboardApi::GetMousePosScreenspace() };
    const v2i windowPos_screenspace{ AppWindowApi::GetPos( mWindowHandle ) };
    const v2 mousePos_nwhspace{ mousePos_screenspace - windowPos_screenspace };
    v2 mousePos_ndc( ( mousePos_nwhspace.x / windowSize.x ) * 2 - 1,
                     ( mousePos_nwhspace.y / windowSize.y ) * -2 + 1 );
    float aspect{ (float)windowSize.x / ( float )windowSize.y };
    float orthoHeight{ camera->mOrthoHeight };
    float orthoWidth{ camera->mOrthoHeight * aspect };
    v3 origin{ camera->mPos
      + camera->mRight * mousePos_ndc.x * orthoWidth / 2
      + camera->mUp * mousePos_ndc.y * orthoHeight / 2 };
    return Ray
    {
      .mOrigin    { origin },
      .mDirection { camera->mForwards },
    };
  }

  static auto ComputePerspectiveMouseRay_worldspace( WindowHandle mWindowHandle ) -> Ray
  {
    const Camera* camera{ Creation::GetCamera() };
    const v2i windowSize{ AppWindowApi::GetSize( mWindowHandle ) };
    const v2i windowPos{ AppWindowApi::GetPos( mWindowHandle ) };
    const v2 screenspaceCursorPos { AppKeyboardApi::GetMousePosScreenspace() };
    dynmc float xNDC { ( ( screenspaceCursorPos.x - ( float )windowPos.x ) / ( float )windowSize.x ) };
    dynmc float yNDC { ( ( screenspaceCursorPos.y - ( float )windowPos.y ) / ( float )windowSize.y ) };
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    TAC_ASSERT(camera->mType == Camera::Type::kPerspective);
    const m4 projInv{ camera->ProjInv(  (float)windowSize.x / (float)windowSize.y  ) };
    const m4 viewInv{ camera->ViewInv() };
    const v4 mousePos_ndc( xNDC, yNDC, 1, 1 ); // on far plane
    dynmc v4 mouseDir_view{ projInv * mousePos_ndc };
    mouseDir_view.xyz() /= mouseDir_view.w;
    const v3 mouseDir_world{ Normalize( ( viewInv * v4( mouseDir_view.xyz(), 0 ) ).xyz() ) };
    return 
      Ray
      {
          .mOrigin    { camera->mPos },
          .mDirection { mouseDir_world },
      };
  }

  void CreationMousePicking::BeginFrame( WindowHandle windowHandle )
  {
    mWindowHovered = AppWindowApi::IsHovered( windowHandle );
    const Camera* camera{ Creation::GetCamera() };
    switch( camera->mType )
    {
      case Camera::Type::kOrthographic:
        sMouseRay_worldspace = ComputeOrthoMouseRay_worldspace(windowHandle);
        break;
      case Camera::Type::kPerspective:
        sMouseRay_worldspace = ComputePerspectiveMouseRay_worldspace(windowHandle);
        break;
      default:
        TAC_ASSERT_INVALID_CASE( camera->mType );
        break;
    }
  }

  auto CreationMousePicking::GetWorldspaceMouseRay() -> Ray { return sMouseRay_worldspace; }


} // namespace Tac

