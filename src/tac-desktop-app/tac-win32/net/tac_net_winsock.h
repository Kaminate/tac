// This file implements the networking backend using the winsock library

#pragma once

namespace Tac { struct Errors; }
namespace Tac::Network
{

  void NetWinsockInit( Errors& );

}

