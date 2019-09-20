#include "common/tacNet.h"
#include "common/tacPreprocessor.h"
#include "common/tacUtility.h"
#include "common/tacTime.h"

TacString ToString( TacAddressFamily addressFamily )
{
  switch( addressFamily )
  {
  case TacAddressFamily::IPv4: return "IPv4";
  case TacAddressFamily::IPv6:return "IPv6";
    TacInvalidDefaultCase( addressFamily );
  }
  return "";
}
TacString ToString( TacSocketType socketType )
{
  switch( socketType )
  {
  case TacSocketType::TCP: return "TCP";
  case TacSocketType::UDP: return "UDP";
    TacInvalidDefaultCase( socketType );
  }
  return "";
}

void TacSocket::DebugImgui()
{
  //if( !ImGui::CollapsingHeader( mName.c_str() ) )
  //  return;
  //ImGui::Indent();
  //OnDestruct( ImGui::Unindent() );
  //TacString time = TacFormatFrameTime( mNet->mShell->mElapsedSeconds - mElapsedSecondsOnLastRecv );
  //ImGui::Text( "Address Family: %s", ToString( mAddressFamily ).c_str() );
  //ImGui::Text( "Socket Type: %s", ToString( mSocketType ).c_str() );
  //ImGui::Text( "Last recvd msg: %s", time.c_str() );
  //if( ImGui::Button( "Request socket deletion" ) )
  //  mRequestDeletion = true;
  //ImGui::Checkbox( "TCP is connected", &mTCPIsConnected );
}

void TacSocket::Send( const TacString& s, TacErrors& errors )
{
  Send( ( void* )s.data(), ( int )s.size(), errors );
}
void TacSocket::Send( const TacHTTPRequest& httpRequest, TacErrors& errors )
{
  Send( ( void* )httpRequest.mBytes.data(), ( int )httpRequest.mBytes.size(), errors );
}
void TacSocket::OnMessage( void* bytes, int byteCount )
{
  for( auto socketCallback : mTCPOnMessage )
    socketCallback.mCallback( socketCallback.mUserData, this, bytes, byteCount );
}

void TacHTTPRequest::AddString( const TacString& s )
{
  for( char c:s )
  {
    mBytes.push_back( c );
  }
}
void TacHTTPRequest::AddNewline()
{
  AddString( "\r\n" );
}
void TacHTTPRequest::AddLine( const TacString& s )
{
  AddString( s );
  AddNewline();
}
TacString TacHTTPRequest::ToString()
{
  return TacString( mBytes.data(), ( int )mBytes.size() );
}
void TacHTTPRequest::FormatRequestHTTP(
  const TacString& requestMethod,
  const TacString& host,
  const TacString& messageBody )
{
  TacString serverRoot = "/";
  // uniform resource identifier
  TacString& uri = serverRoot;
  TacString request = TacSeparateSpace( { requestMethod, uri, httpVersion } );
  TacString connectionType = "keep-alive";
  TacString fieldConnection = "Connection: " + connectionType;
  TacString fieldHost = "Host: " + host;
  AddLine( request );
  AddLine( fieldConnection );
  AddLine( fieldHost );
  AddNewline();
  AddString( messageBody );
}

TacString TacBase64Encode( const TacVector< uint8_t >& input )
{
  // See also:
  //   - https://tools.ietf.org/html/rfc4648#section-4
  //   - https://en.wikipedia.org/wiki/Base64
  //   - atlenc.h
  TacString result;
  const char* lookupTable =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";
  int iInput = 0;
  while( iInput < int( input.size() / 3 ) * 3 )
  {
    uint32_t accumulator = 0;
    // Push inputs onto the right
    for( int n = 0; n < 3; n++ )
    {
      accumulator |= input[ iInput++ ];
      accumulator <<= 8;
    }
    // Pop lookup keys off of the left
    for( int k = 0; k < 4; k++ )
    {
      result += lookupTable[ accumulator >> 26 ];
      accumulator <<= 6;
    }
  }
  int remaining = ( int )input.size() - iInput;
  switch( remaining )
  {
  case 0:
    // Do nothing, input is cleanly divisible by 3
    break;
  case 1:
    result += lookupTable[ input[ iInput + 0 ] >> 2 ];
    result += lookupTable[ ( input[ iInput + 0 ] << 4 ) & 0b110000 ];
    result += '=';
    result += '=';
    break;
  case 2:
    result += lookupTable[ input[ iInput + 0 ] >> 2 ];
    result += lookupTable[ ( ( input[ iInput + 0 ] << 4 ) & 0b110000 ) | ( ( input[ iInput + 1 ] >> 4 ) & 0b001111 ) ];
    result += lookupTable[ ( input[ iInput + 1 ] << 2 ) & 0b111100 ];
    result += '=';
    break;
    TacInvalidDefaultCase( remaining );
  }
  return result;
}
TacString TacBase64Encode( const TacString& input )
{
  return TacBase64Encode( TacVector< uint8_t >( ( uint8_t*) input.begin(), ( uint8_t*)input.end() ) );
}
void TacBase64EncodeRunTests()
{
  for( auto test : TacVector< std::pair< TacString, TacString >>{
    { "any carnal pleasure.", "YW55IGNhcm5hbCBwbGVhc3VyZS4=" },
    { "any carnal pleasure", "YW55IGNhcm5hbCBwbGVhc3VyZQ==" },
    { "any carnal pleasur", "YW55IGNhcm5hbCBwbGVhc3Vy" },
    { "any carnal pleasu", "YW55IGNhcm5hbCBwbGVhc3U=" },
    { "any carnal pleas", "YW55IGNhcm5hbCBwbGVhcw==" },
    } )
  {
    TacString input = test.first;
    TacString outputExpected = test.second;
    TacString outputActual = TacBase64Encode( input );
    bool correct = outputActual == outputExpected;
    TacAssert( correct );
  }
}

const int websocketKeyByteCount = 16;
TacVector< uint8_t > TacGenerateSecWebsocketKey()
{
  TacVector< uint8_t > result;
  for( int i = 0; i < websocketKeyByteCount; ++i )
  {
    uint8_t element( std::rand() % 256 );
    result.push_back( element );
  }
  return result;
}

void TacHTTPRequest::FormatRequestWebsocket(
  const TacString& uri,
  const TacString& host,
  const TacVector< uint8_t > & secWebsocketKey )
{
  TacAssert( secWebsocketKey.size() == websocketKeyByteCount );
  TacString encoded = TacBase64Encode( secWebsocketKey );

  // The Websocket Protocol - Opening Handshake
  // https://tools.ietf.org/html/rfc6455#section-1.3
  AddLine( TacSeparateSpace( { requestMethodGET, uri, httpVersion } ) );
  AddLine( "Host: " + host );
  AddLine( "Upgrade: websocket" );
  AddLine( "Connection: upgrade" );
  AddLine( "Sec-WebSocket-Key: " + encoded );
  AddLine( "Sec-WebSocket-Version: 13" );
  AddLine( "Sec-WebSocket-Protocol: tacbogus" );
  AddNewline();
}

TacNet* TacNet::Instance = nullptr;
TacNet::TacNet()
{
  Instance = this;
}
