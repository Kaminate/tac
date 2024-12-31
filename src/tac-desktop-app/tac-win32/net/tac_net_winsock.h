// This file implements the networking backend using the winsock library

#pragma once

#include "tac-std-lib/error/tac_error_handling.h"

namespace Tac::Network
{
  void NetWinsockInit( Errors& );
}

