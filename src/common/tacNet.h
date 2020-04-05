
// Networking interface ( udp, tcp websockets )

#pragma once

#include "src/common/tacShell.h"
#include "src/common/tacString.h"
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

typedef void( SocketCallback )( void* userData, Socket* socket );
typedef void( SocketCallbackMessage )( void* userData, Socket* socket, void* bytes, int byteCount );

// Should these have a debug name?
// Should these struct S be merged and the callback be a union?
struct SocketCallbackData
{
  SocketCallback* mCallback = nullptr;
  void* mUserData = nullptr;
};
struct SocketCallbackDataMessage
{
  SocketCallbackMessage* mCallback = nullptr;
  void* mUserData = nullptr;
};

struct Socket
{
  virtual ~Socket() = default;
  void DebugImgui();
  void Send( const HTTPRequest& httpRequest, Errors& errors );
  void Send( const String& s, Errors& errors );
  virtual void Send( void* bytes, int byteCount, Errors& errors ) = 0;
  virtual void TCPTryConnect( const String& hostname, uint16_t port, Errors& errors ) = 0;
  void OnMessage( void* bytes, int byteCount );

  String mName;
  SocketType mSocketType = SocketType::Count;
  AddressFamily mAddressFamily = AddressFamily::Count;
  double mElapsedSecondsOnLastRecv = 0;
  Net* mNet = nullptr;
  bool mTCPIsConnected = false;
  bool mRequestDeletion = false;
  Vector< SocketCallbackDataMessage > mTCPOnMessage;
  Vector< SocketCallbackData > mTCPOnConnectionClosed;
  bool mRequiresWebsocketFrame = false;
  SocketCallbackData mKeepaliveOverride;
};

struct HTTPRequest
{
  void AddString( const String& s );
  void AddNewline();
  void AddLine( const String& s );
  void FormatRequestHTTP(
    const String& requestMethod,
    const String& host,
    const String& messageBody );
  void FormatRequestWebsocket(
    const String& uri,
    const String& host,
    const Vector< uint8_t > & secWebsocketKey );
  String ToString();
  Vector< char > mBytes;
};

struct Net
{
  static Net* Instance;
  Net();
  virtual ~Net() = default;
  virtual Socket* CreateSocket(
    const String& name,
    AddressFamily addressFamily,
    SocketType socketType,
    Errors& errors ) = 0;
  virtual Vector< Socket* > GetSockets() = 0;
  virtual void DebugImgui() = 0;
  virtual void Update( Errors& errors ) = 0;
  
};

Vector< uint8_t > GenerateSecWebsocketKey();

String Base64Encode( const Vector< uint8_t >& input );
String Base64Encode( const String& input );
void Base64EncodeRunTests();

}

