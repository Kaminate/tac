#include "tacNetWinsock.h"
#include "tacPreprocessor.h"
#include "tacUtility.h"
#include "tacJson.h"
#include "tacSerialization.h"
#include "tacSettings.h"
#include "tacAlgorithm.h"

#include "imgui.h"

#include <iostream>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::miliseconds
#include <Ws2tcpip.h> // inet_pton, getaddrinfo

#pragma comment( lib, "ws2_32.lib" )

static int GetWinsockAddressFamily( TacAddressFamily addressFamily )
{
  switch( addressFamily )
  {
  case TacAddressFamily::IPv4: return AF_INET;
  case TacAddressFamily::IPv6: return AF_INET6;
    TacInvalidDefaultCase( addressFamily );
  }
  return 0;
}
static int GetWinsockSocketType( TacSocketType socketType )
{
  switch( socketType )
  {
  case TacSocketType::TCP: return SOCK_STREAM;
  case TacSocketType::UDP: return SOCK_DGRAM;
    TacInvalidDefaultCase( socketType );
  }
  return 0;
}
static TacString TacGetLastWSAErrorString()
{
  int wsaErrorCode = WSAGetLastError();
  TacString result = TacWin32ErrorToString( wsaErrorCode );
  return result;
}

TacSocketWinsock::~TacSocketWinsock()
{

  // do I care about the return value? what about the "linger" struct
  closesocket( mSocket );
}
void TacSocketWinsock::SetIsBlocking( bool isBlocking, TacErrors& errors )
{
  u_long iMode = isBlocking?0:1;
  int wsaErrorCode = ioctlsocket( mSocket, FIONBIO, &iMode );
  if( wsaErrorCode == SOCKET_ERROR )
  {
    errors = TacGetLastWSAErrorString();
    return;
  }
}
void TacSocketWinsock::Send( void* bytes, int byteCount, TacErrors& errors )
{
  TacVector< uint8_t > framed;
  if( mRequiresWebsocketFrame )
  {
    uint8_t payloadByteCount7Bit = 0;
    int payloadByteCountExtra = 0;
    TacWriter writer;
    writer.mFrom = TacGetEndianness();
    writer.mTo = TacEndianness::Big;
    if( byteCount < 126 )
      payloadByteCount7Bit = byteCount;
    else if( byteCount < 1 << 16 )
    {
      payloadByteCount7Bit = 126;
      writer.Write( ( uint16_t )byteCount );
    }
    else
    {
      payloadByteCount7Bit = 127;
      writer.Write( ( uint64_t )byteCount );
    }
    int frameByteCount = 2 + ( int )writer.mBytes.size() + 4 + byteCount;
    framed.resize( frameByteCount );
    int iByte = 0;
    framed[ iByte++ ] =
      0b10000000 | // fin
      0x2; // opcode binary frame
    framed[ iByte++ ] =
      0b10000000 | // is masked
      payloadByteCount7Bit;
    for( char b:writer.mBytes )
      framed[ iByte++ ] = b;
    uint8_t masks[ 4 ];
    for( int i = 0; i < 4; ++i )
    {
      uint8_t mask = std::rand() % 256;
      masks[ i ] = mask;
      framed[ iByte++ ] = mask;
    }
    uint8_t* unmaskedBytes = ( uint8_t* )bytes;
    for( int i = 0; i < byteCount; ++i )
    {
      uint8_t unmaskedByte = *unmaskedBytes++;
      uint8_t mask = masks[ i % 4 ];
      uint8_t maskedByte = unmaskedByte ^ mask;
      framed[ iByte++ ] = maskedByte;
    }
    TacAssert( iByte == frameByteCount );

    bytes = framed.data();
    byteCount = ( int )framed.size();
  }

  // Should we send right now or queue it for TacNetWinsock::Update?
  int wsaErrorCode = send(
    mSocket,
    ( const char* )bytes,
    byteCount,
    0 );
  if( wsaErrorCode == SOCKET_ERROR )
  {
    wsaErrorCode = WSAGetLastError();
    if( TacContains( { WSAECONNRESET, WSAECONNABORTED }, wsaErrorCode ) )
    {
      mRequestDeletion = true;
      return;
    }
    errors = TacWin32ErrorToString( wsaErrorCode );
    return;
  }
}
void TacSocketWinsock::SetKeepalive( bool keepAlive, TacErrors& errors )
{
  DWORD enableKeepalive = keepAlive?TRUE:FALSE;
  int wsaErrorCode = setsockopt( mSocket, SOL_SOCKET, SO_KEEPALIVE, ( const char* )&enableKeepalive, sizeof( enableKeepalive ) );
  if( wsaErrorCode == SOCKET_ERROR )
  {
    errors = TacGetLastWSAErrorString();
    return;
  }
}
void TacSocketWinsock::TCPTryConnect( const TacString& hostname, uint16_t port, TacErrors& errors )
{
  TacAssert( mSocketType == TacSocketType::TCP );
  auto portString = TacToString( port );
  addrinfo* addrinfos;
  int wsaErrorCode = getaddrinfo( hostname.c_str(), portString.c_str(), nullptr, &addrinfos );
  if( wsaErrorCode )
  {
    errors = TacWin32ErrorToString( wsaErrorCode );
    return;
  }
  OnDestruct( freeaddrinfo( addrinfos ) );
  addrinfo* targetAddrInfo = nullptr;
  for( addrinfo* curAddrInfo = addrinfos; curAddrInfo; curAddrInfo = curAddrInfo->ai_next )
  {
    if( curAddrInfo->ai_family != mWinsockAddressFamily )
      continue;
    targetAddrInfo = curAddrInfo;
  }
  if( !targetAddrInfo )
  {
    errors = "Cannot find addr info";
    return;
  }
  wsaErrorCode = connect( mSocket, targetAddrInfo->ai_addr, ( int )targetAddrInfo->ai_addrlen );
  if( wsaErrorCode == SOCKET_ERROR )
  {
    wsaErrorCode = WSAGetLastError();
    if( wsaErrorCode == WSAEISCONN )
    {
      mTCPIsConnected = true;
      return;
    }
    if( TacContains( {
      WSAEWOULDBLOCK, // non-blocking socket
      WSAEALREADY }, // already connected
      wsaErrorCode ) )
      return;
    errors = TacGetLastWSAErrorString();
    return;
  }
  mTCPIsConnected = true;
}

TacNetWinsock::TacNetWinsock( TacErrors& errors )
{
  WORD wsaVersion = MAKEWORD( 2, 2 );
  WSAData wsaData;
  int wsaErrorCode = WSAStartup( wsaVersion, &wsaData );
  if( wsaErrorCode )
  {
    errors = TacWin32ErrorToString( wsaErrorCode );
    return;
  }
  mPrintReceivedMessages = true;
}
TacNetWinsock::~TacNetWinsock()
{
  for( auto netWinsocket : mSocketWinsocks )
    closesocket( netWinsocket->mSocket );
  WSACleanup();
}
TacSocket* TacNetWinsock::CreateSocket(
  const TacString& name,
  TacAddressFamily addressFamily,
  TacSocketType socketType,
  TacErrors& errors )
{
  auto winsockSocketType = GetWinsockSocketType( socketType );
  auto winsockAddressFamily = GetWinsockAddressFamily( addressFamily );
  int winsockProtocol = 0; // don't really know what this is
  auto winsockSocket = socket( winsockAddressFamily, winsockSocketType, winsockProtocol );
  if( winsockSocket == INVALID_SOCKET )
  {
    errors = TacGetLastWSAErrorString();
    return nullptr;
  }

  auto netWinsocket = new TacSocketWinsock();
  OnDestruct( if( errors.size() ) delete netWinsocket );
  netWinsocket->mSocket = winsockSocket;
  netWinsocket->mName = name;
  netWinsocket->mNet = this;
  netWinsocket->mAddressFamily = addressFamily;
  netWinsocket->mSocketType = socketType;
  netWinsocket->mWinsockAddressFamily = winsockAddressFamily;
  netWinsocket->mWinsockSocketType = winsockSocketType;
  netWinsocket->mLastRecvFrame = mShell->mElapsedFrames;
  netWinsocket->SetKeepalive( true, errors );
  if( errors.size() )
    return nullptr;
  netWinsocket->SetIsBlocking( false, errors );
  if( errors.size() )
    return nullptr;
  mSocketWinsocks.insert( netWinsocket );
  return ( TacSocket* )netWinsocket;
}
TacVector< TacSocket* > TacNetWinsock::GetSockets()
{
  TacVector< TacSocket* > result;
  for( auto netWinsocket : mSocketWinsocks )
    result.push_back( netWinsocket );
  return result;
}
void TacNetWinsock::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "Network" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::Checkbox( "Print received messages", &mPrintReceivedMessages );
  ImGui::DragFloat( "Keepalive interval seconds", &mKeepaliveIntervalSeconds );
  if( ImGui::Button( "Send keepalive now" ) )
    mKeepaliveNextSeconds = 0;
  for( auto netWinsocket : mSocketWinsocks )
  {
    netWinsocket->DebugImgui();
  }
}
void TacNetWinsock::Update( TacErrors& errors )
{
  bool shouldSendKeepalive = mShell->mElapsedSeconds > mKeepaliveNextSeconds;
  if( shouldSendKeepalive )
    mKeepaliveNextSeconds += mKeepaliveIntervalSeconds;

  for( auto socketWinsock : mSocketWinsocks )
  {
    if( socketWinsock->mSocketType == TacSocketType::TCP && !socketWinsock->mTCPIsConnected )
      continue;

    if( shouldSendKeepalive )
    {
      TacString keepalive = "keepalive";
      if( socketWinsock->mKeepaliveOverride.mUserData )
      {
        socketWinsock->mKeepaliveOverride.mCallback( socketWinsock->mKeepaliveOverride.mUserData, socketWinsock );
      }
      else
      {
        socketWinsock->Send( ( void* )keepalive.data(), ( int )keepalive.size(), errors );
        if( errors.size() )
          return;
      }
    }

    const int recvBufByteCount = 1024;
    char recvBuf[ recvBufByteCount ] = {};
    int recvResult = recv( socketWinsock->mSocket, recvBuf, recvBufByteCount, 0 );
    if( recvResult == SOCKET_ERROR )
    {
      int wsaErrorCode = WSAGetLastError();
      if( wsaErrorCode == WSAEWOULDBLOCK )
        continue;
      if( TacContains( {
        WSAECONNRESET, // An existing connection was forcibly closed by the remote host
        WSAECONNABORTED // An established connection was aborted by the software in your host machine.
        }, wsaErrorCode ) )
      {
        socketWinsock->mRequestDeletion = true;
        continue;
      }
      errors = TacWin32ErrorToString( wsaErrorCode );
      return;
    }
    else if( recvResult == 0 )
    {
      // connection has been gracefully closed
      socketWinsock->mRequestDeletion = true;
      continue;
    }
    socketWinsock->mLastRecvFrame = mShell->mElapsedFrames;
    if( mPrintReceivedMessages )
    {
      auto recvString = TacString( recvBuf, recvResult );
      std::cout << "Received message: " << recvString << std::endl;
    }

    socketWinsock->OnMessage( recvBuf, recvResult );
  }

  TacVector< TacSocketWinsock* > socketWinsocks;
  for( auto socketWinsock : mSocketWinsocks )
    if( socketWinsock->mRequestDeletion )
      socketWinsocks.push_back( socketWinsock );
  for( auto socketWinsock : socketWinsocks )
  {
    for( auto socketCallback : socketWinsock->mTCPOnConnectionClosed )
      socketCallback.mCallback( socketCallback.mUserData, socketWinsock );
    mSocketWinsocks.erase( socketWinsock );
    delete socketWinsock;
  }
}
