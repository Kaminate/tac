// This file implements the input backend using the direct input library

#pragma once

namespace Tac { struct Errors; }

namespace Tac::Controller
{
  void XInputInit( Errors& );
}
