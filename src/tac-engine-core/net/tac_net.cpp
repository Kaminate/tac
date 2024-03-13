#include "tac_net.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/string/tac_string_util.h"
#include "tac-std-lib/tac_ints.h"

//#include <cstdlib> // std::rand

namespace Tac::Network
{
  String ToString( AddressFamily addressFamily )
  {
    switch( addressFamily )
    {
      case AddressFamily::IPv4: return "IPv4";
      case AddressFamily::IPv6:return "IPv6";
      default: TAC_ASSERT_INVALID_CASE( addressFamily ); return "";
    }
  }
  String ToString( SocketType socketType )
  {
    switch( socketType )
    {
      case SocketType::TCP: return "TCP";
      case SocketType::UDP: return "UDP";
      default: TAC_ASSERT_INVALID_CASE( socketType ); return "";
    }
  }

  void Socket::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( mName.c_str() ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //String time = FormatFrameTime( mNet->Timestep::GetElapsedTime() - mElapsedSecondsOnLastRecv );
    //ImGui::Text( "Address Family: %s", ToString( mAddressFamily ).c_str() );
    //ImGui::Text( "Socket Type: %s", ToString( mSocketType ).c_str() );
    //ImGui::Text( "Last recvd msg: %s", time.c_str() );
    //if( ImGui::Button( "Request socket deletion" ) )
    //  mRequestDeletion = true;
    //ImGui::Checkbox( "TCP is connected", &mTCPIsConnected );
  }

  void Socket::Send( StringView s, Errors& errors )
  {
    Send( ( void* )s.data(), ( int )s.size(), errors );
  }
  void Socket::Send( const HTTPRequest& httpRequest, Errors& errors )
  {
    Send( ( void* )httpRequest.mBytes.data(), ( int )httpRequest.mBytes.size(), errors );
  }
  void Socket::OnMessage( void* bytes, int byteCount )
  {
    for( const SocketCallbackDataMessage& socketCallback : mTCPOnMessage )
      socketCallback.mCallback( socketCallback.mUserData, this, bytes, byteCount );
  }

  void HTTPRequest::AddString( StringView s )
  {
    for( char c : s )
    {
      mBytes.push_back( c );
    }
  }
  void HTTPRequest::AddNewline()
  {
    AddString( "\r\n" );
  }
  void HTTPRequest::AddLine( StringView s )
  {
    AddString( s );
    AddNewline();
  }
  String HTTPRequest::ToString()
  {
    return String( mBytes.data(), ( int )mBytes.size() );
  }
  void HTTPRequest::FormatRequestHTTP( StringView requestMethod,
                                       StringView host,
                                       StringView messageBody )
  {
    String serverRoot = "/";
    // uniform resource identifier
    String& uri = serverRoot;
    String request = requestMethod + " " + uri + " " + httpVersion;
    String connectionType = "keep-alive";
    String fieldConnection = "Connection: " + connectionType;
    String fieldHost = "Host: " + String(host);
    AddLine( request );
    AddLine( fieldConnection );
    AddLine( fieldHost );
    AddNewline();
    AddString( messageBody );
  }

  String Base64Encode( const Vector< u8 >& input )
  {
    // See also:
    //   - https://tools.ietf.org/html/rfc4648#section-4
    //   - https://en.wikipedia.org/wiki/Base64
    //   - atlenc.h
    String result;
    const char* lookupTable =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
      "abcdefghijklmnopqrstuvwxyz"
      "0123456789+/";
    int iInput = 0;
    while( iInput < int( input.size() / 3 ) * 3 )
    {
      u32 accumulator = 0;
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
      {
        int a = ( input[ iInput + 0 ] >> 2 ) & 0b111111;
        int b = ( input[ iInput + 0 ] << 4 ) & 0b110000;
        result += lookupTable[ a ];
        result += lookupTable[ b ];
        result += '=';
        result += '=';
      } break;
      case 2:
      {
        int a = ( input[ iInput + 0 ] >> 2 ) & 0b111111;
        int b = ( input[ iInput + 0 ] << 4 ) & 0b110000;
        int c = ( input[ iInput + 1 ] >> 4 ) & 0b001111;
        int d = ( input[ iInput + 1 ] << 2 ) & 0b111100;
        result += lookupTable[ a ];
        result += lookupTable[ b | c ];
        result += lookupTable[ d ];
        result += '=';
      } break;
      default: TAC_ASSERT_INVALID_CASE( remaining ); return 0;
    }
    return result;
  }

  String Base64Encode( StringView input )
  {
    return Base64Encode( Vector< u8 >( ( u8* )input.begin(), ( u8* )input.end() ) );
  }

  void Base64EncodeRunTests()
  {
    struct Test
    {
      String decoded;
      String encoded;
    };

    const Test tests[] =
    {
      { "any carnal pleasure.", "YW55IGNhcm5hbCBwbGVhc3VyZS4=" },
      { "any carnal pleasure", "YW55IGNhcm5hbCBwbGVhc3VyZQ==" },
      { "any carnal pleasur", "YW55IGNhcm5hbCBwbGVhc3Vy" },
      { "any carnal pleasu", "YW55IGNhcm5hbCBwbGVhc3U=" },
      { "any carnal pleas", "YW55IGNhcm5hbCBwbGVhcw==" },
    };

    for( const Test& test : tests )
    {
      const String encoded = Base64Encode( test.decoded );
      TAC_ASSERT( encoded == test.encoded );
    }
  }

  const int websocketKeyByteCount = 16;
  Vector< u8 > GenerateSecWebsocketKey()
  {
    Vector< u8 > result;
    for( int i = 0; i < websocketKeyByteCount; ++i )
    {
      u8 element( std::rand() % 256 );
      result.push_back( element );
    }
    return result;
  }

  void HTTPRequest::FormatRequestWebsocket( StringView uri,
                                            StringView host,
                                            const Vector< u8 > & secWebsocketKey )
  {
    TAC_ASSERT( secWebsocketKey.size() == websocketKeyByteCount );
    String encoded = Base64Encode( secWebsocketKey );

    // The Websocket Protocol - Opening Handshake
    // https://tools.ietf.org/html/rfc6455#section-1.3
    AddLine( requestMethodGET + String( " " ) + String( uri ) + String( " " ) + String( httpVersion ) );
    AddLine( "Host: " + String( host ) );
    AddLine( "Upgrade: websocket" );
    AddLine( "Connection: upgrade" );
    AddLine( "Sec-WebSocket-Key: " + encoded );
    AddLine( "Sec-WebSocket-Version: 13" );
    AddLine( "Sec-WebSocket-Protocol: Bogus" );
    AddNewline();
  }

  Net* Net::Instance = nullptr;
  Net::Net()
  {
    Instance = this;
  }

  // -----------------------------------------------------------------------------------------------


  void NetApi::Update(Errors& errors )
  {
    Net* net = Net::Instance;
    if( !net )
      return;

    net->Update( errors );
  }

} // namespace Tac::Network

