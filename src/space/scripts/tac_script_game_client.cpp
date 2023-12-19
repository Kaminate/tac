#include "space/scripts/tac_script_game_client.h" // self-inc

#include "src/common/dataprocess/tac_json.h"
#include "src/common/dataprocess/tac_log.h"
#include "src/common/dataprocess/tac_settings.h"
#include "src/common/graphics/imgui/tac_imgui.h"
#include "src/common/math/tac_math.h"
#include "src/common/memory/tac_memory.h"
#include "src/common/net/tac_net.h"
#include "src/common/shell/tac_shell.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/common/system/tac_filesystem.h"
#include "src/common/system/tac_job_queue.h"
#include "src/common/system/tac_os.h"
#include "src/common/thirdparty/stb_image.h"
#include "space/graphics/tac_graphics.h"
#include "space/ghost/tac_ghost.h"
#include "space/net/tac_server.h"
#include "space/world/tac_world.h"

import std; // #include <cstdlib> // itoa

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
    TAC_UNUSED_PARAMETER( seconds );
    TAC_UNUSED_PARAMETER( errors );
		TAC_TIMELINE_BEGIN;

		auto scriptMatchmaker = TAC_NEW ScriptMatchmaker;
		mScriptRoot->AddChild( scriptMatchmaker );

		auto scriptSplash = TAC_NEW ScriptSplash;
		mScriptRoot->AddChild( scriptSplash );

		TAC_TIMELINE_KEYFRAME;
		TAC_TIMELINE_KEYFRAME;
    RunForever();
		TAC_TIMELINE_END;
	}
	void ScriptGameClient::DebugImgui( Errors& errors )
	{
    TAC_UNUSED_PARAMETER( errors );
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
		//mScriptRoot->AddChild( TAC_NEW ScriptMainMenu );
		mScriptRoot->AddChild( TAC_NEW ScriptMainMenu2 );
	}
	void ScriptSplash::Update( float seconds, Errors& errors )
	{
    TAC_UNUSED_PARAMETER( seconds );
    TAC_UNUSED_PARAMETER( errors );
		//Ghost* ghost = mScriptRoot->mGhost;
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

		//auto vis = TAC_NEW UIHierarchyVisualText;
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
    TAC_UNUSED_PARAMETER( errors );
		//ImGui::DragFloat( "Fully visible sec", &mFullyVisibleSec );
		//ImGui::DragFloat( "Fade sec total", &mFadeSecTotal );
		//ImGui::Checkbox( "Skip splash screen", &mSkipSplashScreen );
	}

	ScriptMatchmaker::ScriptMatchmaker()
	{
		mName = scriptMatchmakerName;
	}

	void ScriptMatchmaker::OnScriptGameConnectionClosed( [[maybe_unused]] Network::Socket* socket )
	{
    TAC_ASSERT( socket == mSocket );
		Log( "on script game connection closed" );
		mSocket = nullptr;
		mPretendWebsocketHandshakeDone = false;
		mLine = 0;
		mScriptRoot->OnMsg( scriptMsgDisconnect );
	}

  void ScriptMatchmaker::OnScriptGameMessage( [[maybe_unused ]] Network::Socket* socket,
                                              void* bytes,
                                              int byteCount )
	{
		if( mLogReceivedMessages )
			Log( String( ( const char* )bytes, byteCount ) );
	}

	void ScriptMatchmaker::PokeServer( Errors& errors )
	{
		TAC_ASSERT( mSocket );
		if( !mSocket->mTCPIsConnected )
			return;

		const String s =
			"ScriptGameClient messsage: elapsed time is " +
			FormatFrameTime( ShellGetElapsedSeconds() );

		Json json;
		json[ "name" ].SetString( "Ping" );
		Json& args = json[ "args" ];
		args.mType = JsonType::Array;

		Json* arg0 = TAC_NEW( kHappyGrl ) Json;
		arg0->SetString( s );
		args.mArrayElements.push_back( arg0 );

		String toSend = json.Stringify();
		mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
	}
	void ScriptMatchmaker::ClearServerLog( Errors& errors )
	{
		TAC_ASSERT( mSocket );
		if( !mSocket->mTCPIsConnected )
			return;
		Json json;
		json[ "name" ] = "clear console";
		String toSend = json.Stringify();
		mSocket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
	}
	void ScriptMatchmaker::Log( StringView text )
	{
    TAC_UNUSED_PARAMETER( text );
		if( !mShouldLog )
			return;
		//auto log = Shell::Instance.mLog;
		//if( !log )
		//	return;
		//log->Push( "ScriptGameClient: " + text );
	}
	void ScriptMatchmaker::TryConnect()
	{
		mConnectionErrors.clear();
		if( mSocket->mTCPIsConnected )
			return;
		mSocket->TCPTryConnect( mHostname, mPort, mConnectionErrors );
	}

  void ScriptMatchmaker::TCPOnMessage( void* userData,
                                       Network::Socket* socket,
                                       void* bytes,
                                       int byteCount )
  {
    auto matchMaker = ( ScriptMatchmaker* )userData;
    matchMaker->OnScriptGameMessage( socket, bytes, byteCount );
  };

  void ScriptMatchmaker::TCPOnConnectionClosed( void* userData, Network::Socket* socket )
  {
    auto matchMaker = ( ScriptMatchmaker* )userData;
    matchMaker->OnScriptGameConnectionClosed( socket );
  };

  void ScriptMatchmaker::KeepAlive(void* userData, Network::Socket* )
  {
    auto* scriptMatchmaker = ( ScriptMatchmaker* )userData;
    Errors errors; // ???
    scriptMatchmaker->PokeServer( errors );
  };

	void ScriptMatchmaker::Update( float seconds, Errors& errors )
	{
    TAC_UNUSED_PARAMETER( seconds );
		TAC_TIMELINE_BEGIN;
    mSocket = TAC_CALL( Network::Net::Instance->CreateSocket(
      "Matchmaking socket",
      Network::AddressFamily::IPv4,
      Network::SocketType::TCP, errors ) );


    const Network::SocketCallbackDataMessage socketCallbackDataMessage =
    {
      .mCallback = ScriptMatchmaker::TCPOnMessage,
      .mUserData = this,
    };
		mSocket->mTCPOnMessage.push_back( socketCallbackDataMessage );

    const Network::SocketCallbackData socketCallbackData =
    {
      .mCallback = TCPOnConnectionClosed,
      .mUserData = this,
    };
		mSocket->mTCPOnConnectionClosed.push_back( socketCallbackData );

		//String hostname = SettingsGetString( "hostname" , defaultHostname );
		mPort = ( uint16_t )SettingsGetNumber( "port" , ( JsonNumber )defaultPort );

		mConnectionAttemptStartSeconds = ShellGetElapsedSeconds();

    {
      String text;
      text += "Attempting to connect to ";
      text += mHostname;
      text += ":";
      text += ToString( mPort );
      Log( text );
    }

		TAC_TIMELINE_KEYFRAME;
		SetNextKeyDelay( 1.0f );
		TAC_TIMELINE_KEYFRAME;


		if( mTryAutoConnect )
			TryConnect();

		if( !mSocket->mTCPIsConnected )
			return;

    {
      String text;
      text += "Connected to ";
      text += mHostname;
      text += ":";
      text += ToString( mPort );
      Log( text );
    }

		TAC_TIMELINE_KEYFRAME;
		Network::HTTPRequest httpRequest;
		auto websocketKey = Network::GenerateSecWebsocketKey();
		httpRequest.FormatRequestWebsocket( "/game", mHostname, websocketKey );
		if( mPrintHTTPRequest )
          OS::OSDebugPrintLine(httpRequest.ToString());
		TAC_CALL( mSocket->Send( httpRequest, errors ) );
		mPretendWebsocketHandshakeDone = true;
		mSocket->mRequiresWebsocketFrame = true;
		mSocket->mKeepaliveOverride.mUserData = this;
    mSocket->mKeepaliveOverride.mCallback = ScriptMatchmaker::KeepAlive;

		mScriptRoot->OnMsg( scriptMsgConnect );


		TAC_TIMELINE_KEYFRAME;
		if( mShouldSpamServer )
			PokeServer( errors );
    RunForever();
		TAC_TIMELINE_END;
	}
	void ScriptMatchmaker::DebugImgui( Errors& errors )
	{
    TAC_UNUSED_PARAMETER( errors );
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
		Render::DestroyTexture( mPower, TAC_STACK_FRAME );
	}
	void ScriptMainMenu::AddCallbackConnect()
  {
    AddScriptCallback(
      this,
      []( ScriptCallbackData* scriptCallbackData, const ScriptMsg* scriptMsg )
      {
        TAC_UNUSED_PARAMETER(scriptCallbackData);
        TAC_UNUSED_PARAMETER(scriptMsg);
        //if( scriptMsg->mType != scriptMsgConnect )
        //  return;
        //auto* scriptMainMenu = ( ScriptMainMenu* )scriptCallbackData->mUserData;
        //scriptCallbackData->mRequestDeletion = true;
        //UITextData uiTextData;
        //uiTextData.mUtf8 = "Status: Connected";
        //scriptMainMenu->mUITextServerConnectionStatus->SetText( uiTextData );
        //scriptMainMenu->AddCallbackDisconnect();

        //UIButtonCallback buttonCallback;
        //buttonCallback.mUserData = scriptMainMenu;
        //buttonCallback.mUserCallback = []( void* userData, Errors& errors )
        //{
        //  auto* scriptMainMenu = ( ScriptMainMenu* )userData;
        //  auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );
        //  scriptMatchmaker->mSocket->mRequestDeletion = true;
        //};
        //scriptMainMenu->mUITextDisconnectFromServer->mButtonCallbacks.push_back( buttonCallback );

        //buttonCallback.mUserCallback = []( void* userData, Errors& errors )
        //{
        //  auto* scriptMainMenu = ( ScriptMainMenu* )userData;
        //  auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptMainMenu->mScriptRoot->GetThread( scriptMatchmakerName );

        //  Json json;
        //  json[ "name" ] = "create room";
        //  String toSend = json.Stringify();
        //  Socket* socket = scriptMatchmaker->mSocket;
        //  socket->Send( ( void* )toSend.data(), ( int )toSend.size(), errors );
        //};
        ////scriptMainMenu->mUITextCreateRoom->mButtonCallbacks.push_back( buttonCallback );

        //for( UIText* uiText : {
        //  scriptMainMenu->mUITextDisconnectFromServer
        //  //scriptMainMenu->mUITextCreateRoom
        //  } )
        //{
        //  UITextData uiTextData = *uiText->GetUITextData();
        //  uiTextData.mColor = colorMagenta;
        //  uiText->SetText( uiTextData, false );
        //}
      } );
  }
	void ScriptMainMenu::AddCallbackDisconnect()
	{
		//AddScriptCallback( this, []( ScriptCallbackData* scriptCallbackData, const ScriptMsg* scriptMsg )
		//                   {
		//                     if( scriptMsg->mType != scriptMsgDisconnect )
		//                       return;
		//                     auto* scriptMainMenu = ( ScriptMainMenu* )scriptCallbackData->mUserData;
		//                     scriptCallbackData->mRequestDeletion = true;
		//                     UIText* uiText = scriptMainMenu->mUITextServerConnectionStatus;
		//                     UITextData uiTextData = *uiText->GetUITextData();
		//                     uiTextData.mUtf8 = "Status: Disconnected";
		//                     uiText->SetText( uiTextData );
		//                     scriptMainMenu->AddCallbackConnect();
		//                     for( UIText* uiText : {
		//                       scriptMainMenu->mUITextDisconnectFromServer
		//                       //scriptMainMenu->mUITextCreateRoom
		//                          } )
		//                     {
		//                       UITextData uiTextData = *uiText->GetUITextData();
		//                       uiTextData.mColor = colorGrey;
		//                       uiText->SetText( uiTextData );
		//                       uiText->mButtonCallbacks.clear();
		//                     }
		//                   } );
	}
	void ScriptMainMenu::Update( float seconds, Errors& errors )
	{
    TAC_UNUSED_PARAMETER( errors );
    TAC_UNUSED_PARAMETER( seconds );
		//Ghost* ghost = mScriptRoot->mGhost;
		//;
		////UIRoot* uiRoot = ghost->mUIRoot;
		//auto* scriptMatchmaker = ( ScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );
		//ServerData* serverData = ghost->mServerData;
		//World* world = serverData->mWorld;
		//Graphics* graphics = GetGraphics( world );
		//float boxWidth = 5;
		//if( false )
		//{
		//  //DebugDrawAABB debugDrawAABB = DebugDrawAABB::FromPosExtents( v3( 0, 0, boxWidth / 2 ), v3( 1, 1, 1 ) * boxWidth );
		//  //graphics->DebugDrawAABB( debugDrawAABB );
		//}


		//if( !mPower.IsValid() )
		//{
		//  // TODO: use the asset manager to load this shit async
		//  auto memory = TAC_CALL( FileToString( "assets/power.png", errors ) );

		//  int x;
		//  int y;
		//  int previousChannelCount;

		//  // rgba
		//  stbi_uc* loaded = stbi_load_from_memory( ( const stbi_uc* )memory.data(),
		//    ( int )memory.size(),
		//                                           &x,
		//                                           &y,
		//                                           &previousChannelCount,
		//                                           4 );
		//  TAC_ON_DESTRUCT( stbi_image_free( loaded ) );

		//  stbi_uc* l = loaded;
		//  for( int i = 0; i < y; ++i )
		//  {
		//    for( int j = 0; j < x; ++j )
		//    {
		//      uint8_t* r = l++;
		//      uint8_t* g = l++;
		//      uint8_t* b = l++;
		//      uint8_t* a = l++;
		//      float percent = *a / 255.0f;

		//      *r = ( uint8_t )( *r * percent );
		//      *g = ( uint8_t )( *g * percent );
		//      *b = ( uint8_t )( *b * percent );
		//    }
		//  }

		//  Image image;
		//  //image.mData = loaded;
		//  image.mFormat.mElementCount = 4;
		//  image.mFormat.mPerElementByteCount = 1;
		//  image.mFormat.mPerElementDataType = GraphicsType::unorm;
		//  image.mWidth = x;
		//  image.mHeight = y;

		//  Render::TexSpec textureData;
		//  textureData.mAccess = Access::Default;
		//  textureData.mBinding = Binding::ShaderResource;
		//  textureData.mImage = image;
		//  textureData.mPitch = image.mFormat.mElementCount * image.mFormat.mPerElementByteCount * image.mWidth;
		//  mPower = Render::CreateTexture( "power", textureData, TAC_STACK_FRAME );
		//}

		//float dotPeriodSeconds = 1;
		//if( !scriptMatchmaker->mPretendWebsocketHandshakeDone && mUITextServerConnectionStatus )
		//{
		//  String utf8 = "Trying to connect";
		//  int maxDotCount = 3;
		//  double elapsedSeconds = ShellGetElapsedSeconds() - scriptMatchmaker->mConnectionAttemptStartSeconds;
		//  double partialDotSeconds = std::fmod( elapsedSeconds, ( double )( ( maxDotCount + 1 ) * dotPeriodSeconds ) );
		//  for( int i = 0; i < int( partialDotSeconds / dotPeriodSeconds ); ++i )
		//    utf8 += '.';
		//  UITextData uiTextData;
		//  uiTextData.mColor = colorText;
		//  uiTextData.mUtf8 = utf8;
		//  mUITextServerConnectionStatus->SetText( uiTextData, false );
		//}

		//if( mUITextPressStart )
		//{
		//  double pressStartPeriod = 2.0f;
		//  double s = std::fmod( ShellGetElapsedSeconds(), pressStartPeriod );
		//  bool b = s > pressStartPeriod * 0.5f;
		//  if( mPressStart != b )
		//  {
		//    mPressStart = b;
		//    String utf8 = b ? "Press Start" : "";
		//    UITextData uiTextData;
		//    uiTextData.mUtf8 = utf8;
		//    mUITextPressStart->SetText( uiTextData );
		//  }
		//}




		//TAC_TIMELINE_BEGIN;


		//float menuPositionX = 200;
		////UIAnchorHorizontal menuAnchorHorizontal = UIAnchorHorizontal::Left;
		////UIAnchorVertical menuAnchorVertical = UIAnchorVertical::Center;

		//double timelineSeconds = ShellGetElapsedSeconds();

		////auto createGameTitle = [ = ]()
		////{
		////  if( !mCreateGraveStoryButton )
		////    return;
		////  UILayout* uiMenu = uiRoot->AddMenu( "Game title layout" );
		////  uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
		////  uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
		////  uiMenu->mUiWidth = 0;
		////  uiMenu->mHeightTarget = 0;
		////  //uiMenu->mPosition.x = menuPositionX;
		////  //uiMenu->mPosition.y = 200;

		////  auto* uiText = uiMenu->Add< UIText >( "Game title text" );
		////  UITextData uiTextData;
		////  uiTextData.mColor = colorText;
		////  uiTextData.mUtf8 = "GRAVE STORY";
		////  uiTextData.mFontSize = 60;
		////  uiText->SetText( uiTextData );
		////  uiText->GoNuts();
		////};
		////mTimeline.Add( TAC_NEW TimelineOnce( timelineSeconds, createGameTitle ) );
		////timelineSeconds += 0.2f;

		////auto createMainMenu = [ = ]()->void
		////{
		////  UILayout* uiMenu = uiRoot->AddMenu( "main menu layout" );
		////  uiMenu->mAnchor.mAnchorHorizontal = menuAnchorHorizontal;
		////  uiMenu->mAnchor.mAnchorVertical = menuAnchorVertical;
		////  uiMenu->mUiWidth = 300;
		////  uiMenu->mHeightTarget = 200;
		////  uiMenu->mMenuPadding = 8;
		////  //uiMenu->mPosition.x = 200;
		////  //uiMenu->mPosition.y = 0;
		////  uiMenu->mColor = v4( v3( 1, 1, 1 ) * 52.0f, 58.0f ) / 255.0f;

		////  // Server host / port
		////  {
		////    String serverDispalyName =
		////      scriptMatchmaker->mHostname +
		////      String( ":" ) +
		////      ToString( scriptMatchmaker->mPort );

		////    UIText* uiText = uiMenu->Add< UIText >( "server display name" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    UITextData uiTextData;
		////    uiTextData.mColor = colorText;
		////    uiTextData.mUtf8 = "Server: " + serverDispalyName;
		////    uiText->SetText( uiTextData );
		////  }

		////  // server connection status
		////  {
		////    auto* uiText = uiMenu->Add< UIText >( "server connection status" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    mUITextServerConnectionStatus = uiText;
		////    UITextData uiTextData;
		////    uiTextData.mColor = colorText;
		////    if( scriptMatchmaker->mPretendWebsocketHandshakeDone )
		////    {
		////      uiTextData.mUtf8 = "Status: Connected";
		////      AddCallbackDisconnect();
		////    }
		////    else
		////    {
		////      AddCallbackConnect();
		////    }
		////    uiText->SetText( uiTextData );
		////  }

		////  // server autoconnect
		////  {
		////    bool nestedLayouts = true;
		////    if( nestedLayouts )
		////    {

		////      float sideLength = 20;
		////      UILayout* uiLayout = uiMenu->Add< UILayout >( "server autoconnect layout" );
		////      uiLayout->mUiWidth = sideLength;
		////      uiLayout->mHeightTarget = sideLength;
		////      uiLayout->mUILayoutType = UILayoutType::Horizontal;
		////      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
		////      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

		////      UILayout* parentUiLayout = uiLayout;

		////      uiLayout = parentUiLayout->Add< UILayout >( "server autoconnect left image" );
		////      uiLayout->mColor = v4( 1, 1, 0, 1 );
		////      uiLayout->mUiWidth = sideLength;
		////      uiLayout->mHeightTarget = sideLength;
		////      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
		////      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

		////      UIText* uiText = parentUiLayout->Add< UIText >( "server autoconnect text" );
		////      uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////      UITextData uiTextData;
		////      uiTextData.mUtf8 = "Server Autoconnect: On";
		////      uiTextData.mColor = colorMagenta;
		////      UIButtonCallback buttonCallback;
		////      buttonCallback.mUserData = this;
		////      buttonCallback.mUserCallback = []( void* userData, Errors& errors )
		////      {
		////        auto* scriptMainMenu = ( ScriptMainMenu* )userData;
		////        ScriptRoot* scriptRoot = scriptMainMenu->mScriptRoot;
		////        auto* scriptMatchmaker = ( ScriptMatchmaker* )scriptRoot->GetThread( scriptMatchmakerName );
		////        scriptMatchmaker->mTryAutoConnect = !scriptMatchmaker->mTryAutoConnect;
		////        String text = "Server Autoconnect: ";
		////        if( scriptMatchmaker->mTryAutoConnect )
		////          text += "On";
		////        else
		////          text += "Off";
		////        UITextData uiTextData;
		////        uiTextData.mUtf8 = text;
		////        uiTextData.mColor = colorMagenta;
		////        scriptMainMenu->mUITextServerAutoconnect->SetText( uiTextData, false );
		////      };
		////      uiText->mButtonCallbacks.push_back( buttonCallback );
		////      mUITextServerAutoconnect = uiText;
		////      uiText->SetText( uiTextData );

		////      uiLayout = parentUiLayout->Add< UILayout >( "server autoconnect right image" );
		////      uiLayout->mColor = v4( 0, 1, 1, 1 );
		////      uiLayout->mUiWidth = sideLength;
		////      uiLayout->mHeightTarget = sideLength;
		////      uiLayout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
		////      uiLayout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;

		////    }



		////  }

		////  // disconenct from server
		////  {
		////    auto* uiText = uiMenu->Add< UIText >( "disconnect from server text" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    UITextData uiTextData;
		////    uiTextData.mUtf8 = "Disconnect From Server";
		////    uiTextData.mColor = colorGrey;
		////    uiText->SetText( uiTextData );
		////    mUITextDisconnectFromServer = uiText;
		////  }

		////  // create room
		////  {
		////    auto* uiText = uiMenu->Add< UIText >( "create room button" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    UITextData uiTextData;
		////    uiTextData.mUtf8 = "Create Room";
		////    uiTextData.mColor = colorGrey;
		////    uiText->SetText( uiTextData );
		////  }

		////  // controllers
		////  {
		////    auto* uiText = uiMenu->Add< UIText >( "controllers text" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    UITextData uiTextData;
		////    uiTextData.mUtf8 = "Controllers";
		////    uiTextData.mColor = colorMagenta;
		////    uiText->SetText( uiTextData );
		////    UIButtonCallback buttonCallback;
		////    buttonCallback.mUserData = this;
		////    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
		////    {
		////      auto* scriptMainMenu = ( ScriptMainMenu* )userData;
		////      static int ihi;
		////    };
		////    uiText->mButtonCallbacks.push_back( buttonCallback );
		////  }

		////  // Exit game
		////  {
		////    auto* layout = uiMenu->Add< UILayout >( "exit game layout" );
		////    layout->mUILayoutType = UILayoutType::Horizontal;
		////    layout->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
		////    layout->mAnchor.mAnchorVertical = UIAnchorVertical::Top;
		////    layout->mHeightTarget = 100.0f;
		////    layout->mColor = { 0, 0, 0, 0 };
		////    layout->mExpandWidth = true;
		////    float powerSize = 20;
		////    auto* power = layout->Add< UILayout >( "power icon" );
		////    power->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Left;
		////    power->mAnchor.mAnchorVertical = UIAnchorVertical::Center;
		////    power->mColor = {
		////      186 / 255.0f,
		////      164 / 255.0f,
		////      236 / 255.0f,
		////      1 };
		////    power->mHeightTarget = powerSize;
		////    power->mUiWidth = powerSize;
		////    power->mTexture = mPower;

		////    auto* uiText = layout->Add< UIText >( "exit game text" );
		////    uiText->mInitialDelaySecs = uiMenu->GetInitialDelaySeconds();
		////    UITextData uiTextData;
		////    uiTextData.mUtf8 = "Exit Game";
		////    uiTextData.mColor = colorMagenta;
		////    uiText->SetText( uiTextData );
		////    UIButtonCallback buttonCallback;
		////    buttonCallback.mUserData = this;
		////    buttonCallback.mUserCallback = []( void* userData, Errors& errors )
		////    {
		////      OSmShouldStopRunning = true;
		////    };
		////    uiText->mButtonCallbacks.push_back( buttonCallback );
		////  }

		////  mMenu = uiMenu;
		////};
		////mTimeline.Add( TAC_NEW TimelineOnce( timelineSeconds, createMainMenu ) );
		////timelineSeconds += 0.2f;

		////auto createPressStart = [ = ]()
		////{
		////  if( !mCreatePressStartButton )
		////    return;
		////  UILayout* uiMenu = uiRoot->AddMenu( "press start layout" );
		////  uiMenu->mAnchor.mAnchorHorizontal = UIAnchorHorizontal::Right;
		////  uiMenu->mAnchor.mAnchorVertical = UIAnchorVertical::Bottom;
		////  //uiMenu->mPosition = { -100, 100 };
		////  uiMenu->mColor = {};
		////  auto* uiText = uiMenu->Add< UIText >( "press start text" );
		////  mUITextPressStart = uiText;
		////};
		////mTimeline.Add( TAC_NEW TimelineOnce( timelineSeconds, createPressStart ) );
		////timelineSeconds += 0.2f;



		//TAC_TIMELINE_KEYFRAME;


		//TAC_CALL(mTimeline.Update( ShellGetElapsedSeconds(), errors ));

		//return;

		//TAC_TIMELINE_END;
	}
	void ScriptMainMenu::DebugImgui( Errors& errors )
	{
    TAC_UNUSED_PARAMETER( errors );

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
    TAC_UNUSED_PARAMETER( percent );

	}

	Timeline::~Timeline()
	{
		for( TimelineAction* timelineAction : mTimelineActions )
			delete timelineAction;
	}

	void Timeline::Update( double time, Errors& errors )
	{
    TAC_UNUSED_PARAMETER( errors );
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
    void Execute() override;
		
		ScriptMatchmaker* mMatchmaker = nullptr;
	};

	ScriptMainMenu2::ScriptMainMenu2()
	{
		mName = "Main Menu 2";
	}

	ScriptMainMenu2::~ScriptMainMenu2()
	{
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
		//  JobState status = mConnectToServerJob->GetStatus();
		//  if( status == JobState::ThreadQueued ||
		//    status == JobState::ThreadRunning )
		//  {
		//    String text = "Connecting to server";
		//    for( int i = 0; i < ( int )ShellGetElapsedSeconds() % 4; ++i )
		//      text += ".";
		//    ImGuiText( text );
		//  }
		//  else
		//  {
		//    if( status == JobState::ThreadFailed )
		//      ImGuiText( mConnectToServerJob->mErrors.mMessage );
		//    if( ImGuiButton( "Connect to server" ) )
		//    {
		//      JobQueue::Instance->Push( mConnectToServerJob );
		//    }
		//  }
		//}
		//if( ImGuiButton( "Exit Game" ) )
		//{
		//  OSmShouldStopRunning = true;
		//}
		//ImGuiEnd();
	}
	void ScriptMainMenu2::Update( float seconds, Errors& errors )
	{
    TAC_UNUSED_PARAMETER( seconds );
    TAC_UNUSED_PARAMETER( errors );

		TAC_TIMELINE_BEGIN;

		auto matchMaker = ( ScriptMatchmaker* )mScriptRoot->GetThread( scriptMatchmakerName );

		ConnectToServerJob* job = TAC_NEW ConnectToServerJob;
		job->mMatchmaker = matchMaker;
    
		mConnectToServerJob = job;

		TAC_TIMELINE_KEYFRAME;
		TAC_TIMELINE_KEYFRAME;
		TAC_TIMELINE_KEYFRAME;
		TAC_TIMELINE_KEYFRAME;
		TAC_TIMELINE_KEYFRAME;
		RenderMainMenu();
    RunForever();
		TAC_TIMELINE_END;
	}


  void ConnectToServerJob::Execute() 
  {
    mMatchmaker->TryConnect();
    mErrors = mMatchmaker->mConnectionErrors;
  }

} // namespace Tac
