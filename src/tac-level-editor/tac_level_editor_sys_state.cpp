#include "tac_level_editor_sys_state.h"

#include "tac-ecs/presentation/tac_game_presentation.h"
#include "tac-ecs/presentation/tac_shadow_presentation.h"
#include "tac-ecs/presentation/tac_skybox_presentation.h"
#include "tac-ecs/presentation/tac_voxel_gi_presentation.h"

namespace Tac
{

  void CreationSysState::Init( IconRenderer* iconRenderer,
                               WidgetRenderer* widgetRenderer,
                               Errors& errors )
  {
    mIconRenderer = iconRenderer;
    mWidgetRenderer = widgetRenderer;

    TAC_CALL( GamePresentationInit( errors ) );


  }
  
  void CreationSysState:: Uninit()
  {
    GamePresentationUninit();


  }

} // namespace Tac

