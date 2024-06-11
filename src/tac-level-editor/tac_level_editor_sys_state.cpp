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

    TAC_CALL( SkyboxPresentationInit( errors ) );
    TAC_CALL( GamePresentationInit( errors ) );
    TAC_CALL( ShadowPresentationInit( errors ) );

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    TAC_CALL( VoxelGIPresentationInit( errors ) );
#endif

  }
  
  void CreationSysState:: Uninit()
  {
    SkyboxPresentationUninit();
    GamePresentationUninit();
    ShadowPresentationUninit();

#if TAC_VOXEL_GI_PRESENTATION_ENABLED()
    VoxelGIPresentationUninit();
#endif

  }

} // namespace Tac

