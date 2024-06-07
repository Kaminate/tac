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
  float        lightWidgetSize       { 6.0f };

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


  struct GameWindowVertex
  {
    v3 pos;
    v3 nor;
  };


  // http://palitri.com/vault/stuff/maths/Rays%20closest%20point.pdf
  //
  //              \ /
  //               \
  //              / \
  //             /   \
  //            /     \    
  //           /   z__\.D 
  //         E.__---  / \
  //         /           \
  //        /             \
  //       /               \
  //     _/                 \_
  //     /| b               |\ a
  //    /                     \
  //  B. <-------- c ----------. A
  //
  //     A - first ray origin
  //     a - first ray direction ( not necessarily normalized )
  //     B - second ray origin
  //     b - second ray direction ( not necessarily normalized )
  //     c - vector from A to B ( c = B - A )
  //     E - closest point on the first ray to the second ray
  //     D - closest point on the second ray to the first ray
  //
  // +-------------+
  // | 3 Equations |
  // +-------------+
  // |
  // +--> Dot( a, z ) = 0
  // +--> Dot( b, z ) = 0
  // +--> c + (b*e) + z - (a*d) = 0
  //
  // +------------+
  // | 3 unknowns |
  // +------------+
  // |
  // +--> e - scalar such that b*e=E
  // +--> d - scalar such that a*d=D
  // +--> z - vector perpendicular to both a and b ( z = D - E )
  static void ClosestPointTwoRays( const v3 A,
                                   const v3 a,
                                   const v3 B,
                                   const v3 b,
                                   float* d,
                                   float* e )
  {
    const v3 c { B - A };
    const float ab { Dot( a, b ) };
    const float bc { Dot( b, c ) };
    const float ac { Dot( a, c ) };
    const float aa { Dot( a, a ) };
    const float bb { Dot( b, b ) };
    const float denom { aa * bb - ab * ab };
    if( d )
    {
      *d = ( -ab * bc + ac * bb ) / denom;
    }
    if( e )
    {
      *e = ( ab * ac - bc * aa ) / denom;
    }
  }

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

  static void AddDrawCall( Render::IContext* renderContext,
                           const Mesh* mesh,
                           Render::ViewHandle viewHandle )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      const Render::DrawArgs drawArgs
      {
        .mIndexCount    { subMesh.mIndexCount },
      };

      Render::SetPrimitiveTopology( subMesh.mPrimitiveTopology );
      Render::SetShader( CreationGameWindow::Instance->m3DShader );
      Render::SetBlendState( CreationGameWindow::Instance->mBlendState );
      Render::SetDepthState( CreationGameWindow::Instance->mDepthState );
      Render::SetRasterizerState( CreationGameWindow::Instance->mRasterizerState );
      Render::SetVertexFormat( CreationGameWindow::Instance->m3DVertexFormat );
      Render::SetSamplerState( { CreationGameWindow::Instance->mSamplerState }  );
      Render::Submit( viewHandle, TAC_STACK_FRAME );

      renderContext->SetVertexBuffer( subMesh.mVertexBuffer );
      renderContext->SetIndexBuffer( subMesh.mIndexBuffer );
      renderContext->Draw( drawArgs );
    }
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
      if( KeyboardIsKeyDown( keyDir.key ) )
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
      if( KeyboardIsKeyDown( keyDir.key ) )
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

  static void CameraUpdateSaved()
  {
    static AssetPathString savedPrefabPath;
    static Camera savedCamera;

    const AssetPathString loadedPrefab = PrefabGetLoaded();
    if( loadedPrefab != savedPrefabPath )
    {
      savedPrefabPath = loadedPrefab;
      savedCamera = *gCreation.mEditorCamera;
    }

    const bool cameraSame{
      savedCamera.mPos == gCreation.mEditorCamera->mPos &&
      savedCamera.mForwards == gCreation.mEditorCamera->mForwards &&
      savedCamera.mRight == gCreation.mEditorCamera->mRight &&
      savedCamera.mUp == gCreation.mEditorCamera->mUp };
    if( cameraSame )
      return;

    savedCamera = *gCreation.mEditorCamera;
    PrefabSaveCamera( gCreation.mEditorCamera );
  }

  static Render::ProgramParams GetProgramParams3DTest()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DTest" },
      .mStackFrame { TAC_STACK_FRAME },
    };
  }

  static Render::ProgramParams GetProgramParams3DSprite()
  {
    return Render::ProgramParams
    {
      .mFileStem   { "3DSprite" },
      .mStackFrame { TAC_STACK_FRAME },
    };
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

  static Render::BlendState GetBlendStateGame()
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::One },
      .mDstRGB   { Render::BlendConstants::Zero },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add},
    };
  }

  static Render::BlendState GetBlendStateGameAlpha()
  {
    return Render::BlendState
    {
      .mSrcRGB   { Render::BlendConstants::SrcA },
      .mDstRGB   { Render::BlendConstants::OneMinusSrcA },
      .mBlendRGB { Render::BlendMode::Add },
      .mSrcA     { Render::BlendConstants::Zero },
      .mDstA     { Render::BlendConstants::One },
      .mBlendA   { Render::BlendMode::Add},
    };
  }

  static Render::DepthState GetDepthState()
  {
    return Render::DepthState
    {
      .mDepthTest  { true },
      .mDepthWrite { true },
      .mDepthFunc  { Render::DepthFunc::Less },
    };
  }
    
  static Render::RasterizerState GetRasterizerState()
  {
    return Render::RasterizerState
    {
      .mFillMode              { Render::FillMode::Solid },
      .mCullMode              { Render::CullMode::None },
      .mFrontCounterClockwise { true },
      .mMultisample           { false },
    };
  }

  static Render::CreateSamplerParams GetSamplerParams()
  {
    return Render::CreateSamplerParams
    {
      .mFilter { Render::Filter::Linear },
      .mName   { "game-window-samp" },
    };
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
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    renderDevice->DestroyProgram( m3DShader);
    renderDevice->DestroyProgram( mSpriteShader);
    renderDevice->DestroyPipeline( mSpritePipeline);
    renderDevice->DestroyPipeline( m3DPipeline);

    SimWindowApi windowApi{};
    windowApi.DestroyWindow( mWindowHandle );
    TAC_DELETE mDebug3DDrawData;
  }

  void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
  {
    Render::IDevice* renderDevice { Render::RenderApi::GetRenderDevice() };

    const Render::ProgramParams programParams3DTest{ GetProgramParams3DTest() };
    TAC_CALL( m3DShader = renderDevice->CreateProgram( programParams3DTest, errors ) );

    const Render::ProgramParams programParams3DSprite{ GetProgramParams3DSprite() };
    TAC_CALL( mSpriteShader = renderDevice->CreateProgram( programParams3DSprite, errors ) );

    const Render::VertexDeclarations m3DvertexFormatDecls{ GetVtxDecls3D() };
    Render::SetRenderObjectDebugName( m3DVertexFormat, "game-window-vtx-fmt" );

    const Render::BlendState mBlendState{ GetBlendStateGame() };
    Render::SetRenderObjectDebugName( mBlendState, "game-window-blend" );

    const Render::BlendState mAlphaBlendState{ GetBlendStateGameAlpha() };
    Render::SetRenderObjectDebugName( mAlphaBlendState, "game-window-alpha-blend" );

    const Render::DepthState mDepthState{ GetDepthState() };
    Render::SetRenderObjectDebugName( mDepthState, "game-window-depth" );
    
    const Render::RasterizerState mRasterizerState{ GetRasterizerState() };
    Render::SetRenderObjectDebugName( mRasterizerState, "game-window-rast" );

    const Render::CreateSamplerParams mSamplerState{ GetSamplerParams() };
    Render::SetRenderObjectDebugName( mSamplerState, "game-window-samp" );
  }

  void CreationGameWindow::Init( Errors& errors )
  {
    mWindowHandle = gCreation.mWindowManager.CreateDesktopWindow( gGameWindowName );

    TAC_CALL( CreateGraphicsObjects( errors ) );



    const Render::VertexDeclarations m3DvertexFormatDecls{ GetVtxDecls3D() };
    TAC_CALL( mCenteredUnitCube = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/box.gltf",
                                                                          0,
                                                                          m3DvertexFormatDecls,
                                                                          errors ) );

    TAC_CALL( mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                               0,
                                                               m3DvertexFormatDecls,
                                                               errors ) );

    mDebug3DDrawData = TAC_NEW Debug3DDrawData;

    TAC_CALL( PlayGame( errors ) );

  }

  void CreationGameWindow::MousePickingGizmos()
  {
    if( gCreation.mSelectedEntities.empty() || !sGizmosEnabled )
      return;

    const v3 selectionGizmoOrigin { gCreation.mSelectedEntities.GetGizmoOrigin() };

    const m4 invArrowRots[]  {
      m4::RotRadZ( 3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( -3.14f / 2.0f ), };

    for( int i { 0 }; i < 3; ++i )
    {
      // 1/3: inverse transform
      v3 modelSpaceRayPos3 { gCreation.mEditorCamera->mPos - selectionGizmoOrigin };
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
      modelSpaceRayPos3 /= mArrowLen;

      bool hit { false };
      float dist { 0 };
      mArrow->MeshModelSpaceRaycast( modelSpaceRayPos3, modelSpaceRayDir3, &hit, &dist );
      dist *= mArrowLen;
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;

      pickData.arrowAxis = i;
      pickData.closestDist = dist;
      pickData.pickedObject = PickedObject::WidgetTranslationArrow;
    }
  }

  void CreationGameWindow::MousePickingEntities()
  {
    for( Entity* entity : gCreation.mWorld->mEntities )
    {
      bool hit { false };
      float dist { 0 };
      MousePickingEntity( entity, &hit, &dist );
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;
      pickData.closestDist = dist;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }
  }

  void CreationGameWindow::MousePickingSelection()
  {
    SimKeyboardApi keyboardApi{};
    if( !keyboardApi.JustPressed( Key::MouseLeft ) )
      return;

    const v3 worldSpaceHitPoint { gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir };

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        const v3 gizmoOrigin { gCreation.mSelectedEntities.GetGizmoOrigin() };

        v3 arrowDir{};
        arrowDir[ pickData.arrowAxis ] = 1;

        gCreation.mSelectedGizmo = true;
        gCreation.mTranslationGizmoDir = arrowDir;
        gCreation.mTranslationGizmoOffset = Dot( arrowDir, worldSpaceHitPoint - gizmoOrigin );
      } break;
      case PickedObject::Entity:
      {
        const v3 entityWorldOrigin {
          ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz() };
        gCreation.mSelectedEntities.Select( pickData.closest );
        gCreation.mSelectedHitOffsetExists = true;
        gCreation.mSelectedHitOffset = worldSpaceHitPoint - entityWorldOrigin;
      } break;
      case PickedObject::None:
      {
        gCreation.mSelectedEntities.clear();
      } break;
    }
  }

  void CreationGameWindow::MousePickingAll()
  {
    pickData = {};

    SimWindowApi windowApi{};

    if( !windowApi.IsHovered( mWindowHandle ) )
      return;

    MousePickingEntities();

    MousePickingGizmos();

    MousePickingSelection();
  }

  void CreationGameWindow::MousePickingInit()
  {

    SimWindowApi windowApi{};
    if( !windowApi.IsHovered( mWindowHandle ) )
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
    const float theta { gCreation.mEditorCamera->mFovyrad / 2.0f };
    const float cotTheta { 1.0f / Tan( theta ) };
    const float sX { cotTheta / aspect };
    const float sY { cotTheta };

    const m4 viewInv{ m4::ViewInv( gCreation.mEditorCamera->mPos,
                                    gCreation.mEditorCamera->mForwards,
                                    gCreation.mEditorCamera->mRight,
                                    gCreation.mEditorCamera->mUp ) };
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

  void CreationGameWindow::MousePickingEntityLight( const Light* light, bool* hit, float* dist )
  {
    //const Entity* entity = light->mEntity;
    //const v3 worldPos = entity->mWorldPosition;
    //const v3 viewPos = ( gCreation.mEditorCamera->View() * v4( entity->mWorldPosition, 1 ) ).xyz();

    //v3 worldSpaceMouseRayDir = mViewSpaceUnitMouseDir;

    //lightWidgetSize;

    const float t{ RaySphere( gCreation.mEditorCamera->mPos,
                         mWorldSpaceMouseDir,
                         light->mEntity->mWorldPosition,
                         lightWidgetSize ) };
    if( t > 0 )
    {
      *hit = true;
      *dist = t;
    }
  }

  void CreationGameWindow::MousePickingEntityModel( const Model* model, bool* hit, float* dist )
  {
    const Entity* entity { model->mEntity };
    const Mesh* mesh { GamePresentationGetModelMesh( model ) };
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

    const Camera* camera { gCreation.mEditorCamera };

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

  void CreationGameWindow::MousePickingEntity( const Entity* entity,
                                               bool* hit,
                                               float* dist )
  {
    if( const Model * model{ Model::GetModel( entity ) } )
    {
      MousePickingEntityModel( model, hit, dist );
      if( hit )
        return;
    }

    if( const Light * light{ Light::GetLight( entity ) } )
    {
      MousePickingEntityLight( light, hit, dist );
      if( hit )
        return;
    }
  }

  void CreationGameWindow::ComputeArrowLen()
  {
    if( gCreation.mSelectedEntities.empty() )
      return;

    const m4 view{ m4::View( gCreation.mEditorCamera->mPos,
                        gCreation.mEditorCamera->mForwards,
                        gCreation.mEditorCamera->mRight,
                        gCreation.mEditorCamera->mUp ) };
    const v3 pos { gCreation.mSelectedEntities.GetGizmoOrigin() };
    const v4 posVS4 { view * v4( pos, 1 ) };
    const float clip_height { Abs( Tan( gCreation.mEditorCamera->mFovyrad / 2.0f ) * posVS4.z * 2.0f ) };
    const float arrowLen { clip_height * 0.2f };
    mArrowLen = arrowLen;
  }


  void CreationGameWindow::RenderEditorWidgetsPicking()
  {
    if( pickData.pickedObject == PickedObject::None )
      return;

    const v3 worldSpaceHitPoint{
      gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir };

    mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );

      //static Timestamp mouseMovement;
      //Mouse::TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );
  }


  void CreationGameWindow::RenderEditorWidgetsSelection( Render::IContext* renderContext,
    const WindowHandle viewHandle )
  {
    if( !sGizmosEnabled || gCreation.mSelectedEntities.empty() )
      return;

    TAC_RENDER_GROUP_BLOCK( renderContext, "Editor Selection" );

    const v3 selectionGizmoOrigin { gCreation.mSelectedEntities.GetGizmoOrigin() };
    const m4 rots[]  {
      m4::RotRadZ( -3.14f / 2.0f ),
      m4::Identity(),
      m4::RotRadX( 3.14f / 2.0f ), };

    mDebug3DDrawData->DebugDraw3DCircle( selectionGizmoOrigin,
                                         gCreation.mEditorCamera->mForwards,
                                         mArrowLen );

    const v3 axises[3]
    {
      v3( 1, 0, 0 ),
      v3( 0, 1, 0 ),
      v3( 0, 0, 1 ),
    };

    for( int i { 0 }; i < 3; ++i )
    {

      const v3 axis { axises[ i ] };
      const Render::PremultipliedAlpha axisPremultipliedColor {
        Render::PremultipliedAlpha::From_sRGB( axis ) };


      // Widget Translation Arrow
      {
        const bool picked{
          pickData.pickedObject == PickedObject::WidgetTranslationArrow &&
          pickData.arrowAxis == i };

        const bool usingTranslationArrow{
          gCreation.mSelectedGizmo &&
          gCreation.mTranslationGizmoDir == axis };

        const bool shine { picked || usingTranslationArrow };

        Render::PremultipliedAlpha arrowColor { axisPremultipliedColor };
        if( shine )
        {
          float t { float( Sin( Timestep::GetElapsedTime() * 6.0 ) ) };
          t *= t;
          arrowColor.mColor = Lerp( v4( 1, 1, 1, 1 ), axisPremultipliedColor.mColor, t );

        }


        const m4 World
        { m4::Translate( selectionGizmoOrigin )
        * rots[ i ]
        * m4::Scale( v3( 1, 1, 1 ) * mArrowLen ) };

        const Render::DefaultCBufferPerObject perObjectData
        {
          .World { World },
          .Color { arrowColor },
        };

        Render::UpdateConstantBuffer( Render::DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( Render::DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        AddDrawCall( renderContext, mArrow, viewHandle );
      }


      // Widget Scale Cube
      // ( it is not current interactable )
      if( false )
      {
        const m4 World{
          m4::Translate( selectionGizmoOrigin ) *
          m4::Translate( axis * ( mArrowLen * 1.1f ) ) *
          rots[ i ] *
          m4::Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f ) };

        const Render::DefaultCBufferPerObject perObjectData
        {
          .World { World },
          .Color { axisPremultipliedColor },
        };

        Render::UpdateConstantBuffer( Render::DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( Render::DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        AddDrawCall( renderContext, mCenteredUnitCube, viewHandle );
      }
    }
  }

  void CreationGameWindow::RenderEditorWidgetsLights( Render::IContext* renderContext,
                                                      WindowHandle viewHandle,
                                                      Errors& errors )
  {
    TAC_RENDER_GROUP_BLOCK( renderContext, "light widgets" );

    struct : public LightVisitor
    {
      void operator()( Light* light ) override { mLights.push_back( light ); }
      Vector< const Light* > mLights;
    } lightVisitor;


    Graphics* graphics { GetGraphics( gCreation.mWorld ) };
    graphics->VisitLights( &lightVisitor );

    const Render::TextureHandle textureHandle{
      TextureAssetManager::GetTexture( "assets/editor/light.png", errors ) };

    for( int iLight { 0 }; iLight < lightVisitor.mLights.size(); ++iLight )
    {
      const Light* light { lightVisitor.mLights[ iLight ] };

      // Q: why am ii only scaling the m00, and not the m11 and m22?
      m4 world { light->mEntity->mWorldTransform };
      world.m00 = lightWidgetSize;

      const Render::DefaultCBufferPerObject perObjectData
      {
        .World { world },
      };

      const Render::ConstantBufferHandle hPerObj{ Render::DefaultCBufferPerObject::Handle };
      const int perObjSize{ sizeof( Render::DefaultCBufferPerObject ) };

      const ShortFixedString groupname{
        ShortFixedString::Concat( "Editor light ", ToString( iLight ) ) };
      TAC_RENDER_GROUP_BLOCK( renderContext, groupname );

      const Render::DrawArgs drawArgs
      {
        .mVertexCount { 6 },
      };

      renderContext->SetPipeline( mSpritePipeline );
      renderContext->SetVertexBuffer( {} );
      renderContext->SetIndexBuffer( {}  );
      renderContext->SetPrimitiveTopology( Render::PrimitiveTopology::TriangleList );
      renderContext->SetTexture( { textureHandle } );
      renderContext->Render::UpdateConstantBuffer( hPerObj,
                                                   &perObjectData,
                                                   perObjSize,
                                                   TAC_STACK_FRAME );
      renderContext->Draw( drawArgs );
    }
  }

  void CreationGameWindow::RenderEditorWidgets( Render::IContext* renderContext,
                                                WindowHandle viewHandle,
                                                Errors& errors )
  {
    SimWindowApi windowApi{};
    if( !windowApi.IsShown( viewHandle ) )
      return;

    const v2i windowSize{ windowApi.GetSize( viewHandle ) };

    TAC_RENDER_GROUP_BLOCK( renderContext, "Editor Widgets" );
    const float w { ( float )windowSize.x };
    const float h { ( float )windowSize.y };
    const Render::DefaultCBufferPerFrame perFrameData { GetPerFrame( w, h ) };
    const Render::ConstantBufferHandle hPerFrame { Render::DefaultCBufferPerFrame::Handle };
    const int perFrameSize { sizeof( Render::DefaultCBufferPerFrame ) };
    Render::UpdateConstantBuffer( hPerFrame, &perFrameData, perFrameSize, TAC_STACK_FRAME );

    RenderEditorWidgetsSelection( renderContext, viewHandle );
    TAC_CALL( RenderEditorWidgetsLights( renderContext, viewHandle, errors ) );
  }

  void CreationGameWindow::PlayGame( Errors& errors )
  {
    if( mSoul )
      return;

    auto ghost { TAC_NEW Ghost };
    TAC_CALL( ghost->Init( errors ) );
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

    const int mouseDeltaScroll { keyboardApi.GetMouseWheelDelta() };
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
        ( float )mouseDeltaScroll *
        unitsPerTick;
    }

    CameraWASDControls( gCreation.mEditorCamera );
  }


  void                          CreationGameWindow::Render( Errors& errors )
  {
    SysWindowApi windowApi{};
    if( !windowApi.IsShown( mWindowHandle ) )
      return;

    const v2i size{ windowApi.GetSize( mWindowHandle ) };

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
      .mColors{ rtColor },
      .mDepth{ rtDepth },
    };

    renderContext->SetViewport( size );
    renderContext->SetScissor( size );
    renderContext->SetRenderTargets( renderTargets );

    GamePresentationRender( gCreation.mWorld,
                            gCreation.mEditorCamera,
                            size,
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

    mDebug3DDrawData->DebugDraw3DToTexture( viewHandle,
                                            gCreation.mEditorCamera,
                                            desktopWindowState->mWidth,
                                            desktopWindowState->mHeight,
                                            errors );
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


    MousePickingInit();
    CameraUpdateSaved();
    CameraUpdateControls();
    ComputeArrowLen();
    MousePickingAll();

    UpdateGizmo();

    if( drawGrid )
      mDebug3DDrawData->DebugDraw3DGrid();

    RenderEditorWidgetsPicking( viewHandle );

    TAC_CALL( ImGuiOverlay( errors ) );
  }

  void CreationGameWindow::UpdateGizmo()
  {
    if( !gCreation.mSelectedGizmo )
      return;

    const v3 origin { gCreation.mSelectedEntities.GetGizmoOrigin() };
    float gizmoMouseDist;
    float secondDist;
    ClosestPointTwoRays( gCreation.mEditorCamera->mPos,
                         mWorldSpaceMouseDir,
                         origin,
                         gCreation.mTranslationGizmoDir,
                         &gizmoMouseDist,
                         &secondDist );
    const v3 translate { gCreation.mTranslationGizmoDir * ( secondDist - gCreation.mTranslationGizmoOffset ) };
    for( Entity* entity : gCreation.mSelectedEntities )
    {
      entity->mRelativeSpace.mPosition += translate;
    }
    //for( Prefab* prefab : gCreation.mSelectedPrefabs )
    //{
    //  prefab->mPosition += translate;
    //}
    if( !Mouse::ButtonIsDown( Mouse::Button::MouseLeft ) )
    {
      gCreation.mSelectedGizmo = false;
    }

  }

  void CreationGameWindow::SetStatusMessage( const StringView& msg,
                                             const TimestampDifference& duration )
  {
    const Timestamp curTime { Timestep::GetElapsedTime() };
    mStatusMessage = msg;
    mStatusMessageEndTime = curTime + duration;
  }
} // namespace Tac
