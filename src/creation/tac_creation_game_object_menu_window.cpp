#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "src/common/graphics/tac_ui_2d.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/system/tac_desktop_window.h"
#include "src/common/core/tac_event.h"
#include "src/common/input/tac_keyboard_input.h"
#include "src/common/system/tac_os.h"
#include "src/common/shell/tac_shell.h"
#include "src/creation/tac_creation.h"
#include "src/creation/tac_creation_game_object_menu_window.h"
#include "src/creation/tac_creation_main_window.h"
#include "src/shell/tac_desktop_app.h"
#include "src/common/shell/tac_shell_timer.h"
#include "src/space/tac_entity.h"
#include "src/space/tac_world.h"

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

