#include "common/graphics/tacFont.h"
#include "common/graphics/tacImGui.h"
#include "common/graphics/tacImGui.h"
#include "common/graphics/tacRenderer.h"
#include "common/graphics/tacUI.h"
#include "common/math/tacMath.h"
#include "common/tacAlgorithm.h"
#include "common/tacLocalization.h"
#include "common/tacPreprocessor.h"
#include "common/tacSettings.h"
#include "common/tacShell.h"
#include "common/tacTime.h"
#include "common/tacUtility.h"
#include "common/taccontrollerinput.h"
#include "common/tackeyboardinput.h"
#include "common/tacmeta.h"
#include "space/tacGhost.h"
#include "space/tacclient.h"
#include "space/collider/taccollider.h"
#include "space/tacentity.h"
#include "space/graphics/tacgraphics.h"
#include "space/physics/tacphysics.h"
#include "space/tacplayer.h"
#include "space/tacscript.h"
#include "space/tacscriptgameclient.h"
#include "space/tacserver.h"
#include "space/tacworld.h"
#include <fstream>
#include <iostream>
#include <utility>

TacUser::TacUser( TacGhost* ghost, const TacString& name, TacErrors& errors )
{
  mName = name;
  mGhost = ghost;
  TAC_HANDLE_ERROR( errors );
  auto serverData = mGhost->mServerData;
  auto player = serverData->SpawnPlayer();
  player->mCameraPos = v3(
    -15,
    12,
    25 );
  mPlayer = player;
  if( mGhost->mShouldPopulateWorldInitial )
  {
    TacEntity* entity = serverData->SpawnEntity();
    entity->mRelativeSpace.mPosition = v3(
      TacRandomFloatMinus1To1() * 3.0f,
      5.2f,
      TacRandomFloatMinus1To1() * 3.0f );
    entity->AddNewComponent( TacCollider::ColliderComponentRegistryEntry );
    player->mEntityUUID = entity->mEntityUUID;
  }
}
void TacUser::DebugImgui()
{
  //if( mPlayer )
  //  mPlayer->DebugImgui();
  //ImGui::InputText( "Name", mName );
}
void TacUser::Update( TacErrors& errors )
{
  if( !mGhost->mIsGrabbingInput )
    return;
  auto shell = mGhost->mShell;
  auto serverData = mGhost->mServerData;
  v2 inputDirection = { 0, 0 };
  if( TacKeyboardInput::Instance->IsKeyDown( TacKey::RightArrow ) ) inputDirection += { 1, 0 };
  if( TacKeyboardInput::Instance->IsKeyDown( TacKey::UpArrow ) ) inputDirection += { 0, 1 };
  if( TacKeyboardInput::Instance->IsKeyDown( TacKey::DownArrow ) ) inputDirection += { 0, -1 };
  if( TacKeyboardInput::Instance->IsKeyDown( TacKey::LeftArrow ) ) inputDirection += { -1, 0 };
  if( inputDirection.Length() )
    inputDirection.Normalize();
  mPlayer->mInputDirection = inputDirection;
  mPlayer->mIsSpaceJustDown = TacKeyboardInput::Instance->IsKeyDown( TacKey::Spacebar );
}

TacGhost::TacGhost()
{
  mScriptRoot = new TacScriptRoot;
  //mUIRoot = new TacUIRoot;
  mServerData = new TacServerData;
}
void TacGhost::Init( TacErrors& errors )
{
  mShouldPopulateWorldInitial = false;
  mScriptRoot->mGhost = this;
  //int w = shell->mWindowWidth;
  //int h = shell->mWindowHeight;

  //TacImage image;
  //image.mWidth = w;
  //image.mHeight = h;
  //image.mFormat.mPerElementByteCount = 1;
  //image.mFormat.mElementCount = 4;
  //image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
  //TacTextureData textureData;
  //textureData.access = TacAccess::Default;
  //textureData.binding = { TacBinding::RenderTarget, TacBinding::ShaderResource };
  //textureData.cpuAccess = {};
  //textureData.mName = "client view fbo";
  //textureData.mStackFrame = TAC_STACK_FRAME;
  //textureData.myImage = image;
  //TacRenderer::Instance->AddTextureResource( &mFBOTexture, textureData, errors );
  //TAC_HANDLE_ERROR( errors );

  //TacDepthBufferData depthBufferData;
  //depthBufferData.height = h;
  //depthBufferData.mName = "client view depth buffer";
  //depthBufferData.mStackFrame = TAC_STACK_FRAME;
  //depthBufferData.width = w;
  //TacRenderer::Instance->AddDepthBuffer( &mFBODepthBuffer, depthBufferData, errors );
  //TAC_HANDLE_ERROR( errors );

  const TacString serverTypeGameClient = TacStringify( TacScriptGameClient );
  TacString serverType = mShell->mSettings->GetString(
    nullptr,
    { "server type" },
    serverTypeGameClient,
    errors );
  TAC_HANDLE_ERROR( errors );
  TacScriptThread* child = nullptr;
  if( serverType == serverTypeGameClient )
    child = new TacScriptGameClient();
  else
    TacInvalidCodePath;
  mScriptRoot->AddChild( child );
  TAC_HANDLE_ERROR( errors );
  if( mShouldPopulateWorldInitial )
    PopulateWorldInitial();
  auto playerNames = {
    "Server",
    //"Client",
  };
  for( auto playerName : playerNames )
  {
    AddPlayer( playerName, errors );
    TAC_HANDLE_ERROR( errors );
  }

  //mUIRoot->mElapsedSeconds = &mShell->mElapsedSeconds; // eww
  //mUIRoot->mGhost = this;
}
TacGhost::~TacGhost()
{
  //delete mFBOTexture;
  //delete mFBODepthBuffer;
  for( TacUser* user : mUsers )
    delete user;
  //delete mUIRoot;
  delete mServerData;
  delete mClientData;
}
TacUser* TacGhost::AddPlayer( const TacString& name, TacErrors& errors )
{
  auto* user = new TacUser( this, name, errors );
  mUsers.push_back( user );
  TacScriptMsg scriptMsg;
  scriptMsg.mType = scriptMsgNameUserConnect;
  scriptMsg.mData = user;
  mScriptRoot->OnMsg( &scriptMsg );
  return user;
}
void TacGhost::ImguiCreatePlayerPopup( TacErrors& errors )
{
  //const char* popupName = "Enter Player Name";
  //const int playerNameBufSize = 100;
  //static TacString playerName;
  //if( ImGui::Button( "Spawn Player" ) )
  //  ImGui::OpenPopup( popupName );
  //if( !ImGui::BeginPopup( popupName ) )
  //  return;
  //OnDestruct( ImGui::EndPopup() );
  //ImGui::InputText( "Player Name: ", playerName );
  //if( playerName.empty() )
  //  return;
  //if( !ImGui::Button( "Go" ) )
  //  return;
  //AddPlayer( playerName, errors );
  //if( errors.size() )
  //  return;
}
void TacGhost::Update( TacErrors& errors )
{

  if( CanDrawImgui() )
  {
    //mMouserCursorNDC.x = ( ( float )mShell->mMouseRelTopLeftX - mImguiImagePosRelTopLeftX ) / mImguiImageW;
    //mMouserCursorNDC.y = ( ( float )mShell->mMouseRelTopLeftY - mImguiImagePosRelTopLeftY ) / mImguiImageH;
    //mMouseHoveredOverWindow &= mShell->mMouseInWindow;
    //mDrawDepthBuffer = mFBODepthBuffer;
    //mDrawTexture = mFBOTexture;
  }
  else
  {
    //TacInvalidCodePath;
    //mShell->TacRenderer::Instance->GetBackbufferColor( &mDrawTexture  );
    //mShell->TacRenderer::Instance->GetBackbufferDepth( &mDrawDepthBuffer );
    //mMouserCursorNDC.x = ( float )mShell->mMouseRelTopLeftX / mDrawTexture->myImage.mWidth;
    //mMouserCursorNDC.y = ( float )mShell->mMouseRelTopLeftY / mDrawTexture->myImage.mHeight;
    //mMouseHoveredOverWindow = mShell->mMouseInWindow;
  }
  mMouserCursorNDC.y = 1 - mMouserCursorNDC.y;
  mMouserCursorNDC *= 2;
  mMouserCursorNDC -= v2( 1, 1 );

  for( auto user : mUsers )
    user->Update( errors );
  mScriptRoot->Update( TAC_DELTA_FRAME_SECONDS, errors );
  mServerData->Update( TAC_DELTA_FRAME_SECONDS, nullptr, nullptr, errors );
  TAC_HANDLE_ERROR( errors );
  //mUIRoot->Update();
  Draw( errors );
  TAC_HANDLE_ERROR( errors );

  AddMorePlayers( errors );
  TAC_HANDLE_ERROR( errors );
}
void TacGhost::AddMorePlayers( TacErrors& errors )
{
  if( IsPartyFull() )
    return;

  TacVector< TacControllerIndex > claimedControllerIndexes;
  for( TacUser* user : mUsers )
    if( user->mHasControllerIndex )
      claimedControllerIndexes.push_back( user->mControllerIndex );

  TacAssert( ( int )claimedControllerIndexes.size() < TAC_CONTROLLER_COUNT_MAX );

  for( TacControllerIndex controllerIndex = 0; controllerIndex < TAC_CONTROLLER_COUNT_MAX; ++controllerIndex )
  {
    if( TacContains( claimedControllerIndexes, controllerIndex ) )
      continue;
    TacController* controller = TacControllerInput::Instance->mControllers[ controllerIndex ];
    if( !controller )
      continue;
    if( !controller->IsButtonJustPressed( TacControllerButton::Start ) )
      continue;
    TacUser* user = AddPlayer( "Player " + TacToString( ( int )mUsers.size() ), errors );
    if( errors.size() )
      return;
    user->mHasControllerIndex = true;
    user->mControllerIndex = controllerIndex;
    if( IsPartyFull() )
      return;
  }

}
bool TacGhost::IsPartyFull()
{
  bool result = ( int )mUsers.size() == sPlayerCountMax;
  return result;
}
void TacGhost::DebugImgui( TacErrors& errors )
{
  #if COMPILE_PLS
  if( !mIsImGuiVisible )
    return;
  ImGui::PushID( this );
  OnDestruct( ImGui::PopID() );
  ImGui::Begin( GetDebugName().c_str(), &mIsImGuiVisible );
  OnDestruct( ImGui::End() );
  ImGui::PushItemWidth( 200 );
  OnDestruct( ImGui::PopItemWidth() );


  if( CanDrawImgui() )
  {
    float aspect = mFBOTexture->GetAspect();
    auto w = ( int )ImGui::GetWindowContentRegionWidth();
    auto h = ( int )( w / aspect );
    auto size = ImVec2( ( float )w, ( float )h );
    ImGui::Image( mFBOTexture->GetImguiTextureID(), size );
    mMouseHoveredOverWindow = ImGui::IsItemHovered();

    mImguiImageW = ( float )w;
    mImguiImageH = ( float )h;


    ImVec2 itemRectMin = ImGui::GetItemRectMin();
    ImVec2 itemRectMax = ImGui::GetItemRectMax();
    ImVec2 itemRectSize = ImGui::GetItemRectSize();

    mImguiImagePosRelTopLeftX = itemRectMin.x;
    mImguiImagePosRelTopLeftY = itemRectMin.y;
    ImGui::DragFloat2( "Rect min", &itemRectMin.x );
    ImGui::DragFloat2( "Rect max", &itemRectMax.x );

  }
  ImGui::Checkbox( "Mouse hovered over window", &mMouseHoveredOverWindow );
  ImGui::DragFloat2( "Mouse NDC", mMouserCursorNDC.data() );
  ImGui::Text( "Level load errors", mLevelLoadErrors.mMessage );
  ImGui::Checkbox( "Draw to screen", &mDrawDirectlyToScreen );
  ImGui::Checkbox( "Grabbing Input", &mIsGrabbingInput );
  mServerData->DebugImgui();
  mScriptRoot->DebugImgui( errors );
  TAC_HANDLE_ERROR( errors );
  ImguiCreatePlayerPopup( errors );
  TAC_HANDLE_ERROR( errors );
  if( ImGui::CollapsingHeader( "Users" ) )
  {
    ImGui::Indent();
    OnDestruct( ImGui::Unindent() );
    for( int iUser = 0; iUser < mUsers.size(); ++iUser )
    {
      auto user = mUsers[ iUser ];
      TacString userHeader = va( "User %i: %s", iUser, user->mName.c_str() );
      if( !ImGui::CollapsingHeader( userHeader.c_str() ) )
        continue;
      ImGui::Indent();
      OnDestruct( ImGui::Unindent() );
      user->DebugImgui();
    }
  }
  TacLanguageDebugImgui( "Default Language", &mLanguage );
  ImGui::ColorEdit4( "Clear color", &mClearColor.x );
  mUIRoot->DebugImgui();
  ImGui::DragFloat( "Splash alpha", &mSplashAlpha, 0.1f, 0.0f, 1.0f );
  #endif
}
void TacGhost::Draw( TacErrors& errors )
{
  TacWorld* world = mServerData->mWorld;
  TacFontStuff* fontStuff = mShell->mFontStuff;
  TacGraphics* graphics = TacGraphics::GetSystem( world );

  TacRenderer::Instance->DebugBegin( "Draw world" );
  OnDestruct( TacRenderer::Instance->DebugEnd() );
  //TacTexture* fboTexture = mDrawTexture;
  //TacDepthBuffer* fboDepth = mDrawDepthBuffer;
  TacTexture* fboTexture = mRenderView->mFramebuffer;
  TacDepthBuffer* fboDepth = mRenderView->mFramebufferDepth;

  const float aspect = fboTexture->GetAspect();
  //TacRenderer::Instance->ClearColor( fboTexture, mClearColor );
  //TacRenderer::Instance->ClearDepthStencil( fboDepth, true, 1.0f, false, 0 );
  //TacRenderer::Instance->SetRenderTarget( fboTexture, fboDepth );


  //TacRenderer::Instance->SetViewport(
  //  0, // x rel bot left
  //  0, // y rel bot left
  //  ( float )fboTexture->myImage.mWidth, // width increasing right
  //  ( float )fboTexture->myImage.mHeight ); // height increasing up


  v3 camPos = {};
  if( !mUsers.empty() )
  {
    for( auto user : mUsers )
      camPos += user->mPlayer->mCameraPos;
    camPos /= ( float )mUsers.size();
  }

  v3 camViewDir( 0, 0, -1 );
  v3 camR( 1, 0, 0 );
  v3 camU( 0, 1, 0 );
  float farPlane = 10000.0f;
  float nearPlane = 0.1f;
  float fovYRad = 100.0f * ( 3.14f / 180.0f );
  float projA;
  float projB;
  TacRenderer::Instance->GetPerspectiveProjectionAB( farPlane, nearPlane, projA, projB );

  auto world_to_view = M4View( camPos, camViewDir, camR, camU );
  auto view_to_clip = M4ProjPerspective( projA, projB, fovYRad, aspect );

  //TacInvalidCodePath;
  //TacRenderer::Instance->SetBlendState( nullptr );
  //TacRenderer::Instance->SetScissorRect( 0, 0, ( float )fboTexture->mWidth(), ( float )fboTexture->mHeight() );

  //TacRenderer::Instance->DebugDraw(
  //  world_to_view,
  //  view_to_clip,
  //  fboTexture,
  //  fboDepth,
  //  graphics->mDebugDrawVerts,
  //  errors );
  //if( errors.size() )
  //  return;
  //graphics->mDebugDrawVerts.clear();



  //TacRenderer::Instance->SetScissorRect(
  //  0,
  //  0,
  //  ( float )fboTexture->myImage.mWidth,
  //  ( float )fboTexture->myImage.mHeight );

  if( mSplashAlpha )
  {
    TacInvalidCodePath;
    //v4 color( 0, 0, 0, mSplashAlpha );
    //m4 world = m4::Identity();
    //m4 view = m4::Identity();
    //m4 proj = m4::Identity();
    //TacRenderer::Instance->DebugBegin( "Splash background" );
    //OnDestruct( TacRenderer::Instance->DebugEnd() );
    //TacRenderer::Instance->SetBlendState( TacRenderer::Instance->mAlphaBlendState );
    //TacRenderer::Instance->SetActiveShader( TacRenderer::Instance->m2DShader );
    //TacRenderer::Instance->SetRasterizerState( TacRenderer::Instance->mRasterizerStateNoCull );
    //TacRenderer::Instance->SetVertexFormat( TacRenderer::Instance->mDefaultVertex2DFormat );
    //TacRenderer::Instance->SetDepthState( TacRenderer::Instance->mNoDepthReadOrWrite );
    //TacRenderer::Instance->SetTexture( "atlas", TacRenderer::Instance->m1x1White );
    //TacRenderer::Instance->SetSamplerState( "linearSampler", TacRenderer::Instance->mSamplerStateLinearWrap );
    //TacRenderer::Instance->SetPrimitiveTopology( TacPrimitive::TriangleList );
    //TacRenderer::Instance->SetVertexBuffer( TacRenderer::Instance->m2DNDCQuadVB );
    //TacRenderer::Instance->SetIndexBuffer( TacRenderer::Instance->m2DNDCQuadIB );
    //TacRenderer::Instance->SendUniform( "Color", color.data() );
    //TacRenderer::Instance->SendUniform( "World", world.data() );
    //TacRenderer::Instance->SendUniform( "View", view.data() );
    //TacRenderer::Instance->SendUniform( "Projection", proj.data() );
    //TacRenderer::Instance->Apply();
    //TacRenderer::Instance->DrawIndexed( TacRenderer::Instance->m2DNDCQuadIB->indexCount, 0, 0 );
  }
  //if( mDrawText )
  //{
  //  mUIRoot->Render( errors );
  //}
}
void TacGhost::PopulateWorldInitial()
{
  TacWorld* world = mServerData->mWorld;
  TacPhysics* physics = TacPhysics::GetSystem( world );
  TacString levelpath = "mylevel.txt";
  std::ifstream ifs( levelpath.c_str() );
  if( !ifs.is_open() )
  {
    mLevelLoadErrors = "failed to open " + levelpath;
    return;
  }

  //auto meta = TacMeta::GetInstance();
  //auto terrain = new TacTerrain();
  //auto metaTerrain = meta->GetType( TacStringify( TacTerrain ) );
  //meta->Load( ifs, metaTerrain, terrain, mLevelLoadErrors );
  //if( mLevelLoadErrors.size() )
  //  return;
  //physics->mTerrains.insert( terrain );
}
bool TacGhost::CanDrawImgui()
{
  return !mDrawDirectlyToScreen;
}
//extern "C" TAC_EXPORT TacSoul* TAC_GHOST_CREATE( TacShell* shell, TacErrors& errors )
//{
//  return new TacGhost( shell, errors );
//}
//static_assert( std::is_same< decltype( TAC_GHOST_CREATE ), TacGhostCreateFn > ::value, "" );
