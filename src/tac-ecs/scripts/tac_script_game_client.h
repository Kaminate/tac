#pragma once

#include "tac-ecs/script/tac_script.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/containers/tac_set.h"

namespace Tac::Network
{
  struct Socket;
}

namespace Tac
{
  struct UILayout;
  struct UIText;
  struct Job;
  struct ScriptMainMenu;
  struct ScriptSplash;
  struct ScriptMatchmaker;
  struct ScriptGameClient;

  struct TimelineAction
  {
    virtual ~TimelineAction();
    virtual void Begin();
    virtual void End();
    virtual void Update( float percent );
    double       mTimeBegin    {};
    double       mTimeEnd      {};
    bool         mPlayedBegin  {};
    bool         mPlayedEnd    {};
    bool         mPlayedUpdate {};
  };

  struct Timeline
  {
    ~Timeline();
    void Update( double time, Errors& );
    void Add( TimelineAction* );
    Vector< TimelineAction* > mTimelineActions;
  };

  struct ScriptSplash : public ScriptThread
  {
    ScriptSplash();
    ~ScriptSplash();
    void Update( GameTimeDelta, Errors& ) override;
    void DebugImgui( Errors& ) override;
    UIText* mStudioName       {};
    float   mFullyVisibleSec  {};
    float   mFadeSecTotal     {};
    bool    mSkipSplashScreen {};
  };

  struct ScriptGameClient : public ScriptThread
  {
    ScriptGameClient();
    void Update( GameTimeDelta, Errors& ) override;
    void DebugImgui( Errors& ) override;
  };

  struct ScriptMatchmaker : public ScriptThread
  {
    ScriptMatchmaker();
    void Update( GameTimeDelta, Errors& ) override;
    void DebugImgui( Errors& ) override;
    void OnScriptGameConnectionClosed( Network::Socket* );
    void OnScriptGameMessage( Network::Socket* , void* bytes, int byteCount );
    void PokeServer( Errors& );
    void ClearServerLog( Errors& );
    void Log( StringView text );
    void TryConnect();

    static void TCPOnMessage( void*, Network::Socket*, void*, int );
    static void TCPOnConnectionClosed( void*, Network::Socket* );
    static void KeepAlive( void*, Network::Socket* );

    Network::Socket* mSocket                        {};
    String           mHostname                      {};
    Errors           mConnectionErrors              {};
    u16              mPort                          {};
    bool             mPrintHTTPRequest              {};
    bool             mPretendWebsocketHandshakeDone {};
    bool             mShouldSpamServer              {};
    bool             mShouldLog                     {};
    bool             mLogReceivedMessages           {};
    bool             mTryAutoConnect                {};
    GameTime        mConnectionAttemptStartSeconds {};
  };

  // Mirrored in server.js
  static const String kMatchMessageCreateRoom = "create room";
  static const String kScriptMatchmakerName = "matchmaker";
  static const String kScriptMsgDisconnect = "websocket disconnect";
  static const String kScriptMsgConnect = "websocket connect";

  using ScriptMainMenuMessageCallback = void ( ScriptMainMenu:: * )( const ScriptMsg* scriptMsg );
  using ScriptCallbacks = Set< ScriptMainMenuMessageCallback > ;

  struct ScriptMainMenu : public ScriptThread
  {
    ScriptMainMenu();
    void Update( GameTimeDelta, Errors& ) override;
    void DebugImgui( Errors& ) override;
    void AddCallbackConnect();
    void AddCallbackDisconnect();
    float                 mFadeSecTotal                 { 0.2f };
    ScriptCallbacks       mMsgCallbacks                 {};
    ScriptCallbacks       mMsgCallbacksToRemove         {};
    Timeline              mTimeline                     {};
    UILayout*             mMenu                         {};
    UIText*               mUITextServerConnectionStatus {};
    UIText*               mUITextDisconnectFromServer   {};
    UIText*               mUITextCreateRoom             {};
    UIText*               mUITextServerAutoconnect      {};
    UIText*               mUITextPressStart             {};
    bool                  mPressStart                   {};
    bool                  mCreatePressStartButton       {};
    bool                  mCreateGraveStoryButton       {};
    Render::TextureHandle mPower                        {};
  };

  struct ScriptMainMenu2 : public ScriptThread
  {
    ScriptMainMenu2();
    ~ScriptMainMenu2();
    void RenderMainMenu();
    void Update( GameTimeDelta, Errors& ) override;
    Job* mConnectToServerJob {};
  };
}
