#include "tac_ghost.h" // self-inc

#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/graphics/tac_graphics.h"
#include "tac-ecs/net/tac_client.h"
#include "tac-ecs/net/tac_server.h"
#include "tac-ecs/physics/collider/tac_collider.h"
#include "tac-ecs/physics/tac_physics.h"
#include "tac-ecs/player/tac_player.h"
#include "tac-ecs/script/tac_script.h"
#include "tac-ecs/scripts/tac_script_game_client.h"
#include "tac-ecs/world/tac_world.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"

#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-engine-core/shell/tac_shell_time.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"

namespace Tac
{

  // -----------------------------------------------------------------------------------------------

  User::User( StringView name,
              Ghost* ghost,
              Errors& )
  {
    mName = name;
    mGhost = ghost;
    ServerData* serverData { mGhost->mServerData };
    Player* player { serverData->SpawnPlayer() };
    player->mCameraPos = v3( -15, 12, 25 );
    mPlayer = player;
    if( mGhost->mShouldPopulateWorldInitial )
    {
      Entity* entity { serverData->SpawnEntity() };
      entity->mRelativeSpace.mPosition = v3( RandomFloatMinus1To1() * 3.0f,
                                             5.2f,
                                             RandomFloatMinus1To1() * 3.0f );
      entity->AddNewComponent( Collider().GetEntry() );
      player->mEntityUUID = entity->mEntityUUID;
    }
  }

  void User::DebugImgui()
  {
    //if( mPlayer )
    //  mPlayer->DebugImgui();
    //ImGui::InputText( "Name", mName );
  }

  void User::Update( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    if( !mGhost->mIsGrabbingInput )
      return;

    //auto serverData = mGhost->mServerData;
    v2 inputDirection{};
    if( AppKeyboardApi::IsPressed( Key::RightArrow ) ) inputDirection += { 1, 0 };
    if( AppKeyboardApi::IsPressed( Key::UpArrow ) ) inputDirection += { 0, 1 };
    if( AppKeyboardApi::IsPressed( Key::DownArrow ) ) inputDirection += { 0, -1 };
    if( AppKeyboardApi::IsPressed( Key::LeftArrow ) ) inputDirection += { -1, 0 };
    if( inputDirection.Length() )
      inputDirection.Normalize();

    mPlayer->mInputDirection = inputDirection;
    mPlayer->mIsSpaceJustDown = AppKeyboardApi::JustPressed( Key::Spacebar );
  }

  // -----------------------------------------------------------------------------------------------

  Ghost::Ghost()
  {
    //mScriptRoot = TAC_NEW ScriptRoot;
    mServerData = TAC_NEW ServerData;
  }

  Ghost::~Ghost()
  {
    //delete mFBOTexture;
    //delete mFBODepthBuffer;
    for( User* user : mUsers )
      TAC_DELETE user;
    TAC_DELETE mServerData;
    TAC_DELETE mClientData;
  }

  void Ghost::Init( Errors& errors )
  {
    mShouldPopulateWorldInitial = false;
    //mScriptRoot->mGhost = this;
    //int w = shell->mWindowWidth;
    //int h = shell->mWindowHeight;

    //Image image;
    //image.mWidth = w;
    //image.mHeight = h;
    //image.mFormat.mPerElementByteCount = 1;
    //image.mFormat.mElementCount = 4;
    //image.mFormat.mPerElementDataType = GraphicsType::unorm;
    //TextureData textureData;
    //textureData.access = Access::Default;
    //textureData.binding = { Binding::RenderTarget, Binding::ShaderResource };
    //textureData.cpuAccess = {};
    //textureData.mName = "client view fbo";
    //textureData.mFrame = TAC_FRAME;
    //textureData.myImage = image;
    //Renderer::Instance->AddTextureResource( &mFBOTexture, textureData, errors );
    //_HANDLE_ERROR( errors );

    //DepthBufferData depthBufferData;
    //depthBufferData.height = h;
    //depthBufferData.mName = "client view depth buffer";
    //depthBufferData.mFrame = TAC_FRAME;
    //depthBufferData.width = w;
    //Renderer::Instance->AddDepthBuffer( &mFBODepthBuffer, depthBufferData, errors );
    //_HANDLE_ERROR( errors );

    const String serverTypeGameClient { TAC_TYPESAFE_STRINGIFY_TYPE( ScriptGameClient ) };
    const String serverType {
      Shell::sShellSettings.GetChild( "server type" ).GetValueWithFallback( serverTypeGameClient ) };

    ScriptThread* child {};
    if( serverType == serverTypeGameClient )
      child = TAC_NEW ScriptGameClient();
    else
      TAC_ASSERT_INVALID_CODE_PATH;

    //mScriptRoot->AddChild( child );

    if( mShouldPopulateWorldInitial )
      PopulateWorldInitial();

    const char* playerNames[] {
      "Server",
      //"Client",
    };

    for( const char* playerName : playerNames )
    {
      TAC_CALL( AddPlayer( playerName, errors ));
    }

    //mUIRoot->mElapsedSeconds = &GameTimer::GetElapsedTime(); // eww
    //mUIRoot->mGhost = this;
  }

  User* Ghost::AddPlayer( StringView name, Errors& errors )
  {
    auto user{ TAC_NEW User( name, this, errors ) };
    mUsers.push_back( user );
    const String scriptMsgNameUserConnect { "user connect" };
    const ScriptMsg scriptMsg
    {
      .mType { scriptMsgNameUserConnect },
      .mData { user },
    };
    mScriptRoot->OnMsg( &scriptMsg );
    return user;
  }

  void Ghost::ImguiCreatePlayerPopup( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //const char* popupName = "Enter Player Name";
    //const int playerNameBufSize = 100;
    //static String playerName;
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
    //if( errors )
    //  return;
  }

  void Ghost::Update( Errors& errors )
  {

    if( CanDrawImgui() )
    {
      //mMouserCursorNDC.x = ( ( float )Shell::Instance.mMouseRelTopLeftX - mImguiImagePosRelTopLeftX ) / mImguiImageW;
      //mMouserCursorNDC.y = ( ( float )Shell::Instance.mMouseRelTopLeftY - mImguiImagePosRelTopLeftY ) / mImguiImageH;
      //mMouseHoveredOverWindow &= Shell::Instance.mMouseInWindow;
      //mDrawDepthBuffer = mFBODepthBuffer;
      //mDrawTexture = mFBOTexture;
    }
    else
    {
      //InvalidCodePath;
      //Shell::Instance.Renderer::Instance->GetBackbufferColor( &mDrawTexture  );
      //Shell::Instance.Renderer::Instance->GetBackbufferDepth( &mDrawDepthBuffer );
      //mMouserCursorNDC.x = ( float )Shell::Instance.mMouseRelTopLeftX / mDrawTexture->myImage.mWidth;
      //mMouserCursorNDC.y = ( float )Shell::Instance.mMouseRelTopLeftY / mDrawTexture->myImage.mHeight;
      //mMouseHoveredOverWindow = Shell::Instance.mMouseInWindow;
    }
    mMouserCursorNDC.y = 1 - mMouserCursorNDC.y;
    mMouserCursorNDC *= 2;
    mMouserCursorNDC -= v2( 1, 1 );

    for( User* user : mUsers )
      user->Update( errors );

    TAC_CALL( mScriptRoot->Update( TAC_DT, errors ));
    //mUIRoot->Update();
    TAC_CALL( Draw( errors ));

    TAC_CALL( AddMorePlayers( errors ));
  }

  void Ghost::AddMorePlayers( Errors& errors )
  {
    if( IsPartyFull() )
      return;

    Vector< ControllerIndex > claimedControllerIndexes;
    for( User* user : mUsers )
      if( user->mHasControllerIndex )
        claimedControllerIndexes.push_back( user->mControllerIndex );

    TAC_ASSERT( ( int )claimedControllerIndexes.size() < TAC_CONTROLLER_COUNT_MAX );

    for( ControllerIndex controllerIndex{};
         controllerIndex < TAC_CONTROLLER_COUNT_MAX;
         ++controllerIndex )
    {
      if( Contains( claimedControllerIndexes, controllerIndex ) )
        continue;

      Controller* controller { ControllerApi::GetController( controllerIndex ) };
      if( !controller )
        continue;

      if( !ControllerApi::IsButtonJustPressed( ControllerButton::Start, controller ) )
        continue;

      const String playerName { String() + "Player " + ToString( mUsers.size() ) };
      User* user { AddPlayer( playerName, errors ) };
      if( errors )
        return;

      user->mHasControllerIndex = true;
      user->mControllerIndex = controllerIndex;
      if( IsPartyFull() )
        return;
    }

  }

  bool Ghost::IsPartyFull()
  {
    bool result { ( int )mUsers.size() == sPlayerCountMax };
    return result;
  }

  void Ghost::DebugImgui( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
#if COMPILE_PLS
    if( !mIsImGuiVisible )
      return;
    ImGui::PushID( this );
    TAC_ON_DESTRUCT( ImGui::PopID() );
    ImGui::Begin( GetDebugName().c_str(), &mIsImGuiVisible );
    TAC_ON_DESTRUCT( ImGui::End() );
    ImGui::PushItemWidth( 200 );
    TAC_ON_DESTRUCT( ImGui::PopItemWidth() );


    if( CanDrawImgui() )
    {
      float aspect { mFBOTexture->GetAspect() };
      auto w { ( int )ImGui::GetWindowContentRegionWidth() };
      auto h { ( int )( w / aspect ) };
      auto size { ImVec2( ( float )w, ( float )h ) };
      ImGui::Image( mFBOTexture->GetImguiTextureID(), size );
      mMouseHoveredOverWindow = ImGui::IsItemHovered();

      mImguiImageW = ( float )w;
      mImguiImageH = ( float )h;


      ImVec2 itemRectMin { ImGui::GetItemRectMin() };
      ImVec2 itemRectMax { ImGui::GetItemRectMax() };
      ImVec2 itemRectSize { ImGui::GetItemRectSize() };

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
    TAC_CALL( mScriptRoot->DebugImgui( errors ));
    TAC_CALL( ImguiCreatePlayerPopup( errors ));
    if( ImGui::CollapsingHeader( "Users" ) )
    {
      ImGui::Indent();
      TAC_ON_DESTRUCT( ImGui::Unindent() );
      for( int iUser {}; iUser < mUsers.size(); ++iUser )
      {
        auto user = mUsers[ iUser ];
        const String userHeader = String() + "User: " + ToString( iUser ) + ": " user->mName;
        if( !ImGui::CollapsingHeader( userHeader.c_str() ) )
          continue;

        ImGui::Indent();
        TAC_ON_DESTRUCT( ImGui::Unindent() );
        user->DebugImgui();
      }
    }
    LanguageDebugImgui( "Default Language", &mLanguage );
    ImGui::ColorEdit4( "Clear color", &mClearColor.x );
    mUIRoot->DebugImgui();
    ImGui::DragFloat( "Splash alpha", &mSplashAlpha, 0.1f, 0.0f, 1.0f );
#endif
  }

  void Ghost::Draw( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    //World* world { mServerData->mWorld };
    //Graphics* graphics { Graphics::Get( world ) };

    //Renderer::Instance->DebugBegin( "Draw world" );
    //TAC_ON_DESTRUCT( Renderer::Instance->DebugEnd() );
    //Texture* fboTexture { mDrawTexture };
    //DepthBuffer* fboDepth { mDrawDepthBuffer };
    //Texture* fboTexture { mRenderView->mFramebuffer };
    //DepthBuffer* fboDepth { mRenderView->mFramebufferDepth };

    //const float aspect { fboTexture->GetAspect() };
    //Renderer::Instance->ClearColor( fboTexture, mClearColor );
    //Renderer::Instance->ClearDepthStencil( fboDepth, true, 1.0f, false, 0 );
    //Renderer::Instance->SetRenderTarget( fboTexture, fboDepth );


    //Renderer::Instance->SetViewport(
    //  0, // x rel bot left
    //  0, // y rel bot left
    //  ( float )fboTexture->myImage.mWidth, // width increasing right
    //  ( float )fboTexture->myImage.mHeight ); // height increasing up


    v3 camPos  {};
    if( !mUsers.empty() )
    {
      for( User* user : mUsers )
        camPos += user->mPlayer->mCameraPos;
      camPos /= ( float )mUsers.size();
    }

    v3 camViewDir( 0, 0, -1 );
    v3 camR( 1, 0, 0 );
    v3 camU( 0, 1, 0 );
    //float farPlane { 10000.0f };
    //float nearPlane { 0.1f };
    //float fovYRad { 100.0f * ( 3.14f / 180.0f ) };
    //const Render::InProj inProj  { .mNear = nearPlane, .mFar = farPlane };
    //const Render::OutProj outProj { Render::GetPerspectiveProjectionAB( inProj ) };
    //const float projA { outProj.mA };
    //const float projB { outProj.mB };

    const m4 world_to_view { m4::View( camPos, camViewDir, camR, camU ) };
    //auto view_to_clip { M4ProjPerspective( projA, projB, fovYRad, aspect ) };

    //InvalidCodePath;
    //Renderer::Instance->SetBlendState( nullptr );
    //Renderer::Instance->SetScissorRect( 0, 0, ( float )fboTexture->mWidth(), ( float )fboTexture->mHeight() );

    //Renderer::Instance->DebugDraw(
    //  world_to_view,
    //  view_to_clip,
    //  fboTexture,
    //  fboDepth,
    //  graphics->mDebugDrawVerts,
    //  errors );
    //if( errors )
    //  return;
    //graphics->mDebugDrawVerts.clear();



    //Renderer::Instance->SetScissorRect(
    //  0,
    //  0,
    //  ( float )fboTexture->myImage.mWidth,
    //  ( float )fboTexture->myImage.mHeight );

    if( mSplashAlpha )
    {
      TAC_ASSERT_INVALID_CODE_PATH;
      //v4 color( 0, 0, 0, mSplashAlpha );
      //m4 world = m4::Identity();
      //m4 view = m4::Identity();
      //m4 proj = m4::Identity();
      //Renderer::Instance->DebugBegin( "Splash background" );
      //OnDestruct( Renderer::Instance->DebugEnd() );
      //Renderer::Instance->SetBlendState( Renderer::Instance->mAlphaBlendState );
      //Renderer::Instance->SeTiveShader( Renderer::Instance->m2DShader );
      //Renderer::Instance->SetRasterizerState( Renderer::Instance->mRasterizerStateNoCull );
      //Renderer::Instance->SetVertexFormat( Renderer::Instance->mDefaultVertex2DFormat );
      //Renderer::Instance->SetDepthState( Renderer::Instance->mNoDepthReadOrWrite );
      //Renderer::Instance->SetTexture( "atlas", Renderer::Instance->m1x1White );
      //Renderer::Instance->SetSamplerState( "linearSampler", Renderer::Instance->mSamplerStateLinearWrap );
      //Renderer::Instance->SetPrimitiveTopology( Primitive::TriangleList );
      //Renderer::Instance->SetVertexBuffer( Renderer::Instance->m2DNDCQuadVB );
      //Renderer::Instance->SetIndexBuffer( Renderer::Instance->m2DNDCQuadIB );
      //Renderer::Instance->SendUniform( "Color", color.data() );
      //Renderer::Instance->SendUniform( "World", world.data() );
      //Renderer::Instance->SendUniform( "View", view.data() );
      //Renderer::Instance->SendUniform( "Projection", proj.data() );
      //Renderer::Instance->Apply();
      //Renderer::Instance->DrawIndexed( Renderer::Instance->m2DNDCQuadIB->indexCount, 0, 0 );
    }
    //if( mDrawText )
    //{
    //  mUIRoot->Render( errors );
    //}
  }

  void Ghost::PopulateWorldInitial()
  {
    //World* world = mServerData->mWorld;
    //Physics* physics = Physics::GetSystem( world );

    //UTF8Path levelpath { "mylevel.txt" };
    //Errors errors;
    //String mem { FileToString( levelpath, errors ) };
    //if( mem.empty() )
    //{
    //  const String errorMsg { "failed to open " + levelpath.u8string() };
    //  mLevelLoadErrors.Append( errorMsg );
    //  mLevelLoadErrors.Append( TAC_STACK_FRAME );
    //  return;
    //}

    //auto meta { Meta::GetInstance() };
    //auto terrain { new Terrain() };
    //auto metaTerrain { meta->GetType( Stringify( Terrain ) ) };
    //meta->Load( ifs, metaTerrain, terrain, mLevelLoadErrors );
    //if( mLevelLoaderrors )
    //  return;
    //physics->mTerrains.insert( terrain );
  }

  bool Ghost::CanDrawImgui()
  {
    return !mDrawDirectlyToScreen;
  }

  //extern "C" TAC_EXPORT Soul* TAC_GHOST_CREATE( Shell* shell, Errors& errors )
  //{
  //  return new Ghost( shell, errors );
  //}
  //static_assert( std::is_same< decltype( TAC_GHOST_CREATE ), GhostCreateFn > ::value, "" );

} // namespace Tac

