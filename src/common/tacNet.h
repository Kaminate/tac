// Networking interface ( udp, tcp websockets )

#pragma once

#include "tacShell.h"

#include "common/tacString.h"
#include "common/tacErrorHandling.h"
#include "common/containers/tacVector.h"

const TacString requestMethodGET = "GET";
const TacString httpVersion = "HTTP/1.1";

enum class TacAddressFamily;
enum class TacSocketType;
struct TacSocket;
struct TacHTTPRequest;
struct TacNet;

enum class TacAddressFamily
{
  IPv4,
  IPv6,
  Count,
};
TacString ToString( TacAddressFamily addressFamily );

enum class TacSocketType
{
  TCP, // transmission control protocol
  UDP, // user datagram protocol
  Count,
};
TacString ToString( TacSocketType socketType );

typedef void( TacSocketCallback )( void* userData, TacSocket* socket );
typedef void( TacSocketCallbackMessage )( void* userData, TacSocket* socket, void* bytes, int byteCount );

// Should these have a debug name?
// Should these structs be merged and the callback be a union?
struct TacSocketCallbackData
{
  TacSocketCallback* mCallback = nullptr;
  void* mUserData = nullptr;
};
struct TacSocketCallbackDataMessage
{
  TacSocketCallbackMessage* mCallback = nullptr;
  void* mUserData = nullptr;
};

struct TacSocket
{
  virtual ~TacSocket() = default;
  void DebugImgui();
  void Send( const TacHTTPRequest& httpRequest, TacErrors& errors );
  void Send( const TacString& s, TacErrors& errors );
  virtual void Send( void* bytes, int byteCount, TacErrors& errors ) = 0;
  virtual void TCPTryConnect( const TacString& hostname, uint16_t port, TacErrors& errors ) = 0;
  void OnMessage( void* bytes, int byteCount );

  TacString mName;
  TacSocketType mSocketType = TacSocketType::Count;
  TacAddressFamily mAddressFamily = TacAddressFamily::Count;
  double mElapsedSecondsOnLastRecv = 0;
  TacNet* mNet = nullptr;
  bool mTCPIsConnected = false;
  bool mRequestDeletion = false;
  TacVector< TacSocketCallbackDataMessage > mTCPOnMessage;
  TacVector< TacSocketCallbackData > mTCPOnConnectionClosed;
  bool mRequiresWebsocketFrame = false;
  TacSocketCallbackData mKeepaliveOverride;
};

struct TacHTTPRequest
{
  void AddString( const TacString& s );
  void AddNewline();
  void AddLine( const TacString& s );
  void FormatRequestHTTP(
    const TacString& requestMethod,
    const TacString& host,
    const TacString& messageBody );
  void FormatRequestWebsocket(
    const TacString& uri,
    const TacString& host,
    const TacVector< uint8_t > & secWebsocketKey );
  TacString ToString();
  TacVector< char > mBytes;
};

struct TacNet
{
  static TacNet* Instance;
  TacNet();
  virtual ~TacNet() = default;
  virtual TacSocket* CreateSocket(
    const TacString& name,
    TacAddressFamily addressFamily,
    TacSocketType socketType,
    TacErrors& errors ) = 0;
  virtual TacVector< TacSocket* > GetSockets() = 0;
  virtual void DebugImgui() = 0;
  virtual void Update( TacErrors& errors ) = 0;
  TacShell* mShell = nullptr;
};

TacVector< uint8_t > TacGenerateSecWebsocketKey();

TacString TacBase64Encode( const TacVector< uint8_t >& input );
TacString TacBase64Encode( const TacString& input );
void TacBase64EncodeRunTests();
