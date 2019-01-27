
#include "common/math/tacMath.h"
#include "common/tacSettings.h"
#include "common/tacShell.h"
#include "common/tacLog.h"
#include "common/tacNet.h"
#include "common/tacJson.h"
#include "common/tacMemory.h"
#include "common/imgui.h"
#include "common/stb_image.h"
#include "common/tacOS.h"
#include "common/tacUI.h"
#include "common/tacTime.h"

#include "space/tacGhost.h"
#include "space/tacserver.h"
#include "space/tacgraphics.h"
#include "space/tacworld.h"
#include "space/tacscriptgameclient.h"

const TacString defaultHostname = "tac.nate.rocks";
const uint16_t defaultPort = 8081;

v4 colorText = v4( 202, 234, 241, 255 ) / 255.0f;

TacScriptFader::TacScriptFader()
{
  mName = "Text fader";
  mShouldFade = true;
  mPreFadeSec = 0;
  mPostFadeSec = 0;
  mFadeSecTotal = 0.5f;
  mFadeSecElapsed = 0;
}
void TacScriptFader::DebugImgui( TacErrors& errors )
{
  ImGui::DragFloat( "Pre fade sec", &mPreFadeSec );
  ImGui::DragFloat( "Post fade sec", &mPostFadeSec );
  ImGui::DragFloat( "Fade sec total", &mFadeSecTotal );
  ImGui::DragFloat( "Fade sec elapsed", &mFadeSecElapsed );
  ImGui::DragFloat( "Fade alpha", mValue );
  ImGui::Checkbox( "should fade in", &mShouldFade );
}
void TacScriptFader::SetAlpha( float alpha )
{
  *mValue = alpha;
}
void TacScriptFader::Update( float seconds, TacErrors& errors )
{
  TAC_TIMELINE_BEGIN;
  mFadeSecElapsed = 0;
  SetAlpha( mValueInitial );

  if( mShouldFade )
    SetNextKeyDelay( mPreFadeSec );

  TAC_TIMELINE_KEYFRAME;
  if( mShouldFade )
  {
    mFadeSecElapsed += seconds;
    float t = mFadeSecElapsed / mFadeSecTotal;
    float alpha = TacSaturate( TacLerp( mValueInitial, mValueFinal, t ) );
    SetAlpha( alpha );
    if( mFadeSecElapsed < mFadeSecTotal )
      return;
  }
  TAC_TIMELINE_KEYFRAME;
  SetAlpha( mValueFinal );
  if( mShouldFade )
    SetNextKeyDelay( mPostFadeSec );
  TAC_TIMELINE_END
}

TacScriptGameClient::TacScriptGameClient()
{
  mName = "Game Client";
}
void TacScriptGameClient::Update( float seconds, TacErrors& errors )
{
  auto shell = mScriptRoot->mGhost->mShell;
  TAC_TIMELINE_BEGIN;

  auto scriptMatchmaker = new TacScriptMatchmaker();
  mScriptRoot->AddChild( scriptMatchmaker );

  auto scriptSplash = new TacScriptSplash();
  mScriptRoot->AddChild( scriptSplash );

  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  return;
  TAC_TIMELINE_END;
}
void TacScriptGameClient::DebugImgui( TacErrors& errors )
{
}

TacScriptSplash::TacScriptSplash()
{
  mName = "Splash";
  mFullyVisibleSec = 1.5f;
  mFadeSecTotal = 0.5f;
  mSkipSplashScreen = true;
}
TacScriptSplash::~TacScriptSplash()
{
  mScriptRoot->AddChild( new TacScriptMainMenu() );
}
void TacScriptSplash::Update( float seconds, TacErrors& errors )
{
  TAC_TIMELINE_BEGIN;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_KEYFRAME;
  TAC_TIMELINE_END;
}
void TacScriptSplash::DebugImgui( TacErrors& errors )
{
  ImGui::DragFloat( "Fully visible sec", &mFullyVisibleSec );
  ImGui::DragFloat( "Fade sec total", &mFadeSecTotal );
  ImGui::Checkbox( "Skip splash screen", &mSkipSplashScreen );
}

TacScriptMatchmaker::TacScriptMatchmaker()
{
  mName = scriptMatchmakerName;
  mPrintHTTPRequest = false;
  mShouldSpamServer = false;
  mTryAutoConnect = true;
  mShouldLog = false;
  mPort = 0;
  mConnectionAttemptStartSeconds = 0;
}
void TacScriptMatchmaker::OnScriptGameConnectionClosed( TacSocket* socket )
{
  Log( "on script game connection closed" );
  mSocket = nullptr;
  mPretendWebsocketHandshakeDone = false;
  mLine = 0;
  mScriptRoot->OnMsg( scriptMsgDisconnect );
}
void TacScriptMatchmaker::OnScriptGameMessage( TacSocket* socket, void* bytes, int byteCount )
{
  if( mLogReceivedMessages )
    Log( TacString( ( const char* )bytes, byteCount ) );
}
void TacScriptMatchmaker::PokeServer( TacErrors& errors )
{
  TacAssert( mSocket );
  if( !mSocket->mTCPIsConnected )
    return;
  auto shell = mScriptRoot->mGhost->mShell;
  TacString s =
    "TacScriptGameClient messsage: elapsed time is " +
    TacFormatFrameTime( shell->mElapsedSeconds );
  TacJson json;
  json[ "name" ] = "Ping";
  TacJson& args = json[ "args" ];
  args.mType = TacJsonType::Array;
  args.mElements.push_back( new TacJson( s ) );

  TacString toSend = json.Stringify();
  mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
}
void TacScriptMatchmaker::ClearServerLog( TacErrors& errors )
{
  TacAssert( mSocket );
  if( !mSocket->mTCPIsConnected )
    return;
  auto shell = mScriptRoot->mGhost->mShell;
  TacJson json;
  json[ "name" ] = "clear console";
  TacString toSend = json.Stringify();
  mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
}
void TacScriptMatchmaker::Log( const TacString& text )
{
  if( !mShouldLog )
    return;
  auto log = mScriptRoot->mGhost->mShell->mLog;
  if( !log )
    return;
  log->Push( "TacScriptGameClient: " + text );
}
void TacScriptMatchmaker::Update( float seconds, TacErrors& errors )
{
  auto shell = mScriptRoot->mGhost->mShell;
  auto net = shell->mNet;
  auto settings = shell->mSettings;
  TAC_TIMELINE_BEGIN;
  mSocket = net->CreateSocket( "Matchmaking socket", TacAddressFamily::IPv4, TacSocketType::TCP, errors );
  TAC_HANDLE_ERROR( errors );

  auto tCPOnMessage = []( void* userData, TacSocket* socket, void* bytes, int byteCount )
  {
    ( ( TacScriptMatchmaker* )userData )->OnScriptGameMessage( socket, bytes, byteCount );
  };
  TacSocketCallbackDataMessage socketCallbackDataMessage;
  socketCallbackDataMessage.mCallback = tCPOnMessage;
  socketCallbackDataMessage.mUserData = this;
  mSocket->mTCPOnMessage.push_back( socketCallbackDataMessage );

  auto tcpOnConnectionClosed = []( void* userData, TacSocket* socket )
  {
    ( ( TacScriptMatchmaker* )userData )->OnScriptGameConnectionClosed( socket );
  };
  TacSocketCallbackData socketCallbackData;
  socketCallbackData.mCallback = tcpOnConnectionClosed;
  socketCallbackData.mUserData = this;
  mSocket->mTCPOnConnectionClosed.push_back( socketCallbackData );

  TacString hostname = settings->GetString( nullptr, { "hostname" }, defaultHostname, errors );
  mPort = ( uint16_t )settings->GetNumber( nullptr, { "port" }, ( TacJsonNumber )defaultPort, errors );

  mConnectionAttemptStartSeconds = shell->mElapsedSeconds;
  TacString text = "Attempting to connect to " + mHostname + ":" + TacToString( mPort );
  Log( text );
  TAC_TIMELINE_KEYFRAME;
  SetNextKeyDelay( 1.0f );
  TAC_TIMELINE_KEYFRAME;


  if( mTryAutoConnect )
  {
    mSocket->TCPTryConnect( mHostname, mPort, errors );
    TAC_HANDLE_ERROR( errors );
  }
  if( !mSocket->mTCPIsConnected )
    return;
  TacString text = "Connected to " + mHostname + ":" + TacToString( mPort );
  Log( text );
  TAC_TIMELINE_KEYFRAME;
  TacHTTPRequest httpRequest;
  auto websocketKey = TacGenerateSecWebsocketKey();
  httpRequest.FormatRequestWebsocket( "/game", mHostname, websocketKey );
  if( mPrintHTTPRequest )
    std::cout << httpRequest.ToString() << std::endl;
  mSocket->Send( httpRequest, errors );
  TAC_HANDLE_ERROR( errors );
  mPretendWebsocketHandshakeDone = true;
  mSocket->mRequiresWebsocketFrame = true;
  mSocket->mKeepaliveOverride.mUserData = this;
  mSocket->mKeepaliveOverride.mCallback = []( void* userData, TacSocket* socket )
  {
    auto* scriptMatchmaker = ( TacScriptMatchmaker* )userData;
    TacErrors errors; // ???
    scriptMatchmaker->PokeServer( errors );
  };

  mScriptRoot->OnMsg( scriptMsgConnect );


  TAC_TIMELINE_KEYFRAME;
  if( mShouldSpamServer )
    PokeServer( errors );
  return;
  TAC_TIMELINE_END;
}
void TacScriptMatchmaker::DebugImgui( TacErrors& errors )
{
  ImGui::Checkbox( "auto-spam server", &mShouldSpamServer );
  ImGui::Checkbox( "Log received messages", &mLogReceivedMessages );
  ImGui::Checkbox( "Try auto connect", &mTryAutoConnect );


  if( mSocket )
  {
    if( ImGui::Button( "Poke server" ) )
      PokeServer( errors );
    if( ImGui::Button( "Clear server log" ) )
      ClearServerLog( errors );
    if( !mSocket->mTCPIsConnected && !mTryAutoConnect && ImGui::Button( "Try Connect" ) )
    {
      mSocket->TCPTryConnect( mHostname, mPort, errors );
      if( errors.size() )
        return;
    }
    mSocket->DebugImgui();
  }
}

TacScriptMainMenu::TacScriptMainMenu()
{
  mName = "Main Menu";
  mCreateGraveStoryButton = true;
  mCreatePressStartButton = false;
  delete mPower;
}
void TacScriptMainMenu::AddCallbackConnect()
{
  AddScriptCallback( this, []( TacScriptCallbackData* scriptCallbackData, const TacScriptMsg* scriptMsg )
  {
    if( scriptMsg->mType != scriptMsgConnect )
      return;
    auto* scriptMainMenu = ( TacScriptMainMenu* )scriptCallbackData->mUserData;
    scriptCallbackData->mRequestDeletion = true;
    TacUITextData uiTextData;
    uiTextData.mUtf8 = "Status: Connected";
    scriptMainMenu->mUITextServerConnectionStatus->SetText( uiTextData );
    scriptMainMenu->AddCallbackDisconnect();

    TacUIButtonCallback buttonCallback;
    buttonCallback.mUserData = scriptMainMenu;
    buttonCallback.mUserCallback = []( void* userData, TacErrors& errors )
    {
      auto* scriptMainMenu = ( TacScriptMainMenu* )userData;
      auto* scriptMatchmaker = ( TacScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );
      scriptMatchmaker->mSocket->mRequestDeletion = true;
    };
    scriptMainMenu->mUITextDisconnectFromServer->mButtonCallbacks.push_back( buttonCallback );

    buttonCallback.mUserCallback = []( void* userData, TacErrors& errors )
    {
      auto* scriptMainMenu = ( TacScriptMainMenu* )userData;
      auto* scriptMatchmaker = ( TacScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );

      TacJson json;
      json[ "name" ] = "create room";
      TacString toSend = json.Stringify();
      TacSocket* socket = scriptMatchmaker->mSocket;
      socket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
    };
    //scriptMainMenu->mUITextCreateRoom->mButtonCallbacks.push_back( buttonCallback );

    for( TacUIText* uiText : {
      scriptMainMenu->mUITextDisconnectFromServer
      //scriptMainMenu->mUITextCreateRoom
      } )
    {
      TacUITextData uiTextData = *uiText->GetUITextData();
      uiTextData.mColor = colorMagenta;
      uiText->SetText( uiTextData, false );
    }
  } );
}
void TacScriptMainMenu::AddCallbackDisconnect()
{
  AddScriptCallback( this, []( TacScriptCallbackData* scriptCallbackData, const TacScriptMsg* scriptMsg )
  {
    if( scriptMsg->mType != scriptMsgDisconnect )
      return;
    auto* scriptMainMenu = ( TacScriptMainMenu* )scriptCallbackData->mUserData;
    scriptCallbackData->mRequestDeletion = true;
    TacUIText* uiText = scriptMainMenu->mUITextServerConnectionStatus;
    TacUITextData uiTextData = *uiText->GetUITextData();
    uiTextData.mUtf8 = "Status: Disconnected";
    uiText->SetText( uiTextData );
    scriptMainMenu->AddCallbackConnect();


    for( TacUIText* uiText : {
      scriptMainMenu->mUITextDisconnectFromServer
      //scriptMainMenu->mUITextCreateRoom
      } )
    {
      TacUITextData uiTextData = *uiText->GetUITextData();
      uiTextData.mColor = colorGrey;
      uiText->SetText( uiTextData );
      uiText->mButtonCallbacks.clear();
    }
  } );
}
void TacScriptMainMenu::Update( float seconds, TacErrors& errors )
{
  auto ghost = mScriptRoot->mGhost;
  auto shell = ghost->mShell;
  TacUIRoot* uiRoot = ghost->mUIRoot;
  auto* scriptMatchmaker = ( TacScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );
  auto serverData = ghost->mServerData;
  auto world = serverData->mWorld;
  auto graphics = ( TacGraphics* )world->GetSystem( TacSystemType::Graphics );
  float boxWidth = 5;
  if( false )
  {
    TacDebugDrawAABB debugDrawAABB = TacDebugDrawAABB::FromPosExtents( v3( 0, 0, boxWidth / 2 ), v3( 1, 1, 1 ) * boxWidth );
    graphics->DebugDrawAABB( debugDrawAABB );
  }


  if( !mPower )
  {
    // TODO: use the asset manager to load this shit async
    auto memory = TacTemporaryMemory( "assets/power.png", errors );
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
    OnDestruct( stbi_image_free( loaded ) );

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

    TacImage image;
    image.mData = loaded;
    image.mFormat.mElementCount = 4;
    image.mFormat.mPerElementByteCount = 1;
    image.mFormat.mPerElementDataType = TacGraphicsType::unorm;
    image.mWidth = x;
    image.mHeight = y;
    image.mPitch = image.mFormat.mElementCount * image.mFormat.mPerElementByteCount * image.mWidth;

    TacTextureData textureData;
    textureData.access = TacAccess::Default;
    textureData.binding = { TacBinding::ShaderResource };
    textureData.cpuAccess = {};
    textureData.mName = "power";
    textureData.mStackFrame = TAC_STACK_FRAME;
    textureData.myImage = image;
    mScriptRoot->mGhost->mShell->mRenderer->AddTextureResource( &mPower, textureData, errors );
    TAC_HANDLE_ERROR( errors );
  }

  float dotPeriodSeconds = 1;
  if( !scriptMatchmaker->mPretendWebsocketHandshakeDone && mUITextServerConnectionStatus )
  {
    TacString utf8 = "Trying to connect";
    int maxDotCount = 3;
    double elapsedSeconds = shell->mElapsedSeconds - scriptMatchmaker->mConnectionAttemptStartSeconds;
    double partialDotSeconds = std::fmod( elapsedSeconds, ( double )( ( maxDotCount + 1 ) * dotPeriodSeconds ) );
    for( int i = 0; i < int( partialDotSeconds / dotPeriodSeconds ); ++i )
      utf8 += '.';
    TacUITextData uiTextData;
    uiTextData.mColor = colorText;
    uiTextData.mUtf8 = utf8;
    mUITextServerConnectionStatus->SetText( uiTextData, false );
  }

  if( mUITextPressStart )
  {
    double pressStartPeriod = 2.0f;
    double s = std::fmod( shell->mElapsedSeconds, pressStartPeriod );
    bool b = s > pressStartPeriod * 0.5f;
    if( mPressStart != b )
    {
      mPressStart = b;
      TacString utf8 = b ? "Press Start" : "";
      TacUITextData uiTextData;
      uiTextData.mUtf8 = utf8;
      mUITextPressStart->SetText( uiTextData );
    }
  }




  TAC_TIMELINE_BEGIN;


  float menuPositionX = 200;
  TacUIAnchorHorizontal menuAnchorHorizontal = TacUIAnchorHorizontal::Left;
  TacUIAnchorVertical menuAnchorVertical = TacUIAnchorVertical::Center;

  double timelineSeconds = shell->mElapsedSeconds;

  auto createGameTitle = [ = ]()
  {
    if( !mCreateGraveStoryButton )
      return;
    TacUILayout* uiMenu = uiRoot->AddMenu( "Game title layout" );
    uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
    uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
    uiMenu->mUiWidth = 0;
    uiMenu->mHeightTarget = 0;
    //uiMenu->mPosition.x = menuPositionX;
    //uiMenu->mPosition.y = 200;

    auto* uiText = uiMenu->Add< TacUIText >( "Game title text" );
    TacUITextData uiTextData;
    uiTextData.mColor = colorText;
    uiTextData.mUtf8 = "GRAVE STORY";
    uiTextData.mFontSize = 60;
    uiText->SetText( uiTextData );
    uiText->GoNuts();
  };
  mTimeline.Add( new TacTimelineOnce( timelineSeconds, createGameTitle ) );
  timelineSeconds += 0.2f;

  auto createMainMenu = [ = ]()->void
  {
    TacUILayout* uiMenu = uiRoot->AddMenu( "main menu layout" );
    uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
    uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
    uiMenu->mUiWidth = 300;
    uiMenu->mHeightTarget = 200;
    uiMenu->mMenuPadding = 8;
    //uiMenu->mPosition.x = 200;
    //uiMenu->mPosition.y = 0;
    uiMenu->mColor = v4( v3( 1, 1, 1 ) * 52.0f, 58.0f ) / 255.0f;

    {
      TacString serverDispalyName =
        scriptMatchmaker->mHostname +
        TacString( ":" ) +
        TacToString( scriptMatchmaker->mPort );

      TacUIText* uiText = uiMenu->Add< TacUIText >( "server display name" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      TacUITextData uiTextData;
      uiTextData.mColor = colorText;
      uiTextData.mUtf8 = "Server: " + serverDispalyName;
      uiText->SetText( uiTextData );
    }

    {
      auto* uiText = uiMenu->Add< TacUIText >( "server connection status" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      mUITextServerConnectionStatus = uiText;
      TacUITextData uiTextData;
      uiTextData.mColor = colorText;
      if( scriptMatchmaker->mPretendWebsocketHandshakeDone )
      {
        uiTextData.mUtf8 = "Status: Connected";
        AddCallbackDisconnect();
      }
      else
      {
        AddCallbackConnect();
      }
      uiText->SetText( uiTextData );
    }

    {
      bool nestedLayouts = true;
      if( nestedLayouts )
      {

        float sideLength = 20;
        TacUILayout* uiLayout = uiMenu->Add< TacUILayout >( "server autoconnect layout" );
        uiLayout->mUiWidth = sideLength;
        uiLayout->mHeightTarget = sideLength;
        uiLayout->mUILayoutType = TacUILayoutType::Horizontal;
        uiLayout->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Left;
        uiLayout->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;

        TacUILayout* parentUiLayout = uiLayout;

        uiLayout = parentUiLayout->Add< TacUILayout >( "server autoconnect left image" );
        uiLayout->mColor = v4( 1, 1, 0, 1 );
        uiLayout->mUiWidth = sideLength;
        uiLayout->mHeightTarget = sideLength;
        uiLayout->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Left;
        uiLayout->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;

        TacUIText* uiText = parentUiLayout->Add< TacUIText >( "server autoconnect text" );
        uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
        TacUITextData uiTextData;
        uiTextData.mUtf8 = "Server Autoconnect: On";
        uiTextData.mColor = colorMagenta;
        TacUIButtonCallback buttonCallback;
        buttonCallback.mUserData = this;
        buttonCallback.mUserCallback = []( void* userData, TacErrors& errors )
        {
          auto* scriptMainMenu = ( TacScriptMainMenu* )userData;
          TacScriptRoot* scriptRoot = scriptMainMenu->mScriptRoot;
          auto* scriptMatchmaker = ( TacScriptMatchmaker* )scriptRoot->GetThread( scriptMatchmakerName );
          scriptMatchmaker->mTryAutoConnect = !scriptMatchmaker->mTryAutoConnect;
          TacString text = "Server Autoconnect: ";
          if( scriptMatchmaker->mTryAutoConnect )
            text += "On";
          else
            text += "Off";
          TacUITextData uiTextData;
          uiTextData.mUtf8 = text;
          uiTextData.mColor = colorMagenta;
          scriptMainMenu->mUITextServerAutoconnect->SetText( uiTextData, false );
        };
        uiText->mButtonCallbacks.push_back( buttonCallback );
        mUITextServerAutoconnect = uiText;
        uiText->SetText( uiTextData );

        uiLayout = parentUiLayout->Add< TacUILayout >( "server autoconnect right image" );
        uiLayout->mColor = v4( 0, 1, 1, 1 );
        uiLayout->mUiWidth = sideLength;
        uiLayout->mHeightTarget = sideLength;
        uiLayout->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Left;
        uiLayout->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;

      }



    }

    {
      auto* uiText = uiMenu->Add< TacUIText >( "disconnect from server text" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      TacUITextData uiTextData;
      uiTextData.mUtf8 = "Disconnect From Server";
      uiTextData.mColor = colorGrey;
      uiText->SetText( uiTextData );
      mUITextDisconnectFromServer = uiText;
    }

    {
      auto* uiText = uiMenu->Add< TacUIText >( "create room button" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      TacUITextData uiTextData;
      uiTextData.mUtf8 = "Create Room";
      uiTextData.mColor = colorGrey;
      uiText->SetText( uiTextData );
    }

    {
      auto* uiText = uiMenu->Add< TacUIText >( "controllers text" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      TacUITextData uiTextData;
      uiTextData.mUtf8 = "Controllers";
      uiTextData.mColor = colorMagenta;
      uiText->SetText( uiTextData );
      TacUIButtonCallback buttonCallback;
      buttonCallback.mUserData = this;
      buttonCallback.mUserCallback = []( void* userData, TacErrors& errors )
      {
        auto* scriptMainMenu = ( TacScriptMainMenu* )userData;
        static int ihi;
        std::cout << "hi" << " " << ihi++ << std::endl;
      };
      uiText->mButtonCallbacks.push_back( buttonCallback );
    }

    {
      auto* layout = uiMenu->Add< TacUILayout >( "exit game layout" );
      layout->mUILayoutType = TacUILayoutType::Horizontal;
      layout->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Left;
      layout->mAnchor.mAnchorVertical = TacUIAnchorVertical::Top;
      layout->mHeightTarget = 100.0f;
      layout->mColor = { 0, 0, 0, 0 };
      layout->mExpandWidth = true;
      float powerSize = 20;
      auto* power = layout->Add< TacUILayout >( "power icon" );
      power->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Left;
      power->mAnchor.mAnchorVertical = TacUIAnchorVertical::Center;
      power->mColor = {
        186 / 255.0f,
        164 / 255.0f,
        236 / 255.0f,
        1 };
      power->mHeightTarget = powerSize;
      power->mUiWidth = powerSize;
      power->mTexture = mPower;

      auto* uiText = layout->Add< TacUIText >( "exit game text" );
      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
      TacUITextData uiTextData;
      uiTextData.mUtf8 = "Exit Game";
      uiTextData.mColor = colorMagenta;
      uiText->SetText( uiTextData );
      TacUIButtonCallback buttonCallback;
      buttonCallback.mUserData = this;
      buttonCallback.mUserCallback = []( void* userData, TacErrors& errors )
      {
        TacOS::Instance->mShouldStopRunning = true;
      };
      uiText->mButtonCallbacks.push_back( buttonCallback );
    }

    mMenu = uiMenu;
  };
  mTimeline.Add( new TacTimelineOnce( timelineSeconds, createMainMenu ) );
  timelineSeconds += 0.2f;

  auto createPressStart = [ = ]()
  {
    if( !mCreatePressStartButton )
      return;

    TacUILayout* uiMenu = uiRoot->AddMenu( "press start layout" );
    uiMenu->mAnchor.mAnchorHorizontal = TacUIAnchorHorizontal::Right;
    uiMenu->mAnchor.mAnchorVertical = TacUIAnchorVertical::Bottom;
    //uiMenu->mPosition = { -100, 100 };
    uiMenu->mColor = {};
    auto* uiText = uiMenu->Add< TacUIText >( "press start text" );
    mUITextPressStart = uiText;
  };
  mTimeline.Add( new TacTimelineOnce( timelineSeconds, createPressStart ) );
  timelineSeconds += 0.2f;



  TAC_TIMELINE_KEYFRAME;


  mTimeline.Update( ghost->mShell->mElapsedSeconds, errors );
  TAC_HANDLE_ERROR( errors );

  return;

  TAC_TIMELINE_END;
}
void TacScriptMainMenu::DebugImgui( TacErrors& errors )
{

}

TacTimelineAction::~TacTimelineAction() = default;

void TacTimelineAction::Begin()
{

}
void TacTimelineAction::End()
{

}
void TacTimelineAction::Update( float percent )
{

}

TacTimeline::~TacTimeline()
{
  for( TacTimelineAction* timelineAction : mTimelineActions )
    delete timelineAction;
}
void TacTimeline::Update( double time, TacErrors& errors )
{
  for( TacTimelineAction* timelineAction : mTimelineActions )
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

    double percent = ( time - timelineAction->mTimeBegin ) / ( timelineAction->mTimeEnd - timelineAction->mTimeBegin );
    percent = TacSaturate( percent );
    timelineAction->Update( ( float )percent );

    if( time > timelineAction->mTimeEnd )
    {
      timelineAction->End();
      timelineAction->mPlayedEnd = true;
    }
  }
}
void TacTimeline::Add( TacTimelineAction* timelineAction )
{
  mTimelineActions.push_back( timelineAction );
}

