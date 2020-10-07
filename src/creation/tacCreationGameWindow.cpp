#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/tacShell.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/shell/tacDesktopWindowManager.h"
#include "src/space/graphics/tacGraphics.h"
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
  static void ClosestPointTwoRays(
    v3 A,
    v3 a,
    v3 B,
    v3 b,
    float* d,
    float* e )
  {
    v3 c = B - A;
    float ab = Dot( a, b );
    float bc = Dot( b, c );
    float ac = Dot( a, c );
    float aa = Dot( a, a );
    float bb = Dot( b, b );
    float denom = aa * bb - ab * ab;

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
    delete mUI2DDrawData;
    delete mDebug3DDrawData;
  }
  void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
  {
    mPerFrame = Render::CreateConstantBuffer( "tac 3d per frame",
                                              sizeof( DefaultCBufferPerFrame ),
                                              0,
                                              TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    mPerObj = Render::CreateConstantBuffer( "tac 3d per obj",
                                            sizeof( DefaultCBufferPerObject ),
                                            1,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    m3DShader = Render::CreateShader( "game window 3d shader",
                                      Render::ShaderSource::FromPath( "3DTest" ),
                                      Render::ConstantBuffers( mPerFrame, mPerObj ),
                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    VertexDeclaration posDecl;
    posDecl.mAlignedByteOffset = 0;
    posDecl.mAttribute = Attribute::Position;
    posDecl.mTextureFormat.mElementCount = 3;
    posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
    posDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;

    m3DVertexFormat = Render::CreateVertexFormat( "game window renderer",
                                                  Render::VertexDeclarations( posDecl ),
                                                  m3DShader,
                                                  TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
    m3DvertexFormatDecls[ 0 ] = posDecl;

    Render::BlendState blendStateData;
    blendStateData.srcRGB = BlendConstants::One;
    blendStateData.dstRGB = BlendConstants::Zero;
    blendStateData.blendRGB = BlendMode::Add;
    blendStateData.srcA = BlendConstants::Zero;
    blendStateData.dstA = BlendConstants::One;
    blendStateData.blendA = BlendMode::Add;
    mBlendState = Render::CreateBlendState( "tac 3d opaque blend", blendStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::DepthState depthStateData;
    depthStateData.mDepthTest = true;
    depthStateData.mDepthWrite = true;
    depthStateData.mDepthFunc = DepthFunc::Less;
    mDepthState = Render::CreateDepthState( "tac 3d depth state",
                                            depthStateData,
                                            TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::RasterizerState rasterizerStateData;
    rasterizerStateData.mCullMode = CullMode::None; // todo
    rasterizerStateData.mFillMode = FillMode::Solid;
    rasterizerStateData.mFrontCounterClockwise = true;
    rasterizerStateData.mMultisample = false;
    rasterizerStateData.mScissor = true;
    mRasterizerState = Render::CreateRasterizerState( "tac 3d rast state",
                                                      rasterizerStateData,
                                                      TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );

    Render::SamplerState samplerStateData;
    samplerStateData.mFilter = Filter::Linear;
    mSamplerState = Render::CreateSamplerState( "tac 3d tex sampler", samplerStateData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }
  void CreationGameWindow::Init( Errors& errors )
  {
    mDesktopWindowHandle = Creation::Instance->CreateWindow( gGameWindowName );

    Creation* creation = Creation::Instance;
    auto uI2DDrawData = TAC_NEW UI2DDrawData;
    mUI2DDrawData = uI2DDrawData;
    mUIRoot = TAC_NEW UIRoot;
    mUIRoot->mElapsedSeconds = &Shell::Instance.mElapsedSeconds;
    mUIRoot->mUI2DDrawData = mUI2DDrawData;
    //mUIRoot->mDesktopWindow = mDesktopWindow;
    CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );

    mSkyboxPresentation = TAC_NEW SkyboxPresentation;
    mSkyboxPresentation->mCamera = &creation->mEditorCamera;
    //mSkyboxPresentation->mDesktopWindow = mDesktopWindow;
    mSkyboxPresentation->Init( errors );
    TAC_HANDLE_ERROR( errors );

    mGamePresentation = TAC_NEW GamePresentation;
    mGamePresentation->mWorld = creation->mWorld;
    mGamePresentation->mCamera = &creation->mEditorCamera;
    //mGamePresentation->mDesktopWindow = mDesktopWindow;
    mGamePresentation->mSkyboxPresentation = mSkyboxPresentation;
    mGamePresentation->CreateGraphicsObjects( errors );
    TAC_HANDLE_ERROR( errors );


    ModelAssetManager::Instance->GetMesh(
      &mCenteredUnitCube,
      "assets/editor/box.gltf",
      m3DVertexFormat,
      m3DvertexFormatDecls,
      k3DvertexFormatDeclCount,
      errors );
    TAC_HANDLE_ERROR( errors );

    ModelAssetManager::Instance->GetMesh(
      &mArrow,
      "assets/editor/arrow.gltf",
      m3DVertexFormat,
      m3DvertexFormatDecls,
      k3DvertexFormatDeclCount,
      errors );
    TAC_HANDLE_ERROR( errors );

    mDebug3DDrawData = TAC_NEW Debug3DDrawData;
    //mDebug3DDrawData->mCommonData = shell->Debug3DCommonData::Instance;
    //mDebug3DDrawData->mRenderView = mDesktopWindow->mRenderView;

    PlayGame( errors );
    TAC_HANDLE_ERROR( errors );
  }

  void CreationGameWindow::MousePickingAll()
  {
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    if( !desktopWindowState->mCursorUnobscured )
      return;
    Creation* creation = Creation::Instance;

    enum class PickedObject
    {
      None,
      Entity,
      WidgetTranslationArrow,
      WidgetScaleCube,
    };

    struct
    {
      PickedObject pickedObject = PickedObject::None;
      float closestDist = 0;
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
    } pickData;

    bool hit;
    float dist;

    for( Entity* entity : creation->mWorld->mEntities )
    {
      MousePickingEntity( entity, &hit, &dist );
      if( !hit || !pickData.IsNewClosest( dist ) )
        continue;
      pickData.closestDist = dist;
      pickData.closest = entity;
      pickData.pickedObject = PickedObject::Entity;
    }

    if( creation->mSelectedEntities.size() ) // || creation->mSelectedPrefabs.size() )
    {
      v3 selectionGizmoOrigin = creation->GetSelectionGizmoOrigin();

      m4 invArrowRots[] = {
        M4RotRadZ( 3.14f / 2.0f ),
        m4::Identity(),
        M4RotRadX( -3.14f / 2.0f ), };

      for( int i = 0; i < 3; ++i )
      {
        // 1/3: inverse transform
        v3 modelSpaceRayPos3 = creation->mEditorCamera.mPos - selectionGizmoOrigin;
        v4 modelSpaceRayPos4 = v4( modelSpaceRayPos3, 1 );
        v3 modelSpaceRayDir3 = worldSpaceMouseDir;
        v4 modelSpaceRayDir4 = v4( worldSpaceMouseDir, 0 );

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
      worldSpaceHitPoint = creation->mEditorCamera.mPos + pickData.closestDist * worldSpaceMouseDir;
      mDebug3DDrawData->DebugDrawSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
    }

    if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
    {
      switch( pickData.pickedObject )
      {
        case PickedObject::WidgetTranslationArrow:
        {
          v3 gizmoOrigin = creation->GetSelectionGizmoOrigin();
          v3 pickPoint = creation->mEditorCamera.mPos + worldSpaceMouseDir * pickData.closestDist;
          v3 arrowDir = {};
          arrowDir[ pickData.arrowAxis ] = 1;
          creation->mSelectedGizmo = true;
          creation->mTranslationGizmoDir = arrowDir;
          creation->mTranslationGizmoOffset = Dot(
            arrowDir,
            worldSpaceHitPoint - gizmoOrigin );
        } break;
        case PickedObject::Entity:
        {
          v3 entityWorldOrigin = ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
          creation->ClearSelection();
          creation->mSelectedEntities = { pickData.closest };
          creation->mSelectedHitOffsetExists = true;
          creation->mSelectedHitOffset = worldSpaceHitPoint - entityWorldOrigin;
        } break;
        case PickedObject::None:
        {
          creation->ClearSelection();
        } break;
      }
    }
  }
  void CreationGameWindow::MousePickingInit()
  {
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;

    Creation* creation = Creation::Instance;
    const float w = ( float )desktopWindowState->mWidth;
    const float h = ( float )desktopWindowState->mHeight;
    const float x = ( float )desktopWindowState->mX;
    const float y = ( float )desktopWindowState->mY;
    if( KeyboardInput::Instance->mCurr.mScreenspaceCursorPosErrors )
      return;
    const v2 screenspaceCursorPos = KeyboardInput::Instance->mCurr.mScreenspaceCursorPos;
    float xNDC = ( ( screenspaceCursorPos.x - x ) / w );
    float yNDC = ( ( screenspaceCursorPos.y - y ) / h );
    yNDC = 1 - yNDC;
    xNDC = xNDC * 2 - 1;
    yNDC = yNDC * 2 - 1;
    const float aspect = w / h;
    const float theta = creation->mEditorCamera.mFovyrad / 2.0f;
    const float cotTheta = 1.0f / std::tan( theta );
    const float sX = cotTheta / aspect;
    const float sY = cotTheta;

    const m4 viewInv = M4ViewInv(
      creation->mEditorCamera.mPos,
      creation->mEditorCamera.mForwards,
      creation->mEditorCamera.mRight,
      creation->mEditorCamera.mUp );
    const v3 viewSpaceMousePosNearPlane =
    {
      xNDC / sX,
      yNDC / sY,
      -1,
    };

    const v3 viewSpaceMouseDir = Normalize( viewSpaceMousePosNearPlane );
    const v4 viewSpaceMouseDir4 = v4( viewSpaceMouseDir, 0 );
    const v4 worldSpaceMouseDir4 = viewInv * viewSpaceMouseDir4;
    worldSpaceMouseDir = worldSpaceMouseDir4.xyz();
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

    m4 transformInv;
    bool transformInvExists;
    M4Inverse( entity->mWorldTransform, &transformInv, &transformInvExists );
    if( !transformInvExists )
    {
      *hit = false;
      return;
    }

    Camera* camera = &Creation::Instance->mEditorCamera;

    v3 modelSpaceMouseRayPos3 = ( transformInv * v4( camera->mPos, 1 ) ).xyz();
    v3 modelSpaceMouseRayDir3 = Normalize( ( transformInv * v4( worldSpaceMouseDir, 0 ) ).xyz() );
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
  void CreationGameWindow::AddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& cbuf )
  {
    for( const SubMesh& subMesh : mesh->mSubMeshes )
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
    Creation* creation = Creation::Instance;
    if( !creation->IsAnythingSelected() )
    {
      return;
    }
    m4 view = M4View(
      creation->mEditorCamera.mPos,
      creation->mEditorCamera.mForwards,
      creation->mEditorCamera.mRight,
      creation->mEditorCamera.mUp );
    v3 pos = creation->GetSelectionGizmoOrigin();
    v4 posVS4 = view * v4( pos, 1 );
    float clip_height = std::abs( std::tan( creation->mEditorCamera.mFovyrad / 2.0f ) * posVS4.z * 2.0f );
    float arrowLen = clip_height * 0.2f;
    mArrowLen = arrowLen;
  }
  void CreationGameWindow::RenderGameWorldToGameWindow()
  {
    MousePickingAll();
    Camera* camera = &Creation::Instance->mEditorCamera;
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
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
    const m4 proj = M4ProjPerspective( a, b, camera->mFovyrad, aspect );
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

    if( Creation::Instance->IsAnythingSelected() )
    {
      v3 selectionGizmoOrigin = Creation::Instance->GetSelectionGizmoOrigin();
      v3 colors[] = {
        { 1, 0, 0 },
      { 0, 1, 0 },
      { 0, 0, 1 }, };
      m4 rots[] = {
        M4RotRadZ( -3.14f / 2.0f ),
        m4::Identity(),
        M4RotRadX( 3.14f / 2.0f ), };

      for( int i = 0; i < 3; ++i )
      {
        // Widget Translation Arrow
        DefaultCBufferPerObject perObjectData;
        perObjectData.Color = { colors[ i ], 1 };
        perObjectData.World =
          M4Translate( selectionGizmoOrigin ) *
          rots[ i ] *
          M4Scale( v3( 1, 1, 1 ) * mArrowLen ) *
          mArrow->mTransform;
        AddDrawCall( mArrow, perObjectData );

        // Widget Scale Cube
        v3 axis = {};
        axis[ i ] = 1;
        perObjectData.World =
          M4Translate( selectionGizmoOrigin ) *
          M4Translate( axis * ( mArrowLen * 1.1f ) ) *
          rots[ i ] *
          M4Scale( v3( 1, 1, 1 ) * mArrowLen * 0.1f );
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
    ImGuiBegin( "gameplay overlay", { 300, 75 } );
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

    if( Shell::Instance.mElapsedSeconds < mStatusMessageEndTime )
    {
      ImGuiText( mStatusMessage );
    }

    ImGuiEnd();
  }
  void CreationGameWindow::CameraControls()
  {
    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread.FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;
    if( !desktopWindowState->mCursorUnobscured )
      return;
    Creation* creation = Creation::Instance;
    Camera oldCamera = creation->mEditorCamera;

    if( KeyboardInput::Instance->IsKeyDown( Key::MouseRight ) &&
        KeyboardInput::Instance->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
    {
      float pixelsPerDeg = 400.0f / 90.0f;
      float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
      v2 angleRadians = KeyboardInput::Instance->mMouseDeltaPosScreenspace * radiansPerPixel;

      if( angleRadians.x != 0 )
      {
        m3 matrix = M3AngleAxis( -angleRadians.x, creation->mEditorCamera.mUp );
        creation->mEditorCamera.mForwards =
          matrix *
          creation->mEditorCamera.mForwards;
        creation->mEditorCamera.mRight = Cross(
          creation->mEditorCamera.mForwards,
          creation->mEditorCamera.mUp );
      }

      if( angleRadians.y != 0 )
      {
        m3 matrix = M3AngleAxis( -angleRadians.y, creation->mEditorCamera.mRight );
        creation->mEditorCamera.mForwards =
          matrix *
          creation->mEditorCamera.mForwards;
        creation->mEditorCamera.mUp = Cross(
          creation->mEditorCamera.mRight,
          creation->mEditorCamera.mForwards );
      }

      // Snapping right.y to the x-z plane prevents the camera from tilting side-to-side.
      creation->mEditorCamera.mForwards.Normalize();
      creation->mEditorCamera.mRight.y = 0;
      creation->mEditorCamera.mRight.Normalize();
      creation->mEditorCamera.mUp = Cross(
        creation->mEditorCamera.mRight,
        creation->mEditorCamera.mForwards );
      creation->mEditorCamera.mUp.Normalize();
    }

    if( KeyboardInput::Instance->IsKeyDown( Key::MouseMiddle ) &&
        KeyboardInput::Instance->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
    {
      float unitsPerPixel = 5.0f / 100.0f;
      creation->mEditorCamera.mPos +=
        creation->mEditorCamera.mRight *
        -KeyboardInput::Instance->mMouseDeltaPosScreenspace.x *
        unitsPerPixel;
      creation->mEditorCamera.mPos +=
        creation->mEditorCamera.mUp *
        KeyboardInput::Instance->mMouseDeltaPosScreenspace.y *
        unitsPerPixel;
    }

    if( KeyboardInput::Instance->mMouseDeltaScroll )
    {
      //float unitsPerTick = 0.35f;
      creation->mEditorCamera.mPos +=
        creation->mEditorCamera.mForwards *
        ( float )KeyboardInput::Instance->mMouseDeltaScroll;
    }

    if(
      oldCamera.mPos != creation->mEditorCamera.mPos ||
      oldCamera.mForwards != creation->mEditorCamera.mForwards ||
      oldCamera.mRight != creation->mEditorCamera.mRight ||
      oldCamera.mUp != creation->mEditorCamera.mUp )
    {
      for( Prefab* prefab : creation->mPrefabs )
      {
        creation->SavePrefabCameraPosition( prefab );
      }
    }
  }
  void CreationGameWindow::Update( Errors& errors )
  {
    Creation* creation = Creation::Instance;





    WindowFramebufferInfo* info = WindowFramebufferManager::Instance.FindWindowFramebufferInfo( mDesktopWindowHandle );

    DesktopWindowState* desktopWindowState = DesktopWindowStateCollection::InstanceStuffThread. FindDesktopWindowState( mDesktopWindowHandle );
    if( !desktopWindowState )
      return;

    Viewport viewport(
      ( float )desktopWindowState->mWidth,
      ( float )desktopWindowState->mHeight );

    ScissorRect scissorRect(
      ( float )desktopWindowState->mWidth,
      ( float )desktopWindowState->mHeight );

    Render::SetViewFramebuffer( ViewIdGameWindow, info->mFramebufferHandle );
    Render::SetViewport( ViewIdGameWindow, viewport );
    Render::SetViewScissorRect( ViewIdGameWindow, scissorRect );


    //mDesktopWindow->SetRenderViewDefaults();
    //TAC_INVALID_CODE_PATH;
    SetCreationWindowImGuiGlobals( desktopWindowState,
                                   mUI2DDrawData );
    if( mSoul )
    {
      //static bool once;
      //if( !once )
      //{
      //  once = true;
      //  Entity* entity = creation->CreateEntity();
      //  entity->mName = "Starry-eyed girl";
      //  entity->mPosition = {}; // { 4.5f, -4.0f, -0.5f };
      //  auto model = ( Model* )entity->AddNewComponent( ComponentRegistryEntryIndex::Model );
      //  model->mGLTFPath = "assets/editor/Box.gltf";
      //}
      mSoul->Update( errors );
      TAC_HANDLE_ERROR( errors );
    }

    //mDebug3DDrawData->DebugDrawGrid();

    if( creation->IsAnythingSelected() )
    {
      v3 origin = creation->GetSelectionGizmoOrigin();
      mDebug3DDrawData->DebugDrawCircle(
        origin,
        creation->mEditorCamera.mForwards,
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
                                                     ViewIdGameWindow );

    if( creation->mSelectedGizmo )
    {
      v3 origin = creation->GetSelectionGizmoOrigin();
      float gizmoMouseDist;
      float secondDist;
      ClosestPointTwoRays( creation->mEditorCamera.mPos,
                           worldSpaceMouseDir,
                           origin,
                           creation->mTranslationGizmoDir,
                           &gizmoMouseDist,
                           &secondDist );
      v3 translate = creation->mTranslationGizmoDir *
        ( secondDist - creation->mTranslationGizmoOffset );
      for( Entity* entity : creation->mSelectedEntities )
      {
        entity->mRelativeSpace.mPosition += translate;
      }
      //for( Prefab* prefab : creation->mSelectedPrefabs )
      //{
      //  prefab->mPosition += translate;
      //}
      if( !KeyboardInput::Instance->IsKeyDown( Key::MouseLeft ) )
      {
        creation->mSelectedGizmo = false;
      }
    }

    DrawPlaybackOverlay( errors );
    TAC_HANDLE_ERROR( errors );

    mUI2DDrawData->DrawToTexture( desktopWindowState->mWidth,
                                  desktopWindowState->mHeight,
                                  ViewIdGameWindow,
                                  errors );
    TAC_HANDLE_ERROR( errors );
  }
}
