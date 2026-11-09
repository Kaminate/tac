#include "tac_microtex.h"

#include "tac-std-lib/error/tac_assert.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-std-lib/math/tac_vector3.h"
#include "tac-std-lib/math/tac_matrix3.h"

#include <clocale>
#include <cuchar>
//#include <climits>

namespace Tac
{
  static void TODO()
  {
    static int todo;
    todo++;
  }
  static m3 sTransform;
  static float sLastGlyphFontSize;

  static v2 Transform( float x, float y )
  {
    const v3 xyz{ x, y, 1 };
    const v3 xyzprime{ sTransform * xyz };
    return { xyzprime.x, xyzprime.y };
  }

  struct MicroTeXImGuiFont : public microtex::Font
  {
    MicroTeXImGuiFont( const std::string& file );
    bool operator==( const Font& f ) const override;
    std::string mFile;
  };

  struct MicroTeXImGuiTextLayout : public microtex::TextLayout
  {
    MicroTeXImGuiTextLayout( const std::string& src, microtex::FontStyle style, float size );
    void getBounds( microtex::Rect& bounds ) override;
    void draw( microtex::Graphics2D& g2, float x, float y ) override;

    std::wstring _txt;
    microtex::sptr<MicroTeXImGuiFont> _font;
    float _fontSize;
  };

  MicroTeXImGuiFont::MicroTeXImGuiFont( const std::string& file )
  {
    mFile = file;
  }

  bool MicroTeXImGuiFont::operator==( const Font& f ) const
  {
    return ( ( MicroTeXImGuiFont& )f ).mFile == mFile;
  }

  // ---------------------------------------------------------------------------------------------
  MicroTeXImGuiTextLayout::MicroTeXImGuiTextLayout(
    [[maybe_unused]]const std::string& src, 
    [[maybe_unused]]microtex::FontStyle style,
    float size )
    : _fontSize( size )
  {
    TODO();
  }
  void MicroTeXImGuiTextLayout::getBounds( [[maybe_unused]]microtex::Rect& bounds )
  {
    TODO();
  }
  void MicroTeXImGuiTextLayout::draw( [[maybe_unused]]microtex::Graphics2D& g2, [[maybe_unused]]float x, [[maybe_unused]]float y )
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
  void MicroTeXImGuiGraphics::setDash( [[maybe_unused]]const std::vector<float>& dash )
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
  void MicroTeXImGuiGraphics::rotate( [[maybe_unused]]float angle )
  {
    TODO();
  }
  void MicroTeXImGuiGraphics::rotate( [[maybe_unused]]float angle, [[maybe_unused]]float px, [[maybe_unused]]float py )
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

    UI2DDrawData* drawList{ ImGuiGetDrawData() }; // ImGui::GetWindowDrawList();
    v2 pos{ Transform( x, y ) };

    ImGuiPushFontSize( _fontSize * sTransform.m11 );

    const float fontSize{ ImGuiGetFontSizePx() * sMagicTextScale };
    sLastGlyphFontSize = fontSize;

    
    pos.y -= fontSize;
    pos += ImGuiGetWindowPos(); // is this needed? theres UI2DDrawData::Text::mFontSize
    pos.x -= ImGuiGetWindowContentRegionMin().x; // ???
    pos.y += ImGuiGetWindowContentRegionMin().y; // ???

    v4 imCol32{ ToImGuiCol32( _color ) };

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
  bool MicroTeXImGuiGraphics::beginPath( [[maybe_unused]]microtex::i32 id )
  {
    // not supported
    return false;
  }
  void MicroTeXImGuiGraphics::moveTo( [[maybe_unused]]float x, [[maybe_unused]]float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::lineTo( [[maybe_unused]]float x, [[maybe_unused]]float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::cubicTo( [[maybe_unused]]float x1,
                                       [[maybe_unused]]float y1,
                                       [[maybe_unused]]float x2,
                                       [[maybe_unused]]float y2,
                                       [[maybe_unused]]float x3,
                                       [[maybe_unused]]float y3 )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::quadTo( [[maybe_unused]]float x1, [[maybe_unused]]float y1, [[maybe_unused]]float x2, [[maybe_unused]]float y2 )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::closePath()
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillPath( [[maybe_unused]]microtex::i32 id )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawText(
    [[maybe_unused]] const std::wstring & src,
    [[maybe_unused]] float x,
    [[maybe_unused]] float y )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawLine( float x1, float y1, float x2, float y2 )
  {
    if( UI2DDrawData* drawList{ ImGuiGetDrawData() } )
    {
      v2 p0 { Transform( x1, y1 ) + ImGuiGetWindowPos() };
      v2 p1{ Transform( x2, y2 ) + ImGuiGetWindowPos() };
      p0.x -= ImGuiGetWindowContentRegionMin().x; // ???
      p0.y += ImGuiGetWindowContentRegionMin().y; // ???
      p1.y += ImGuiGetWindowContentRegionMin().y; // ???
      const float thickness{ sLastGlyphFontSize * sMagicLineWidth };
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
  void MicroTeXImGuiGraphics::drawRect(
    [[maybe_unused]]float x,
    [[maybe_unused]]float y,
    [[maybe_unused]]float w,
    [[maybe_unused]]float h )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillRect(
    [[maybe_unused]]float x,
    [[maybe_unused]]float y,
    [[maybe_unused]]float w,
    [[maybe_unused]]float h )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::drawRoundRect(
    [[maybe_unused]]float x,
    [[maybe_unused]]float y,
    [[maybe_unused]]float w,
    [[maybe_unused]]float h,
    [[maybe_unused]]float rx,
    [[maybe_unused]]float ry )
  {
    TODO();
    TAC_ASSERT( false );
  }
  void MicroTeXImGuiGraphics::fillRoundRect(
    [[maybe_unused]]float x,
    [[maybe_unused]]float y,
    [[maybe_unused]]float w,
    [[maybe_unused]]float h,
    [[maybe_unused]]float rx,
    [[maybe_unused]]float ry )
  {
    TODO();
    TAC_ASSERT( false );
  }
} // namespace Tac
