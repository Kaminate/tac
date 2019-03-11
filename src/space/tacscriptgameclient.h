#pragma once
#include "space/tacscript.h"
#include "common/graphics/tacFont.h"
#include <list>
#include <set>

struct TacUILayout;
struct TacUIText;
struct TacJob;
struct TacGhost;
struct TacSocket;
struct TacScriptMainMenu;
struct TacScriptSplash;
struct TacScriptMatchmaker;
struct TacScriptGameClient;


struct TacTimelineAction
{
  virtual ~TacTimelineAction();
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
struct TacTimelineOnceAux : public TacTimelineAction
{
  TacTimelineOnceAux( double time, T t ) : mT( t )
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

#define TacTimelineOnce( time, lambdaName ) TacTimelineOnceAux< decltype( lambdaName ) >( time, lambdaName )

//#define TacTimelineOnce2( code, timelineLambdaName, timelineInstanceName )\
//  auto timelineLambdaName = [&](){ code; };\
//  OnDestructAux< decltype( lambdaName ) > dtorName( lambdaName );
//#define TacTimelineOnce( code ) TacTimelineOnce2( code, TacConcat( timelineLambda, __LINE__ ), TacConcat( timelineInstance, __LINE__ ) )


struct TacTimeline
{
  ~TacTimeline();
  void Update( double time, TacErrors& errors );
  void Add( TacTimelineAction* timelineAction );
  TacVector< TacTimelineAction* > mTimelineActions;
};


//struct TacScriptFader : public TacScriptThread
//{
//  TacScriptFader();
//  void Update( float seconds, TacErrors& errors ) override;
//  void DebugImgui( TacErrors& errors ) override;
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

struct TacScriptSplash : public TacScriptThread
{
  TacScriptSplash();
  ~TacScriptSplash();
  void Update( float seconds, TacErrors& errors ) override;
  void DebugImgui( TacErrors& errors ) override;

  TacUIText* mStudioName = nullptr;
  float mFullyVisibleSec;
  float mFadeSecTotal;
  bool mSkipSplashScreen;
};

struct TacScriptGameClient : public TacScriptThread
{
  TacScriptGameClient();
  void Update( float seconds, TacErrors& errors ) override;
  void DebugImgui( TacErrors& errors ) override;
};

struct TacScriptMatchmaker : public TacScriptThread
{
  TacScriptMatchmaker();
  void Update( float seconds, TacErrors& errors ) override;
  void DebugImgui( TacErrors& errors ) override;
  void OnScriptGameConnectionClosed( TacSocket* socket );
  void OnScriptGameMessage( TacSocket* socket, void* bytes, int byteCount );
  void PokeServer( TacErrors& errors );
  void ClearServerLog( TacErrors& errors );
  void Log( const TacString& text );
  void TryConnect();

  TacSocket* mSocket = nullptr;
  TacString mHostname;
  TacErrors mConnectionErrors;
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
const TacString MatchMessageCreateRoom = "create room";

static const TacString scriptMatchmakerName = "matchmaker";
static const TacString scriptMsgDisconnect = "websocket disconnect";
static const TacString scriptMsgConnect = "websocket connect";

typedef void ( TacScriptMainMenu:: *TacScriptMainMenuMessageCallback )( const TacScriptMsg* scriptMsg );
struct TacScriptMainMenu : public TacScriptThread
{
  TacScriptMainMenu();
  void Update( float seconds, TacErrors& errors ) override;
  void DebugImgui( TacErrors& errors ) override;

  void AddCallbackConnect();
  void AddCallbackDisconnect();

  float mFadeSecTotal = 0.2f;
  std::set< TacScriptMainMenuMessageCallback > mMsgCallbacks;
  std::set< TacScriptMainMenuMessageCallback > mMsgCallbacksToRemove;

  TacTimeline mTimeline;

  TacUILayout* mMenu = nullptr;
  TacUIText* mUITextServerConnectionStatus = nullptr;
  TacUIText* mUITextDisconnectFromServer = nullptr;
  TacUIText* mUITextCreateRoom = nullptr;
  TacUIText* mUITextServerAutoconnect = nullptr;
  TacUIText* mUITextPressStart = nullptr;
  bool mPressStart = false;


  bool mCreatePressStartButton;
  bool mCreateGraveStoryButton;

  TacTexture* mPower = nullptr;
};

struct TacScriptMainMenu2 : public TacScriptThread
{
  TacScriptMainMenu2();
  ~TacScriptMainMenu2();
  void RenderMainMenu();
  void Update( float seconds, TacErrors& errors ) override;
  TacJob* mConnectToServerJob = nullptr;
};



