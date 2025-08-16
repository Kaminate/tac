#include "tac_microtex.h"

#include "tac-std-lib/error/tac_assert.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix3.h"

#include <clocale>
#include <cuchar>
#include <climits>

namespace Tac
{
  static void TODO()
  {
    static int todo;
    todo++;
  }
  static m3 sTransform;

  static v2 Transform( float x, float y )
  {
    const v3 xyz{ x, y, 1 };
    const v3 xyzprime{ sTransform * xyz };
    return { xyzprime.x, xyzprime.y };
  }

  MicroTeXImGuiFont::MicroTeXImGuiFont( const std::string& file )
  {
    mFile = file;
  }

  bool MicroTeXImGuiFont::operator==( const Font& f ) const
  {
    return ( ( MicroTeXImGuiFont& )f ).mFile == mFile;
  }

  // ---------------------------------------------------------------------------------------------
  MicroTeXImGuiTextLayout::MicroTeXImGuiTextLayout( const std::string& src, microtex::FontStyle style, float size )
    : _fontSize( size )
  {
    TODO();
  }
  void MicroTeXImGuiTextLayout::getBounds( microtex::Rect& bounds )
  {
    TODO();
  }
  void MicroTeXImGuiTextLayout::draw( microtex::Graphics2D& g2, float x, float y )
  {
    TODO();
  }

  // ---------------------------------------------------------------------------------------------
  auto MicroTexImGuiPlatformFactory::createFont( const std::string& file ) -> microtex::sptr<microtex::Font>
  {
    return microtex::sptr<microtex::Font>( new MicroTeXImGuiFont( file ) );
  };
  auto MicroTexImGuiPlatformFactory::createTextLayout( const std::string& src, microtex::FontStyle style, float size ) -> microtex::sptr<microtex::TextLayout>
  {
    return microtex::sptr<microtex::TextLayout>( new MicroTeXImGuiTextLayout( src, style, size ) );
  };

  // ---------------------------------------------------------------------------------------------

  void MicroTeXImGuiGraphics::setColor( microtex::color c )
  {
    _color = c;
  }
  auto MicroTeXImGuiGraphics::getColor() const -> microtex::color { return _color; }
  void MicroTeXImGuiGraphics::setStroke( const microtex::Stroke& s )
  {
    _stroke = s;
  }
  auto MicroTeXImGuiGraphics::getStroke() const -> const microtex::Stroke& { return _stroke; }
  void MicroTeXImGuiGraphics::setStrokeWidth( float w )
  {
    _stroke.lineWidth = w;
  }
  void MicroTeXImGuiGraphics::setDash( const std::vector<float>& dash )
  {
    TODO();
  }
  auto MicroTeXImGuiGraphics::getDash() -> std::vector<float>
  {
    TODO();
    return {};
  }
  auto MicroTeXImGuiGraphics::getFont() const -> microtex::sptr<microtex::Font> { return _font; }
  void MicroTeXImGuiGraphics::setFont( const microtex::sptr<microtex::Font>& font )
  {
    _font = std::static_pointer_cast< MicroTeXImGuiFont >( font );
  }
  auto MicroTeXImGuiGraphics::getFontSize() const -> float { return _fontSize; }
  void MicroTeXImGuiGraphics::setFontSize( float size ) { _fontSize = size; }
  void MicroTeXImGuiGraphics::translate( float dx, float dy )
  {
    //sTransform = m3::Translate( dx, dy ) * sTransform;
    sTransform = sTransform * m3::Translate( dx, dy );
  }
  void MicroTeXImGuiGraphics::scale( float sx, float sy )
  {
    sTransform = sTransform * m3::Scale( sx, sy );
  }
  void MicroTeXImGuiGraphics::rotate( float angle )
  {
    TODO();
  }
  void MicroTeXImGuiGraphics::rotate( float angle, float px, float py )
  {
    TODO();
  }
  void MicroTeXImGuiGraphics::reset()
  {
    sTransform = {};
    _sx = 1;
    _sy = 1;
    TODO();
    TAC_ASSERT( false );
  }
  auto MicroTeXImGuiGraphics::sx() const -> float { return _sx; }
  auto MicroTeXImGuiGraphics::sy() const -> float { return _sy; }



  size_t to_utf8( char32_t codepoint, char* buf )
  {
    const char* loc { std::setlocale( LC_ALL, "en_US.utf8" ) };
    std::mbstate_t state{};
    std::size_t len{ std::c32rtomb( buf, codepoint, &state ) };
    std::setlocale( LC_ALL, loc );
    return len;
  }

  float MicroTeXImGuiGraphics::sMagicTextScale{ .9f };
  float MicroTeXImGuiGraphics::sMagicLineWidth{ 0.05f };

  static v4 ToImGuiCol32( microtex::color _color )
  {
    float r{ microtex::color_r( _color ) / 255.0f };
    float g{ microtex::color_g( _color ) / 255.0f };
    float b{ microtex::color_b( _color ) / 255.0f };
    float a{ microtex::color_a( _color ) / 255.0f };
    return { r, g, b, a };
  }

  struct UTF8CodepointString
  {
    UTF8CodepointString( microtex::u16 glyph )
    {
      const char* loc = std::setlocale( LC_ALL, "en_US.utf8" );
      std::setlocale( LC_ALL, loc );
      std::mbstate_t state{};
      len = std::c16rtomb( buf, glyph, &state );
    }
    char buf[ 10 ]{};
    std::size_t len;
  };

  void MicroTeXImGuiGraphics::drawGlyph( microtex::u16 glyph, float x, float y )
  {
    TAC_ASSERT( _font );

    UTF8CodepointString str( glyph );

    //const char* loc = std::setlocale( LC_ALL, "en_US.utf8" );
    //std::mbstate_t state{};
    //char buf[ 10 ]{};
    //std::size_t len = std::c16rtomb( buf, glyph, &state );
    //std::setlocale( LC_ALL, loc );

    auto drawList = ImGuiGetDrawData(); // ImGui::GetWindowDrawList();
    v2 pos = Transform( x, y );

    ImGuiPushFontSize( _fontSize * sTransform.m11 );

    float fontSize = ImGuiGetFontSize() * sMagicTextScale;
    sLastGlyphFontSize = fontSize;

    
    pos.y -= fontSize;
    pos += ImGuiGetWindowPos();
    pos.x -= ImGuiGetWindowContentRegionMin().x; // ???
    pos.y += ImGuiGetWindowContentRegionMin().y; // ???

    auto imCol32 = ToImGuiCol32( _color );

    UI2DDrawData::Text text
    {
      .mPos      { pos },
      .mFontSize { fontSize },
      .mUtf8     { StringView( str.buf, str.buf + str.len ) },
      .mColor    { imCol32 },
    };

    drawList->AddText( text );
    ImGuiPopFontSize();
  }
  bool MicroTeXImGuiGraphics::beginPath( microtex::i32 id )
  {
    // not supported
    return false;
  }
  void MicroTeXImGuiGraphics::moveTo( float x, float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::lineTo( float x, float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::cubicTo( float x1, float y1, float x2, float y2, float x3, float y3 )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::quadTo( float x1, float y1, float x2, float y2 )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::closePath()
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillPath( microtex::i32 id )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawText( const std::wstring& src, float x, float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawLine( float x1, float y1, float x2, float y2 )
  {
    if( auto drawList = ImGuiGetDrawData() )
    {
      v2 p0 = Transform( x1, y1 ) + ImGuiGetWindowPos();
      v2 p1 = Transform( x2, y2 ) + ImGuiGetWindowPos();
      p0.x -= ImGuiGetWindowContentRegionMin().x; // ???
      p0.y += ImGuiGetWindowContentRegionMin().y; // ???
      p1.y += ImGuiGetWindowContentRegionMin().y; // ???
      const float thickness = sLastGlyphFontSize * sMagicLineWidth;
      const UI2DDrawData::Line line
      {
        .mP0         { p0 },
        .mP1         { p1 },
        .mLineRadius { thickness },
        .mColor      { ToImGuiCol32( _color ) },
      };
      drawList->AddLine( line );
    }
  }
  void MicroTeXImGuiGraphics::drawRect( float x, float y, float w, float h )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillRect( float x, float y, float w, float h )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawRoundRect( float x, float y, float w, float h, float rx, float ry )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillRoundRect( float x, float y, float w, float h, float rx, float ry )
  {
    TODO();
    TAC_ASSERT( false );
  }
} // namespace Tac
