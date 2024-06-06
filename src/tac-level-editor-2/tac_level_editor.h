#pragma once

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/window/tac_sim_window_api.h"

namespace Tac
{
  struct Creation
  {
    void                Init( Errors& );
    void                Uninit( Errors& );
    void                Update( const SimKeyboardApi,
                                const SimWindowApi,
                                Errors& );

    bool mShowUnownedWindow{};
    bool mShowOwnedWindow{};
  };

  extern Creation sCreation;

} // namespace Tac

