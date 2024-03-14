#pragma once

#include "tac-ecs/script/tac_script.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
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
  struct Ghost;
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
    double       mTimeBegin = 0;
    double       mTimeEnd = 0;
    bool         mPlayedBegin = false;
    bool         mPlayedEnd = false;
    bool         mPlayedUpdate = false;
  };

  struct Timeline
  {
    ~Timeline();
    void                      Update( double time, Errors& );
    void                      Add( TimelineAction* );
    Vector< TimelineAction* > mTimelineActions;
  };

  struct ScriptSplash : public ScriptThread
  {
    ScriptSplash();
    ~ScriptSplash();
    void    Update( float seconds, Errors& ) override;
    void    DebugImgui( Errors& ) override;
    UIText* mStudioName = nullptr;
    float   mFullyVisibleSec;
    float   mFadeSecTotal;
    bool    mSkipSplashScreen;
  };

  struct ScriptGameClient : public ScriptThread
  {
    ScriptGameClient();
    void Update( float seconds, Errors& ) override;
    void DebugImgui( Errors& ) override;
  };

  struct ScriptMatchmaker : public ScriptThread
  {
    ScriptMatchmaker();
    void     Update( float seconds, Errors& ) override;
    void     DebugImgui( Errors& ) override;
    void     OnScriptGameConnectionClosed( Network::Socket* );
    void     OnScriptGameMessage( Network::Socket* , void* bytes, int byteCount );
    void     PokeServer( Errors& );
    void     ClearServerLog( Errors& );
    void     Log( StringView text );
    void     TryConnect();

    static void TCPOnMessage( void*, Network::Socket*, void*, int );
    static void TCPOnConnectionClosed( void*, Network::Socket* );
    static void KeepAlive( void*, Network::Socket* );

    Network::Socket* mSocket = nullptr;
    String           mHostname;
    Errors           mConnectionErrors;
    u16              mPort = 0;
    bool             mPrintHTTPRequest = false;
    bool             mPretendWebsocketHandshakeDone = false;
    bool             mShouldSpamServer = false;
    bool             mShouldLog = false;
    bool             mLogReceivedMessages = false;
    bool             mTryAutoConnect = false;
    Timestamp        mConnectionAttemptStartSeconds;
  };

  // Mirrored in server.js
  const String MatchMessageCreateRoom = "create room";

  static const String scriptMatchmakerName = "matchmaker";
  static const String scriptMsgDisconnect = "websocket disconnect";
  static const String scriptMsgConnect = "websocket connect";

  typedef void ( ScriptMainMenu:: *ScriptMainMenuMessageCallback )( const ScriptMsg* scriptMsg );
  typedef Set< ScriptMainMenuMessageCallback > ScriptCallbacks;

  struct ScriptMainMenu : public ScriptThread
  {
    ScriptMainMenu();
    void                  Update( float seconds, Errors& ) override;
    void                  DebugImgui( Errors& ) override;
    void                  AddCallbackConnect();
    void                  AddCallbackDisconnect();
    float                 mFadeSecTotal = 0.2f;
    ScriptCallbacks       mMsgCallbacks;
    ScriptCallbacks       mMsgCallbacksToRemove;
    Timeline              mTimeline;
    UILayout*             mMenu = nullptr;
    UIText*               mUITextServerConnectionStatus = nullptr;
    UIText*               mUITextDisconnectFromServer = nullptr;
    UIText*               mUITextCreateRoom = nullptr;
    UIText*               mUITextServerAutoconnect = nullptr;
    UIText*               mUITextPressStart = nullptr;
    bool                  mPressStart = false;
    bool                  mCreatePressStartButton;
    bool                  mCreateGraveStoryButton;
    Render::TextureHandle mPower;
  };

  struct ScriptMainMenu2 : public ScriptThread
  {
    ScriptMainMenu2();
    ~ScriptMainMenu2();
    void RenderMainMenu();
    void Update( float seconds, Errors& ) override;
    Job* mConnectToServerJob = nullptr;
  };
}
