#include "tac_color_util.h" // self-inc

#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/dataprocess/tac_serialization.h"
#include "tac-std-lib/math/tac_math.h"

namespace Tac
{

  v4        GetColorSchemeA( float t )
  {
    v4 clearColorRGBA;
    v3 a = { 0.8f, 0.5f, 0.4f };
    v3 b = { 0.2f, 0.4f, 0.2f };
    v3 c = { 2.0f, 1.0f, 1.0f };
    v3 d = { 0.0f, 0.25f, 0.25f };
    for( int i = 0; i < 3; ++i )
    {
      float v = c[ i ];
      v *= ( float )t;
      v *= 0.15f;
      v += d[ i ];
      v *= 2.0f;
      v *= 3.14f;
      v = Cos( v );
      v *= b[ i ];
      v += a[ i ];
      clearColorRGBA[ i ] = v;
    }
    clearColorRGBA[ 3 ] = 1.0f;
    return clearColorRGBA;
  }

  // -----------------------------------------------------------------------------------------------

  void      RGBToHSV( const v3& inputRGB, float* h, float* s, float* v )
  {
    float r = inputRGB.x;
    float g = inputRGB.y;
    float b = inputRGB.z;
    float maxi = Max( Max( r, g ), b );
    float mini = Min( Min( r, g ), b );
    float delta = maxi - mini;
    if( maxi == r ) *h = ( ( ( g - b ) / delta ) + 0 ) / 6.0f;
    else if( maxi == g ) *h = ( ( ( b - r ) / delta ) + 2 ) / 6.0f;
    else if( maxi == b ) *h = ( ( ( r - g ) / delta ) + 4 ) / 6.0f;
    else *h = 0;
    *v = maxi;
    *s = *v > 0 ? delta / *v : 0;
  }

  v3        RGBToHSV( const v3& rgb)
  {
    v3 hsv;
    RGBToHSV(rgb, &hsv[0], &hsv[1], &hsv[2]);
    return hsv;
  }

  void      HSVToRGB( float h, float s, float v, v3* outputRGB )
  {
    float c = v * s; // c = chroma
    float h_prime = h * 6.0f;
    float x = c * ( 1 - Abs( Fmod( h_prime, 2.0f ) - 1 ) );
    v3 rgb1 = {};
    if( h_prime <= 6 ) rgb1 = { c, 0, x };
    if( h_prime <= 5 ) rgb1 = { x, 0, c };
    if( h_prime <= 4 ) rgb1 = { 0, x, c };
    if( h_prime <= 3 ) rgb1 = { 0, c, x };
    if( h_prime <= 2 ) rgb1 = { x, c, 0 };
    if( h_prime <= 1 ) rgb1 = { c, x, 0 };
    float m = v - c;
    *outputRGB = rgb1 + v3( 1, 1, 1 ) * m;
  }

  v3        HSVToRGB( const v3& hsv)
  {
    v3 rgb;
    HSVToRGB(hsv[0], hsv[1], hsv[2], &rgb);
    return rgb;
  }

  // -----------------------------------------------------------------------------------------------

  static v3 HexToRGBAux( u32 hexColor, int i0, int i1, int i2 )
  {
    return v3(
      ( ( u8* )&hexColor )[ i0 ],
      ( ( u8* )&hexColor )[ i1 ],
      ( ( u8* )&hexColor )[ i2 ] ) / 255.0f;
  }

  v3        HexToRGB( u32 hexColor ) // 0xRRGGBB
  {
    const Endianness endianness = GetEndianness();
    switch( endianness )
    {
      case Endianness::Big: return HexToRGBAux( hexColor, 1, 2, 3 );
      case Endianness::Little: return HexToRGBAux( hexColor, 2, 1, 0 );
      default: TAC_ASSERT_INVALID_CASE( endianness ); return {};
    }
  }
}
