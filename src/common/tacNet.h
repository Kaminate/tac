
// Networking interface ( udp, tcp websockets )

#pragma once

#include "src/common/shell/tacShell.h"
#include "src/common/string/tacString.h"
#include "src/common/tacErrorHandling.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{
  const String requestMethodGET = "GET";
  const String httpVersion = "HTTP/1.1";

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
  String ToString( AddressFamily addressFamily );

  enum class SocketType
  {
    TCP, // transmission control protocol
    UDP, // user datagram protocol
    Count,
  };
  String ToString( SocketType socketType );

  typedef void( SocketCallback )( void* userData, Socket* );
  typedef void( SocketCallbackMessage )( void* userData, Socket*, void* bytes, int byteCount );

  // Should these have a debug name?
  // Should these struct S be merged and the callback be a union?
  struct SocketCallbackData
  {
    SocketCallback*        mCallback = nullptr;
    void*                  mUserData = nullptr;
  };

  struct SocketCallbackDataMessage
  {
    SocketCallbackMessage* mCallback = nullptr;
    void*                  mUserData = nullptr;
  };

  struct Socket
  {
    virtual ~Socket() = default;
    void DebugImgui();
    void                                Send( const HTTPRequest&, Errors& );
    void                                Send( StringView s, Errors& );
    virtual void                        Send( void* bytes, int byteCount, Errors& ) = 0;
    virtual void                        TCPTryConnect( StringView hostname,
                                                       uint16_t port,
                                                       Errors& ) = 0;
    void                                OnMessage( void* bytes, int byteCount );
    String                              mName;
    SocketType                          mSocketType = SocketType::Count;
    AddressFamily                       mAddressFamily = AddressFamily::Count;
    double                              mElapsedSecondsOnLastRecv = 0;
    Net*                                mNet = nullptr;
    bool                                mTCPIsConnected = false;
    bool                                mRequestDeletion = false;
    Vector< SocketCallbackDataMessage > mTCPOnMessage;
    Vector< SocketCallbackData >        mTCPOnConnectionClosed;
    bool                                mRequiresWebsocketFrame = false;
    SocketCallbackData                  mKeepaliveOverride;
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
                                           const Vector< uint8_t > & secWebsocketKey );
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
    virtual void              Update( Errors& ) = 0;

  };

  Vector< uint8_t > GenerateSecWebsocketKey();

  String Base64Encode( const Vector< uint8_t >& );
  String Base64Encode( StringView );
  void Base64EncodeRunTests();

}

