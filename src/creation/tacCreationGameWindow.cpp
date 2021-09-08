#include "src/common/assetmanagers/tacMesh.h"
#include "src/common/profile/tacProfile.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacCamera.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/shell/tacShell.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/common/math/tacMath.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreationPrefab.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/model/tacmodel.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/space/presentation/tacVoxelGIPresentation.h"
#include "src/space/tacEntity.h"
#include "src/space/tacGhost.h"
#include "src/space/tacWorld.h"

#include <cmath>

namespace Tac
{
  static bool drawGrid = false;
  static bool drawGizmos = true;
  static float sWASDCameraPanSpeed = 10;
  static float sWASDCameraOrbitSpeed = 0.1f;
  static bool sWASDCameraOrbitSnap;

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
    const v3 c = B - A;
    const float ab = Dot( a, b );
    const float bc = Dot( b, c );
    const float ac = Dot( a, c );
    const float aa = Dot( a, a );
    const float bb = Dot( b, b );
    const float denom = aa * bb - ab * ab;
    if( d )
    {
      *d = ( -ab * bc + ac * bb ) / denom;
    }
    if( e )
    {
      *e = ( ab * ac - bc * aa ) / denom;
    }
  }

  static void AddDrawCall( const Mesh* mesh, Render::ViewHandle viewHandle )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      Render::SetShader( CreationGameWindow::Instance->m3DShader );
      Render::SetVertexBuffer( subMesh.mVertexBuffer, 0, subMesh.mVertexCount );
      Render::SetIndexBuffer( subMesh.mIndexBuffer, 0, subMesh.mIndexCount );
      Render::SetBlendState( CreationGameWindow::Instance->mBlendState );
      Render::SetDepthState( CreationGameWindow::Instance->mDepthState );
      Render::SetRasterizerState( CreationGameWindow::Instance->mRasterizerState );
      Render::SetVertexFormat( CreationGameWindow::Instance->m3DVertexFormat );
      Render::SetSamplerState( CreationGameWindow::Instance->mSamplerState );
      Render::Submit( viewHandle, TAC_STACK_FRAME );
    }
  }

  CreationGameWindow* CreationGameWindow::Instance = nullptr;

  CreationGameWindow::CreationGameWindow()
  {
    Instance = this;
  }

  CreationGameWindow::~CreationGameWindow()
  {
    Instance = nullptr;
    Render::DestroyShader( m3DShader, TAC_STACK_FRAME );
    Render::DestroyVertexFormat( m3DVertexFormat, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
    DesktopAppDestroyWindow( mDesktopWindowHandle );
    delete mDebug3DDrawData;
  }

  void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
  {


    m3DShader = Render::CreateShader( Render::ShaderSource::FromPath( "3DTest" ), TAC_STACK_FRAME );

    const Render::VertexDeclaration posDecl = []()
    {
      Render::VertexDeclaration decl = {};
      decl.mAlignedByteOffset = TAC_OFFSET_OF( GameWindowVertex, pos );
      decl.mAttribute = Render::Attribute::Position;
      decl.mTextureFormat.mElementCount = 3;
      decl.mTextureFormat.mPerElementByteCount = sizeof( float );
      decl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
      return decl;
    }( );
    const Render::VertexDeclaration norDecl = []()
    {
      Render::VertexDeclaration decl = {};
      decl.mAlignedByteOffset = TAC_OFFSET_OF( GameWindowVertex, nor );
      decl.mAttribute = Render::Attribute::Normal;
      decl.mTextureFormat.mElementCount = 3;
      decl.mTextureFormat.mPerElementByteCount = sizeof( float );
      decl.mTextureFormat.mPerElementDataType = Render::GraphicsType::real;
      return decl;
    }( );
    m3DvertexFormatDecls = Render::VertexDeclarations{ posDecl, norDecl };
    m3DVertexFormat = Render::CreateVertexFormat( m3DvertexFormatDecls,
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( m3DVertexFormat, "game-window-vtx-fmt" );
    TAC_HANDLE_ERROR( errors );

    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = Render::BlendConstants::One;
    blendStateData.mDstRGB = Render::BlendConstants::Zero;
    blendStateData.mBlendRGB = Render::BlendMode::Add;
    blendStateData.mSrcA = Render::BlendConstants::Zero;
    blendStateData.mDstA = Render::BlendConstants::One;
    blendStateData.mBlendA = Render::BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mBlendState, "game-window-blend" );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = Render::DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData,
                                            TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mDepthState, "game-window-depth" );
    TAC_HANDLE_ERROR( errors );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = Render::CullMode::None; // todo
    rasterizerStateData.mFillMode = Render::FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mRasterizerState, "game-window-rast" );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Render::Filter::Linear;
    mSamplerState = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    Render::SetRenderObjectDebugName( mSamplerState, "game-window-samp" );
    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::Init( Errors& errors )
  {
    mDesktopWindowHandle = gCreation.CreateWindow( gGameWindowName );

    CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );



    mCenteredUnitCube = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/box.gltf",
                                                                0,
                                                                m3DvertexFormatDecls,
                                                                errors );
    TAC_HANDLE_ERROR( errors );

    mArrow = ModelAssetManagerGetMeshTryingNewThing( "assets/editor/arrow.gltf",
                                                     0,
                                                     m3DvertexFormatDecls,
                                                     errors );
    TAC_HANDLE_ERROR( errors );

    mDebug3DDrawData = TAC_NEW Debug3DDrawData;

    PlayGame( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::MousePickingAll()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    if( !IsWindowHovered( mDesktopWindowHandle ) )
      return;

    enum class PickedObject
    {
      None = 0,
      Entity,
      WidgetTranslationArrow,
      WidgetScaleCube,
    };

    struct
    {
      PickedObject pickedObject;
      float closestDist;
      union
      {
        Entity* closest;
        int arrowAxis;
      };
      bool IsNewClosest( float dist )
      {
        bool result = pickedObject == PickedObject::None || dist < closestDist;
        return result;
      }
    } pickData = {};

    bool hit;
    float dist;

    for( Entity* entity : gCreation.mWorld->mEntities )
    {
      MousePickingEntity( entity, &hit, &dist );
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;
      pickData.closestDist = dist;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }

    if( gCreation.mSelectedEntities.size() && drawGizmos )
    {
      const v3 selectionGizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();

      const m4 invArrowRots[] = {
        m4::RotRadZ( 3.14f / 2.0f ),
        m4::Identity(),
        m4::RotRadX( -3.14f / 2.0f ), };

      for( int i = 0; i < 3; ++i )
      {
        // 1/3: inverse transform
        v3 modelSpaceRayPos3 = gCreation.mEditorCamera->mPos - selectionGizmoOrigin;
        v4 modelSpaceRayPos4 = v4( modelSpaceRayPos3, 1 );
        v3 modelSpaceRayDir3 = mWorldSpaceMouseDir;
        v4 modelSpaceRayDir4 = v4( mWorldSpaceMouseDir, 0 );

        // 2/3: inverse rotate
        const m4& invArrowRot = invArrowRots[ i ];
        modelSpaceRayPos4 = invArrowRot * modelSpaceRayPos4;
        modelSpaceRayPos3 = modelSpaceRayPos4.xyz();
        modelSpaceRayDir4 = invArrowRot * modelSpaceRayDir4;
        modelSpaceRayDir3 = modelSpaceRayDir4.xyz();

        // 3/3: inverse scale
        modelSpaceRayPos3 /= mArrowLen;

        mArrow->MeshModelSpaceRaycast( modelSpaceRayPos3, modelSpaceRayDir3, &hit, &dist );
        dist *= mArrowLen;
        if( !hit || !pickData.IsNewClosest( dist ) )
          continue;
        pickData.arrowAxis = i;
        pickData.closestDist = dist;
        pickData.pickedObject = PickedObject::WidgetTranslationArrow;
      }
    }

    v3 worldSpaceHitPoint = {};
    if( pickData.pickedObject != PickedObject::None )
    {
      worldSpaceHitPoint = gCreation.mEditorCamera->mPos + pickData.closestDist * mWorldSpaceMouseDir;
      mDebug3DDrawData->DebugDraw3DSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );

      static double mouseMovement;
      TryConsumeMouseMovement( &mouseMovement, TAC_STACK_FRAME );
    }

    if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
    {
      switch( pickData.pickedObject )
      {
        case PickedObject::WidgetTranslationArrow:
        {
          v3 gizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();
          v3 pickPoint = gCreation.mEditorCamera->mPos + mWorldSpaceMouseDir * pickData.closestDist;
          v3 arrowDir = {};
          arrowDir[ pickData.arrowAxis ] = 1;
          gCreation.mSelectedGizmo = true;
          gCreation.mTranslationGizmoDir = arrowDir;
          gCreation.mTranslationGizmoOffset = Dot( arrowDir,
                                                   worldSpaceHitPoint - gizmoOrigin );
        } break;
        case PickedObject::Entity:
        {
          v3 entityWorldOrigin = ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
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
  }

  void CreationGameWindow::MousePickingInit()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const float x = ( float )desktopWindowState->mX;
    const float y = ( float )desktopWindowState->mY;
    const v2 screenspaceCursorPos = gKeyboardInput.mCurr.mScreenspaceCursorPos;
    float xNDC = ( ( screenspaceCursorPos.x - x ) / w );
    float yNDC = ( ( screenspaceCursorPos.y - y ) / h );
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    const float aspect = w / h;
    const float theta = gCreation.mEditorCamera->mFovyrad / 2.0f;
    const float cotTheta = 1.0f / std::tan( theta );
    const float sX = cotTheta / aspect;
    const float sY = cotTheta;

    const m4 viewInv = m4::ViewInv( gCreation.mEditorCamera->mPos,
                                    gCreation.mEditorCamera->mForwards,
                                    gCreation.mEditorCamera->mRight,
                                    gCreation.mEditorCamera->mUp );
    const v3 viewSpaceMousePosNearPlane =
    {
      xNDC / sX,
      yNDC / sY,
      -1,
    };

    const v3 viewSpaceMouseDir = Normalize( viewSpaceMousePosNearPlane );
    const v4 viewSpaceMouseDir4 = v4( viewSpaceMouseDir, 0 );
    const v4 worldSpaceMouseDir4 = viewInv * viewSpaceMouseDir4;
    mWorldSpaceMouseDir = worldSpaceMouseDir4.xyz();
  }

  void CreationGameWindow::MousePickingEntity( const Entity* entity,
                                               bool* hit,
                                               float* dist )
  {
    const Model* model = Model::GetModel( entity );
    if( !model )
    {
      *hit = false;
      return;
    }

    const Mesh* mesh = GamePresentationGetModelMesh( model );
    if( !mesh )
    {
      *hit = false;
      return;
    }

    bool transformInvExists;
    m4 transformInv = m4::Inverse( entity->mWorldTransform, &transformInvExists );
    if( !transformInvExists )
    {
      *hit = false;
      return;
    }

    const Camera* camera = gCreation.mEditorCamera;

    v3 modelSpaceMouseRayPos3 = ( transformInv * v4( camera->mPos, 1 ) ).xyz();
    v3 modelSpaceMouseRayDir3 = Normalize( ( transformInv * v4( mWorldSpaceMouseDir, 0 ) ).xyz() );
    float modelSpaceDist;
    mesh->MeshModelSpaceRaycast( modelSpaceMouseRayPos3, modelSpaceMouseRayDir3, hit, &modelSpaceDist );

    // Recompute the distance by transforming the model space hit point into world space in order to
    // account for non-uniform scaling
    if( *hit )
    {
      v3 modelSpaceHitPoint = modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * modelSpaceDist;
      v3 worldSpaceHitPoint = ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz();
      *dist = Distance( camera->mPos, worldSpaceHitPoint );
    }
  }

  void CreationGameWindow::ComputeArrowLen()
  {
    if( gCreation.mSelectedEntities.empty() )
      return;
    m4 view = m4::View( gCreation.mEditorCamera->mPos,
                        gCreation.mEditorCamera->mForwards,
                        gCreation.mEditorCamera->mRight,
                        gCreation.mEditorCamera->mUp );
    v3 pos = gCreation.mSelectedEntities.GetGizmoOrigin();
    v4 posVS4 = view * v4( pos, 1 );
    float clip_height = std::abs( std::tan( gCreation.mEditorCamera->mFovyrad / 2.0f ) * posVS4.z * 2.0f );
    float arrowLen = clip_height * 0.2f;
    mArrowLen = arrowLen;
  }

  void CreationGameWindow::RenderGameWorldToGameWindow( Render::ViewHandle viewHandle )
  {
    MousePickingAll();
    const Camera* camera = gCreation.mEditorCamera;
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;

    const m4 view = camera->View();
    float w = ( float )desktopWindowState->mWidth;
    float h = ( float )desktopWindowState->mHeight;
    float a;
    float b;
    Render::GetPerspectiveProjectionAB( camera->mFarPlane,
                                        camera->mNearPlane,
                                        a,
                                        b );
    const float aspect = w / h;
    const m4 proj = m4::ProjPerspective( a, b, camera->mFovyrad, aspect );
    DefaultCBufferPerFrame perFrameData;
    perFrameData.mFar = camera->mFarPlane;
    perFrameData.mNear = camera->mNearPlane;
    perFrameData.mView = view;
    perFrameData.mProjection = proj;
    perFrameData.mGbufferSize = { w, h };

    Render::UpdateConstantBuffer( DefaultCBufferPerFrame::Handle,
                                  &perFrameData,
                                  sizeof( DefaultCBufferPerFrame ),
                                  TAC_STACK_FRAME );

    if( !gCreation.mSelectedEntities.empty() && drawGizmos )
    {
      const v3 selectionGizmoOrigin = gCreation.mSelectedEntities.GetGizmoOrigin();
      const v3 red = { 1, 0, 0 };
      const v3 grn = { 0, 1, 0 };
      const v3 blu = { 0, 0, 1 };
      const v3 colors[] = { red, grn, blu };
      const m4 rots[] = {
        m4::RotRadZ( -3.14f / 2.0f ),
        m4::Identity(),
        m4::RotRadX( 3.14f / 2.0f ), };

      for( int i = 0; i < 3; ++i )
      {
        // Widget Translation Arrow
        DefaultCBufferPerObject perObjectData;
        perObjectData.Color = { colors[ i ], 1 };
        perObjectData.World
          = m4::Translate( selectionGizmoOrigin )
          * rots[ i ]
          * m4::Scale( v3( 1, 1, 1 ) * mArrowLen )
          ;// *mArrow->mTransform;
        Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        AddDrawCall( mArrow, viewHandle );

        // Widget Scale Cube
        //const v3 axis = { 0 == i, 1 == i, 2 == i };
        v3 axis = {};
        axis[ i ] = 1;
        perObjectData.World =
          m4::Translate( selectionGizmoOrigin ) *
          m4::Translate( axis * ( mArrowLen * 1.1f ) ) *
          rots[ i ] *
          m4::Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f );
        Render::UpdateConstantBuffer( DefaultCBufferPerObject::Handle,
                                      &perObjectData,
                                      sizeof( DefaultCBufferPerObject ),
                                      TAC_STACK_FRAME );
        AddDrawCall( mCenteredUnitCube, viewHandle );
      }
    }
  }

  void CreationGameWindow::PlayGame( Errors& errors )
  {
    if( mSoul )
      return;
    auto ghost = TAC_NEW Ghost;
    //ghost->mRenderView = mDesktopWindow->mRenderView;
    ghost->Init( errors );
    TAC_HANDLE_ERROR( errors );
    mSoul = ghost;
  }

  void CreationGameWindow::DrawPlaybackOverlay( Errors& errors )
  {
    ImGuiSetNextWindowSize( { 300, 405 } );
    ImGuiSetNextWindowHandle( mDesktopWindowHandle );
    ImGuiBegin( "gameplay overlay" );

    static bool mHideUI = false;
    if( !mHideUI )
    {
      ImGuiCheckbox( "Draw grid", &drawGrid );
      ImGuiCheckbox( "hide ui", &mHideUI );
      ImGuiCheckbox( "draw gizmos", &drawGizmos );

      if( mSoul )
      {
        if( ImGuiButton( "End simulation" ) )
        {
          delete mSoul;
          mSoul = nullptr;
        }
      }
      else
      {
        if( ImGuiButton( "Begin simulation" ) )
        {
          PlayGame( errors );
          TAC_HANDLE_ERROR( errors );
        }
      }

      if( ImGuiCollapsingHeader( "Camera" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        Camera* cam = gCreation.mEditorCamera;
        if( ImGuiCollapsingHeader( "transform" ) )
        {
          TAC_IMGUI_INDENT_BLOCK;
          if( ImGuiCollapsingHeader( "pos" ) )
            ImGuiDragFloat3( "cam pos", cam->mPos.data() );
          if( ImGuiCollapsingHeader( "forward" ) )
            ImGuiDragFloat3( "cam forward", cam->mForwards.data() );
          if( ImGuiCollapsingHeader( "right" ) )
            ImGuiDragFloat3( "cam right", cam->mRight.data() );
          if( ImGuiCollapsingHeader( "up" ) )
            ImGuiDragFloat3( "cam up", cam->mUp.data() );
        }
        if( ImGuiCollapsingHeader( "clipping planes" ) )
        {
          TAC_IMGUI_INDENT_BLOCK;
          ImGuiDragFloat( "cam far", &cam->mFarPlane );
          ImGuiDragFloat( "cam near", &cam->mNearPlane );
        }
        ImGuiDragFloat( "cam fovyrad", &cam->mFovyrad );
        if( ImGuiButton( "cam snap pos" ) )
        {
          cam->mPos.x = ( float )( int )cam->mPos.x;
          cam->mPos.y = ( float )( int )cam->mPos.y;
          cam->mPos.z = ( float )( int )cam->mPos.z;
        }
        if( ImGuiButton( "cam snap dir" ) )
        {
          v3 unitDirs[] =
          {
            {1,0,0},
            {-1,0,0},
            {0,1,0},
            {0,-1,0},
            {0,0,1},
            {0,0,-1},
          };
          for( v3* camDir : { &cam->mUp,  &cam->mRight, &cam->mForwards } )
          {
            float biggestDot = 0;
            v3 biggestUnitDir = {};
            for( v3 unitDir : unitDirs )
            {
              float d = Dot( *camDir, unitDir );
              if( d > biggestDot )
              {
                biggestDot = d;
                biggestUnitDir = unitDir;
              }
            }
            *camDir = biggestUnitDir;
          }
        }
      }

      if( ShellGetElapsedSeconds() < mStatusMessageEndTime )
      {
        ImGuiText( mStatusMessage );
      }

    }
    ImGuiEnd();
  }

  static v3 SphericalToCartesian( const float r, const float t, const float p ) // radius, theta, phi
  {
    const float x = r * std::cos( p ) * std::sin( t );
    const float y = r * std::cos( t );
    const float z = r * std::sin( p ) * std::sin( t );
    return v3( x, y, z );
  }

  static v3 SphericalToCartesian( const v3 v ) { return SphericalToCartesian( v.x, v.y, v.z ); }

  static v3 CartesianToSpherical( const float x, const float y, const float z )
  {
    const float q = x * x + y * y + z * z;
    if( q < 0.01f )
      return {};
    const float r = std::sqrt( q );
    const float t = std::acos( y / r );
    const float p = std::atan2( z, x );
    return v3( r, t, p );
  }

  static v3 CartesianToSpherical( const v3 v ) { return CartesianToSpherical( v.x, v.y, v.z ); }

  static void CameraWASDControlsPan( Camera* camera )
  {
    v3 combinedDir = {};
    struct
    {
      Key key;
      v3 dir;
    } keyDirs[] =
    {
      { Key::W, camera->mForwards},
      { Key::A, -camera->mRight},
      { Key::S, -camera->mForwards},
      { Key::D, camera->mRight},
      { Key::Q, -camera->mUp },
      { Key::E, camera->mUp},
    };
    for( const auto keyDir : keyDirs )
      if( gKeyboardInput.IsKeyDown( keyDir.key ) )
        combinedDir += keyDir.dir;
    if( combinedDir == v3( 0, 0, 0 ) )
      return;
    camera->mPos += combinedDir * sWASDCameraPanSpeed;
  }


  static void CameraWASDControlsOrbit( Camera* camera, const v3 orbitCenter )
  {
    const float vertLimit = 0.1f;

    struct
    {
      Key key;
      v3 spherical;
    } keyDirs[] = { { Key::W, v3( 0, -1, 0 ) },
                    { Key::A, v3( 0,  0, 1 ) },
                    { Key::S, v3( 0, 1, 0 ) },
                    { Key::D, v3( 0, 0, -1 ) } };


    v3 camOrbitSphericalOffset = {};
    for( auto keyDir : keyDirs )
      if( gKeyboardInput.IsKeyDown( keyDir.key ) )
        camOrbitSphericalOffset += keyDir.spherical;
    if( camOrbitSphericalOffset == v3( 0, 0, 0 ) )
      return;

    v3 camOrbitSpherical = CartesianToSpherical( camera->mPos - orbitCenter );
    camOrbitSpherical += camOrbitSphericalOffset * sWASDCameraOrbitSpeed;
    camOrbitSpherical.y = Clamp( camOrbitSpherical.y, vertLimit, 3.14f - vertLimit );

    camera->mPos = orbitCenter + SphericalToCartesian( camOrbitSpherical );

    if( sWASDCameraOrbitSnap )
    {
      camera->SetForwards( orbitCenter - camera->mPos );
    }
    else
    {
      v3 dirCart = camera->mForwards;
      v3 dirSphe = CartesianToSpherical( dirCart );
      dirSphe.y += -camOrbitSphericalOffset.y * sWASDCameraOrbitSpeed;
      dirSphe.z += camOrbitSphericalOffset.z * sWASDCameraOrbitSpeed;
      dirSphe.y = Clamp( dirSphe.y, vertLimit, 3.14f - vertLimit );
      v3 newForwards = SphericalToCartesian( dirSphe );
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
    static String savedPrefabPath;
    static Camera savedCamera;

    const StringView loadedPrefab = PrefabGetLoaded();
    if( loadedPrefab != savedPrefabPath )
    {
      savedPrefabPath = loadedPrefab;
      savedCamera = *gCreation.mEditorCamera;
    }

    const bool cameraSame =
      savedCamera.mPos == gCreation.mEditorCamera->mPos  &&
      savedCamera.mForwards == gCreation.mEditorCamera->mForwards &&
      savedCamera.mRight == gCreation.mEditorCamera->mRight &&
      savedCamera.mUp == gCreation.mEditorCamera->mUp;
    if( cameraSame )
      return;

    savedCamera = *gCreation.mEditorCamera;
    PrefabSaveCamera( gCreation.mEditorCamera );
  }

  void CreationGameWindow::CameraControls()
  {
    DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;
    if( !IsWindowHovered( mDesktopWindowHandle ) )
      return;
    const Camera oldCamera = *gCreation.mEditorCamera;

    if( gKeyboardInput.IsKeyDown( Key::MouseRight ) &&
        gKeyboardInput.mMouseDeltaPos != v2( 0, 0 ) )
    {
      const float pixelsPerDeg = 400.0f / 90.0f;
      const float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
      const v2 angleRadians = gKeyboardInput.mMouseDeltaPos * radiansPerPixel;

      if( angleRadians.x != 0 )
      {
        m3 matrix = m3::RotRadAngleAxis( -angleRadians.x, gCreation.mEditorCamera->mUp );
        gCreation.mEditorCamera->mForwards = matrix * gCreation.mEditorCamera->mForwards;
        gCreation.mEditorCamera->mRight = Cross( gCreation.mEditorCamera->mForwards,
                                                 gCreation.mEditorCamera->mUp );
      }

      if( angleRadians.y != 0 )
      {
        m3 matrix = m3::RotRadAngleAxis( -angleRadians.y, gCreation.mEditorCamera->mRight );
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

    if( gKeyboardInput.IsKeyDown( Key::MouseMiddle ) &&
        gKeyboardInput.mMouseDeltaPos != v2( 0, 0 ) )
    {
      const float unitsPerPixel = 5.0f / 100.0f;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mRight *
        -gKeyboardInput.mMouseDeltaPos.x *
        unitsPerPixel;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mUp *
        gKeyboardInput.mMouseDeltaPos.y *
        unitsPerPixel;
    }

    if( gKeyboardInput.mMouseDeltaScroll )
    {
      float unitsPerTick = 1.0f;

      if( gCreation.mSelectedEntities.size() )
      {
        const v3 origin = gCreation.mSelectedEntities.GetGizmoOrigin();
        unitsPerTick = Distance( origin, gCreation.mEditorCamera->mPos ) * 0.1f;
      }

      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mForwards *
        ( float )gKeyboardInput.mMouseDeltaScroll *
        unitsPerTick;
    }

    CameraWASDControls( gCreation.mEditorCamera );
  }

  void CreationGameWindow::Update( Errors& errors )
  {
    TAC_PROFILE_BLOCK;

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );

    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;


    DesktopAppMoveControls( mDesktopWindowHandle );

    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const Render::Viewport viewport( w, h );
    const Render::ScissorRect scissorRect( w, h );

    Render::SetViewFramebuffer( viewHandle, framebufferHandle );
    Render::SetViewport( viewHandle, viewport );
    Render::SetViewScissorRect( viewHandle, scissorRect );


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
      mSoul->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }

    if( drawGrid )
      mDebug3DDrawData->DebugDraw3DGrid();

    if( !gCreation.mSelectedEntities.empty() )
    {
      const v3 origin = gCreation.mSelectedEntities.GetGizmoOrigin();
      mDebug3DDrawData->DebugDraw3DCircle( origin,
                                           gCreation.mEditorCamera->mForwards,
                                           mArrowLen );
    }

    MousePickingInit();
    CameraUpdateSaved();
    CameraControls();
    ComputeArrowLen();
    RenderGameWorldToGameWindow( viewHandle );

    GamePresentationRender( gCreation.mWorld,
                            gCreation.mEditorCamera,
                            desktopWindowState->mWidth,
                            desktopWindowState->mHeight,
                            viewHandle );

    VoxelGIPresentationRender( gCreation.mWorld,
                               gCreation.mEditorCamera,
                               desktopWindowState->mWidth,
                               desktopWindowState->mHeight,
                               viewHandle );

    mDebug3DDrawData->DebugDraw3DToTexture( viewHandle,
                                            gCreation.mEditorCamera,
                                            desktopWindowState->mWidth,
                                            desktopWindowState->mHeight,
                                            errors );

    if( gCreation.mSelectedGizmo )
    {
      const v3 origin = gCreation.mSelectedEntities.GetGizmoOrigin();
      float gizmoMouseDist;
      float secondDist;
      ClosestPointTwoRays( gCreation.mEditorCamera->mPos,
                           mWorldSpaceMouseDir,
                           origin,
                           gCreation.mTranslationGizmoDir,
                           &gizmoMouseDist,
                           &secondDist );
      const v3 translate = gCreation.mTranslationGizmoDir *
        ( secondDist - gCreation.mTranslationGizmoOffset );
      for( Entity* entity : gCreation.mSelectedEntities )
      {
        entity->mRelativeSpace.mPosition += translate;
      }
      //for( Prefab* prefab : gCreation.mSelectedPrefabs )
      //{
      //  prefab->mPosition += translate;
      //}
      if( !gKeyboardInput.IsKeyDown( Key::MouseLeft ) )
      {
        gCreation.mSelectedGizmo = false;
      }
    }

    DrawPlaybackOverlay( errors );
    TAC_HANDLE_ERROR( errors );
  }
}
