#include "creation/tacCreationGameWindow.h"
#include "creation/tacCreation.h"
#include "common/tacShell.h"
#include "common/tacDesktopWindow.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI2D.h"
#include "common/graphics/tacUI.h"
#include "common/graphics/tacImGui.h"
#include "common/tackeyboardinput.h"
#include "common/graphics/tacDebug3D.h"
#include "common/assetmanagers/tacTextureAssetManager.h"
#include "common/assetmanagers/tacModelAssetManager.h"
#include "common/tacOS.h"
#include "shell/tacDesktopApp.h"
#include "space/tacGhost.h"
#include "space/tacgraphics.h"
#include "space/tacworld.h"
#include "space/tacentity.h"
#include "space/presentation/tacGamePresentation.h"
#include "space/presentation/tacSkyboxPresentation.h"

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
  float ab = TacDot( a, b );
  float bc = TacDot( b, c );
  float ac = TacDot( a, c );
  float aa = TacDot( a, a );
  float bb = TacDot( b, b );
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

TacCreationGameWindow::~TacCreationGameWindow()
{
  TacRenderer* renderer = mShell->mRenderer;
  renderer->RemoveRendererResource( m3DShader );
  renderer->RemoveRendererResource( m3DVertexFormat );
  renderer->RemoveRendererResource( mPerFrame );
  renderer->RemoveRendererResource( mPerObj );
  renderer->RemoveRendererResource( mDepthState );
  renderer->RemoveRendererResource( mBlendState );
  renderer->RemoveRendererResource( mRasterizerState );
  renderer->RemoveRendererResource( mSamplerState );
  delete mUI2DDrawData;
  delete mDebug3DDrawData;
}
void TacCreationGameWindow::CreateGraphicsObjects( TacErrors& errors )
{
  TacRenderer* renderer = mShell->mRenderer;

  TacCBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "tac 3d per frame";
  cBufferDataPerFrame.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( TacDefaultCBufferPerFrame );
  renderer->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  TAC_HANDLE_ERROR( errors );

  TacCBufferData cBufferDataPerObj = {};
  cBufferDataPerObj.mName = "tac 3d per obj";
  cBufferDataPerObj.mStackFrame = TAC_STACK_FRAME;
  cBufferDataPerObj.shaderRegister = 1;
  cBufferDataPerObj.byteCount = sizeof( TacDefaultCBufferPerObject );
  renderer->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
  TAC_HANDLE_ERROR( errors );


  TacShaderData shaderData;
  shaderData.mStackFrame = TAC_STACK_FRAME;
  shaderData.mName = "game window 3d shader";
  shaderData.mShaderPath = "3DTest";
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  renderer->AddShader( &m3DShader, shaderData, errors );
  TAC_HANDLE_ERROR( errors );

  TacVertexDeclaration posDecl;
  posDecl.mAlignedByteOffset = 0;
  posDecl.mAttribute = TacAttribute::Position;
  posDecl.mTextureFormat.mElementCount = 3;
  posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
  posDecl.mTextureFormat.mPerElementDataType = TacGraphicsType::real;
  TacVertexFormatData vertexFormatData = {};
  vertexFormatData.shader = m3DShader;
  vertexFormatData.vertexFormatDatas = { posDecl };
  vertexFormatData.mStackFrame = TAC_STACK_FRAME;
  vertexFormatData.mName = "game window renderer"; // cpresentation?
  renderer->AddVertexFormat( &m3DVertexFormat, vertexFormatData, errors );
  TAC_HANDLE_ERROR( errors );

  TacBlendStateData blendStateData;
  blendStateData.srcRGB = TacBlendConstants::One;
  blendStateData.dstRGB = TacBlendConstants::Zero;
  blendStateData.blendRGB = TacBlendMode::Add;
  blendStateData.srcA = TacBlendConstants::Zero;
  blendStateData.dstA = TacBlendConstants::One;
  blendStateData.blendA = TacBlendMode::Add;
  blendStateData.mName = "tac 3d opaque blend";
  blendStateData.mStackFrame = TAC_STACK_FRAME;
  renderer->AddBlendState( &mBlendState, blendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacDepthStateData depthStateData;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.depthFunc = TacDepthFunc::Less;
  depthStateData.mName = "tac 3d depth state";
  depthStateData.mStackFrame = TAC_STACK_FRAME;
  renderer->AddDepthState( &mDepthState, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacRasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = TacCullMode::None; // todo
  rasterizerStateData.fillMode = TacFillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "tac 3d rast state";
  rasterizerStateData.mStackFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  renderer->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  TAC_HANDLE_ERROR( errors );

  TacSamplerStateData samplerStateData;
  samplerStateData.mName = "tac 3d tex sampler";
  samplerStateData.mStackFrame = TAC_STACK_FRAME;
  samplerStateData.filter = TacFilter::Linear;
  renderer->AddSamplerState( &mSamplerState, samplerStateData, errors );
  TAC_HANDLE_ERROR( errors );
}
void TacCreationGameWindow::Init( TacErrors& errors )
{
  TacShell* shell = mShell;
  TacRenderer* renderer = mShell->mRenderer;
  TacModelAssetManager* modelAssetManager = mShell->mModelAssetManager;

  auto uI2DDrawData = new TacUI2DDrawData();
  uI2DDrawData->mUI2DCommonData = shell->mUI2DCommonData;
  uI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData = uI2DDrawData;
  mUIRoot = new TacUIRoot;
  mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mKeyboardInput = mShell->mKeyboardInput;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  CreateGraphicsObjects( errors );
  TAC_HANDLE_ERROR( errors );

  mSkyboxPresentation = new TacSkyboxPresentation;
  mSkyboxPresentation->mTextureAssetManager = mShell->mTextureAssetManager;
  mSkyboxPresentation->mRenderer = mShell->mRenderer;
  mSkyboxPresentation->mModelAssetManager = mShell->mModelAssetManager;
  mSkyboxPresentation->mCamera = &mCreation->mEditorCamera;
  mSkyboxPresentation->mDesktopWindow = mDesktopWindow;
  mSkyboxPresentation->Init( errors );
  TAC_HANDLE_ERROR( errors );

  mGamePresentation = new TacGamePresentation;
  mGamePresentation->mWorld = mCreation->mWorld;
  mGamePresentation->mCamera = &mCreation->mEditorCamera;
  mGamePresentation->mModelAssetManager = mShell->mModelAssetManager;
  mGamePresentation->mRenderer = mShell->mRenderer;
  mGamePresentation->mDesktopWindow = mDesktopWindow;
  mGamePresentation->mSkyboxPresentation = mSkyboxPresentation;
  mGamePresentation->mDebug3DCommonData = mShell->mDebug3DCommonData;
  mGamePresentation->CreateGraphicsObjects( errors );
  TAC_HANDLE_ERROR( errors );


  modelAssetManager->GetMesh(
    &mCenteredUnitCube,
    "assets/editor/box.gltf",
    m3DVertexFormat,
    errors );
  TAC_HANDLE_ERROR( errors );

  modelAssetManager->GetMesh(
    &mArrow,
    "assets/editor/arrow.gltf",
    m3DVertexFormat,
    errors );
  TAC_HANDLE_ERROR( errors );

  mDebug3DDrawData = new TacDebug3DDrawData;
  //mDebug3DDrawData->mCommonData = shell->mDebug3DCommonData;
  //mDebug3DDrawData->mRenderView = mDesktopWindow->mRenderView;

  PlayGame( errors );
  TAC_HANDLE_ERROR( errors );
}

void TacCreationGameWindow::MousePickingAll()
{
  if( !mDesktopWindow->mCursorUnobscured )
    return;

  enum PickedObject
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
      TacEntity* closest;
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

  for( TacEntity* entity : mCreation->mWorld->mEntities )
  {
    MousePickingEntity( entity, &hit, &dist );
    if( !hit || !pickData.IsNewClosest( dist ) )
      continue;
    pickData.closestDist = dist;
    pickData.closest = entity;
    pickData.pickedObject = PickedObject::Entity;
  }

  if( mCreation->mSelectedEntities.size() ) // || mCreation->mSelectedPrefabs.size() )
  {
    v3 selectionGizmoOrigin = mCreation->GetSelectionGizmoOrigin();

    m4 invArrowRots[] = {
      M4RotRadZ( 3.14f / 2.0f ),
      m4::Identity(),
      M4RotRadX( -3.14f / 2.0f ), };

    for( int i = 0; i < 3; ++i )
    {
      // 1/3: inverse transform
      v3 modelSpaceRayPos3 = mCreation->mEditorCamera.mPos - selectionGizmoOrigin;
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
    worldSpaceHitPoint = mCreation->mEditorCamera.mPos + pickData.closestDist * worldSpaceMouseDir;
    mDebug3DDrawData->DebugDrawSphere( worldSpaceHitPoint, 0.2f, v3( 1, 1, 0 ) );
  }

  if( mShell->mKeyboardInput->IsKeyJustDown( TacKey::MouseLeft ) )
  {

    switch( pickData.pickedObject )
    {
      case PickedObject::WidgetTranslationArrow:
      {
        v3 gizmoOrigin = mCreation->GetSelectionGizmoOrigin();
        v3 pickPoint = mCreation->mEditorCamera.mPos + worldSpaceMouseDir * pickData.closestDist;
        v3 arrowDir = {};
        arrowDir[ pickData.arrowAxis ] = 1;
        mCreation->mSelectedGizmo = true;
        mCreation->mTranslationGizmoDir = arrowDir;
        mCreation->mTranslationGizmoOffset = TacDot(
          arrowDir,
          worldSpaceHitPoint - gizmoOrigin );
      } break;
      case PickedObject::Entity:
      {
        v3 entityWorldOrigin = ( pickData.closest->mWorldTransform * v4( 0, 0, 0, 1 ) ).xyz();
        mCreation->ClearSelection();
        mCreation->mSelectedEntities = { pickData.closest };
        mCreation->mSelectedHitOffsetExists = true;
        mCreation->mSelectedHitOffset = worldSpaceHitPoint - entityWorldOrigin;
      } break;
      case PickedObject::None:
      {
        mCreation->ClearSelection();
      } break;
    }
  }
}
void TacCreationGameWindow::MousePickingInit()
{
  float w = ( float )mDesktopWindow->mWidth;
  float h = ( float )mDesktopWindow->mHeight;
  v2 screenspaceCursorPos;
  TacErrors errors;
  TacOS::Instance->GetScreenspaceCursorPos( screenspaceCursorPos, errors );
  if( errors.size() )
    return;
  float xNDC = ( ( screenspaceCursorPos.x - mDesktopWindow->mX ) / w );
  float yNDC = ( ( screenspaceCursorPos.y - mDesktopWindow->mY ) / h );
  yNDC = 1 - yNDC;
  xNDC = xNDC * 2 - 1;
  yNDC = yNDC * 2 - 1;
  float aspect = w / h;
  float theta = mCreation->mEditorCamera.mFovyrad / 2.0f;
  float cotTheta = 1.0f / std::tan( theta );
  float sX = cotTheta / aspect;
  float sY = cotTheta;

  m4 viewInv = M4ViewInv(
    mCreation->mEditorCamera.mPos,
    mCreation->mEditorCamera.mForwards,
    mCreation->mEditorCamera.mRight,
    mCreation->mEditorCamera.mUp );
  v3 viewSpaceMousePosNearPlane =
  {
    xNDC / sX,
    yNDC / sY,
    -1,
  };

  v3 viewSpaceMouseDir = Normalize( viewSpaceMousePosNearPlane );
  v4 viewSpaceMouseDir4 = v4( viewSpaceMouseDir, 0 );
  v4 worldSpaceMouseDir4 = viewInv * viewSpaceMouseDir4;
  worldSpaceMouseDir = worldSpaceMouseDir4.xyz();
}
void TacCreationGameWindow::MousePickingEntity(
  const TacEntity* entity,
  bool* hit,
  float* dist )
{
  const TacModel* model = TacModel::GetModel( entity );
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

  v3 modelSpaceMouseRayPos3 = ( transformInv * v4( mCreation->mEditorCamera.mPos, 1 ) ).xyz();
  v3 modelSpaceMouseRayDir3 = Normalize( ( transformInv * v4( worldSpaceMouseDir, 0 ) ).xyz() );
  float modelSpaceDist;
  model->mesh->Raycast( modelSpaceMouseRayPos3, modelSpaceMouseRayDir3, hit, &modelSpaceDist );

  // Recompute the distance by transforming the model space hit point into world space in order to
  // account for non-uniform scaling
  if( *hit )
  {
    v3 modelSpaceHitPoint = modelSpaceMouseRayPos3 + modelSpaceMouseRayDir3 * modelSpaceDist;
    v3 worldSpaceHitPoint = ( entity->mWorldTransform * v4( modelSpaceHitPoint, 1 ) ).xyz();
    *dist = Distance( mCreation->mEditorCamera.mPos, worldSpaceHitPoint );
  }
}
void TacCreationGameWindow::AddDrawCall( const TacMesh* mesh, const TacDefaultCBufferPerObject& cbuf )
{
  TacRenderer* renderer = mShell->mRenderer;
  for( const TacSubMesh& subMesh : mesh->mSubMeshes )
  {
    TacDrawCall2 drawCall = {};
    drawCall.mShader = mesh->mVertexFormat->shader;
    drawCall.mVertexBuffer = subMesh.mVertexBuffer;
    drawCall.mIndexBuffer = subMesh.mIndexBuffer;
    drawCall.mStartIndex = 0;
    drawCall.mIndexCount = subMesh.mIndexBuffer->indexCount;
    drawCall.mRenderView = mDesktopWindow->mRenderView;
    drawCall.mBlendState = mBlendState;
    drawCall.mRasterizerState = mRasterizerState;
    drawCall.mSamplerState = mSamplerState;
    drawCall.mDepthState = mDepthState;
    drawCall.mVertexFormat = mesh->mVertexFormat;
    drawCall.mTexture = nullptr;
    drawCall.mUniformDst = mPerObj;
    drawCall.mUniformSrcc = TacTemporaryMemory( &cbuf, sizeof( TacDefaultCBufferPerObject ) );
    drawCall.mStackFrame = TAC_STACK_FRAME;
    renderer->AddDrawCall( drawCall );
  }
}
void TacCreationGameWindow::ComputeArrowLen()
{
  if( !mCreation->IsAnythingSelected() )
  {
    return;
  }
  m4 view = M4View(
    mCreation->mEditorCamera.mPos,
    mCreation->mEditorCamera.mForwards,
    mCreation->mEditorCamera.mRight,
    mCreation->mEditorCamera.mUp );
  v3 pos = mCreation->GetSelectionGizmoOrigin();
  v4 posVS4 = view * v4( pos, 1 );
  float clip_height = std::abs( std::tan( mCreation->mEditorCamera.mFovyrad / 2.0f ) * posVS4.z * 2.0f );
  float arrowLen = clip_height * 0.2f;
  mArrowLen = arrowLen;
}
void TacCreationGameWindow::RenderGameWorldToGameWindow()
{
  MousePickingAll();

  TacRenderer* renderer = mShell->mRenderer;
  TacModelAssetManager* modelAssetManager = mShell->mModelAssetManager;
  m4 view = mCreation->mEditorCamera.View();
  float w = ( float )mDesktopWindow->mWidth;
  float h = ( float )mDesktopWindow->mHeight;
  float a;
  float b;
  renderer->GetPerspectiveProjectionAB(
    mCreation->mEditorCamera.mFarPlane,
    mCreation->mEditorCamera.mNearPlane,
    a,
    b );
  float aspect = w / h;
  m4 proj = M4ProjPerspective( a, b, mCreation->mEditorCamera.mFovyrad, aspect );
  TacDefaultCBufferPerFrame perFrameData;
  perFrameData.mFar = mCreation->mEditorCamera.mFarPlane;
  perFrameData.mNear = mCreation->mEditorCamera.mNearPlane;
  perFrameData.mView = view;
  perFrameData.mProjection = proj;
  perFrameData.mGbufferSize = { w, h };
  TacDrawCall2 setPerFrame = {};
  setPerFrame.mUniformDst = mPerFrame;
  setPerFrame.mUniformSrcc = TacTemporaryMemory( &perFrameData, sizeof( TacDefaultCBufferPerFrame ) );
  renderer->AddDrawCall( setPerFrame );
  if( mCreation->IsAnythingSelected() )
  {
    v3 selectionGizmoOrigin = mCreation->GetSelectionGizmoOrigin();
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
      TacDefaultCBufferPerObject perObjectData;
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

  TacErrors ignored;
  mDebug3DDrawData->DrawToTexture(
    ignored,
    &perFrameData,
    mShell->mDebug3DCommonData,
    mDesktopWindow->mRenderView );

  mGamePresentation->RenderGameWorldToDesktopView();
}
void TacCreationGameWindow::PlayGame( TacErrors& errors )
{
  if( mSoul )
    return;
  auto ghost = new TacGhost;
  ghost->mShell = mShell;
  ghost->mRenderView = mDesktopWindow->mRenderView;
  ghost->Init( errors );
  TAC_HANDLE_ERROR( errors );
  mSoul = ghost;
}
void TacCreationGameWindow::DrawPlaybackOverlay( TacErrors& errors )
{
  TacImGuiBegin( "gameplay overlay", { 300, 75 } );
  if( mSoul )
  {
    if( TacImGuiButton( "End simulation" ) )
    {
      delete mSoul;
      mSoul = nullptr;
    }
  }
  else
  {
    if( TacImGuiButton( "Begin simulation" ) )
    {
      PlayGame( errors );
      TAC_HANDLE_ERROR( errors );
    }
  }
  TacImGuiEnd();
}
void TacCreationGameWindow::CameraControls()
{
  if( !mDesktopWindow->mCursorUnobscured )
    return;
  TacCamera oldCamera = mCreation->mEditorCamera;

  TacKeyboardInput* keyboardInput = mShell->mKeyboardInput;
  if( keyboardInput->IsKeyDown( TacKey::MouseRight ) &&
    keyboardInput->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
  {
    float pixelsPerDeg = 400.0f / 90.0f;
    float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
    v2 angleRadians = keyboardInput->mMouseDeltaPosScreenspace * radiansPerPixel;

    if( angleRadians.x != 0 )
    {
      m3 matrix = M3AngleAxis( -angleRadians.x, mCreation->mEditorCamera.mUp );
      mCreation->mEditorCamera.mForwards =
        matrix *
        mCreation->mEditorCamera.mForwards;
      mCreation->mEditorCamera.mRight = Cross(
        mCreation->mEditorCamera.mForwards,
        mCreation->mEditorCamera.mUp );
    }

    if( angleRadians.y != 0 )
    {
      m3 matrix = M3AngleAxis( -angleRadians.y, mCreation->mEditorCamera.mRight );
      mCreation->mEditorCamera.mForwards =
        matrix *
        mCreation->mEditorCamera.mForwards;
      mCreation->mEditorCamera.mUp = Cross(
        mCreation->mEditorCamera.mRight,
        mCreation->mEditorCamera.mForwards );
    }

    // Snapping right.y to the x-z plane prevents the camera from tilting side-to-side.
    mCreation->mEditorCamera.mForwards.Normalize();
    mCreation->mEditorCamera.mRight.y = 0;
    mCreation->mEditorCamera.mRight.Normalize();
    mCreation->mEditorCamera.mUp = Cross(
      mCreation->mEditorCamera.mRight,
      mCreation->mEditorCamera.mForwards );
    mCreation->mEditorCamera.mUp.Normalize();
  }

  if( keyboardInput->IsKeyDown( TacKey::MouseMiddle ) &&
    keyboardInput->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
  {
    float unitsPerPixel = 5.0f / 100.0f;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mRight *
      -keyboardInput->mMouseDeltaPosScreenspace.x *
      unitsPerPixel;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mUp *
      keyboardInput->mMouseDeltaPosScreenspace.y *
      unitsPerPixel;
  }

  if( keyboardInput->mMouseDeltaScroll )
  {
    float unitsPerTick = 0.35f;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mForwards *
      ( float )keyboardInput->mMouseDeltaScroll;
  }

  if(
    oldCamera.mPos != mCreation->mEditorCamera.mPos ||
    oldCamera.mForwards != mCreation->mEditorCamera.mForwards ||
    oldCamera.mRight != mCreation->mEditorCamera.mRight ||
    oldCamera.mUp != mCreation->mEditorCamera.mUp )
  {
    for( TacPrefab* prefab : mCreation->mPrefabs )
    {
      mCreation->SavePrefabCameraPosition( prefab );
    }
  }
}
void TacCreationGameWindow::Update( TacErrors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  TacImGuiSetGlobals( mShell, mDesktopWindow, mUI2DDrawData );
  if( auto ghost = ( TacGhost* )mSoul )
  {
    //static bool once;
    //if( !once )
    //{
    //  once = true;
    //  TacEntity* entity = mCreation->CreateEntity();
    //  entity->mName = "Starry-eyed girl";
    //  entity->mPosition = {}; // { 4.5f, -4.0f, -0.5f };
    //  auto model = ( TacModel* )entity->AddNewComponent( TacComponentRegistryEntryIndex::Model );
    //  model->mGLTFPath = "assets/editor/Box.gltf";
    //}
    ghost->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  mDebug3DDrawData->DebugDrawGrid();

  if( mCreation->IsAnythingSelected() )
  {
    v3 origin = mCreation->GetSelectionGizmoOrigin();
    mDebug3DDrawData->DebugDrawCircle(
      origin,
      mCreation->mEditorCamera.mForwards,
      mArrowLen );
  }

  MousePickingInit();
  CameraControls();
  ComputeArrowLen();
  RenderGameWorldToGameWindow();
  TAC_HANDLE_ERROR( errors );

  if( mCreation->mSelectedGizmo )
  {
    v3 origin = mCreation->GetSelectionGizmoOrigin();
    float gizmoMouseDist;
    float secondDist;
    ClosestPointTwoRays(
      mCreation->mEditorCamera.mPos,
      worldSpaceMouseDir,
      origin,
      mCreation->mTranslationGizmoDir,
      &gizmoMouseDist,
      &secondDist );
    v3 translate = mCreation->mTranslationGizmoDir *
      ( secondDist - mCreation->mTranslationGizmoOffset );
    for( TacEntity* entity : mCreation->mSelectedEntities )
    {
      entity->mLocalPosition += translate;
    }
    //for( TacPrefab* prefab : mCreation->mSelectedPrefabs )
    //{
    //  prefab->mPosition += translate;
    //}
    if( !mShell->mKeyboardInput->IsKeyDown( TacKey::MouseLeft ) )
    {
      mCreation->mSelectedGizmo = false;
    }
  }

  DrawPlaybackOverlay( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture( errors );
  TAC_HANDLE_ERROR( errors );
}
