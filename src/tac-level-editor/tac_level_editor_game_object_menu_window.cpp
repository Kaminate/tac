#include "src/common/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "tac-engine-core/window/tac_window_handle.h"
#include "src/common/input/tac_keyboard_input.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-level-editor/tac_level_editor.h"
#include "tac-level-editor/tac_level_editor_game_object_menu_window.h"
#include "tac-level-editor/tac_level_editor_main_window.h"
#include "tac-desktop-app/desktop_app/tac_desktop_app.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-ecs/entity/tac_entity.h"
#include "tac-ecs/world/tac_world.h"

namespace Tac
{
  CreationGameObjectMenuWindow* CreationGameObjectMenuWindow::Instance { nullptr };
  CreationGameObjectMenuWindow::CreationGameObjectMenuWindow()
  {
    Instance = this;
  }

  CreationGameObjectMenuWindow::~CreationGameObjectMenuWindow()
  {
    Instance = nullptr;
  }

  void CreationGameObjectMenuWindow::Init( Errors& errors )
  {
    TAC_UNUSED_PARAMETER( errors );
    mCreationSeconds = Timestep::GetElapsedTime();
  }

  void CreationGameObjectMenuWindow::Update( Errors& )
  {
  }
}

