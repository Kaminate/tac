#include "tacGame.h"

#include "shell/tacDesktopApp.h"
#include "common/tacOS.h"

#include <functional>

struct TacGame
{
  void Update( TacErrors& errors )
  {
  }
  void Init( TacErrors& errors )

  {
  }
  TacDesktopApp* mApp;
  TacShell* mShell;
};

void TacDesktopApp::DoStuff( TacDesktopApp* desktopApp, TacErrors& errors )
{
  TacString appDataPath;
  TacOS::Instance->GetApplicationDataPath( appDataPath, errors );

  TacString appName = "Gravestory";
  TacString studioPath = appDataPath + "\\Sleeping Studio\\";
  TacString prefPath = studioPath + appName;

  bool appDataPathExists;
  TacOS::Instance->DoesFolderExist( appDataPath, appDataPathExists, errors );
  TacAssert( appDataPathExists );

  TacOS::Instance->CreateFolderIfNotExist( studioPath, errors );
  TAC_HANDLE_ERROR( errors );

  TacOS::Instance->CreateFolderIfNotExist( prefPath, errors );
  TAC_HANDLE_ERROR( errors );


  TacShell* shell = desktopApp->mShell;
  shell->mAppName = appName;
  shell->mPrefPath = prefPath;
  shell->SetScopedGlobals();
  shell->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->OnShellInit( errors );
  TAC_HANDLE_ERROR( errors );


  struct TacUpdater : public TacEvent<>::Handler
  {
    virtual ~TacUpdater() = default;
    void HandleEvent() override { f(); }
    std::function< void() > f;
  };


  // should this really be on the heap?
  auto game = new TacGame();
  game->mApp = desktopApp;
  game->mShell = shell;

  auto updater = new TacUpdater;
  updater->f = [ & ]() { game->Update( errors ); };

  shell->mOnUpdate.AddCallback( updater );

  game->Init( errors );
  TAC_HANDLE_ERROR( errors );

  desktopApp->Loop( errors );
  TAC_HANDLE_ERROR( errors );

  delete game;
}

