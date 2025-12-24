#include "tac_level_editor_game_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/renderpass/game/tac_game_presentation.h"
#include "tac-ecs/renderpass/skybox/tac_skybox_presentation.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-ecs/terrain/tac_numgrid.h"
#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_app_window_api.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/windows/main/tac_level_editor_main_window.h"
#include "tac-level-editor/selection/tac_level_editor_entity_selection.h"
#include "tac-level-editor/prefab/tac_level_editor_prefab.h"
#include "tac-level-editor/render/tac_level_editor_widget_renderer.h"
#include "tac-level-editor/windows/property/tac_level_editor_property_window.h"
#include "tac-level-editor/gizmo/tac_level_editor_gizmo_mgr.h"
#include "tac-level-editor/picking/tac_level_editor_mouse_picking.h"
#include "tac-level-editor/tools/tac_level_editor_tool.h"
#include "tac-level-editor/tools/tac_level_editor_selection_tool.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"

namespace Tac
{
  static bool                          sDrawGrid                 {};
  static float                         sWASDCameraPanSpeed       { 10 };
  static float                         sWASDCameraOrbitSpeed     { 5 };
  static bool                          sWASDCameraOrbitSnap      { true };
  static float                         sWASDCameraNoSnapScale    { .01f };
  static StringView                    sImguiWindowName          { "Level Editor Game Window" };
#if 0
  static Ghost*                        sSoul                     {};
#endif
  static Debug3DDrawBuffers            sBuffers                  {};
  static String                        sStatusMessage            {};
  static GameTime                      sStatusMessageEndTime     {};


  bool CreationGameWindow::sShowWindow{};

  static auto SnapToUnitDir( const v3 v ) -> v3 // Returns the unit vector that best aligns with v
  {
    float biggestDot {};
    v3 biggestUnitDir  {};
    for( int iAxis {}; iAxis < 3; ++iAxis )
    {
      for( float sign : { -1.0f, 1.0f } )
      {
        v3 unitDir  {};
        unitDir[ iAxis ] = sign;
        const float d { Dot( v, unitDir ) };
        if( d > biggestDot )
        {
          biggestDot = d;
          biggestUnitDir = unitDir;
        }
      }
    }
    return biggestUnitDir;
  }

  static void CameraWASDControlsPan( Camera* camera )
  {
    v3 combinedDir  {};
     
    struct PanKeyDir
    {
      Key key{};
      v3  dir{};
    };
    
    const PanKeyDir keyDirs[]
    {
      { Key::W, camera->mForwards},
      { Key::A, -camera->mRight},
      { Key::S, -camera->mForwards},
      { Key::D, camera->mRight},
      { Key::Q, -camera->mUp },
      { Key::E, camera->mUp},
    };
    for( const PanKeyDir& keyDir : keyDirs )
      if( AppKeyboardApi::IsPressed( keyDir.key ) )
        combinedDir += keyDir.dir;
    if( combinedDir == v3( 0, 0, 0 ) )
      return;
    camera->mPos += combinedDir * sWASDCameraPanSpeed * TAC_DT;
  }

  static void CameraWASDControlsOrbit( Camera* camera, const v3 orbitCenter )
  {
    const float vertLimit { 0.1f };

    struct OrbitKeyDir
    {
      Key   key          {};
      float mThetaOffset {};
      float mPhiOffset   {};
    };
    
    const OrbitKeyDir keyDirs[]
    {
      OrbitKeyDir{.key{ Key::W }, .mThetaOffset{-1} },
      OrbitKeyDir{.key{ Key::A }, .mPhiOffset{1}},
      OrbitKeyDir{.key{ Key::S }, .mThetaOffset{1}},
      OrbitKeyDir{.key{ Key::D }, .mPhiOffset{-1}},
    };


    float thetaOffset{};
    float phiOffset{};
    for( const OrbitKeyDir& keyDir : keyDirs )
    {
      if( AppKeyboardApi::IsPressed( keyDir.key ) )
      {
        thetaOffset += keyDir.mThetaOffset;
        phiOffset += keyDir.mPhiOffset;
      }
    }

    static float sThetaVel{};
    static float sPhiVel{};
    if( !thetaOffset && !phiOffset && !sThetaVel && !sPhiVel)
      return;

    auto camOrbitSpherical{ SphericalCoordinate::FromCartesian( camera->mPos - orbitCenter ) };
    //if( thetaOffset && !phiOffset )
    //  sPhiVel = 0;
    //if( phiOffset && !thetaOffset )
    //  sThetaVel = 0;
    if( thetaOffset )
      sThetaVel = thetaOffset * sWASDCameraOrbitSpeed;
    if( phiOffset )
      sPhiVel = phiOffset * sWASDCameraOrbitSpeed;

    // damping
    sThetaVel *= .5f;
    sPhiVel *= .5f;

    
    const SphericalCoordinate prevSpherical{ camOrbitSpherical };
    
    camOrbitSpherical.mTheta += sThetaVel * TAC_DT;
    camOrbitSpherical.mPhi += sPhiVel * TAC_DT;
    camOrbitSpherical.mTheta = Clamp( camOrbitSpherical.mTheta, vertLimit, 3.14f - vertLimit );

    camera->mPos = orbitCenter + camOrbitSpherical.ToCartesian();

    if( sWASDCameraOrbitSnap )
    {
      camera->SetForwards( orbitCenter - camera->mPos );
    }
    else
    {
      const v3 dirCart{ camera->mForwards };
      SphericalCoordinate dirSphe{ SphericalCoordinate::FromCartesian( dirCart ) };
      dirSphe.mPhi += sPhiVel * sWASDCameraOrbitSpeed * sWASDCameraNoSnapScale;
      dirSphe.mTheta += -sThetaVel * sWASDCameraOrbitSpeed * sWASDCameraNoSnapScale;
      dirSphe.mTheta = Clamp( dirSphe.mTheta, vertLimit, 3.14f - vertLimit );
      const v3 newForwards{ dirSphe.ToCartesian() };
      camera->SetForwards( newForwards );
    }
  }

  static void CameraWASDControls( Camera* camera )
  {
    if( SelectedEntities::empty() &&
        Toolbox::GetActiveTool() == &SelectionTool::sInstance )
    {
      CameraWASDControlsOrbit( camera, GizmoMgr::sInstance.mGizmoOrigin );
    }
    else
    {
      CameraWASDControlsPan( camera );
    }
  }

  static void CameraUpdateSaved()
  {
    if( Creation::IsGameRunning() )
      return;

    const Camera* camera{ Creation::GetEditorCamera() };
    static AssetPathString savedPrefabPath;
    static Camera savedCamera;

    if( const AssetPathStringView loadedPrefab{ PrefabGetLoaded() };
        ( StringView )loadedPrefab != ( StringView )savedPrefabPath )
    {
      savedPrefabPath = loadedPrefab;
      savedCamera = *camera;
    }

    if( const bool cameraSame{
        savedCamera.mPos == camera->mPos &&
        savedCamera.mForwards == camera->mForwards &&
        savedCamera.mRight == camera->mRight &&
        savedCamera.mUp == camera->mUp };
        cameraSame )
      return;

    savedCamera = *camera;
    PrefabSaveCamera( &savedCamera, savedPrefabPath );
  }

#if 0
  static void PlayGame( Errors& errors )
  {
    if( !sSoul )
    {
      sSoul = TAC_NEW Ghost;
      TAC_CALL( sSoul->Init( errors ) );
    }
  }
#endif

  static void ImGuiCamera()
  {
    Camera* camera{ Creation::GetCamera() };
    if( !ImGuiCollapsingHeader( "Camera" ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    ImGuiDragFloat( "pan speed", &sWASDCameraPanSpeed );
    ImGuiDragFloat( "orbit speed", &sWASDCameraOrbitSpeed );

    if( ImGuiCollapsingHeader( "transform" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiDragFloat3( "cam pos", camera->mPos.data() );
      ImGuiDragFloat3( "cam forward", camera->mForwards.data() );
      ImGuiDragFloat3( "cam right", camera->mRight.data() );
      ImGuiDragFloat3( "cam up", camera->mUp.data() );
    }
    if( ImGuiCollapsingHeader( "clipping planes" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiDragFloat( "cam far", &camera->mFarPlane );
      ImGuiDragFloat( "cam near", &camera->mNearPlane );
    }

    if( camera->mType == Camera::kPerspective )
      if( float deg{ RadiansToDegrees( camera->mFovyrad ) };
          ImGuiDragFloat( "entire y fov(deg)", &deg ) )
        camera->mFovyrad = DegreesToRadians( deg );

    if( camera->mType == Camera::kOrthographic )
      if( ImGuiDragFloat( "ortho height", &camera->mOrthoHeight ) )
        camera->mOrthoHeight = Max( camera->mOrthoHeight, 0.0f );

    ImGuiText( "Type: %s", Camera::TypeToString( camera->mType ) );
    if( ImGuiSameLine(), ImGuiButton( "Change" ) )
      camera->mType = ( Camera::Type )( ( camera->mType + 1 ) % Camera::Type::kCount );


    if( ImGuiButton( "cam snap pos" ) )
    {
      camera->mPos.x = ( float )( int )camera->mPos.x;
      camera->mPos.y = ( float )( int )camera->mPos.y;
      camera->mPos.z = ( float )( int )camera->mPos.z;
    }

    if( ImGuiButton( "cam snap dir" ) )
      camera->SetForwards( SnapToUnitDir( camera->mForwards ) );
  }

  void CreationGameWindow::DebugImGui( Errors& errors )
  {
    static bool mHideUI {};
    if( mHideUI )
      return;

    const ImGuiRect contentRect{ Tac::ImGuiGetContentRect() };
    const v2 cursorPos{ ImGuiGetCursorPos() };
    const float w { 400 };
    const float h{ contentRect.mMaxi.y - cursorPos.y };
    ImGuiBeginChild( "gameplay overlay", v2( w, h ) );
    TAC_ON_DESTRUCT(ImGuiEndChild());

    if( ImGuiButton( "Close Game Window" ) )
      CreationGameWindow::sShowWindow = false;

    if( ImGuiButton( "Close all but Game Window" ) )
    {
      CreationMainWindow::sShowWindow = false;
      CreationPropertyWindow::sShowWindow = false;
    }

    ImGuiCheckbox( "Draw grid", &sDrawGrid );
    ImGuiCheckbox( "hide ui", &mHideUI ); // for screenshots
    ImGuiCheckbox( "draw gizmos", &GizmoMgr::sInstance.mGizmosEnabled );
    ImGuiCheckbox( "Draw raycast", &CreationMousePicking::sDrawRaycast );

#if 0
    if( sSoul )
    {
      if( ImGuiButton( "End simulation" ) )
      {
        TAC_DELETE sSoul;
        sSoul = nullptr;
      }
    }
    else
    {
      if( ImGuiButton( "Begin simulation" ) )
      {
        TAC_CALL( PlayGame( errors ) );
      }
    }
#endif

    ImGuiCamera();
    if( GameTimer::GetElapsedTime() < sStatusMessageEndTime )
      ImGuiText( sStatusMessage );

    //ImGuiSeparator();
    //TAC_CALL( Toolbox::DebugImGui( errors ) );
    //ImGuiSeparator();

  }

  static void CameraUpdateControls()
  {
    WindowHandle mWindowHandle{ ImGuiGetWindowHandle( sImguiWindowName ) };
    if( !AppWindowApi::IsHovered( mWindowHandle ) )
      return;
    
    dynmc Camera* camera{ Creation::GetCamera() };
    const Camera oldCamera { *camera };
    const v2 mouseDeltaPos { AppKeyboardApi::GetMousePosDelta() };
    if( AppKeyboardApi::IsPressed( Key::MouseRight ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float pixelsPerDeg { 400.0f / 90.0f };
      const float radiansPerPixel { ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f ) };
      const v2 angleRadians { mouseDeltaPos * radiansPerPixel };

      if( angleRadians.x != 0 )
      {
        const m3 matrix { m3::RotRadAngleAxis( -angleRadians.x, camera->mUp ) };
        camera->mForwards = Normalize( matrix * camera->mForwards );
        camera->mRight = Normalize( Cross( camera->mForwards, camera->mUp ) );
      }

      if( angleRadians.y != 0 )
      {
        const m3 matrix { m3::RotRadAngleAxis( -angleRadians.y, camera->mRight ) };
        camera->mForwards = Normalize( matrix * camera->mForwards );
        camera->mUp = Cross( camera->mRight, camera->mForwards );
      }

      camera->mForwards.Normalize();
      camera->mUp = Normalize( Cross( v3( camera->mRight.x, 0, camera->mRight.z ), camera->mForwards ) );
      camera->mRight = Normalize( Cross( camera->mForwards, camera->mUp ) );
    }

    if( AppKeyboardApi::IsPressed( Key::MouseMiddle ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float unitsPerPixel { 5.0f / 100.0f };
      camera->mPos += camera->mRight * -mouseDeltaPos.x * unitsPerPixel;
      camera->mPos += camera->mUp * mouseDeltaPos.y * unitsPerPixel;
    }

    const float mouseDeltaScroll { AppKeyboardApi::GetMouseWheelDelta() };
    if( mouseDeltaScroll )
    {
      if( camera->mType == Camera::Type::kPerspective )
      {
        float unitsPerTick{ 1.0f };
        if( !SelectedEntities::empty() )
        {
          const v3 origin{ SelectedEntities::ComputeAveragePosition() };
          unitsPerTick = Distance( origin, camera->mPos ) * 0.1f;
        }

        camera->mPos += camera->mForwards * mouseDeltaScroll * unitsPerTick;
      }
      if( camera->mType == Camera::Type::kOrthographic)
      {
        camera->mOrthoHeight = Max( 0.0f, camera->mOrthoHeight - mouseDeltaScroll );
      }
    }

    CameraWASDControls( camera );
  }

  // -----------------------------------------------------------------------------------------------

  void CreationGameWindow::Init( Errors& errors )
  {
#if 0
    TAC_CALL( PlayGame( errors ) );
#endif
  }

  void CreationGameWindow::Render( World* world, const Camera* camera, Errors& errors )
  {
    const WindowHandle windowHandle{ ImGuiGetWindowHandle( sImguiWindowName ) };
    if( !AppWindowApi::IsShown( windowHandle ) )
      return;

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( Render::IContext::Scope renderScope{ renderDevice->CreateRenderContext( errors ) } );
    Render::IContext* renderContext{ renderScope.GetContext() };
    const Render::SwapChainHandle swapChainHandle{ AppWindowApi::GetSwapChainHandle( windowHandle ) };
    GamePresentation::Render(
      GamePresentation::RenderParams
      {
        .mContext            { renderContext },
        .mWorld              { world },
        .mCamera             { camera },
        .mViewSize           { AppWindowApi::GetSize( windowHandle )  },
        .mColor              { renderDevice->GetSwapChainCurrentColor( swapChainHandle )  },
        .mDepth              { renderDevice->GetSwapChainDepth( swapChainHandle )  },
        .mBuffers            { &sBuffers },
        .mIsLevelEditorWorld { true },
      }, errors );

#if 1
    TAC_CALL( WidgetRenderer::RenderTranslationWidget( renderContext,
                                                       windowHandle,
                                                       camera,
                                                       errors ) );
#endif

    NumGridSys* numGridSys{ NumGridSys::GetSystem( world ) };
    numGridSys->DebugDraw3D( errors );

#if 0
    TAC_CALL( IconRenderer::RenderIcons( world,
                                         camera,
                                         renderContext,
                                         windowHandle,
                                         errors ) );
#endif

    TAC_CALL( renderContext->Execute( errors ) );
  }

  void CreationGameWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    if( !sShowWindow )
      return;


    ImGuiSetNextWindowDisableBG();
    if( !ImGuiBegin( sImguiWindowName, &sShowWindow ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd() );

    const WindowHandle windowHandle{ ImGuiGetWindowHandle() };

#if 0
    if( sSoul )
    {
      TAC_CALL( sSoul->Update( errors ) );
    }
#endif

    CreationMousePicking::BeginFrame( windowHandle );
    CameraUpdateSaved();
    CameraUpdateControls();
    GizmoMgr::sInstance.ComputeArrowLen();
    TAC_CALL( CreationMousePicking::Update( errors ) );
    const Ray mousePickingRay{ CreationMousePicking::GetWorldspaceMouseRay() };
    TAC_CALL( GizmoMgr::sInstance.Update( mousePickingRay, errors ) );

    if( sDrawGrid )
      Creation::GetWorld()->mDebug3DDrawData->DebugDraw3DGrid();

    if( GizmoMgr::sInstance.mGizmosEnabled &&
        GizmoMgr::sInstance.mSelectedGizmo )
      Creation::GetWorld()->mDebug3DDrawData->DebugDraw3DCircle( GizmoMgr::sInstance.mGizmoOrigin,
                                                                 Creation::GetCamera()->mForwards,
                                                                 GizmoMgr::sInstance.mArrowLen );
    //TAC_CALL( ImGuiOverlay( errors ) );

    if( Tool * tool{ Toolbox::GetActiveTool() } )
      tool->Update();
  }

  void CreationGameWindow::SetStatusMessage( const StringView msg, const GameTimeDelta duration )
  {
    sStatusMessage = msg;
    sStatusMessageEndTime = GameTimer::GetElapsedTime() + duration;
  }

  auto CreationGameWindow::GetWindowHandle() -> WindowHandle
  {
    return ImGuiGetWindowHandle( sImguiWindowName );
  }

} // namespace Tac

