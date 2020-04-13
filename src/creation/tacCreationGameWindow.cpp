#include "src/creation/tacCreationGameWindow.h"
#include "src/creation/tacCreation.h"
#include "src/common/tacShell.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/graphics/tacDebug3D.h"
#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/assetmanagers/tacModelAssetManager.h"
#include "src/common/tacOS.h"
#include "src/shell/tacDesktopApp.h"
#include "src/space/tacGhost.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/tacWorld.h"
#include "src/space/tacEntity.h"
#include "src/space/presentation/tacGamePresentation.h"
#include "src/space/presentation/tacSkyboxPresentation.h"

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

CreationGameWindow::~CreationGameWindow()
{
  Renderer::Instance->RemoveRendererResource( m3DShader );
  Renderer::Instance->RemoveRendererResource( m3DVertexFormat );
  Renderer::Instance->RemoveRendererResource( mPerFrame );
  Renderer::Instance->RemoveRendererResource( mPerObj );
  Renderer::Instance->RemoveRendererResource( mDepthState );
  Renderer::Instance->RemoveRendererResource( mBlendState );
  Renderer::Instance->RemoveRendererResource( mRasterizerState );
  Renderer::Instance->RemoveRendererResource( mSamplerState );
  delete mUI2DDrawData;
  delete mDebug3DDrawData;
}
void CreationGameWindow::CreateGraphicsObjects( Errors& errors )
{
  CBufferData cBufferDataPerFrame = {};
  cBufferDataPerFrame.mName = "tac 3d per frame";
  cBufferDataPerFrame.mFrame = TAC_STACK_FRAME;
  cBufferDataPerFrame.shaderRegister = 0;
  cBufferDataPerFrame.byteCount = sizeof( DefaultCBufferPerFrame );
  Renderer::Instance->AddConstantBuffer( &mPerFrame, cBufferDataPerFrame, errors );
  TAC_HANDLE_ERROR( errors );

  CBufferData cBufferDataPerObj = {};
  cBufferDataPerObj.mName = "tac 3d per obj";
  cBufferDataPerObj.mFrame = TAC_STACK_FRAME;
  cBufferDataPerObj.shaderRegister = 1;
  cBufferDataPerObj.byteCount = sizeof( DefaultCBufferPerObject );
  Renderer::Instance->AddConstantBuffer( &mPerObj, cBufferDataPerObj, errors );
  TAC_HANDLE_ERROR( errors );


  ShaderData shaderData;
  shaderData.mFrame = TAC_STACK_FRAME;
  shaderData.mName = "game window 3d shader";
  shaderData.mShaderPath = "3DTest";
  shaderData.mCBuffers = { mPerFrame, mPerObj };
  Renderer::Instance->AddShader( &m3DShader, shaderData, errors );
  TAC_HANDLE_ERROR( errors );

  VertexDeclaration posDecl;
  posDecl.mAlignedByteOffset = 0;
  posDecl.mAttribute = Attribute::Position;
  posDecl.mTextureFormat.mElementCount = 3;
  posDecl.mTextureFormat.mPerElementByteCount = sizeof( float );
  posDecl.mTextureFormat.mPerElementDataType = GraphicsType::real;
  VertexFormatData vertexFormatData = {};
  vertexFormatData.shader = m3DShader;
  vertexFormatData.vertexFormatDatas = { posDecl };
  vertexFormatData.mFrame = TAC_STACK_FRAME;
  vertexFormatData.mName = "game window renderer"; // cpresentation?
  Renderer::Instance->AddVertexFormat( &m3DVertexFormat, vertexFormatData, errors );
  TAC_HANDLE_ERROR( errors );

  BlendStateData blendStateData;
  blendStateData.srcRGB = BlendConstants::One;
  blendStateData.dstRGB = BlendConstants::Zero;
  blendStateData.blendRGB = BlendMode::Add;
  blendStateData.srcA = BlendConstants::Zero;
  blendStateData.dstA = BlendConstants::One;
  blendStateData.blendA = BlendMode::Add;
  blendStateData.mName = "tac 3d opaque blend";
  blendStateData.mFrame = TAC_STACK_FRAME;
  Renderer::Instance->AddBlendState( &mBlendState, blendStateData, errors );
  TAC_HANDLE_ERROR( errors );

  DepthStateData depthStateData;
  depthStateData.depthTest = true;
  depthStateData.depthWrite = true;
  depthStateData.depthFunc = DepthFunc::Less;
  depthStateData.mName = "tac 3d depth state";
  depthStateData.mFrame = TAC_STACK_FRAME;
  Renderer::Instance->AddDepthState( &mDepthState, depthStateData, errors );
  TAC_HANDLE_ERROR( errors );

  RasterizerStateData rasterizerStateData;
  rasterizerStateData.cullMode = CullMode::None; // todo
  rasterizerStateData.fillMode = FillMode::Solid;
  rasterizerStateData.frontCounterClockwise = true;
  rasterizerStateData.mName = "tac 3d rast state";
  rasterizerStateData.mFrame = TAC_STACK_FRAME;
  rasterizerStateData.multisample = false;
  rasterizerStateData.scissor = true;
  Renderer::Instance->AddRasterizerState( &mRasterizerState, rasterizerStateData, errors );
  TAC_HANDLE_ERROR( errors );

  SamplerStateData samplerStateData;
  samplerStateData.mName = "tac 3d tex sampler";
  samplerStateData.mFrame = TAC_STACK_FRAME;
  samplerStateData.filter = Filter::Linear;
  Renderer::Instance->AddSamplerState( &mSamplerState, samplerStateData, errors );
  TAC_HANDLE_ERROR( errors );
}
void CreationGameWindow::Init( Errors& errors )
{
  ;

  auto uI2DDrawData = new UI2DDrawData();
  uI2DDrawData->mRenderView = mDesktopWindow->mRenderView;
  mUI2DDrawData = uI2DDrawData;
  mUIRoot = new UIRoot;
  mUIRoot->mElapsedSeconds = &Shell::Instance->mElapsedSeconds;
  mUIRoot->mUI2DDrawData = mUI2DDrawData;
  mUIRoot->mDesktopWindow = mDesktopWindow;
  CreateGraphicsObjects( errors );
  TAC_HANDLE_ERROR( errors );

  mSkyboxPresentation = new SkyboxPresentation;
  mSkyboxPresentation->mCamera = &mCreation->mEditorCamera;
  mSkyboxPresentation->mDesktopWindow = mDesktopWindow;
  mSkyboxPresentation->Init( errors );
  TAC_HANDLE_ERROR( errors );

  mGamePresentation = new GamePresentation;
  mGamePresentation->mWorld = mCreation->mWorld;
  mGamePresentation->mCamera = &mCreation->mEditorCamera;
  mGamePresentation->mDesktopWindow = mDesktopWindow;
  mGamePresentation->mSkyboxPresentation = mSkyboxPresentation;
  mGamePresentation->CreateGraphicsObjects( errors );
  TAC_HANDLE_ERROR( errors );


  ModelAssetManager::Instance->GetMesh(
    &mCenteredUnitCube,
    "assets/editor/box.gltf",
    m3DVertexFormat,
    errors );
  TAC_HANDLE_ERROR( errors );

  ModelAssetManager::Instance->GetMesh(
    &mArrow,
    "assets/editor/arrow.gltf",
    m3DVertexFormat,
    errors );
  TAC_HANDLE_ERROR( errors );

  mDebug3DDrawData = new Debug3DDrawData;
  //mDebug3DDrawData->mCommonData = shell->Debug3DCommonData::Instance;
  //mDebug3DDrawData->mRenderView = mDesktopWindow->mRenderView;

  PlayGame( errors );
  TAC_HANDLE_ERROR( errors );
}

void CreationGameWindow::MousePickingAll()
{
  if( !mDesktopWindow->mCursorUnobscured )
    return;

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

  for( Entity* entity : mCreation->mWorld->mEntities )
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

  if( KeyboardInput::Instance->IsKeyJustDown( Key::MouseLeft ) )
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
        mCreation->mTranslationGizmoOffset = Dot(
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
void CreationGameWindow::MousePickingInit()
{
  float w = ( float )mDesktopWindow->mWidth;
  float h = ( float )mDesktopWindow->mHeight;
  v2 screenspaceCursorPos;
  Errors errors;
  OS::GetScreenspaceCursorPos( screenspaceCursorPos, errors );
  if( errors )
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
void CreationGameWindow::MousePickingEntity(
  const Entity* entity,
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
void CreationGameWindow::AddDrawCall( const Mesh* mesh, const DefaultCBufferPerObject& cbuf )
{
  for( const SubMesh& subMesh : mesh->mSubMeshes )
  {
    DrawCall2 drawCall = {};
    drawCall.mShader = mesh->mVertexFormat->shader;
    drawCall.mVertexBuffer = subMesh.mVertexBuffer;
    drawCall.mIndexBuffer = subMesh.mIndexBuffer;
    drawCall.mStartIndex = 0;
    drawCall.mIndexCount = subMesh.mIndexBuffer->mIndexCount;
    drawCall.mRenderView = mDesktopWindow->mRenderView;
    drawCall.mBlendState = mBlendState;
    drawCall.mRasterizerState = mRasterizerState;
    drawCall.mSamplerState = mSamplerState;
    drawCall.mDepthState = mDepthState;
    drawCall.mVertexFormat = mesh->mVertexFormat;
    drawCall.mUniformDst = mPerObj;
    drawCall.mUniformSrcc = TemporaryMemoryFromT( cbuf );
    drawCall.mFrame = TAC_STACK_FRAME;
    Renderer::Instance->AddDrawCall( drawCall );
  }
}
void CreationGameWindow::ComputeArrowLen()
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
void CreationGameWindow::RenderGameWorldToGameWindow()
{
  MousePickingAll();

  m4 view = mCreation->mEditorCamera.View();
  float w = ( float )mDesktopWindow->mWidth;
  float h = ( float )mDesktopWindow->mHeight;
  float a;
  float b;
  Renderer::Instance->GetPerspectiveProjectionAB(
    mCreation->mEditorCamera.mFarPlane,
    mCreation->mEditorCamera.mNearPlane,
    a,
    b );
  float aspect = w / h;
  m4 proj = M4ProjPerspective( a, b, mCreation->mEditorCamera.mFovyrad, aspect );
  DefaultCBufferPerFrame perFrameData;
  perFrameData.mFar = mCreation->mEditorCamera.mFarPlane;
  perFrameData.mNear = mCreation->mEditorCamera.mNearPlane;
  perFrameData.mView = view;
  perFrameData.mProjection = proj;
  perFrameData.mGbufferSize = { w, h };
  DrawCall2 setPerFrame = {};
  setPerFrame.mUniformDst = mPerFrame;
  setPerFrame.CopyUniformSource(perFrameData);
  Renderer::Instance->AddDrawCall( setPerFrame );
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

  Errors ignored;
  mDebug3DDrawData->DrawToTexture(
    ignored,
    &perFrameData,
    mDesktopWindow->mRenderView );

  mGamePresentation->RenderGameWorldToDesktopView();
}
void CreationGameWindow::PlayGame( Errors& errors )
{
  if( mSoul )
    return;
  auto ghost = new Ghost;
  ghost->mRenderView = mDesktopWindow->mRenderView;
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

  if( Shell::Instance->mElapsedSeconds < mStatusMessageEndTime )
  {
    ImGuiText( mStatusMessage );
  }

  ImGuiEnd();
}
void CreationGameWindow::CameraControls()
{
  if( !mDesktopWindow->mCursorUnobscured )
    return;
  Camera oldCamera = mCreation->mEditorCamera;

  if( KeyboardInput::Instance->IsKeyDown( Key::MouseRight ) &&
    KeyboardInput::Instance->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
  {
    float pixelsPerDeg = 400.0f / 90.0f;
    float radiansPerPixel = ( 1.0f / pixelsPerDeg ) * ( 3.14f / 180.0f );
    v2 angleRadians = KeyboardInput::Instance->mMouseDeltaPosScreenspace * radiansPerPixel;

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

  if( KeyboardInput::Instance->IsKeyDown( Key::MouseMiddle ) &&
    KeyboardInput::Instance->mMouseDeltaPosScreenspace != v2( 0, 0 ) )
  {
    float unitsPerPixel = 5.0f / 100.0f;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mRight *
      -KeyboardInput::Instance->mMouseDeltaPosScreenspace.x *
      unitsPerPixel;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mUp *
      KeyboardInput::Instance->mMouseDeltaPosScreenspace.y *
      unitsPerPixel;
  }

  if( KeyboardInput::Instance->mMouseDeltaScroll )
  {
    float unitsPerTick = 0.35f;
    mCreation->mEditorCamera.mPos +=
      mCreation->mEditorCamera.mForwards *
      ( float )KeyboardInput::Instance->mMouseDeltaScroll;
  }

  if(
    oldCamera.mPos != mCreation->mEditorCamera.mPos ||
    oldCamera.mForwards != mCreation->mEditorCamera.mForwards ||
    oldCamera.mRight != mCreation->mEditorCamera.mRight ||
    oldCamera.mUp != mCreation->mEditorCamera.mUp )
  {
    for( Prefab* prefab : mCreation->mPrefabs )
    {
      mCreation->SavePrefabCameraPosition( prefab );
    }
  }
}
void CreationGameWindow::Update( Errors& errors )
{
  mDesktopWindow->SetRenderViewDefaults();
  SetCreationWindowImGuiGlobals( mDesktopWindow,
                                 mUI2DDrawData,
                                 mDesktopWindowState.mWidth,
                                 mDesktopWindowState.mHeight );
  if( auto ghost = ( Ghost* )mSoul )
  {
    //static bool once;
    //if( !once )
    //{
    //  once = true;
    //  Entity* entity = mCreation->CreateEntity();
    //  entity->mName = "Starry-eyed girl";
    //  entity->mPosition = {}; // { 4.5f, -4.0f, -0.5f };
    //  auto model = ( Model* )entity->AddNewComponent( ComponentRegistryEntryIndex::Model );
    //  model->mGLTFPath = "assets/editor/Box.gltf";
    //}
    ghost->Update( errors );
    TAC_HANDLE_ERROR( errors );
  }

  //mDebug3DDrawData->DebugDrawGrid();

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
    for( Entity* entity : mCreation->mSelectedEntities )
    {
      entity->mRelativeSpace.mPosition += translate;
    }
    //for( Prefab* prefab : mCreation->mSelectedPrefabs )
    //{
    //  prefab->mPosition += translate;
    //}
    if( !KeyboardInput::Instance->IsKeyDown( Key::MouseLeft ) )
    {
      mCreation->mSelectedGizmo = false;
    }
  }

  DrawPlaybackOverlay( errors );
  TAC_HANDLE_ERROR( errors );

  mUI2DDrawData->DrawToTexture(0, 0, 0,  errors );
  TAC_HANDLE_ERROR( errors );
}
}
