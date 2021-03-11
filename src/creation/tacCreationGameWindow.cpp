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
#include "src/common/tacShell.h"
#include "src/common/tacShellTimer.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreationPrefab.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowGraphics.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/model/tacmodel.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacSkyboxPresentation.h"
#include "src/space/tacEntity.h"
#include "src/space/tacGhost.h"
#include "src/space/tacWorld.h"

namespace Tac
{


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
    Render::DestroyConstantBuffer( mPerFrame, TAC_STACK_FRAME );
    Render::DestroyConstantBuffer( mPerObj, TAC_STACK_FRAME );
    Render::DestroyDepthState( mDepthState, TAC_STACK_FRAME );
    Render::DestroyBlendState( mBlendState, TAC_STACK_FRAME );
    Render::DestroyRasterizerState( mRasterizerState, TAC_STACK_FRAME );
    Render::DestroySamplerState( mSamplerState, TAC_STACK_FRAME );
    DesktopAppDestroyWindow( mDesktopWindowHandle );
    delete mDebug3DDrawData;
  }

  void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
  {
    mPerFrame = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerObj = Render::CreateConstantBuffer( sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    m3DShader = Render::CreateShader( Render::ShaderSource::FromPath( "3DTest" ),
                                      Render::ConstantBuffers( mPerFrame, mPerObj ),
                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration posDecl;
    posDecl.mAlignedByteOffset = 0;
    posDecl.mAttribute = Attribute::Position;
    posDecl.mTextureFormat.mElementCount = 3;
    posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    posDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;

    m3DVertexFormat = Render::CreateVertexFormat( Render::VertexDeclarations( posDecl ),
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
    m3DvertexFormatDecls[ 0 ] = posDecl;

    Render::BlendState blendStateData;
    blendStateData.mSrcRGB = BlendConstants::One;
    blendStateData.mDstRGB = BlendConstants::Zero;
    blendStateData.mBlendRGB = BlendMode::Add;
    blendStateData.mSrcA = BlendConstants::Zero;
    blendStateData.mDstA = BlendConstants::One;
    blendStateData.mBlendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( blendStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = DepthFunc::Less;
    mDepthState = Render::CreateDepthState( depthStateData,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None; // todo
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( rasterizerStateData,
                                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Linear;
    mSamplerState = Render::CreateSamplerState( samplerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::Init( Errors& errors )
  {
    mDesktopWindowHandle = gCreation.CreateWindow( gGameWindowName );

    CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );

    mSkyboxPresentation = TAC_NEW SkyboxPresentation;
    mSkyboxPresentation->mCamera = gCreation.mEditorCamera;
    mSkyboxPresentation->Init( errors );
    TAC_HANDLE_ERROR( errors );

    mGamePresentation = TAC_NEW GamePresentation;
    mGamePresentation->mWorld = gCreation.mWorld;
    mGamePresentation->mCamera = gCreation.mEditorCamera;
    mGamePresentation->mSkyboxPresentation = mSkyboxPresentation;
    mGamePresentation->CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );


    ModelAssetManagerGetMesh( &mCenteredUnitCube,
                              "assets/editor/box.gltf",
                              m3DVertexFormat,
                              m3DvertexFormatDecls,
                              k3DvertexFormatDeclCount,
                              errors );
    TAC_HANDLE_ERROR( errors );

    ModelAssetManagerGetMesh( &mArrow,
                              "assets/editor/arrow.gltf",
                              m3DVertexFormat,
                              m3DvertexFormatDecls,
                              k3DvertexFormatDeclCount,
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

    if( gCreation.mSelectedEntities.size() ) // || gCreation.mSelectedPrefabs.size() )
    {
      v3 selectionGizmoOrigin = gCreation.GetSelectionGizmoOrigin();

      m4 invArrowRots[] = {
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

        mArrow->Raycast( modelSpaceRayPos3, modelSpaceRayDir3, &hit, &dist );
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
      mDebug3DDrawData->DebugDrawSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }

    if( gKeyboardInput.IsKeyJustDown( Key::MouseLeft ) )
    {
      switch( pickData.pickedObject )
      {
        case PickedObject::WidgetTranslationArrow:
        {
          v3 gizmoOrigin = gCreation.GetSelectionGizmoOrigin();
          v3 pickPoint = gCreation.mEditorCamera->mPos + mWorldSpaceMouseDir * pickData.closestDist;
          v3 arrowDir = {};
          arrowDir[ pickData.arrowAxis ] = 1;
          gCreation.mSelectedGizmo = true;
          gCreation.mTranslationGizmoDir = arrowDir;
          gCreation.mTranslationGizmoOffset = Dot(
            arrowDir,
            worldSpaceHitPoint - gizmoOrigin );
        } break;
        case PickedObject::Entity:
        {
          v3 entityWorldOrigin = ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
          gCreation.ClearSelection();
          gCreation.mSelectedEntities = { pickData.closest };
          gCreation.mSelectedHitOffsetExists = true;
          gCreation.mSelectedHitOffset = worldSpaceHitPoint - entityWorldOrigin;
        } break;
        case PickedObject::None:
        {
          gCreation.ClearSelection();
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
    if( !model || !model->mesh )
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
    model->mesh->Raycast( modelSpaceMouseRayPos3, modelSpaceMouseRayDir3, hit, &modelSpaceDist );

    // Recompute the distance by transforming the model space hit point into world space in order to
    // account for non-uniform scaling
    if( *hit )
    {
      v3 modelSpaceHitPoint = modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * modelSpaceDist;
      v3 worldSpaceHitPoint = ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz();
      *dist = Distance( camera->mPos, worldSpaceHitPoint );
    }
  }

  void CreationGameWindow::AddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& )
  {
    //for( const SubMesh& subMesh : mesh->mSubMeshes )
    {
      //DrawCall2 drawCall = {};
      //drawCall.mVertexBuffer = subMesh.mVertexBuffer;
      //drawCall.mIndexBuffer = subMesh.mIndexBuffer;
      //drawCall.mStartIndex = 0;
      //drawCall.mIndexCount = subMesh.mIndexCount;
      //drawCall.mBlendState = mBlendState;
      //drawCall.mRasterizerState = mRasterizerState;
      //drawCall.mSamplerState = mSamplerState;
      //drawCall.mDepthState = mDepthState;
      //drawCall.mVertexFormat = mesh->mVertexFormat;
      //drawCall.mUniformDst = mPerObj;
      //drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
      //drawCall.mFrame = TAC_STACK_FRAME;
      //Render::AddDrawCall( drawCall );
    }
  }

  void CreationGameWindow::ComputeArrowLen()
  {
    if( !gCreation.IsAnythingSelected() )
    {
      return;
    }
    m4 view = m4::View( gCreation.mEditorCamera->mPos,
                        gCreation.mEditorCamera->mForwards,
                        gCreation.mEditorCamera->mRight,
                        gCreation.mEditorCamera->mUp );
    v3 pos = gCreation.GetSelectionGizmoOrigin();
    v4 posVS4 = view * v4( pos, 1 );
    float clip_height = std::abs( std::tan( gCreation.mEditorCamera->mFovyrad / 2.0f ) * posVS4.z * 2.0f );
    float arrowLen = clip_height * 0.2f;
    mArrowLen = arrowLen;
  }

  void CreationGameWindow::RenderGameWorldToGameWindow()
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

    //DrawCall2 setPerFrame = {};
    //setPerFrame.mUniformDst = mPerFrame;
    //setPerFrame.CopyUniformSource( perFrameData );
    //Render::AddDrawCall( setPerFrame );

    if( gCreation.IsAnythingSelected() )
    {
      v3 selectionGizmoOrigin = gCreation.GetSelectionGizmoOrigin();
      v3 red = { 1, 0, 0 };
      v3 grn = { 0, 1, 0 };
      v3 blu = { 0, 0, 1 };
      v3 colors[] = { red, grn, blu };
      m4 rots[] = {
        m4::RotRadZ( -3.14f / 2.0f ),
        m4::Identity(),
        m4::RotRadX( 3.14f / 2.0f ), };

      for( int i = 0; i < 3; ++i )
      {
        // Widget Translation Arrow
        DefaultCBufferPerObject perObjectData;
        perObjectData.Color = { colors[ i ], 1 };
        perObjectData.World =
          m4::Translate( selectionGizmoOrigin ) *
          rots[ i ] *
          m4::Scale( v3( 1, 1, 1 ) * mArrowLen ) *
          mArrow->mTransform;
        AddDrawCall( mArrow, perObjectData );

        // Widget Scale Cube
        v3 axis = {};
        axis[ i ] = 1;
        perObjectData.World =
          m4::Translate( selectionGizmoOrigin ) *
          m4::Translate( axis * ( mArrowLen * 1.1f ) ) *
          rots[ i ] *
          m4::Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f );
        AddDrawCall( mCenteredUnitCube, perObjectData );
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

      ImGuiDragFloat3( "cam pos", gCreation.mEditorCamera->mPos.data() );
      ImGuiDragFloat3( "cam forward", gCreation.mEditorCamera->mForwards.data() );
      ImGuiDragFloat3( "cam right", gCreation.mEditorCamera->mRight.data() );
      ImGuiDragFloat3( "cam up", gCreation.mEditorCamera->mUp.data() );
      ImGuiDragFloat3( "cam far", &gCreation.mEditorCamera->mFarPlane );
      ImGuiDragFloat( "cam near", &gCreation.mEditorCamera->mNearPlane );
      ImGuiDragFloat( "cam fovyrad", &gCreation.mEditorCamera->mFovyrad );
    }

    if( ShellGetElapsedSeconds() < mStatusMessageEndTime )
    {
      ImGuiText( mStatusMessage );
    }

    ImGuiEnd();
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
        gKeyboardInput.mMouseDeltaPosScreenspace != v2( 0, 0 ) )
    {
      const float pixelsPerDeg = 400.0f / 90.0f;
      const float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
      const v2 angleRadians = gKeyboardInput.mMouseDeltaPosScreenspace * radiansPerPixel;

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
        gKeyboardInput.mMouseDeltaPosScreenspace != v2( 0, 0 ) )
    {
      const float unitsPerPixel = 5.0f / 100.0f;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mRight *
        -gKeyboardInput.mMouseDeltaPosScreenspace.x *
        unitsPerPixel;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mUp *
        gKeyboardInput.mMouseDeltaPosScreenspace.y *
        unitsPerPixel;
    }

    if( gKeyboardInput.mMouseDeltaScroll )
    {
      //float unitsPerTick = 0.35f;
      gCreation.mEditorCamera->mPos +=
        gCreation.mEditorCamera->mForwards *
        ( float )gKeyboardInput.mMouseDeltaScroll;
    }

    if( oldCamera.mPos != gCreation.mEditorCamera->mPos ||
        oldCamera.mForwards != gCreation.mEditorCamera->mForwards ||
        oldCamera.mRight != gCreation.mEditorCamera->mRight ||
        oldCamera.mUp != gCreation.mEditorCamera->mUp )
      PrefabSaveCamera( gCreation.mEditorCamera );
  }

  void CreationGameWindow::Update( Errors& errors )
  {

    const Render::ViewHandle viewHandle = WindowGraphicsGetView( mDesktopWindowHandle );
    const Render::FramebufferHandle framebufferHandle = WindowGraphicsGetFramebuffer( mDesktopWindowHandle );

    const DesktopWindowState* desktopWindowState = GetDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState->mNativeWindowHandle )
      return;


    DesktopAppMoveControls( mDesktopWindowHandle );

    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const Viewport viewport( w, h );
    const ScissorRect scissorRect( w, h );

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
      //  model->mGLTFPath = "assets/editor/Box.gltf";
      //}
      mSoul->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }

    //mDebug3DDrawData->DebugDrawGrid();

    if( gCreation.IsAnythingSelected() )
    {
      v3 origin = gCreation.GetSelectionGizmoOrigin();
      mDebug3DDrawData->DebugDrawCircle( origin,
                                         gCreation.mEditorCamera->mForwards,
                                         mArrowLen );
    }

    MousePickingInit();
    CameraControls();
    ComputeArrowLen();
    RenderGameWorldToGameWindow();

    //mDebug3DDrawData->DrawToTexture( errors,
    //                                 &perFrameData,
    //                                 mDesktopWindow->mRenderView );

    mGamePresentation->RenderGameWorldToDesktopView( desktopWindowState->mWidth,
                                                     desktopWindowState->mHeight,
                                                     viewHandle );

    if( gCreation.mSelectedGizmo )
    {
      const v3 origin = gCreation.GetSelectionGizmoOrigin();
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
