#pragma once
#include "src/space/tacScript.h"
#include "src/common/graphics/tacFont.h"
#include <list>
#include <set>

struct UILayout;
struct UIText;
struct Job;
struct Ghost;
struct Socket;
struct ScriptMainMenu;
struct ScriptSplash;
struct ScriptMatchmaker;
struct ScriptGameClient;

namespace Tac
{



struct TimelineAction
{
  virtual ~TimelineAction();
  virtual void Begin();
  virtual void End();
  virtual void Update( float percent );
  double mTimeBegin = 0;
  double mTimeEnd = 0;
  bool mPlayedBegin = false;
  bool mPlayedEnd = false;
  bool mPlayedUpdate = false;
};

template< typename T >
struct TimelineOnceAux : public TimelineAction
{
  TimelineOnceAux( double time, T t ) : mT( t )
  {
    mTimeBegin = time;
    mTimeEnd = time;
  }
  void Begin() override
  {
    mT();
  }
  T mT;
};

#define TimelineOnce( time, lambdaName ) TimelineOnceAux< decltype( lambdaName ) >( time, lambdaName )



struct Timeline
{
  ~Timeline();
  void Update( double time, Errors& errors );
  void Add( TimelineAction* timelineAction );
  Vector< TimelineAction* > mTimelineActions;
};


//struct ScriptFader : public ScriptThread
//{
//  ScriptFader();
//  void Update( float seconds, Errors& errors ) override;
//  void DebugImgui( Errors& errors ) override;
//  void SetAlpha( float alpha );
//
//  float mValueInitial = 0;
//  float mValueFinal = 1;
//  float mPreFadeSec;
//  float mPostFadeSec;
//  float mFadeSecTotal;
//  float mFadeSecElapsed;
//  bool mShouldFade;
//  float* mValue = nullptr;
//};

struct ScriptSplash : public ScriptThread
{
  ScriptSplash();
  ~ScriptSplash();
  void Update( float seconds, Errors& errors ) override;
  void DebugImgui( Errors& errors ) override;

  UIText* mStudioName = nullptr;
  float mFullyVisibleSec;
  float mFadeSecTotal;
  bool mSkipSplashScreen;
};

struct ScriptGameClient : public ScriptThread
{
  ScriptGameClient();
  void Update( float seconds, Errors& errors ) override;
  void DebugImgui( Errors& errors ) override;
};

struct ScriptMatchmaker : public ScriptThread
{
  ScriptMatchmaker();
  void Update( float seconds, Errors& errors ) override;
  void DebugImgui( Errors& errors ) override;
  void OnScriptGameConnectionClosed( Socket* socket );
  void OnScriptGameMessage( Socket* socket, void* bytes, int byteCount );
  void PokeServer( Errors& errors );
  void ClearServerLog( Errors& errors );
  void Log( const String& text );
  void TryConnect();

  Socket* mSocket = nullptr;
  String mHostname;
  Errors mConnectionErrors;
  uint16_t mPort;
  bool mPrintHTTPRequest;
  bool mPretendWebsocketHandshakeDone = false;
  bool mShouldSpamServer;
  bool mShouldLog;
  bool mLogReceivedMessages = false;
  bool mTryAutoConnect;
  double mConnectionAttemptStartSeconds;
};

// Mirrored in server.js
const String MatchMessageCreateRoom = "create room";

static const String scriptMatchmakerName = "matchmaker";
static const String scriptMsgDisconnect = "websocket disconnect";
static const String scriptMsgConnect = "websocket connect";

typedef void ( ScriptMainMenu:: *ScriptMainMenuMessageCallback )( const ScriptMsg* scriptMsg );
struct ScriptMainMenu : public ScriptThread
{
  ScriptMainMenu();
  void Update( float seconds, Errors& errors ) override;
  void DebugImgui( Errors& errors ) override;

  void AddCallbackConnect();
  void AddCallbackDisconnect();

  float mFadeSecTotal = 0.2f;
  std::set< ScriptMainMenuMessageCallback > mMsgCallbacks;
  std::set< ScriptMainMenuMessageCallback > mMsgCallbacksToRemove;

  Timeline mTimeline;

  UILayout* mMenu = nullptr;
  UIText* mUITextServerConnectionStatus = nullptr;
  UIText* mUITextDisconnectFromServer = nullptr;
  UIText* mUITextCreateRoom = nullptr;
  UIText* mUITextServerAutoconnect = nullptr;
  UIText* mUITextPressStart = nullptr;
  bool mPressStart = false;


  bool mCreatePressStartButton;
  bool mCreateGraveStoryButton;

  Texture* mPower = nullptr;
};

struct ScriptMainMenu2 : public ScriptThread
{
  ScriptMainMenu2();
  ~ScriptMainMenu2();
  void RenderMainMenu();
  void Update( float seconds, Errors& errors ) override;
  Job* mConnectToServerJob = nullptr;
};



}
