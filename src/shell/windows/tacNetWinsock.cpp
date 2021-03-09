#include "src/shell/windows/tacNetWinsock.h"
#include "src/common/tacPreprocessor.h"
#include "src/common/tacUtility.h"
#include "src/common/tacJson.h"
#include "src/common/tacSerialization.h"
#include "src/common/tacShellTimer.h"
#include "src/common/tacSettings.h"
#include "src/common/tacAlgorithm.h"
#include "src/common/tacMemory.h"
#include <iostream>
#include <thread> // std::this_thread::sleep_for
#include <chrono> // std::chrono::miliseconds
#include <Ws2tcpip.h> // inet_pton, getaddrinfo

#pragma comment( lib, "ws2_32.lib" )

namespace Tac
{
  NetWinsock NetWinsock::Instance;

  static int GetWinsockAddressFamily( AddressFamily addressFamily )
  {
    switch( addressFamily )
    {
      case AddressFamily::IPv4: return AF_INET;
      case AddressFamily::IPv6: return AF_INET6;
      default: TAC_ASSERT_INVALID_CASE( addressFamily ); return 0;
    }
  }

  static int GetWinsockSocketType( SocketType socketType )
  {
    switch( socketType )
    {
      case SocketType::TCP: return SOCK_STREAM;
      case SocketType::UDP: return SOCK_DGRAM;
      default: TAC_ASSERT_INVALID_CASE( socketType ); return 0;
    }
  }

  static String GetLastWSAErrorString()
  {
    return Win32ErrorToString( WSAGetLastError() );
  }

  SocketWinsock::~SocketWinsock()
  {

    // do I care about the return value? what about the "linger" struct
    closesocket( mSocket );
  }

  void SocketWinsock::SetIsBlocking( const bool isBlocking, Errors& errors )
  {
    u_long iMode = isBlocking ? 0 : 1;
    const int wsaErrorCode = ioctlsocket( mSocket, FIONBIO, &iMode );
    if( wsaErrorCode == SOCKET_ERROR )
    {
      const String errMsg = GetLastWSAErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  void SocketWinsock::Send( void* bytes, int byteCount, Errors& errors )
  {
    Vector< uint8_t > framed;
    if( mRequiresWebsocketFrame )
    {
      uint8_t payloadByteCount7Bit = 0;
      //int payloadByteCountExtra = 0;
      Writer writer;
      writer.mFrom = GetEndianness();
      writer.mTo = Endianness::Big;
      if( byteCount < 126 )
        payloadByteCount7Bit = ( uint8_t )byteCount;
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
      for( char b : writer.mBytes )
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
      TAC_ASSERT( iByte == frameByteCount );

      bytes = framed.data();
      byteCount = ( int )framed.size();
    }

    // Should we send right now or queue it for NetWinsock::Update?
    const char* sendBytes = ( const char* )bytes;
    int wsaErrorCode = send( mSocket,
                             sendBytes,
                             byteCount,
                             0 );
    if( wsaErrorCode == SOCKET_ERROR )
    {
      wsaErrorCode = WSAGetLastError();
      if( wsaErrorCode == WSAECONNRESET ||
          wsaErrorCode == WSAECONNABORTED )
      {
        mRequestDeletion = true;
        return;
      }
      const String errMsg = Win32ErrorToString( wsaErrorCode );
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  void SocketWinsock::SetKeepalive( bool keepAlive, Errors& errors )
  {
    const DWORD enableKeepalive = keepAlive ? TRUE : FALSE;
    const int wsaErrorCode = setsockopt( mSocket,
                                         SOL_SOCKET,
                                         SO_KEEPALIVE,
                                         ( const char* )&enableKeepalive,
                                         sizeof( enableKeepalive ) );
    if( wsaErrorCode == SOCKET_ERROR )
    {
      const String errMsg =  GetLastWSAErrorString();
      TAC_RAISE_ERROR( errMsg, errors );
    }
  }

  void SocketWinsock::TCPTryConnect( StringView hostname,
                                     uint16_t port,
                                     Errors& errors )
  {
    TAC_ASSERT( mSocketType == SocketType::TCP );
    auto portString = ToString( port );
    addrinfo* addrinfos;
    int wsaErrorCode = getaddrinfo( hostname.c_str(), portString.c_str(), nullptr, &addrinfos );
    if( wsaErrorCode )
    {
      const String errorMsg = Win32ErrorToString( wsaErrorCode );
      TAC_RAISE_ERROR( errorMsg, errors );
    }
    TAC_ON_DESTRUCT( freeaddrinfo( addrinfos ) );
    addrinfo* targetAddrInfo = nullptr;
    for( addrinfo* curAddrInfo = addrinfos; curAddrInfo; curAddrInfo = curAddrInfo->ai_next )
    {
      if( curAddrInfo->ai_family != mWinsockAddressFamily )
        continue;
      targetAddrInfo = curAddrInfo;
    }
    if( !targetAddrInfo )
      TAC_RAISE_ERROR( "Cannot find addr info", errors );
    wsaErrorCode = connect( mSocket, targetAddrInfo->ai_addr, ( int )targetAddrInfo->ai_addrlen );
    if( wsaErrorCode == SOCKET_ERROR )
    {
      wsaErrorCode = WSAGetLastError();
      if( wsaErrorCode == WSAEISCONN ) // 10056A connect request was made on an already connected socket.
      {
        mTCPIsConnected = true;
        return;
      }
      if( wsaErrorCode == WSAEWOULDBLOCK || // 10035 non-blocking socket
          wsaErrorCode == WSAEALREADY ) // 10037 already connected
        return;
      const String errorMsg = GetLastWSAErrorString();
      TAC_RAISE_ERROR( errorMsg, errors );
    }
    mTCPIsConnected = true;
  }

  void NetWinsock::Init( Errors& errors )
  {
    const WORD wsaVersion = MAKEWORD( 2, 2 );
    WSAData wsaData;
    const int wsaErrorCode = WSAStartup( wsaVersion, &wsaData );
    if( wsaErrorCode )
    {
      const String errorMsg = Win32ErrorToString( wsaErrorCode );
      TAC_RAISE_ERROR( errorMsg, errors );
    }
    mPrintReceivedMessages = true;
  }

  NetWinsock::~NetWinsock()
  {
    for( auto netWinsocket : mSocketWinsocks )
      closesocket( netWinsocket->mSocket );
    WSACleanup();
  }

  Socket* NetWinsock::CreateSocket( StringView name,
                                    AddressFamily addressFamily,
                                    SocketType socketType,
                                    Errors& errors )
  {
    const auto winsockSocketType = GetWinsockSocketType( socketType );
    const auto winsockAddressFamily = GetWinsockAddressFamily( addressFamily );
    const int winsockProtocol = 0; // don't really know what this is
    const SOCKET winsockSocket = socket( winsockAddressFamily, winsockSocketType, winsockProtocol );
    if( winsockSocket == INVALID_SOCKET )
    {
      const String errorMsg = GetLastWSAErrorString();
      errors.Append( errorMsg );
      errors.Append( TAC_STACK_FRAME );
      return nullptr;
    }

    auto netWinsocket = TAC_NEW SocketWinsock;
    TAC_ON_DESTRUCT( if( errors ) delete netWinsocket );
    netWinsocket->mSocket = winsockSocket;
    netWinsocket->mName = name;
    netWinsocket->mNet = this;
    netWinsocket->mAddressFamily = addressFamily;
    netWinsocket->mSocketType = socketType;
    netWinsocket->mWinsockAddressFamily = winsockAddressFamily;
    netWinsocket->mWinsockSocketType = winsockSocketType;
    netWinsocket->mElapsedSecondsOnLastRecv = ShellGetElapsedSeconds();
    netWinsocket->SetKeepalive( true, errors );
    if( errors )
      return nullptr;
    netWinsocket->SetIsBlocking( false, errors );
    if( errors )
      return nullptr;
    mSocketWinsocks.insert( netWinsocket );
    return ( Socket* )netWinsocket;
  }

  Vector< Socket* > NetWinsock::GetSockets()
  {
    Vector< Socket* > result;
    for( auto netWinsocket : mSocketWinsocks )
      result.push_back( netWinsocket );
    return result;
  }

  void NetWinsock::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Network" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::Checkbox( "Print received messages", &mPrintReceivedMessages );
    //ImGui::DragFloat( "Keepalive interval seconds", &mKeepaliveIntervalSeconds );
    //if( ImGui::Button( "Send keepalive now" ) )
    //  mKeepaliveNextSeconds = 0;
    //for( auto netWinsocket : mSocketWinsocks )
    //{
    //  netWinsocket->DebugImgui();
    //}
  }

  void NetWinsock::Update( Errors& errors )
  {
    bool shouldSendKeepalive = ShellGetElapsedSeconds() > mKeepaliveNextSeconds;
    if( shouldSendKeepalive )
      mKeepaliveNextSeconds += mKeepaliveIntervalSeconds;

    for( auto socketWinsock : mSocketWinsocks )
    {
      if( socketWinsock->mSocketType == SocketType::TCP &&
          !socketWinsock->mTCPIsConnected )
        continue;

      if( shouldSendKeepalive )
      {
        String keepalive = "keepalive";
        if( socketWinsock->mKeepaliveOverride.mUserData )
        {
          socketWinsock->mKeepaliveOverride.mCallback( socketWinsock->mKeepaliveOverride.mUserData, socketWinsock );
        }
        else
        {
          socketWinsock->Send( ( void* )keepalive.data(), ( int )keepalive.size(), errors );
          if( errors )
            return;
        }
      }

      const int recvBufByteCount = 1024;
      char recvBuf[ recvBufByteCount ] = {};
      int recvResult = recv( socketWinsock->mSocket, recvBuf, recvBufByteCount, 0 );
      if( recvResult == SOCKET_ERROR )
      {
        const int wsaErrorCode = WSAGetLastError();
        if( wsaErrorCode == WSAEWOULDBLOCK )
          continue;
        if( wsaErrorCode == WSAECONNRESET || // An existing connection was forcibly closed by the remote host
            wsaErrorCode == WSAECONNABORTED ) // An established connection was aborted by the software in your host machine.
        {
          socketWinsock->mRequestDeletion = true;
          continue;
        }
        const String errorMsg = Win32ErrorToString( wsaErrorCode );
        TAC_RAISE_ERROR( errorMsg, errors );
      }
      else if( recvResult == 0 )
      {
        // connection has been gracefully closed
        socketWinsock->mRequestDeletion = true;
        continue;
      }
      socketWinsock->mElapsedSecondsOnLastRecv = ShellGetElapsedSeconds();
      if( mPrintReceivedMessages )
      {
        std::cout << "Received message: " << StringView( recvBuf, recvResult ).c_str() << std::endl;
      }

      socketWinsock->OnMessage( recvBuf, recvResult );
    }

    Vector< SocketWinsock* > socketWinsocks;
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

}

