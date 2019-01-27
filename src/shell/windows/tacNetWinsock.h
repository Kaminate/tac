// This file implements the networking backend using the winsock library

#pragma once

#include "shell/windows/tacWindows.h"
#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"
#include "common/tacNet.h"

#include <set>

#include <WinSock2.h> // SOCKET


struct TacSocketWinsock : public TacSocket
{
  ~TacSocketWinsock();
  void SetIsBlocking( bool isBlocking, TacErrors& errors );
  void SetKeepalive( bool keepAlive, TacErrors& errors );
  void Send( void* bytes, int byteCount, TacErrors& errors ) override;
  void TCPTryConnect(
    const TacString& hostname,
    uint16_t port,
    TacErrors& errors ) override;
  SOCKET mSocket = INVALID_SOCKET;
  int mWinsockAddressFamily = 0;
  int mWinsockSocketType = 0;
  TacString mHostname;
};

struct TacNetWinsock : public TacNet
{
  TacNetWinsock( TacErrors& errors );
  ~TacNetWinsock();
  void DebugImgui() override;
  void Update( TacErrors& errors ) override;
  TacSocket* CreateSocket(
    const TacString& name,
    TacAddressFamily addressFamily,
    TacSocketType socketType,
    TacErrors& errors ) override;
  TacVector< TacSocket* > GetSockets() override;
  std::set< TacSocketWinsock* > mSocketWinsocks;
  bool mPrintReceivedMessages;
  // TODO: Only send a keepalive if we haven't received a message within mKeepaliveIntervalSeconds
  double mKeepaliveNextSeconds = 0;
  float mKeepaliveIntervalSeconds = 30;
};

