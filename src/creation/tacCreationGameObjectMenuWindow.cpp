#include "src/common/assetmanagers/tacTextureAssetManager.h"
#include "src/common/graphics/tacUI2D.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacDesktopWindow.h"
#include "src/common/tacEvent.h"
#include "src/common/tacKeyboardinput.h"
#include "src/common/tacOS.h"
#include "src/common/shell/tacShell.h"
#include "src/creation/tacCreation.h"
#include "src/creation/tacCreationGameObjectMenuWindow.h"
#include "src/creation/tacCreationMainWindow.h"
#include "src/shell/tacDesktopApp.h"
#include "src/common/shell/tacShellTimer.h"
#include "src/space/tacEntity.h"
#include "src/space/tacWorld.h"

namespace Tac
{
  CreationGameObjectMenuWindow* CreationGameObjectMenuWindow::Instance = nullptr;
  CreationGameObjectMenuWindow::CreationGameObjectMenuWindow()
  {
    Instance = this;
    mCreationSeconds = 0;
  }

  CreationGameObjectMenuWindow::~CreationGameObjectMenuWindow()
  {
    Instance = nullptr;
  }

  void CreationGameObjectMenuWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mCreationSeconds = ShellGetElapsedSeconds();
  }

  void CreationGameObjectMenuWindow::Update( Errors& )
  {
  }
}

