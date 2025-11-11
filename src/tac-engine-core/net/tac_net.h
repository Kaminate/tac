
// Networking interface ( udp, tcp websockets )

#pragma once

//#include "tac-engine-core/shell/tac_shell.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-std-lib/string/tac_string.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/tac_ints.h"



namespace Tac::Network
{
  const String requestMethodGET { "GET" };
  const String httpVersion { "HTTP/1.1" };

  enum class AddressFamily;
  enum class SocketType;
  struct Socket;
  struct HTTPRequest;
  struct Net;

  enum class AddressFamily
  {
    IPv4,
    IPv6,
    Count,
  };
  auto ToString( AddressFamily ) -> String;

  enum class SocketType
  {
    TCP, // transmission control protocol
    UDP, // user datagram protocol
    Count,
  };
  auto ToString( SocketType ) -> String;

  using SocketCallback = void( * )( void* userData, Socket* );
  using SocketCallbackMessage = void( * )( void* userData, Socket*, void* bytes, int byteCount );

  // Should these have a debug name?
  // Should these struct S be merged and the callback be a union?
  struct SocketCallbackData
  {
    SocketCallback         mCallback {};
    void*                  mUserData {};
  };

  struct SocketCallbackDataMessage
  {
    SocketCallbackMessage  mCallback {};
    void*                  mUserData {};
  };

  struct Socket
  {
    virtual ~Socket() = default;
    void DebugImgui();
    void Send( const HTTPRequest&, Errors& );
    void Send( StringView, Errors& );
    void OnMessage( void* bytes, int byteCount );
    virtual void Send( void* bytes, int byteCount, Errors& ) = 0;
    virtual void TCPTryConnect( StringView hostname, u16 port, Errors& ) = 0;

    String                              mName                     {};
    SocketType                          mSocketType               { SocketType::Count };
    AddressFamily                       mAddressFamily            { AddressFamily::Count };
    Timestamp                           mElapsedSecondsOnLastRecv {};
    Net*                                mNet                      {};
    bool                                mTCPIsConnected           {};
    bool                                mRequestDeletion          {};
    Vector< SocketCallbackDataMessage > mTCPOnMessage             {};
    Vector< SocketCallbackData >        mTCPOnConnectionClosed    {};
    bool                                mRequiresWebsocketFrame   {};
    SocketCallbackData                  mKeepaliveOverride        {};
  };

  struct HTTPRequest
  {
    void AddString( StringView );
    void AddNewline();
    void AddLine( StringView );
    void FormatRequestHTTP( StringView requestMethod, StringView host, StringView messageBody );
    void FormatRequestWebsocket( StringView uri, StringView host, const Vector< u8 > & secWebsocketKey );
    auto ToString() -> String;
    Vector< char > mBytes;
  };

  struct Net
  {
    static Net* Instance;
    Net();
    virtual ~Net() = default;
    virtual auto CreateSocket( StringView name, AddressFamily, SocketType, Errors& ) -> Socket* = 0;
    virtual auto GetSockets() -> Vector< Socket* > = 0;
    virtual void DebugImgui() = 0;
    virtual void Init( Errors& ) = 0;
    virtual void Update( Errors& ) = 0;
  };

  auto GenerateSecWebsocketKey() -> Vector< u8 >;
  auto Base64Encode( const Vector< u8 >& ) -> String;
  auto Base64Encode( StringView ) -> String;
  void Base64EncodeRunTests();


  // -----------------------------------------------------------------------------------------------

  struct NetApi
  {
    static void Update(Errors&);
  };

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Network

