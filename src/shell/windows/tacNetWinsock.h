
// This file implements the networking backend using the winsock library

#pragma once

#include "src/shell/windows/tacWindows.h"
#include "src/common/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"
#include "src/common/tacNet.h"

#include <set>

#include <WinSock2.h> // SOCKET
namespace Tac
{


struct SocketWinsock : public Socket
{
  ~SocketWinsock();
  void SetIsBlocking( bool isBlocking, Errors& errors );
  void SetKeepalive( bool keepAlive, Errors& errors );
  void Send( void* bytes, int byteCount, Errors& errors ) override;
  void TCPTryConnect(
    const String& hostname,
    uint16_t port,
    Errors& errors ) override;
  SOCKET mSocket = INVALID_SOCKET;
  int mWinsockAddressFamily = 0;
  int mWinsockSocketType = 0;
  String mHostname;
};

struct NetWinsock : public Net
{
  NetWinsock( Errors& errors );
  ~NetWinsock();
  void DebugImgui() override;
  void Update( Errors& errors ) override;
  Socket* CreateSocket(
    const String& name,
    AddressFamily addressFamily,
    SocketType socketType,
    Errors& errors ) override;
  Vector< Socket* > GetSockets() override;
  std::set< SocketWinsock* > mSocketWinsocks;
  bool mPrintReceivedMessages;
  // TODO: Only send a keepalive if we haven't received a message within mKeepaliveIntervalSeconds
  double mKeepaliveNextSeconds = 0;
  float mKeepaliveIntervalSeconds = 30;
};


}

