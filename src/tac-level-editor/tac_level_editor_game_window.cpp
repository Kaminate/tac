#include "tac_level_editor_game_window.h" // self-inc

#include "tac-desktop-app/desktop_app/tac_desktop_app.h"

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/ghost/tac_ghost.h"
#include "tac-ecs/graphics/light/tac_light.h"
#include "tac-ecs/graphics/model/tac_model.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/presentation/tac_voxel_gi_presentation.h"
#include "tac-ecs/world/tac_world.h"

#include "tac-engine-core/assetmanagers/tac_mesh.h"
#include "tac-engine-core/assetmanagers/tac_model_asset_manager.h"
#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/graphics/camera/tac_camera.h"
#include "tac-engine-core/graphics/debug/tac_debug_3d.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/profile/tac_profile.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h"

#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_prefab.h"
#include "tac-level-editor/tac_level_editor_main_window.h"
#include "tac-level-editor/tac_level_editor_property_window.h"

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"

//#include <cmath>

namespace Tac
{
  static bool                          drawGrid                  { false };
  static float                         sWASDCameraPanSpeed       { 10 };
  static float                         sWASDCameraOrbitSpeed     { 0.1f };
  static bool                          sWASDCameraOrbitSnap      {};
  static StringView                    sImguiWindowName          { "Level Editor Game Window" };
  static Soul*                         mSoul                     {};
  static Debug3DDrawBuffers            mWorldBuffers             {};
  static String                        mStatusMessage            {};
  static Timestamp                     mStatusMessageEndTime     {};
  static SettingsNode                  mSettingsNode             {};
  static GizmoMgr*                     mGizmoMgr                 {};
  static CreationMousePicking*         mMousePicking             {};

  bool CreationGameWindow::sShowWindow{};

  //static Render::DefaultCBufferPerFrame GetPerFrame( float w, float h )
  //{
  //  const Camera* camera { gCreation.mEditorCamera };
  //  const float aspectRatio{ ( float )w / ( float )h };
  //  const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
  //  const Render::NDCAttribs ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
  //  const m4::ProjectionMatrixParams projParams
  //  {
  //    .mNDCMinZ       { ndcAttribs.mMinZ },
  //    .mNDCMaxZ       { ndcAttribs.mMaxZ },
  //    .mViewSpaceNear { camera->mNearPlane },
  //    .mViewSpaceFar  { camera->mFarPlane },
  //    .mAspectRatio   { aspectRatio },
  //    .mFOVYRadians   { camera->mFovyrad },
  //  };
  //  const m4 view { camera->View() };
  //  const m4 proj { m4::ProjPerspective( projParams ) };
  //  return Render::DefaultCBufferPerFrame
  //  {
  //    .mView         { view },
  //    .mProjection   { proj },
  //    .mFar          { camera->mFarPlane },
  //    .mNear         { camera->mNearPlane },
  //    .mGbufferSize  { w, h }
  //  };
  //}

  static v3 SnapToUnitDir( const v3 v ) // Returns the unit vector that best aligns with v
  {
    float biggestDot { 0 };
    v3 biggestUnitDir  {};
    for( int iAxis { 0 }; iAxis < 3; ++iAxis )
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
      Key key;
      v3            dir;
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
      if( SimKeyboardApi().IsPressed( keyDir.key ) )
        combinedDir += keyDir.dir;
    if( combinedDir == v3( 0, 0, 0 ) )
      return;
    camera->mPos += combinedDir * sWASDCameraPanSpeed;
  }

  static void CameraWASDControlsOrbit( Camera* camera, const v3 orbitCenter )
  {
    const float vertLimit { 0.1f };

    struct OrbitKeyDir
    {
      Key key;
      v3            spherical;
    };
    
    OrbitKeyDir keyDirs[]
    {
      { Key::W, v3( 0, -1, 0 ) },
      { Key::A, v3( 0,  0, 1 ) },
      { Key::S, v3( 0, 1, 0 ) },
      { Key::D, v3( 0, 0, -1 ) }
    };


    v3 camOrbitSphericalOffset  {};
    for( const OrbitKeyDir& keyDir : keyDirs )
      if( SimKeyboardApi().IsPressed( keyDir.key ) )
        camOrbitSphericalOffset += keyDir.spherical;
    if( camOrbitSphericalOffset == v3( 0, 0, 0 ) )
      return;

    v3 camOrbitSpherical { CartesianToSpherical( camera->mPos - orbitCenter ) };
    camOrbitSpherical += camOrbitSphericalOffset * sWASDCameraOrbitSpeed;
    camOrbitSpherical.y = Clamp( camOrbitSpherical.y, vertLimit, 3.14f - vertLimit );

    camera->mPos = orbitCenter + SphericalToCartesian( camOrbitSpherical );

    if( sWASDCameraOrbitSnap )
    {
      camera->SetForwards( orbitCenter - camera->mPos );
    }
    else
    {
      v3 dirCart { camera->mForwards };
      v3 dirSphe { CartesianToSpherical( dirCart ) };
      dirSphe.y += -camOrbitSphericalOffset.y * sWASDCameraOrbitSpeed;
      dirSphe.z += camOrbitSphericalOffset.z * sWASDCameraOrbitSpeed;
      dirSphe.y = Clamp( dirSphe.y, vertLimit, 3.14f - vertLimit );
      v3 newForwards { SphericalToCartesian( dirSphe ) };
      camera->SetForwards( newForwards );
    }
  }

  static void CameraWASDControls( Camera* camera )
  {
    if( gCreation.mSelectedEntities.empty() )
    {
      CameraWASDControlsPan( camera );
    }
    else
    {
      CameraWASDControlsOrbit( camera, mGizmoMgr->mGizmoOrigin );
    }
  }

  static void CameraUpdateSaved( SettingsNode settingsNode, Camera* camera )
  {
    static AssetPathString savedPrefabPath;
    static Camera savedCamera;

    const AssetPathString loadedPrefab { PrefabGetLoaded() };
    if( loadedPrefab != savedPrefabPath )
    {
      savedPrefabPath = loadedPrefab;
      savedCamera = *camera;
    }

    const bool cameraSame{
      savedCamera.mPos == camera->mPos &&
      savedCamera.mForwards == camera->mForwards &&
      savedCamera.mRight == camera->mRight &&
      savedCamera.mUp == camera->mUp };
    if( cameraSame )
      return;

    savedCamera = *camera;
    PrefabSaveCamera( settingsNode, camera );
  }

  static void PlayGame( Errors& errors )
  {
    if( mSoul )
      return;

    auto ghost { TAC_NEW Ghost };
    TAC_CALL( ghost->Init( mSettingsNode, errors ) );
    mSoul = ghost;
  }

  static void ImGuiCamera( Camera* camera )
  {
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

    float deg = RadiansToDegrees( camera->mFovyrad );
    if( ImGuiDragFloat( "entire y fov(deg)", &deg ) )
      camera->mFovyrad = DegreesToRadians( deg );

    if( ImGuiButton( "cam snap pos" ) )
    {
      camera->mPos.x = ( float )( int )camera->mPos.x;
      camera->mPos.y = ( float )( int )camera->mPos.y;
      camera->mPos.z = ( float )( int )camera->mPos.z;
    }

    if( ImGuiButton( "cam snap dir" ) )
      camera->SetForwards( SnapToUnitDir( camera->mForwards ) );
  }

  static void ImGuiOverlay( Camera* camera, Errors& errors )
  {
    static bool mHideUI { false };
    if( mHideUI )
      return;

    const ImGuiRect contentRect{ Tac::ImGuiGetContentRect() };
    const v2 cursorPos{ ImGuiGetCursorPos() };
    
    const float w { 400 };
    const float h{ contentRect.mMaxi.y - cursorPos.y };

    ImGuiBeginChild( "gameplay overlay", v2( w, h ) );

    if( ImGuiButton( "Close Window" ) )
      CreationGameWindow::sShowWindow = false;

    if( ImGuiButton( "Close Other Windows" ) )
    {
      CreationMainWindow::sShowWindow = false;
      CreationPropertyWindow::sShowWindow = false;
    }


    

    ImGuiCheckbox( "Draw grid", &drawGrid );
    ImGuiCheckbox( "hide ui", &mHideUI ); // for screenshots
    ImGuiCheckbox( "draw gizmos", &mGizmoMgr->mGizmosEnabled );

    if( mSoul )
    {
      if( ImGuiButton( "End simulation" ) )
      {
        TAC_DELETE mSoul;
        mSoul = nullptr;
      }
    }
    else
    {
      if( ImGuiButton( "Begin simulation" ) )
      {
        TAC_CALL( PlayGame( errors ) );
      }
    }

    ImGuiCamera( camera );

    if( Timestep::GetElapsedTime() < mStatusMessageEndTime )
    {
      ImGuiText( mStatusMessage );
    }

    ImGuiEndChild();
    ImGuiSameLine();

    // The purpose of this invisible button is to obscure the game window, so that you
    // can move an entity via the translation widget GizmoMgr::Update, without unintentionally
    // calling ImGuiWindow::BeginMoveControls and dragging the whole damn window
    ImGuiInvisibleButton( "BUTTON", -v2( 8, 8 ) );
  }

  static void CameraUpdateControls( Camera* camera )
  {
    SimWindowApi windowApi{};

    WindowHandle mWindowHandle{ ImGuiGetWindowHandle( sImguiWindowName ) };
    if( !windowApi.IsHovered( mWindowHandle ) )
      return;

    SimKeyboardApi keyboardApi{};

    const Camera oldCamera { *camera };

    const v2 mouseDeltaPos { keyboardApi.GetMousePosDelta() };
    if( keyboardApi.IsPressed( Key::MouseRight ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float pixelsPerDeg { 400.0f / 90.0f };
      const float radiansPerPixel { ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f ) };
      const v2 angleRadians { mouseDeltaPos * radiansPerPixel };

      if( angleRadians.x != 0 )
      {
        m3 matrix { m3::RotRadAngleAxis( -angleRadians.x, camera->mUp ) };
        camera->mForwards = matrix * camera->mForwards;
        camera->mRight = Cross( camera->mForwards, camera->mUp );
      }

      if( angleRadians.y != 0 )
      {
        m3 matrix { m3::RotRadAngleAxis( -angleRadians.y, camera->mRight ) };
        camera->mForwards = matrix * camera->mForwards;
        camera->mUp = Cross( camera->mRight, camera->mForwards );
      }

      // Snapping right.y to the x-z plane prevents the camera from tilting side-to-side.
      camera->mForwards.Normalize();
      camera->mRight.y = 0;
      camera->mRight.Normalize();
      camera->mUp = Cross( camera->mRight, camera->mForwards );
      camera->mUp.Normalize();
    }

    if( keyboardApi.IsPressed( Key::MouseMiddle ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float unitsPerPixel { 5.0f / 100.0f };
      camera->mPos += camera->mRight * -mouseDeltaPos.x * unitsPerPixel;
      camera->mPos += camera->mUp * mouseDeltaPos.y * unitsPerPixel;
    }

    const float mouseDeltaScroll { keyboardApi.GetMouseWheelDelta() };
    if( mouseDeltaScroll )
    {
      float unitsPerTick { 1.0f };

      if( gCreation.mSelectedEntities.size() )
      {
        const v3 origin { gCreation.mSelectedEntities.ComputeAveragePosition() };
        unitsPerTick = Distance( origin, camera->mPos ) * 0.1f;
      }

      camera->mPos += camera->mForwards * mouseDeltaScroll * unitsPerTick;
    }

    CameraWASDControls( camera );
  }

  // -----------------------------------------------------------------------------------------------

  void CreationGameWindow::Init( GizmoMgr* gizmoMgr,
                                 CreationMousePicking* mousePicking,
                                 SettingsNode settingsNode,
                                 Errors& errors )
  {
    mMousePicking = mousePicking;
    mGizmoMgr = gizmoMgr;
    mSettingsNode = settingsNode;

    TAC_CALL( PlayGame( errors ) );
  }



  void CreationGameWindow::Render( World* world, Camera* camera, Errors& errors )
  {
    SysWindowApi windowApi{};
    WindowHandle windowHandle{ ImGuiGetWindowHandle( sImguiWindowName ) };
    if( !windowApi.IsShown( windowHandle ) )
      return;

    const v2i windowSize{ windowApi.GetSize( windowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( Render::IContext::Scope renderScope{
      renderDevice->CreateRenderContext( errors ) } );

    Render::IContext* renderContext{ renderScope.GetContext() };

    const Render::SwapChainHandle swapChainHandle{
      windowApi.GetSwapChainHandle( windowHandle ) };

    const Render::TextureHandle rtColor{
      renderDevice->GetSwapChainCurrentColor( swapChainHandle ) };

    const Render::TextureHandle rtDepth{
      renderDevice->GetSwapChainDepth( swapChainHandle ) };

    const Render::Targets renderTargets
    {
      .mColors { rtColor },
      .mDepth  { rtDepth },
    };

    renderContext->SetViewport( windowSize );
    renderContext->SetScissor( windowSize );
    renderContext->SetRenderTargets( renderTargets );

    GamePresentationRender( renderContext,
                            world,
                            camera,
                            windowSize,
                            rtColor,
                            rtDepth,
                            &mWorldBuffers,
                            errors );

#if 1

    WidgetRenderer* widgetRenderer{ gCreation.mSysState.mWidgetRenderer };
    TAC_CALL( widgetRenderer->RenderTranslationWidget( renderContext,
                                                       windowHandle,
                                                       camera,
                                                       errors ) );
#endif

#if 0
    IconRenderer* IconRenderer{ gCreation.mSysState.mIconRenderer };
    TAC_CALL( IconRenderer->RenderLights( world,
                                          camera,
                                          renderContext,
                                          windowHandle,
                                          errors ) );
#endif

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationRender( gCreation.mWorld,
                               gCreation.mEditorCamera,
                               desktopWindowState->mWidth,
                               desktopWindowState->mHeight,
                               viewHandle );
#endif

    TAC_CALL( renderContext->Execute( errors ) );
  }

  struct GameModelVtx
  {
    v3 mPos;
    v3 mNor;
  };

  static Render::VertexDeclarations    m3DVertexFormatDecls;
  static void Create3DVertexFormat()
  {
    const Render::VertexDeclaration posDecl
    {
      .mAttribute         { Render::Attribute::Position },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mPos ) },
    };

    const Render::VertexDeclaration norDecl
    {
      .mAttribute         { Render::Attribute::Normal },
      .mFormat            { Render::VertexAttributeFormat::GetVector3() },
      .mAlignedByteOffset { ( int )TAC_OFFSET_OF( GameModelVtx, mNor ) },
    };

    m3DVertexFormatDecls.clear();
    m3DVertexFormatDecls.push_back( posDecl );
    m3DVertexFormatDecls.push_back( norDecl );
  }

  static m4 GetProjMtx( const Camera* camera, const v2i viewSize )
  {
    const float aspectRatio{ ( float )viewSize.x / ( float )viewSize.y };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs{ renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };
    const m4 proj{ m4::ProjPerspective( projParams ) };
    return proj;
  }

  void CreationGameWindow::Update( World* world, Camera* camera, Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    if( !sShowWindow )
      return;

    ImGuiSetNextWindowDisableBG();
    if( !ImGuiBegin( sImguiWindowName ) )
      return;

    TAC_ON_DESTRUCT( ImGuiEnd() );

    const WindowHandle windowHandle{ ImGuiGetWindowHandle() };

    if( mSoul )
    {
      TAC_CALL( mSoul->Update( errors ) );
    }

    mMousePicking->BeginFrame( windowHandle, camera );
    CameraUpdateSaved( mSettingsNode, gCreation.mSimState.mEditorCamera );
    CameraUpdateControls( camera );
    mGizmoMgr->ComputeArrowLen( camera );
    mMousePicking->Update( world, camera );

    const v3 wsMouseDir{ mMousePicking->GetWorldspaceMouseDir() };

    TAC_CALL( gCreation.mGizmoMgr.Update( wsMouseDir, camera, errors ) );

    if( drawGrid )
      world->mDebug3DDrawData->DebugDraw3DGrid();

    if( mGizmoMgr->mGizmosEnabled && mGizmoMgr->mSelectedGizmo )
      world->mDebug3DDrawData->DebugDraw3DCircle( mGizmoMgr->mGizmoOrigin,
                                                  camera->mForwards,
                                                  mGizmoMgr->mArrowLen );

    static bool once;
    if( !once )
    {
      once = true;
      Create3DVertexFormat();
    }


    const v2 origCursorPos{ ImGuiGetCursorPos() };

    SimWindowApi windowApi;
    const v2i windowSize{ windowApi.GetSize( windowHandle ) };
    const m4 proj{ GetProjMtx( camera, windowSize ) };
    const m4 view{ camera->View() };

    struct : public ModelVisitor
    {
      void operator()( Model* model ) override
      {
        Debug3DDrawData* drawData{ model->mEntity->mWorld->mDebug3DDrawData };


        Errors getmeshErrors;
        Mesh* mesh{
          ModelAssetManagerGetMeshTryingNewThing( model->mModelPath.c_str(),
                                                  model->mModelIndex,
                                                  m3DVertexFormatDecls,
                                                  getmeshErrors ) };
        if( !mesh )
          return;


        for( SubMesh& subMesh : mesh->mSubMeshes )
        {
          for( SubMeshTriangle& subMeshTri : subMesh.mTris )
          {

            v3 wsTriv0{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 0 ], 1.0f ) ).xyz() };
            v3 wsTriv1{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 1 ], 1.0f ) ).xyz() };
            v3 wsTriv2{ ( model->mEntity->mWorldTransform * v4( subMeshTri[ 2 ], 1.0f ) ).xyz() };
            v3 wsTrivs[ 3 ]{ wsTriv0, wsTriv1,wsTriv2 };

            v3 color{ 1, 0, 0 };
            drawData->DebugDraw3DTriangle( wsTriv0, wsTriv1, wsTriv2 , color );

            for( int i{}; i < 3; ++i )
            {
              const v4 wsTriV{ wsTrivs[ i ], 1.0f };
              const float radius{ 0.01f * Distance( wsTriV.xyz(), camera->mPos ) };

              if( RaySphere( camera->mPos, wsMouseDir, wsTriV.xyz(), radius ) > 0 )
              {
                drawData->DebugDraw3DCircle( wsTriV.xyz(), camera->mForwards, radius );

                const v4 vsTriV{ view * wsTriV };
                const v4 csTriV{ proj * vsTriV };
                const v4 ndcTriV{ csTriV / csTriV.w };
                const v2 ssTriV
                {
                   ( 0 + ( ndcTriV.x * 0.5f + 0.5f ) ) * windowSize.x,
                   ( 1 - ( ndcTriV.y * 0.5f + 0.5f ) ) * windowSize.y,
                };

                ImGuiSetCursorPos( ssTriV );
                ImGuiText( String()
                           + "v" + ToString( i ) + ": ("
                           + ToString( subMeshTri[ i ].x ) + ", "
                           + ToString( subMeshTri[ i ].y ) + ", "
                           + ToString( subMeshTri[ i ].z ) + " )" );
              }

            }
          }
        }

      }
      m4      view;
      m4      proj;
      v2i     windowSize;
      Camera* camera;
      v3      wsMouseDir;
    } sVisitor;
    sVisitor.proj = proj;
    sVisitor.view = view;
    sVisitor.windowSize = windowSize;
    sVisitor.camera = camera;
    sVisitor.wsMouseDir = wsMouseDir;

    Graphics* graphics{ GetGraphics( world ) };
    graphics->VisitModels( &sVisitor );

    ImGuiSetCursorPos( origCursorPos );

    TAC_CALL( ImGuiOverlay( camera, errors ) );
  }

  void CreationGameWindow::SetStatusMessage( const StringView msg,
                                             const TimestampDifference duration )
  {
    const Timestamp curTime { Timestep::GetElapsedTime() };
    mStatusMessage = msg;
    mStatusMessageEndTime = curTime + duration;
  }

} // namespace Tac

