#include "tac_renderer.h" // self-inc

#include "tac-std-lib/containers/tac_fixed_vector.h"
#include "tac-std-lib/math/tac_vector2.h"
#include "tac-std-lib/algorithm/tac_algorithm.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/os/tac_os.h"

#include "tac-rhi/renderer/tac_renderer_backend.h"

#include "tac-engine-core/shell/tac_shell.h"

namespace Tac::Render
{
  Binding operator | ( Binding lhs, Binding rhs ) { return ( Binding )( ( int )lhs | ( int )rhs ); }
  Binding operator & ( Binding lhs, Binding rhs ) { return ( Binding )( ( int )lhs & ( int )rhs ); }

  const FormatElement FormatElement::sFloat = 
  {
      .mPerElementByteCount = sizeof(float),
      .mPerElementDataType = GraphicsType::real,
  };

  Format Format::FromElements( FormatElement element, int n )
  {
    return 
    {
      .mElementCount = n,
      .mPerElementByteCount = element.mPerElementByteCount,
      .mPerElementDataType = element.mPerElementDataType,
    };
  }

  const Format Format::sfloat = Format::FromElements( FormatElement::sFloat, 1 );
  const Format Format::sv2 = Format::FromElements( FormatElement::sFloat, 2 );
  const Format Format::sv3 = Format::FromElements( FormatElement::sFloat, 3 );
  const Format Format::sv4 = Format::FromElements( FormatElement::sFloat, 4 );

  static void Validate( const StringView& );

  const char* GetSemanticName( Attribute attribType )
  {
    switch( attribType )
    {
    case Attribute::Position: return "POSITION";
    case Attribute::Normal: return "NORMAL";
    case Attribute::Texcoord: return "TEXCOORD";
    case Attribute::Color: return "COLOR";
    case Attribute::BoneIndex: return "BONEINDEX";
    case Attribute::BoneWeight: return "BONEWEIGHT";
    case Attribute::Coeffs: return "COEFFS";
    default: TAC_ASSERT_INVALID_CASE( attribType ); return nullptr;
    }
  }


  // ---------------------------------------------------------------------------------------------
  ScissorRect::ScissorRect( float w, float h )
  {
    mXMaxRelUpperLeftCornerPixel = w;
    mYMaxRelUpperLeftCornerPixel = h;
  }
  ScissorRect::ScissorRect( int w, int h )
  {
    mXMaxRelUpperLeftCornerPixel = ( float )w;
    mYMaxRelUpperLeftCornerPixel = ( float )h;
  }
  ScissorRect::ScissorRect(const v2& v) : ScissorRect( v.x, v.y ) { }

  // ---------------------------------------------------------------------------------------------
  Viewport::Viewport( int w, int h ) { mWidth = ( float )w; mHeight = ( float )h; }
  Viewport::Viewport( float w, float h ) { mWidth = w; mHeight = h; }
  Viewport::Viewport( const v2& v ) : Viewport( v.x, v.y ) {}

  // ---------------------------------------------------------------------------------------------
  int Format::CalculateTotalByteCount() const
  {
    return mElementCount * mPerElementByteCount;
  }

  const float sizeInMagicUISpaceUnits = 1024.0f;

  // ---------------------------------------------------------------------------------------------

  static RendererFactory sFactories[ ( int )RendererAPI::Count ];

  RendererFactory GetRendererFactory( RendererAPI api )
  {
    return sFactories[ ( int )api ];
  }

  void SetRendererFactory( RendererAPI api, RendererFactory createFn )
  {
    sFactories[ ( int )api ] = createFn;
  }

  const char* ToString( RendererAPI api )
  {
    constexpr const char* names[] =
    {
      "Vulkan",
      "OpenGL4",
      "DirectX11",
      "DirectX12",
    };
    static_assert( TAC_ARRAY_SIZE( names ) == ( int )RendererAPI::Count );
    return names[ ( int )api ];
  }

  // ---------------------------------------------------------------------------------------------


  // ---------------------------------------------------------------------------------------------

  // ---------------------------------------------------------------------------------------------
  ShaderNameStringView::ShaderNameStringView( const char* s ) : StringView( s ) { Validate(s); }

  ShaderNameStringView::ShaderNameStringView( const StringView& s) : StringView(s) { Validate(s); }

  ShaderNameStringView::ShaderNameStringView( const ShaderNameString& s ) : StringView( s ) {}

  // ---------------------------------------------------------------------------------------------

  ShaderNameString::ShaderNameString( const StringView& s ) : String( s ) { Validate( s ); }

  // ---------------------------------------------------------------------------------------------

  static void Validate( const StringView& s )
  {
    if constexpr( !IsDebugMode )
      return;

    TAC_ASSERT( !s.empty() );
    for( char c : s )
    {
      const StringView blacklist = "./\\";
      TAC_ASSERT( !blacklist.contains( c ) );

      const StringView whitelist = "_ ";
      TAC_ASSERT( IsAlpha( c ) || IsDigit( c ) || whitelist.contains( c ) );
    }
  }

  // ---------------------------------------------------------------------------------------------

} // namespace Tac::Render
