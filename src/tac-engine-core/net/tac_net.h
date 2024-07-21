
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
  String ToString( AddressFamily );

  enum class SocketType
  {
    TCP, // transmission control protocol
    UDP, // user datagram protocol
    Count,
  };
  String ToString( SocketType );

  typedef void( SocketCallback )( void* userData, Socket* );
  typedef void( SocketCallbackMessage )( void* userData, Socket*, void* bytes, int byteCount );

  // Should these have a debug name?
  // Should these struct S be merged and the callback be a union?
  struct SocketCallbackData
  {
    SocketCallback*        mCallback {};
    void*                  mUserData {};
  };

  struct SocketCallbackDataMessage
  {
    SocketCallbackMessage* mCallback {};
    void*                  mUserData {};
  };

  struct Socket
  {
    virtual ~Socket() = default;
    void DebugImgui();
    void                                Send( const HTTPRequest&, Errors& );
    void                                Send( StringView, Errors& );
    virtual void                        Send( void* bytes, int byteCount, Errors& ) = 0;
    virtual void                        TCPTryConnect( StringView hostname,
                                                       u16 port,
                                                       Errors& ) = 0;
    void                                OnMessage( void* bytes, int byteCount );

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
    void           AddString( StringView );
    void           AddNewline();
    void           AddLine( StringView );
    void           FormatRequestHTTP( StringView requestMethod,
                                      StringView host,
                                      StringView messageBody );
    void           FormatRequestWebsocket( StringView uri,
                                           StringView host,
                                           const Vector< u8 > & secWebsocketKey );
    String         ToString();
    Vector< char > mBytes;
  };

  struct Net
  {
    static Net* Instance;
    Net();
    virtual ~Net() = default;
    virtual Socket*           CreateSocket( StringView name,
                                            AddressFamily,
                                            SocketType,
                                            Errors& ) = 0;
    virtual Vector< Socket* > GetSockets() = 0;
    virtual void              DebugImgui() = 0;
    virtual void              Init( Errors& ) = 0;
    virtual void              Update( Errors& ) = 0;

  };

  Vector< u8 > GenerateSecWebsocketKey();

  String Base64Encode( const Vector< u8 >& );
  String Base64Encode( StringView );
  void Base64EncodeRunTests();


  // -----------------------------------------------------------------------------------------------

  struct NetApi
  {
    static void Update(Errors&);
  };

  // -----------------------------------------------------------------------------------------------

} // namespace Tac::Network

