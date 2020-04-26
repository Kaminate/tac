#include "src/common/graphics/imgui/tacImGui.h"
#include "src/common/graphics/tacUI.h"
#include "src/common/math/tacMath.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacJobQueue.h"
#include "src/common/tacJson.h"
#include "src/common/tacLog.h"
#include "src/common/tacMemory.h"
#include "src/common/tacNet.h"
#include "src/common/tacOS.h"
#include "src/common/tacSettings.h"
#include "src/common/tacShell.h"
#include "src/common/tacTime.h"
#include "src/common/thirdparty/stb_image.h"
#include "src/space/tacGhost.h"
#include "src/space/graphics/tacGraphics.h"
#include "src/space/tacScriptgameclient.h"
#include "src/space/tacServer.h"
#include "src/space/tacWorld.h"
#include <cstdlib> // itoa

namespace Tac
{


const String defaultHostname = "tac.nate.rocks";
const uint16_t defaultPort = 8081;

v4 colorText = v4( 202, 234, 241, 255 ) / 255.0f;

//ScriptFader::ScriptFader()
//{
//  mName = "Text fader";
//  mShouldFade = true;
//  mPreFadeSec = 0;
//  mPostFadeSec = 0;
//  mFadeSecTotal = 0.5f;
//  mFadeSecElapsed = 0;
//}
//void ScriptFader::DebugImgui( Errors& errors )
//{
//  ImGui::DragFloat( "Pre fade sec", &mPreFadeSec );
//  ImGui::DragFloat( "Post fade sec", &mPostFadeSec );
//  ImGui::DragFloat( "Fade sec total", &mFadeSecTotal );
//  ImGui::DragFloat( "Fade sec elapsed", &mFadeSecElapsed );
//  ImGui::DragFloat( "Fade alpha", mValue );
//  ImGui::Checkbox( "should fade in", &mShouldFade );
//}
//void ScriptFader::SetAlpha( float alpha )
//{
//  *mValue = alpha;
//}
//void ScriptFader::Update( float seconds, Errors& errors )
//{
//  TAC_TIMELINE_BEGIN;
//  mFadeSecElapsed = 0;
//  SetAlpha( mValueInitial );
//
//  if( mShouldFade )
//    SetNextKeyDelay( mPreFadeSec );
//
//  TAC_TIMELINE_KEYFRAME;
//  if( mShouldFade )
//  {
//    mFadeSecElapsed += seconds;
//    float t = mFadeSecElapsed / mFadeSecTotal;
//    float alpha = Saturate( Lerp( mValueInitial, mValueFinal, t ) );
//    SetAlpha( alpha );
//    if( mFadeSecElapsed < mFadeSecTotal )
//      return;
//  }
//  TAC_TIMELINE_KEYFRAME;
//  SetAlpha( mValueFinal );
//  if( mShouldFade )
//    SetNextKeyDelay( mPostFadeSec );
//  TAC_TIMELINE_END
//}

ScriptGameClient::ScriptGameClient()
{
  mName = "Game Client";
}
void ScriptGameClient::Update( float seconds, Errors& errors )
{
  auto shell = Shell::Instance;
  TAC_TIMELINE_BEGIN;

  auto scriptMatchmaker = new ScriptMatchmaker();
  mScriptRoot->AddChild( scriptMatchmaker );

  auto scriptSplash = new ScriptSplash();
  mScriptRoot->AddChild( scriptSplash );

  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  return;
  TAC_TIMELINE_END;
}
void ScriptGameClient::DebugImgui( Errors& errors )
{
}

ScriptSplash::ScriptSplash()
{
  mName = "Splash";
  mFullyVisibleSec = 1.5f;
  mFadeSecTotal = 0.5f;
  mSkipSplashScreen = false;
}
ScriptSplash::~ScriptSplash()
{
  //mScriptRoot->AddChild( new ScriptMainMenu() );
  mScriptRoot->AddChild( new ScriptMainMenu2() );
}
void ScriptSplash::Update( float seconds, Errors& errors )
{
  Ghost* ghost = mScriptRoot->mGhost;
  ;
  //UIRoot* uiRoot = ghost->mUIRoot;


  TAC_TIMELINE_BEGIN;

  //auto ogroot = uiRoot->mHierarchyRoot;
  //ogroot->mDebugName = "og root";

  //auto child1 = ogroot->Split( UISplit::Before, UILayoutType::Vertical );
  //child1->mDebugName = "child1";

  //auto child2 =ogroot->Split( UISplit::After, UILayoutType::Vertical );
  //child2->mDebugName = "child2";

  //auto child3 = ogroot->Split( UISplit::Before, UILayoutType::Horizontal );
  //child3->mDebugName = "child3";

  //auto child4 = ogroot->Split( UISplit::After, UILayoutType::Horizontal );
  //child4->mDebugName = "child4";

  //auto vis = new UIHierarchyVisualText;
  //vis->mUITextData.mUtf8 = "Moachers";
  //uiRoot->mHierarchyRoot->SetVisual( vis );
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_END;
}
void ScriptSplash::DebugImgui( Errors& errors )
{
  //ImGui::DragFloat( "Fully visible sec", &mFullyVisibleSec );
  //ImGui::DragFloat( "Fade sec total", &mFadeSecTotal );
  //ImGui::Checkbox( "Skip splash screen", &mSkipSplashScreen );
}

ScriptMatchmaker::ScriptMatchmaker()
{
  mName = scriptMatchmakerName;
  mPrintHTTPRequest = false;
  mShouldSpamServer = false;
  mTryAutoConnect = false;
  mShouldLog = false;
  mPort = 0;
  mConnectionAttemptStartSeconds = 0;
}
void ScriptMatchmaker::OnScriptGameConnectionClosed( Socket* socket )
{
  Log( "on script game connection closed" );
  mSocket = nullptr;
  mPretendWebsocketHandshakeDone = false;
  mLine = 0;
  mScriptRoot->OnMsg( scriptMsgDisconnect );
}
void ScriptMatchmaker::OnScriptGameMessage( Socket* socket, void* bytes, int byteCount )
{
  if( mLogReceivedMessages )
    Log( String( ( const char* )bytes, byteCount ) );
}
void ScriptMatchmaker::PokeServer( Errors& errors )
{
  TAC_ASSERT( mSocket );
  if( !mSocket->mTCPIsConnected )
    return;
  auto shell = Shell::Instance;
  String s =
    "ScriptGameClient messsage: elapsed time is " +
    FormatFrameTime( shell->mElapsedSeconds );
  Json json;
  json[ "name" ] = "Ping";
  Json& args = json[ "args" ];
  args.mType = JsonType::Array;
  args.mElements.push_back( new Json( s ) );

  String toSend = json.Stringify();
  mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
}
void ScriptMatchmaker::ClearServerLog( Errors& errors )
{
  TAC_ASSERT( mSocket );
  if( !mSocket->mTCPIsConnected )
    return;
  ;
  Json json;
  json[ "name" ] = "clear console";
  String toSend = json.Stringify();
  mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
}
void ScriptMatchmaker::Log( const String& text )
{
  if( !mShouldLog )
    return;
  auto log = Shell::Instance->mLog;
  if( !log )
    return;
  log->Push( "ScriptGameClient: " + text );
}
void ScriptMatchmaker::TryConnect()
{
  mConnectionErrors.clear();
  if( mSocket->mTCPIsConnected )
    return;
  mSocket->TCPTryConnect( mHostname, mPort, mConnectionErrors );
}
void ScriptMatchmaker::Update( float seconds, Errors& errors )
{
  Settings* settings = Shell::Instance->mSettings;
  TAC_TIMELINE_BEGIN;
  mSocket = Net::Instance->CreateSocket( "Matchmaking socket", AddressFamily::IPv4, SocketType::TCP, errors );
  TAC_HANDLE_ERROR( errors );

  auto tCPOnMessage = []( void* userData, Socket* socket, void* bytes, int byteCount )
  {
    ( ( ScriptMatchmaker* )userData )->OnScriptGameMessage( socket, bytes, byteCount );
  };
  SocketCallbackDataMessage socketCallbackDataMessage;
  socketCallbackDataMessage.mCallback = tCPOnMessage;
  socketCallbackDataMessage.mUserData = this;
  mSocket->mTCPOnMessage.push_back( socketCallbackDataMessage );

  auto tcpOnConnectionClosed = []( void* userData, Socket* socket )
  {
    ( ( ScriptMatchmaker* )userData )->OnScriptGameConnectionClosed( socket );
  };
  SocketCallbackData socketCallbackData;
  socketCallbackData.mCallback = tcpOnConnectionClosed;
  socketCallbackData.mUserData = this;
  mSocket->mTCPOnConnectionClosed.push_back( socketCallbackData );

  String hostname = settings->GetString( nullptr, { "hostname" }, defaultHostname, errors );
  mPort = ( uint16_t )settings->GetNumber( nullptr, { "port" }, ( JsonNumber )defaultPort, errors );

  mConnectionAttemptStartSeconds = Shell::Instance->mElapsedSeconds;
  String text = "Attempting to connect to " + mHostname + ":" + ToString( mPort );
  Log( text );
  TAC_TIMELINE_KEYFRAME;
  SetNextKeyDelay( 1.0f );
  TAC_TIMELINE_KEYFRAME;


  if( mTryAutoConnect )
    TryConnect();
  if( !mSocket->mTCPIsConnected )
    return;
  String text = "Connected to " + mHostname + ":" + ToString( mPort );
  Log( text );
  TAC_TIMELINE_KEYFRAME;
  HTTPRequest httpRequest;
  auto websocketKey = GenerateSecWebsocketKey();
  httpRequest.FormatRequestWebsocket( "/game", mHostname, websocketKey );
  if( mPrintHTTPRequest )
    std::cout << httpRequest.ToString() << std::endl;
  mSocket->Send( httpRequest, errors );
  TAC_HANDLE_ERROR( errors );
  mPretendWebsocketHandshakeDone = true;
  mSocket->mRequiresWebsocketFrame = true;
  mSocket->mKeepaliveOverride.mUserData = this;
  mSocket->mKeepaliveOverride.mCallback = []( void* userData, Socket* socket )
  {
    auto* scriptMatchmaker = ( ScriptMatchmaker* )userData;
    Errors errors; // ???
    scriptMatchmaker->PokeServer( errors );
  };

  mScriptRoot->OnMsg( scriptMsgConnect );


  TAC_TIMELINE_KEYFRAME;
  if( mShouldSpamServer )
    PokeServer( errors );
  return;
  TAC_TIMELINE_END;
}
void ScriptMatchmaker::DebugImgui( Errors& errors )
{
  //ImGui::Checkbox( "auto-spam server", &mShouldSpamServer );
  //ImGui::Checkbox( "Log received messages", &mLogReceivedMessages );
  //ImGui::Checkbox( "Try auto connect", &mTryAutoConnect );


  //if( mSocket )
  //{
  //  if( ImGui::Button( "Poke server" ) )
  //    PokeServer( errors );
  //  if( ImGui::Button( "Clear server log" ) )
  //    ClearServerLog( errors );
  //  if( !mSocket->mTCPIsConnected && !mTryAutoConnect && ImGui::Button( "Try Connect" ) )
  //  {
  //    mSocket->TCPTryConnect( mHostname, mPort, errors );
  //    if( errors )
  //      return;
  //  }
  //  mSocket->DebugImgui();
  //}
}

ScriptMainMenu::ScriptMainMenu()
{
  mName = "Main Menu";
  mCreateGraveStoryButton = true;
  mCreatePressStartButton = false;
  Render::DestroyTexture(mPower, TAC_STACK_FRAME);
}
void ScriptMainMenu::AddCallbackConnect()
{
  AddScriptCallback( this, []( ScriptCallbackData* scriptCallbackData, const ScriptMsg* scriptMsg )
  {
    if( scriptMsg->mType != scriptMsgConnect )
      return;
    auto* scriptMainMenu = ( ScriptMainMenu* )scriptCallbackData->mUserData;
    scriptCallbackData->mRequestDeletion = true;
    UITextData uiTextData;
    uiTextData.mUtf8 = "Status: Connected";
    scriptMainMenu->mUITextServerConnectionStatus->SetText( uiTextData );
    scriptMainMenu->AddCallbackDisconnect();

    UIButtonCallback buttonCallback;
    buttonCallback.mUserData = scriptMainMenu;
    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
    {
      auto* scriptMainMenu = ( ScriptMainMenu* )userData;
      auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );
      scriptMatchmaker->mSocket->mRequestDeletion = true;
    };
    scriptMainMenu->mUITextDisconnectFromServer->mButtonCallbacks.push_back( buttonCallback );

    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
    {
      auto* scriptMainMenu = ( ScriptMainMenu* )userData;
      auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );

      Json json;
      json[ "name" ] = "create room";
      String toSend = json.Stringify();
      Socket* socket = scriptMatchmaker->mSocket;
      socket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
    };
    //scriptMainMenu->mUITextCreateRoom->mButtonCallbacks.push_back( buttonCallback );

    for( UIText* uiText : {
      scriptMainMenu->mUITextDisconnectFromServer
      //scriptMainMenu->mUITextCreateRoom
      } )
    {
      UITextData uiTextData = *uiText->GetUITextData();
      uiTextData.mColor = colorMagenta;
      uiText->SetText( uiTextData, false );
    }
  } );
}
void ScriptMainMenu::AddCallbackDisconnect()
{
  AddScriptCallback( this, []( ScriptCallbackData* scriptCallbackData, const ScriptMsg* scriptMsg )
  {
    if( scriptMsg->mType != scriptMsgDisconnect )
      return;
    auto* scriptMainMenu = ( ScriptMainMenu* )scriptCallbackData->mUserData;
    scriptCallbackData->mRequestDeletion = true;
    UIText* uiText = scriptMainMenu->mUITextServerConnectionStatus;
    UITextData uiTextData = *uiText->GetUITextData();
    uiTextData.mUtf8 = "Status: Disconnected";
    uiText->SetText( uiTextData );
    scriptMainMenu->AddCallbackConnect();


    for( UIText* uiText : {
      scriptMainMenu->mUITextDisconnectFromServer
      //scriptMainMenu->mUITextCreateRoom
      } )
    {
      UITextData uiTextData = *uiText->GetUITextData();
      uiTextData.mColor = colorGrey;
      uiText->SetText( uiTextData );
      uiText->mButtonCallbacks.clear();
    }
  } );
}
void ScriptMainMenu::Update( float seconds, Errors& errors )
{
  Ghost* ghost = mScriptRoot->mGhost;
  ;
  //UIRoot* uiRoot = ghost->mUIRoot;
  auto* scriptMatchmaker = ( ScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );
  ServerData* serverData = ghost->mServerData;
  World* world = serverData->mWorld;
  Graphics* graphics = Graphics::GetSystem( world );
  float boxWidth = 5;
  if( false )
  {
    //DebugDrawAABB debugDrawAABB = DebugDrawAABB::FromPosExtents( v3( 0, 0, boxWidth / 2 ), v3( 1, 1, 1 ) * boxWidth );
    //graphics->DebugDrawAABB( debugDrawAABB );
  }


  if( !mPower.IsValid() )
  {
    // TODO: use the asset manager to load this shit async
    auto memory = TemporaryMemoryFromFile( "assets/power.png", errors );
    TAC_HANDLE_ERROR( errors );

    int x;
    int y;
    int previousChannelCount;

    // rgba
    stbi_uc* loaded = stbi_load_from_memory(
      ( const stbi_uc* )memory.data(),
      ( int )memory.size(),
      &x,
      &y,
      &previousChannelCount,
      4 );
    TAC_ON_DESTRUCT( stbi_image_free( loaded ) );

    stbi_uc* l = loaded;
    for( int i = 0; i < y; ++i )
    {
      for( int j = 0; j < x; ++j )
      {
        uint8_t* r = l++;
        uint8_t* g = l++;
        uint8_t* b = l++;
        uint8_t* a = l++;
        float percent = *a / 255.0f;

        *r = ( uint8_t )( *r * percent );
        *g = ( uint8_t )( *g * percent );
        *b = ( uint8_t )( *b * percent );
      }
      std::cout << std::endl;
    }

    Image image;
    //image.mData = loaded;
    image.mFormat.mElementCount = 4;
    image.mFormat.mPerElementByteCount = 1;
    image.mFormat.mPerElementDataType = GraphicsType::unorm;
    image.mWidth = x;
    image.mHeight = y;

    Render::CommandDataCreateTexture textureData;
    textureData.mAccess = Access::Default;
    textureData.mBinding = Binding::ShaderResource;
    textureData.mImage = image;
    textureData.mPitch = image.mFormat.mElementCount * image.mFormat.mPerElementByteCount * image.mWidth;
    mPower = Render::CreateTexture( "power", textureData, TAC_STACK_FRAME );
    TAC_HANDLE_ERROR( errors );
  }

  float dotPeriodSeconds = 1;
  if( !scriptMatchmaker->mPretendWebsocketHandshakeDone && mUITextServerConnectionStatus )
  {
    String utf8 = "Trying to connect";
    int maxDotCount = 3;
    double elapsedSeconds = Shell::Instance->mElapsedSeconds - scriptMatchmaker->mConnectionAttemptStartSeconds;
    double partialDotSeconds = std::fmod( elapsedSeconds, ( double )( ( maxDotCount + 1 ) * dotPeriodSeconds ) );
    for( int i = 0; i < int( partialDotSeconds / dotPeriodSeconds ); ++i )
      utf8 += '.';
    UITextData uiTextData;
    uiTextData.mColor = colorText;
    uiTextData.mUtf8 = utf8;
    mUITextServerConnectionStatus->SetText( uiTextData, false );
  }

  if( mUITextPressStart )
  {
    double pressStartPeriod = 2.0f;
    double s = std::fmod( Shell::Instance->mElapsedSeconds, pressStartPeriod );
    bool b = s > pressStartPeriod * 0.5f;
    if( mPressStart != b )
    {
      mPressStart = b;
      String utf8 = b ? "Press Start" : "";
      UITextData uiTextData;
      uiTextData.mUtf8 = utf8;
      mUITextPressStart->SetText( uiTextData );
    }
  }




  TAC_TIMELINE_BEGIN;


  float menuPositionX = 200;
  UIAnchorHorizontal menuAnchorHorizontal = UIAnchorHorizontal::Left;
  UIAnchorVertical menuAnchorVertical = UIAnchorVertical::Center;

  double timelineSeconds = Shell::Instance->mElapsedSeconds;

  //auto createGameTitle = [ = ]()
  //{
  //  if( !mCreateGraveStoryButton )
  //    return;
  //  UILayout* uiMenu = uiRoot->AddMenu( "Game title layout" );
  //  uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
  //  uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
  //  uiMenu->mUiWidth = 0;
  //  uiMenu->mHeightTarget = 0;
  //  //uiMenu->mPosition.x = menuPositionX;
  //  //uiMenu->mPosition.y = 200;

  //  auto* uiText = uiMenu->Add< UIText >( "Game title text" );
  //  UITextData uiTextData;
  //  uiTextData.mColor = colorText;
  //  uiTextData.mUtf8 = "GRAVE STORY";
  //  uiTextData.mFontSize = 60;
  //  uiText->SetText( uiTextData );
  //  uiText->GoNuts();
  //};
  //mTimeline.Add( new TimelineOnce( timelineSeconds, createGameTitle ) );
  //timelineSeconds += 0.2f;

  //auto createMainMenu = [ = ]()->void
  //{
  //  UILayout* uiMenu = uiRoot->AddMenu( "main menu layout" );
  //  uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
  //  uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
  //  uiMenu->mUiWidth = 300;
  //  uiMenu->mHeightTarget = 200;
  //  uiMenu->mMenuPadding = 8;
  //  //uiMenu->mPosition.x = 200;
  //  //uiMenu->mPosition.y = 0;
  //  uiMenu->mColor = v4( v3( 1, 1, 1 ) * 52.0f, 58.0f ) / 255.0f;

  //  // Server host / port
  //  {
  //    String serverDispalyName =
  //      scriptMatchmaker->mHostname +
  //      String( ":" ) +
  //      ToString( scriptMatchmaker->mPort );

  //    UIText* uiText = uiMenu->Add< UIText >( "server display name" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    UITextData uiTextData;
  //    uiTextData.mColor = colorText;
  //    uiTextData.mUtf8 = "Server: " + serverDispalyName;
  //    uiText->SetText( uiTextData );
  //  }

  //  // server connection status
  //  {
  //    auto* uiText = uiMenu->Add< UIText >( "server connection status" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    mUITextServerConnectionStatus = uiText;
  //    UITextData uiTextData;
  //    uiTextData.mColor = colorText;
  //    if( scriptMatchmaker->mPretendWebsocketHandshakeDone )
  //    {
  //      uiTextData.mUtf8 = "Status: Connected";
  //      AddCallbackDisconnect();
  //    }
  //    else
  //    {
  //      AddCallbackConnect();
  //    }
  //    uiText->SetText( uiTextData );
  //  }

  //  // server autoconnect
  //  {
  //    bool nestedLayouts = true;
  //    if( nestedLayouts )
  //    {

  //      float sideLength = 20;
  //      UILayout* uiLayout = uiMenu->Add< UILayout >( "server autoconnect layout" );
  //      uiLayout->mUiWidth = sideLength;
  //      uiLayout->mHeightTarget = sideLength;
  //      uiLayout->mUILayoutType = UILayoutType::Horizontal;
  //      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
  //      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

  //      UILayout* parentUiLayout = uiLayout;

  //      uiLayout = parentUiLayout->Add< UILayout >( "server autoconnect left image" );
  //      uiLayout->mColor = v4( 1, 1, 0, 1 );
  //      uiLayout->mUiWidth = sideLength;
  //      uiLayout->mHeightTarget = sideLength;
  //      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
  //      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

  //      UIText* uiText = parentUiLayout->Add< UIText >( "server autoconnect text" );
  //      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //      UITextData uiTextData;
  //      uiTextData.mUtf8 = "Server Autoconnect: On";
  //      uiTextData.mColor = colorMagenta;
  //      UIButtonCallback buttonCallback;
  //      buttonCallback.mUserData = this;
  //      buttonCallback.mUserCallback = []( void* userData, Errors& errors )
  //      {
  //        auto* scriptMainMenu = ( ScriptMainMenu* )userData;
  //        ScriptRoot* scriptRoot = scriptMainMenu->mScriptRoot;
  //        auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptRoot->GetThread( scriptMatchmakerName );
  //        scriptMatchmaker->mTryAutoConnect = !scriptMatchmaker->mTryAutoConnect;
  //        String text = "Server Autoconnect: ";
  //        if( scriptMatchmaker->mTryAutoConnect )
  //          text += "On";
  //        else
  //          text += "Off";
  //        UITextData uiTextData;
  //        uiTextData.mUtf8 = text;
  //        uiTextData.mColor = colorMagenta;
  //        scriptMainMenu->mUITextServerAutoconnect->SetText( uiTextData, false );
  //      };
  //      uiText->mButtonCallbacks.push_back( buttonCallback );
  //      mUITextServerAutoconnect = uiText;
  //      uiText->SetText( uiTextData );

  //      uiLayout = parentUiLayout->Add< UILayout >( "server autoconnect right image" );
  //      uiLayout->mColor = v4( 0, 1, 1, 1 );
  //      uiLayout->mUiWidth = sideLength;
  //      uiLayout->mHeightTarget = sideLength;
  //      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
  //      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

  //    }



  //  }

  //  // disconenct from server
  //  {
  //    auto* uiText = uiMenu->Add< UIText >( "disconnect from server text" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    UITextData uiTextData;
  //    uiTextData.mUtf8 = "Disconnect From Server";
  //    uiTextData.mColor = colorGrey;
  //    uiText->SetText( uiTextData );
  //    mUITextDisconnectFromServer = uiText;
  //  }

  //  // create room
  //  {
  //    auto* uiText = uiMenu->Add< UIText >( "create room button" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    UITextData uiTextData;
  //    uiTextData.mUtf8 = "Create Room";
  //    uiTextData.mColor = colorGrey;
  //    uiText->SetText( uiTextData );
  //  }

  //  // controllers
  //  {
  //    auto* uiText = uiMenu->Add< UIText >( "controllers text" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    UITextData uiTextData;
  //    uiTextData.mUtf8 = "Controllers";
  //    uiTextData.mColor = colorMagenta;
  //    uiText->SetText( uiTextData );
  //    UIButtonCallback buttonCallback;
  //    buttonCallback.mUserData = this;
  //    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
  //    {
  //      auto* scriptMainMenu = ( ScriptMainMenu* )userData;
  //      static int ihi;
  //      std::cout << "hi" << " " << ihi++ << std::endl;
  //    };
  //    uiText->mButtonCallbacks.push_back( buttonCallback );
  //  }

  //  // Exit game
  //  {
  //    auto* layout = uiMenu->Add< UILayout >( "exit game layout" );
  //    layout->mUILayoutType = UILayoutType::Horizontal;
  //    layout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
  //    layout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;
  //    layout->mHeightTarget = 100.0f;
  //    layout->mColor = { 0, 0, 0, 0 };
  //    layout->mExpandWidth = true;
  //    float powerSize = 20;
  //    auto* power = layout->Add< UILayout >( "power icon" );
  //    power->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
  //    power->mAnchor.mAnchorVertical = UIAnchorVertical::Center;
  //    power->mColor = {
  //      186 / 255.0f,
  //      164 / 255.0f,
  //      236 / 255.0f,
  //      1 };
  //    power->mHeightTarget = powerSize;
  //    power->mUiWidth = powerSize;
  //    power->mTexture = mPower;

  //    auto* uiText = layout->Add< UIText >( "exit game text" );
  //    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
  //    UITextData uiTextData;
  //    uiTextData.mUtf8 = "Exit Game";
  //    uiTextData.mColor = colorMagenta;
  //    uiText->SetText( uiTextData );
  //    UIButtonCallback buttonCallback;
  //    buttonCallback.mUserData = this;
  //    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
  //    {
  //      OS::mShouldStopRunning = true;
  //    };
  //    uiText->mButtonCallbacks.push_back( buttonCallback );
  //  }

  //  mMenu = uiMenu;
  //};
  //mTimeline.Add( new TimelineOnce( timelineSeconds, createMainMenu ) );
  //timelineSeconds += 0.2f;

  //auto createPressStart = [ = ]()
  //{
  //  if( !mCreatePressStartButton )
  //    return;
  //  UILayout* uiMenu = uiRoot->AddMenu( "press start layout" );
  //  uiMenu->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Right;
  //  uiMenu->mAnchor.mAnchorVertical = UIAnchorVertical::Bottom;
  //  //uiMenu->mPosition = { -100, 100 };
  //  uiMenu->mColor = {};
  //  auto* uiText = uiMenu->Add< UIText >( "press start text" );
  //  mUITextPressStart = uiText;
  //};
  //mTimeline.Add( new TimelineOnce( timelineSeconds, createPressStart ) );
  //timelineSeconds += 0.2f;



  TAC_TIMELINE_KEYFRAME;


  mTimeline.Update( Shell::Instance->mElapsedSeconds, errors );
  TAC_HANDLE_ERROR( errors );

  return;

  TAC_TIMELINE_END;
}
void ScriptMainMenu::DebugImgui( Errors& errors )
{

}

TimelineAction::~TimelineAction() = default;

void TimelineAction::Begin()
{

}
void TimelineAction::End()
{

}
void TimelineAction::Update( float percent )
{

}

Timeline::~Timeline()
{
  for( TimelineAction* timelineAction : mTimelineActions )
    delete timelineAction;
}
void Timeline::Update( double time, Errors& errors )
{
  for( TimelineAction* timelineAction : mTimelineActions )
  {
    if( time < timelineAction->mTimeBegin )
      continue;
    if( timelineAction->mPlayedEnd )
      continue;

    if( !timelineAction->mPlayedBegin )
    {
      timelineAction->Begin();
      timelineAction->mPlayedBegin = true;
    }

    float percent = ( float )( ( time - timelineAction->mTimeBegin ) / ( timelineAction->mTimeEnd - timelineAction->mTimeBegin ) );
    percent = Saturate( percent );
    timelineAction->Update( percent );

    if( time > timelineAction->mTimeEnd )
    {
      timelineAction->End();
      timelineAction->mPlayedEnd = true;
    }
  }
}
void Timeline::Add( TimelineAction* timelineAction )
{
  mTimelineActions.push_back( timelineAction );
}



struct ConnectToServerJob : public Job
{
  void Execute() override
  {
    mMatchmaker->TryConnect();
    if( mMatchmaker->mConnectionErrors )
    {
      SetStatus( AsyncLoadStatus::ThreadFailed );
      mErrors = mMatchmaker->mConnectionErrors;
    }
  }
  ScriptMatchmaker* mMatchmaker;
};

ScriptMainMenu2::ScriptMainMenu2()
{
  mName = "Main Menu 2";
}
ScriptMainMenu2::~ScriptMainMenu2()
{
  static int j;
  ++j;
}
void ScriptMainMenu2::RenderMainMenu()
{
  //auto* scriptMatchmaker = ( ScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );

  //Image* image = &mScriptRoot->mGhost->mRenderView->mFramebuffer->myImage;
  //v2 mainMenuSize = { 400, 200 };
  //v2 mainMenuPos = { 100, ( image->mHeight - mainMenuSize.y ) / 2 };

  //ImGuiSetNextWindowPos( mainMenuPos );
  //ImGuiBegin( "Main Menu", mainMenuSize );

  //ImGuiPushFontSize( 70 );
  //ImGuiText( "Gravestory" );
  //ImGuiPopFontSize();

  //if( scriptMatchmaker->mSocket->mTCPIsConnected )
  //{
  //  String serverDispalyName =
  //    scriptMatchmaker->mHostname +
  //    String( ":" ) +
  //    ToString( scriptMatchmaker->mPort );
  //  ImGuiText( "Connected to server: " + serverDispalyName );
  //  if( ImGuiButton( "Disconnect from server" ) )
  //    scriptMatchmaker->mSocket->mRequestDeletion = true;
  //  if( ImGuiButton( "Create room" ) )
  //  {
  //    Json json;
  //    json[ "name" ] = "create room";
  //    String toSend = json.Stringify();
  //    Socket* socket = scriptMatchmaker->mSocket;
  //    socket->Send( ( void* )toSend.data(), ( int )toSend.size(), scriptMatchmaker->mConnectionErrors );
  //  }
  //}
  //else
  //{
  //  ImGuiInputText( "Hostname", scriptMatchmaker->mHostname );
  //  String portString = ToString( scriptMatchmaker->mPort );
  //  if( ImGuiInputText( "Port", portString ) )
  //    scriptMatchmaker->mPort = ( uint16_t )std::atoi( portString.c_str() );
  //  AsyncLoadStatus status = mConnectToServerJob->GetStatus();
  //  if( status == AsyncLoadStatus::ThreadQueued ||
  //    status == AsyncLoadStatus::ThreadRunning )
  //  {
  //    String text = "Connecting to server";
  //    for( int i = 0; i < ( int )Shell::Instance->mElapsedSeconds % 4; ++i )
  //      text += ".";
  //    ImGuiText( text );
  //  }
  //  else
  //  {
  //    if( status == AsyncLoadStatus::ThreadFailed )
  //      ImGuiText( mConnectToServerJob->mErrors.mMessage );
  //    if( ImGuiButton( "Connect to server" ) )
  //    {
  //      JobQueue::Instance->Push( mConnectToServerJob );
  //    }
  //  }
  //}
  //if( ImGuiButton( "Exit Game" ) )
  //{
  //  OS::mShouldStopRunning = true;
  //}
  //ImGuiEnd();
}
void ScriptMainMenu2::Update( float seconds, Errors& errors )
{
  TAC_TIMELINE_BEGIN;

  auto job = new ConnectToServerJob;
  job->mMatchmaker = ( ScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );
  mConnectToServerJob = job;



  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  RenderMainMenu();
  return;// prevent IsComplete
  TAC_TIMELINE_END;
}
}
