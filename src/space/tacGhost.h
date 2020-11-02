
#pragma once

#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacShell.h"
#include "src/common/tacLocalization.h"
#include "src/common/math/tacVector4.h"
#include "src/common/math/tacVector2.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacControllerInput.h"

namespace Tac
{
  struct Ghost;
  struct Texture;
  struct Player;
  struct DepthBuffer;
  struct Localization;
  struct ServerData;
  struct ClientData;
  struct Shell;
  struct FontStuff;
  struct Renderer;
  struct FontAtlasCell;
  struct ScriptRoot;
  struct UIText;
  struct UILayout;
  struct UIRoot;
  struct IndexBuffer;
  struct VertexBuffer;
  struct Controller;
  struct NewI;

  //
  // A user stores data for a person interactiong with this application.
  // It's convenient to have multiple users in one application to test multiplayer 
  //
  struct User
  {
    User( Ghost* ghost, StringView name, Errors& );
    void            Update( Errors& );
    void            DebugImgui();
    Player*         mPlayer = nullptr;
    Ghost*          mGhost = nullptr;
    String          mName;
    bool            mHasControllerIndex = false;
    ControllerIndex mControllerIndex = TAC_CONTROLLER_COUNT_MAX;
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
  struct Ghost : public Soul
  {
    Ghost();
    virtual ~Ghost();
    void            Init( Errors& ) override;
    void            Update( Errors& ) override;
    void            ImguiCreatePlayerPopup( Errors& );
    User*           AddPlayer( StringView name, Errors& );
    void            DebugImgui( Errors& ) override;
    void            PopulateWorldInitial();
    void            Draw( Errors& );
    void            AddMorePlayers( Errors& );
    bool            CanDrawImgui();
    bool            IsPartyFull();
    Vector< User* > mUsers;
    ServerData*     mServerData = nullptr;
    ClientData*     mClientData = nullptr;
    ScriptRoot*     mScriptRoot = nullptr;
    Errors          mLevelLoadErrors;
    v4              mClearColor = v4( v3( 27, 33, 40 ) / 255.0f, 1 );
    bool            mDrawText = true;
    float           mSplashAlpha = 0;
    bool            mIsGrabbingInput = true;
    bool            mDrawDirectlyToScreen = true;
    v2              mMouserCursorNDC = {};
    bool            mMouseHoveredOverWindow = false;
    float           mImguiImageW = 0;
    float           mImguiImageH = 0;
    float           mImguiImagePosRelTopLeftX = 0;
    float           mImguiImagePosRelTopLeftY = 0;
    bool            mShouldPopulateWorldInitial = false;
  };

  const String scriptMsgNameUserConnect = "user connect";

}

