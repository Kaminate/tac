#include "tac_net_winsock.h" // self-inc

#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/containers/tac_set.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_json.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/dataprocess/tac_settings.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/net/tac_net.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-win32/tac_win32.h"

#include <WinSock2.h> // SOCKET
#include <Ws2tcpip.h> // inet_pton, getaddrinfo

#pragma comment( lib, "ws2_32.lib" )

namespace Tac::Network
{
  struct SocketWinsock : public Socket
  {
    ~SocketWinsock();
    void        SetIsBlocking( bool isBlocking, Errors& );
    void        SetKeepalive( bool keepAlive, Errors& );
    void        Send( void* bytes, int byteCount, Errors& ) override;
    void        TCPTryConnect( StringView hostname,
                               u16 port,
                               Errors& ) override;
    SOCKET      mSocket = INVALID_SOCKET;
    int         mWinsockAddressFamily = 0;
    int         mWinsockSocketType = 0;
    String      mHostname;
  };

  struct NetWinsock : public Net
  {
    ~NetWinsock();
    void                       Init( Errors& ) override;
    void                       DebugImgui() override;
    void                       Update( Errors& ) override;
    Socket*                    CreateSocket( StringView name,
                                             AddressFamily,
                                             SocketType,
                                             Errors& ) override;
    Vector< Socket* >          GetSockets() override;
    Set< SocketWinsock* > mSocketWinsocks;
    bool                       mPrintReceivedMessages = false;
    // TODO: Only send a keepalive if we haven't received a message within mKeepaliveIntervalSeconds
    Timestamp                  mKeepaliveNextSeconds = 0.0;
    TimestampDifference        mKeepaliveIntervalSeconds = 30.0f;
  };

  static NetWinsock sNetWinsock;

  void NetWinsockInit(Errors& errors)
  {
    sNetWinsock.Init( errors );
  }


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

  static String GetWSAErrorString( int wsaErrorCode )
  {
    // From Win32 API: FormatMessage can obtain the message string for the returned error
    const DWORD dwMessageId = wsaErrorCode;
    return Win32ErrorStringFromDWORD( dwMessageId );
  }

  static String GetLastWSAErrorString()
  {
    const int errorCode = WSAGetLastError();
    return GetWSAErrorString( errorCode );
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
      TAC_RAISE_ERROR( errMsg);
    }
  }

  void SocketWinsock::Send( void* bytes, int byteCount, Errors& errors )
  {
    Vector< u8 > framed;
    if( mRequiresWebsocketFrame )
    {
      u8 payloadByteCount7Bit = 0;

      Writer writer
      {
        .mFrom = GetEndianness(),
        .mTo = Endianness::Big,
      };

      if( byteCount < 126 )
        payloadByteCount7Bit = ( u8 )byteCount;
      else if( byteCount < 1 << 16 )
      {
        payloadByteCount7Bit = 126;
        writer.Write( ( u16 )byteCount );
      }
      else
      {
        payloadByteCount7Bit = 127;
        writer.Write( ( u64 )byteCount );
      }

      const int frameByteCount = 2 + ( int )writer.mBytes.size() + 4 + byteCount;
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

      u8 masks[ 4 ];
      for( int i = 0; i < 4; ++i )
      {
        u8 mask = std::rand() % 256;
        masks[ i ] = mask;
        framed[ iByte++ ] = mask;
      }

      u8* unmaskedBytes = ( u8* )bytes;
      for( int i = 0; i < byteCount; ++i )
      {
        u8 unmaskedByte = *unmaskedBytes++;
        u8 mask = masks[ i % 4 ];
        u8 maskedByte = unmaskedByte ^ mask;
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
      const String errMsg = GetWSAErrorString( wsaErrorCode );
      TAC_RAISE_ERROR( errMsg);
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
      TAC_RAISE_ERROR( errMsg);
    }
  }

  void SocketWinsock::TCPTryConnect( StringView hostname,
                                     u16 port,
                                     Errors& errors )
  {
    TAC_ASSERT( mSocketType == SocketType::TCP );
    const String portString = Tac::ToString( port );

    addrinfo* addrinfos;
    int wsaErrorCode = 0;
      
    wsaErrorCode = getaddrinfo( hostname.c_str(), portString.c_str(), nullptr, &addrinfos );
    if( wsaErrorCode )
    {
      const String errorMsg = GetWSAErrorString( wsaErrorCode );
      TAC_RAISE_ERROR( errorMsg);
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
      TAC_RAISE_ERROR( "Cannot find addr info");
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
      TAC_RAISE_ERROR( errorMsg);
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
      const String errorMsg = GetWSAErrorString( wsaErrorCode );
      TAC_RAISE_ERROR( errorMsg);
    }
    mPrintReceivedMessages = true;
  }

  NetWinsock::~NetWinsock()
  {
    for( SocketWinsock* netWinsocket : mSocketWinsocks )
      closesocket( netWinsocket->mSocket );
    WSACleanup();
  }

  Socket* NetWinsock::CreateSocket( StringView name,
                                    AddressFamily addressFamily,
                                    SocketType socketType,
                                    Errors& errors )
  {
    const int winsockSocketType = GetWinsockSocketType( socketType );
    const int winsockAddressFamily = GetWinsockAddressFamily( addressFamily );
    const int winsockProtocol = 0; // don't really know what this is
    const SOCKET winsockSocket = socket( winsockAddressFamily, winsockSocketType, winsockProtocol );
    TAC_RAISE_ERROR_IF_RETURN( winsockSocket == INVALID_SOCKET,
                               String() + "socket failed with: " + GetLastWSAErrorString(),
                               nullptr );

    auto netWinsocket = TAC_NEW SocketWinsock;
    TAC_ON_DESTRUCT( if( errors ) delete netWinsocket );
    netWinsocket->mSocket = winsockSocket;
    netWinsocket->mName = name;
    netWinsocket->mNet = this;
    netWinsocket->mAddressFamily = addressFamily;
    netWinsocket->mSocketType = socketType;
    netWinsocket->mWinsockAddressFamily = winsockAddressFamily;
    netWinsocket->mWinsockSocketType = winsockSocketType;
    netWinsocket->mElapsedSecondsOnLastRecv = Timestep::GetElapsedTime();
    TAC_CALL_RET( nullptr, netWinsocket->SetKeepalive( true, errors ) );
    TAC_CALL_RET( nullptr, netWinsocket->SetIsBlocking( false, errors ) );

    mSocketWinsocks.insert( netWinsocket );
    return ( Socket* )netWinsocket;
  }

  Vector< Socket* > NetWinsock::GetSockets()
  {
    Vector< Socket* > result;
    for( SocketWinsock* netWinsocket : mSocketWinsocks )
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
    bool shouldSendKeepalive = Timestep::GetElapsedTime() > mKeepaliveNextSeconds;
    if( shouldSendKeepalive )
      mKeepaliveNextSeconds += mKeepaliveIntervalSeconds;

    for( SocketWinsock* socketWinsock : mSocketWinsocks )
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

        const String errorMsg = GetWSAErrorString( wsaErrorCode );
        TAC_RAISE_ERROR( errorMsg);
      }
      else if( recvResult == 0 )
      {
        // connection has been gracefully closed
        socketWinsock->mRequestDeletion = true;
        continue;
      }

      socketWinsock->mElapsedSecondsOnLastRecv = Timestep::GetElapsedTime();
      if( mPrintReceivedMessages )
      {
        const StringView recvMsg ( recvBuf, recvResult );
        OS::OSDebugPrintLine( ShortFixedString::Concat( "Received message: ", recvMsg ) );
      }

      socketWinsock->OnMessage( recvBuf, recvResult );
    }

    Vector< SocketWinsock* > socketWinsocks;
    for( SocketWinsock* socketWinsock : mSocketWinsocks )
      if( socketWinsock->mRequestDeletion )
        socketWinsocks.push_back( socketWinsock );
    for( SocketWinsock* socketWinsock : socketWinsocks )
    {
      for( SocketCallbackData socketCallback : socketWinsock->mTCPOnConnectionClosed )
        socketCallback.mCallback( socketCallback.mUserData, socketWinsock );
      mSocketWinsocks.erase( socketWinsock );
      delete socketWinsock;
    }
  }

} // namespace Tac::Network

