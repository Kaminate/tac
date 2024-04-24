// TODO: The prupose of this file is to define the api that the system thread
// can use to interact with the window system

#pragma once

#undef CreateWindow

#include "tac_window_handle.h"
#include "tac_window_api.h"

#include "tac-std-lib/string/tac_string_view.h"
#include "tac-std-lib/math/tac_vector2i.h"

namespace Tac
{
  // TODO: make not global and dependency inject
  struct SimWindowApi
  {
    bool         IsShown( WindowHandle ) const;
    v2i          GetPos( WindowHandle ) const;
    v2i          GetSize( WindowHandle ) const;
    StringView   GetName( WindowHandle ) const;
    WindowHandle CreateWindow( WindowCreateParams ) const;
    void         DestroyWindow( WindowHandle ) const;
  };

} // namespace Tac


