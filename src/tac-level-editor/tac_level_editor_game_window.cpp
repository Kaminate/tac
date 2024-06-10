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

#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/string/tac_short_fixed_string.h"
#include "tac-std-lib/os/tac_os.h"

//#include <cmath>

namespace Tac
{
  static bool  drawGrid              { false };
  static bool  sGizmosEnabled        { true };
  static float sWASDCameraPanSpeed   { 10 };
  static float sWASDCameraOrbitSpeed { 0.1f };
  static bool  sWASDCameraOrbitSnap  {};



  struct GameWindowVertex
  {
    v3 pos;
    v3 nor;
  };



  static Render::DefaultCBufferPerFrame GetPerFrame( float w, float h )
  {
    const Camera* camera { gCreation.mEditorCamera };
    const float aspectRatio{ ( float )w / ( float )h };
    const Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const Render::NDCAttribs ndcAttribs { renderDevice->GetInfo().mNDCAttribs };
    const m4::ProjectionMatrixParams projParams
    {
      .mNDCMinZ       { ndcAttribs.mMinZ },
      .mNDCMaxZ       { ndcAttribs.mMaxZ },
      .mViewSpaceNear { camera->mNearPlane },
      .mViewSpaceFar  { camera->mFarPlane },
      .mAspectRatio   { aspectRatio },
      .mFOVYRadians   { camera->mFovyrad },
    };

    const m4 view { camera->View() };
    const m4 proj { m4::ProjPerspective( projParams ) };

    return Render::DefaultCBufferPerFrame
    {
      .mView         { view },
      .mProjection   { proj },
      .mFar          { camera->mFarPlane },
      .mNear         { camera->mNearPlane },
      .mGbufferSize  { w, h }
    };
  }

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
      CameraWASDControlsOrbit( camera, gCreation.mSelectedEntities.GetGizmoOrigin() );
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
      savedCamera = *gCreation.mEditorCamera;
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



  static Render::VertexDeclarations GetVtxDecls3D()
  {
    const Render::VertexDeclaration posDecl
    {
        .mAttribute         { Render::Attribute::Position },
        .mFormat            { Render::VertexAttributeFormat::GetVector3() },
        .mAlignedByteOffset { TAC_OFFSET_OF( GameWindowVertex, pos ) },
    };

    const Render::VertexDeclaration norDecl
    {
        .mAttribute         { Render::Attribute::Normal },
        .mFormat            { Render::VertexAttributeFormat::GetVector3() },
        .mAlignedByteOffset { TAC_OFFSET_OF( GameWindowVertex, nor ) },
    };

    Render::VertexDeclarations m3DvertexFormatDecls;
    m3DvertexFormatDecls.push_back( posDecl );
    m3DvertexFormatDecls.push_back( norDecl );
    return m3DvertexFormatDecls;
  }



  // -----------------------------------------------------------------------------------------------

  CreationGameWindow* CreationGameWindow::Instance { nullptr };

  CreationGameWindow::CreationGameWindow()
  {
    Instance = this;
  }

  CreationGameWindow::~CreationGameWindow()
  {
    Instance = nullptr;

    mIconRenderer.Uninit();
    mWidgetRenderer.Uninit();

    SimWindowApi windowApi{};
    windowApi.DestroyWindow( mWindowHandle );
  }


  void CreationGameWindow::Init( Errors& errors )
  {
    mWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gGameWindowName );

    const Render::VertexDeclarations m3DvertexFormatDecls{ GetVtxDecls3D() };
    TAC_CALL( mCenteredUnitCube = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/box.gltf",
                                                                          0,
                                                                          m3DvertexFormatDecls,
                                                                          errors ) );

    TAC_CALL( mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                               0,
                                                               m3DvertexFormatDecls,
                                                               errors ) );


    TAC_CALL( PlayGame( errors ) );
  }


  void CreationGameWindow::ComputeArrowLen()
  {
    if( gCreation.mSelectedEntities.empty() )
      return;

    const m4 view{ m4::View( gCreation.mEditorCamera->mPos,
                             gCreation.mEditorCamera->mForwards,
                             gCreation.mEditorCamera->mRight,
                             gCreation.mEditorCamera->mUp ) };
    const v3 pos{ gCreation.mSelectedEntities.GetGizmoOrigin() };
    const v4 posVS4{ view * v4( pos, 1 ) };
    const float clip_height{ Abs( Tan( gCreation.mEditorCamera->mFovyrad / 2.0f )
                                   * posVS4.z
                                   * 2.0f ) };
    const float arrowLen{ clip_height * 0.2f };
    mArrowLen = arrowLen;
  }




  void CreationGameWindow::RenderSelectionCircle()
  {
    if( !sGizmosEnabled || gCreation.mSelectedEntities.empty() )
      return;


    Debug3DDrawData* debug3DDrawData{ gCreation.mWorld->mDebug3DDrawData };
    const v3 selectionGizmoOrigin { gCreation.mSelectedEntities.GetGizmoOrigin() };

    debug3DDrawData->DebugDraw3DCircle( selectionGizmoOrigin,
                                         gCreation.mEditorCamera->mForwards,
                                         mArrowLen );
  }



  void CreationGameWindow::RenderEditorWidgets( Render::IContext* renderContext,
                                                WindowHandle viewHandle,
                                                Errors& errors )
  {
    SimWindowApi windowApi{};
    if( !windowApi.IsShown( viewHandle ) )
      return;

    RenderSelectionCircle();
    TAC_CALL( mWidgetRenderer.RenderTranslationWidget( renderContext, viewHandle, errors ) );
    //TAC_CALL( 
      gCreation.mSysState.mIconRenderer.RenderLights( gCreation.mSimState.mWorld, gCreation.mSimState.mEditorCamera, renderContext, viewHandle, errors );
    //);
  }

  void CreationGameWindow::PlayGame( Errors& errors )
  {
    if( mSoul )
      return;

    auto ghost { TAC_NEW Ghost };
    TAC_CALL( ghost->Init( mSettingsNode, errors ) );
    mSoul = ghost;
  }

  void CreationGameWindow::ImGuiOverlay( Errors& errors )
  {
    static bool mHideUI { false };
    if( mHideUI )
      return;

    SimWindowApi windowApi{};
    const v2i windowSize{ windowApi.GetSize( mWindowHandle ) };

    const float w { 400 };
    const float h{ ( float )windowSize.y };

    ImGuiSetNextWindowSize( { w, h } );
    ImGuiSetNextWindowHandle( mWindowHandle );
    ImGuiBegin( "gameplay overlay" );

    mCloseRequested |= ImGuiButton( "Close Window" );

    ImGuiCheckbox( "Draw grid", &drawGrid );
    ImGuiCheckbox( "hide ui", &mHideUI ); // for screenshots
    ImGuiCheckbox( "draw gizmos", &sGizmosEnabled );

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

    if( ImGuiCollapsingHeader( "Camera" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      Camera* cam = gCreation.mEditorCamera;

      ImGuiDragFloat( "pan speed", &sWASDCameraPanSpeed );
      ImGuiDragFloat( "orbit speed", &sWASDCameraOrbitSpeed );

      if( ImGuiCollapsingHeader( "transform" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        ImGuiDragFloat3( "cam pos", cam->mPos.data() );
        ImGuiDragFloat3( "cam forward", cam->mForwards.data() );
        ImGuiDragFloat3( "cam right", cam->mRight.data() );
        ImGuiDragFloat3( "cam up", cam->mUp.data() );
      }
      if( ImGuiCollapsingHeader( "clipping planes" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        ImGuiDragFloat( "cam far", &cam->mFarPlane );
        ImGuiDragFloat( "cam near", &cam->mNearPlane );
      }

      float deg = RadiansToDegrees(cam->mFovyrad);
      if( ImGuiDragFloat( "entire y fov(deg)", &deg ) )
        cam->mFovyrad = DegreesToRadians( deg );

      if( ImGuiButton( "cam snap pos" ) )
      {
        cam->mPos.x = ( float )( int )cam->mPos.x;
        cam->mPos.y = ( float )( int )cam->mPos.y;
        cam->mPos.z = ( float )( int )cam->mPos.z;
      }
      if( ImGuiButton( "cam snap dir" ) )
        cam->SetForwards( SnapToUnitDir( cam->mForwards ) );
    }

    if( Timestep::GetElapsedTime() < mStatusMessageEndTime )
    {
      ImGuiText( mStatusMessage );
    }

    ImGuiEnd();
  }

  void CreationGameWindow::CameraUpdateControls()
  {
    SimWindowApi windowApi{};
    if( !windowApi.IsHovered( mWindowHandle ) )
      return;

    SimKeyboardApi keyboardApi{};

    const Camera oldCamera { *gCreation.mEditorCamera };

    const v2 mouseDeltaPos { keyboardApi.GetMousePosDelta() };
    if( keyboardApi.IsPressed( Key::MouseRight ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float pixelsPerDeg { 400.0f / 90.0f };
      const float radiansPerPixel { ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f ) };
      const v2 angleRadians { mouseDeltaPos * radiansPerPixel };

      if( angleRadians.x != 0 )
      {
        m3 matrix { m3::RotRadAngleAxis( -angleRadians.x, gCreation.mEditorCamera->mUp ) };
        gCreation.mEditorCamera->mForwards = matrix * gCreation.mEditorCamera->mForwards;
        gCreation.mEditorCamera->mRight = Cross( gCreation.mEditorCamera->mForwards,
                                                 gCreation.mEditorCamera->mUp );
      }

      if( angleRadians.y != 0 )
      {
        m3 matrix { m3::RotRadAngleAxis( -angleRadians.y, gCreation.mEditorCamera->mRight ) };
        gCreation.mEditorCamera->mForwards = matrix * gCreation.mEditorCamera->mForwards;
        gCreation.mEditorCamera->mUp = Cross( gCreation.mEditorCamera->mRight,
                                              gCreation.mEditorCamera->mForwards );
      }

      // Snapping right.y to the x-z plane prevents the camera from tilting side-to-side.
      gCreation.mEditorCamera->mForwards.Normalize();
      gCreation.mEditorCamera->mRight.y = 0;
      gCreation.mEditorCamera->mRight.Normalize();
      gCreation.mEditorCamera->mUp = Cross( gCreation.mEditorCamera->mRight,
                                            gCreation.mEditorCamera->mForwards );
      gCreation.mEditorCamera->mUp.Normalize();
    }

    if( keyboardApi.IsPressed( Key::MouseMiddle ) &&
        mouseDeltaPos != v2( 0, 0 ) )
    {
      const float unitsPerPixel { 5.0f / 100.0f };
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mRight *
        -mouseDeltaPos.x *
        unitsPerPixel;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mUp *
        mouseDeltaPos.y *
        unitsPerPixel;
    }

    const float mouseDeltaScroll { keyboardApi.GetMouseWheelDelta() };
    if( mouseDeltaScroll )
    {
      float unitsPerTick { 1.0f };

      if( gCreation.mSelectedEntities.size() )
      {
        const v3 origin { gCreation.mSelectedEntities.GetGizmoOrigin() };
        unitsPerTick = Distance( origin, gCreation.mEditorCamera->mPos ) * 0.1f;
      }

      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mForwards *
        mouseDeltaScroll *
        unitsPerTick;
    }

    CameraWASDControls( gCreation.mEditorCamera );
  }


  void CreationGameWindow::Render( Errors& errors )
  {
    SysWindowApi windowApi{};
    if( !windowApi.IsShown( mWindowHandle ) )
      return;

    const v2i windowSize{ windowApi.GetSize( mWindowHandle ) };

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    TAC_CALL( Render::IContext::Scope renderScope{
      renderDevice->CreateRenderContext( errors ) } );

    Render::IContext* renderContext{ renderScope.GetContext() };

    const Render::SwapChainHandle swapChainHandle{
      windowApi.GetSwapChainHandle( mWindowHandle ) };

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

    GamePresentationRender( gCreation.mWorld,
                            gCreation.mEditorCamera,
                            windowSize,
                            rtColor,
                            rtDepth,
                            &mWorldBuffers,
                            errors );

    TAC_CALL( RenderEditorWidgets( renderContext, mWindowHandle, errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationRender( gCreation.mWorld,
                               gCreation.mEditorCamera,
                               desktopWindowState->mWidth,
                               desktopWindowState->mHeight,
                               viewHandle );
#endif

  }

  void CreationGameWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    SimWindowApi windowApi{};
    if( !windowApi.IsShown( mWindowHandle ) )
      return;

    if( mSoul )
    {
      //static bool once;
      //if( !once )
      //{
      //  once = true;
      //  Entity* entity = gCreation.CreateEntity();
      //  entity->mName = "Starry-eyed girl";
      //  entity->mPosition = {}; // { 4.5f, -4.0f, -0.5f };
      //  auto model = ( Model* )entity->AddNewComponent( ComponentRegistryEntryIndex::Model );
      //  model->mModelPath = "assets/editor/Box.gltf";
      //}
      TAC_CALL( mSoul->Update( errors ) );
    }


    mMousePicking.BeginFrame();
    CameraUpdateSaved( mSettingsNode, gCreation.mSimState.mEditorCamera );
    CameraUpdateControls();
    ComputeArrowLen();
    mMousePicking.Update();

    TAC_CALL( gCreation.mGizmoMgr.Update( errors ) );

    if( drawGrid )
    {
      Debug3DDrawData* debug3DDrawData{ gCreation.mWorld->mDebug3DDrawData };
      debug3DDrawData->DebugDraw3DGrid();
    }

    TAC_CALL( ImGuiOverlay( errors ) );
  }


  void CreationGameWindow::SetStatusMessage( const StringView& msg,
                                             const TimestampDifference& duration )
  {
    const Timestamp curTime { Timestep::GetElapsedTime() };
    mStatusMessage = msg;
    mStatusMessageEndTime = curTime + duration;
  }
} // namespace Tac

