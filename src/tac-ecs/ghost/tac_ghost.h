#pragma once

#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/i18n/tac_localization.h"
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-engine-core/hid/controller/tac_controller_input.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-ecs/tac_space.h"

namespace Tac
{
  struct Texture;
  struct DepthBuffer;
  struct Localization;
  struct FontStuff;
  struct Renderer;
  struct FontAtlasCell;
  struct ScriptRoot;
  struct UIText;
  struct UILayout;
  struct IndexBuffer;
  struct VertexBuffer;
  //struct Controller;
  struct Ghost;

  //
  // A user stores data for a person interactiong with this application.
  // It's convenient to have multiple users in one application to test multiplayer 
  //
  struct User
  {
    User( StringView name, Ghost*, Errors& );
    void                   Update( Errors& );
    void                   DebugImgui();

    Player*                mPlayer               {};
    Ghost*                 mGhost                {};
    String                 mName                 {};
    bool                   mHasControllerIndex   {};
    ControllerIndex        mControllerIndex      { TAC_CONTROLLER_COUNT_MAX };
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
  struct Ghost
  {
    Ghost();
    virtual ~Ghost();

    void            Init( SettingsNode, Errors& );
    void            Update( Errors& );
    void            DebugImgui( Errors& );
    void            ImguiCreatePlayerPopup( Errors& );
    User*           AddPlayer( StringView name, Errors& );
    void            PopulateWorldInitial();
    void            Draw( Errors& );
    void            AddMorePlayers( Errors& );
    bool            CanDrawImgui();
    bool            IsPartyFull();

    Vector< User* > mUsers                      {};
    ServerData*     mServerData                 {};
    ClientData*     mClientData                 {};
    ScriptRoot*     mScriptRoot                 {};
    Errors          mLevelLoadErrors            {};
    v4              mClearColor                 { v3( 27, 33, 40 ) / 255.0f, 1 };
    bool            mDrawText                   { true };
    float           mSplashAlpha                {};
    bool            mIsGrabbingInput            { true };
    bool            mDrawDirectlyToScreen       { true };
    v2              mMouserCursorNDC            {};
    bool            mMouseHoveredOverWindow     {};
    float           mImguiImageW                {};
    float           mImguiImageH                {};
    float           mImguiImagePosRelTopLeftX   {};
    float           mImguiImagePosRelTopLeftY   {};
    bool            mShouldPopulateWorldInitial {};
    SettingsNode    mSettingsNode               {};
  };

} // namespace Tac

