#include "tac_level_editor_sys_state.h"

#include "tac-ecs/presentation/game/tac_game_presentation.h"
#include "tac-ecs/presentation/shadow/tac_shadow_presentation.h"
#include "tac-ecs/presentation/skybox/tac_skybox_presentation.h"
#include "tac-ecs/presentation/voxel/tac_voxel_gi_presentation.h"

namespace Tac
{

  void CreationSysState::Init( IconRenderer* iconRenderer,
                               WidgetRenderer* widgetRenderer,
                               Errors& errors )
  {
    mIconRenderer = iconRenderer;
    mWidgetRenderer = widgetRenderer;
    TAC_CALL( GamePresentation::Init( errors ) );
  }
  
  void CreationSysState:: Uninit()
  {
    GamePresentation::Uninit();
  }

} // namespace Tac

