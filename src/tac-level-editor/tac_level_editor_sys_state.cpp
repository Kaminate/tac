#include "tac_level_editor_sys_state.h" // self-inc

#include "tac-ecs/renderpass/game/tac_game_presentation.h"

namespace Tac
{
  void CreationSysState::Init( Errors& errors ) { TAC_CALL( GamePresentation::Init( errors ) ); }
  void CreationSysState:: Uninit() { GamePresentation::Uninit(); }

} // namespace Tac

