#pragma once
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include "common/tacShell.h"
#include "common/tacLocalization.h"
#include "common/math/tacVector4.h"
#include "common/math/tacVector2.h"
#include "common/tacRenderer.h"
#include "common/taccontrollerinput.h"
#include <set>
struct TacGhost;
struct TacTexture;
struct TacPlayer;
struct TacDepthBuffer;
struct TacLocalization;
struct TacServerData;
struct TacClientData;
struct TacShell;
struct TacFontStuff;
struct TacRenderer;
struct TacFontAtlasCell;
struct TacScriptRoot;
struct TacUIText;
struct TacUILayout;
struct TacUIRoot;
struct TacIndexBuffer;
struct TacVertexBuffer;
struct TacController;
struct TacNewI;



//
// A user stores data for a person interactiong with this application.
// It's convenient to have multiple users in one application to test multiplayer 
//
struct TacUser
{
  TacUser( TacGhost* ghost, const TacString& name, TacErrors& errors );
  void Update( TacErrors& errors );
  void DebugImgui();
  TacPlayer* mPlayer = nullptr;
  TacGhost* mGhost = nullptr;
  TacString mName;
  bool mHasControllerIndex = false;
  TacControllerIndex mControllerIndex = TAC_CONTROLLER_COUNT_MAX;
};


//
// The ghost is the platform-agnostic portion of the application
// In release, there's one ghost per application
// In debug, however, you can create multiple ghosts for testing
//
// Should the ghost and the soul be merged?
// The original idea was for the ghost to be the hotloaded game ddl
// for iteration, but that hasn't happened.
//
struct TacGhost : public TacSoul
{
  TacGhost( TacShell* shell, TacErrors& errors );
  virtual ~TacGhost();
  void Init( TacErrors& errors );
  void Update( TacErrors& errors ) override;
  void ImguiCreatePlayerPopup( TacErrors& errors );
  TacUser* AddPlayer( const TacString& name, TacErrors& errors );
  void DebugImgui( TacErrors& errors ) override;
  void PopulateWorldInitial();
  void Draw( TacErrors& errors );
  void AddMorePlayers( TacErrors& errors );
  bool CanDrawImgui();
  bool IsPartyFull();

  TacVector< TacUser* > mUsers;
  TacServerData* mServerData = nullptr;
  TacClientData* mClientData = nullptr;
  TacScriptRoot* mScriptRoot = nullptr;
  TacErrors mLevelLoadErrors;
  v4 mClearColor = v4( v3( 27, 33, 40 ) / 255.0f, 1 );
  bool mDrawText = true;
  float mSplashAlpha = 0;
  //TacTexture* mFBOTexture = nullptr;
  //TacTexture* mDrawTexture = nullptr;
  //TacDepthBuffer* mFBODepthBuffer = nullptr;
  //TacDepthBuffer* mDrawDepthBuffer = nullptr;
  TacUIRoot* mUIRoot = nullptr;
  bool mIsGrabbingInput = true;
  bool mDrawDirectlyToScreen = true;
  v2 mMouserCursorNDC = {};
  bool mMouseHoveredOverWindow = false;
  float mImguiImageW = 0;
  float mImguiImageH = 0;
  float mImguiImagePosRelTopLeftX = 0;
  float mImguiImagePosRelTopLeftY = 0;
  bool mShouldPopulateWorldInitial = false;
};

const TacString scriptMsgNameUserConnect = "user connect";
