
// This file implements the networking backend using the winsock library

#pragma once

#include "src/shell/windows/tacwinlib/tac_win32.h"
#include "src/common/string/tac_string.h"
#include "src/common/tac_error_handling.h"
#include "src/common/containers/tac_vector.h"
#include "src/common/tac_net.h"

#include <set>

#include <WinSock2.h> // SOCKET
namespace Tac
{


  struct SocketWinsock : public Socket
  {
    ~SocketWinsock();
    void        SetIsBlocking( bool isBlocking, Errors& );
    void        SetKeepalive( bool keepAlive, Errors& );
    void        Send( void* bytes, int byteCount, Errors& ) override;
    void        TCPTryConnect( StringView hostname,
                               uint16_t port,
                               Errors& ) override;
    SOCKET      mSocket = INVALID_SOCKET;
    int         mWinsockAddressFamily = 0;
    int         mWinsockSocketType = 0;
    String      mHostname;
  };

  struct NetWinsock : public Net
  {
    ~NetWinsock();
    static NetWinsock          Instance;
    void                       Init( Errors& );
    void                       DebugImgui() override;
    void                       Update( Errors& ) override;
    Socket*                    CreateSocket( StringView name,
                                             AddressFamily,
                                             SocketType,
                                             Errors& ) override;
    Vector< Socket* >          GetSockets() override;
    std::set< SocketWinsock* > mSocketWinsocks;
    bool                       mPrintReceivedMessages = false;
    // TODO: Only send a keepalive if we haven't received a message within mKeepaliveIntervalSeconds
    double                     mKeepaliveNextSeconds = 0;
    float                      mKeepaliveIntervalSeconds = 30;
  };


}
