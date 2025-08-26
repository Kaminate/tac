#include <microtex/microtex.h>

#include <string>
#include <vector>

namespace Tac
{

  struct MicroTexImGuiPlatformFactory : public microtex::PlatformFactory
  {
    auto createFont( const std::string& file ) -> microtex::sptr<microtex::Font> override;
    auto createTextLayout( const std::string& src, microtex::FontStyle style, float size ) -> microtex::sptr<microtex::TextLayout> override;
  };

  class MicroTeXImGuiGraphics : public microtex::Graphics2D
  {
  private:

    microtex::color                _color    { microtex::white };
    microtex::sptr<microtex::Font> _font     {};
    microtex::Stroke               _stroke   {};
    float                          _fontSize { 0 };
    float                          _sx       { 1 };
    float                          _sy       { 1 };

  public:
    static float sMagicTextScale;
    static float sMagicLineWidth;

    void setColor( microtex::color c ) override;
    auto getColor() const -> microtex::color override;
    void setStroke( const microtex::Stroke& s ) override;
    auto getStroke() const -> const microtex::Stroke & override;
    void setStrokeWidth( float w ) override;
    void setDash( const std::vector<float>& dash ) override;
    auto getDash() -> std::vector<float> override;
    auto getFont() const -> microtex::sptr<microtex::Font>override;
    void setFont( const microtex::sptr<microtex::Font>& font ) override;
    auto getFontSize() const -> float override;
    void setFontSize( float size ) override;
    void translate( float dx, float dy ) override;
    void scale( float sx, float sy ) override;
    void rotate( float angle ) override;
    void rotate( float angle, float px, float py ) override;
    void reset() override;
    auto sx() const -> float override;
    auto sy() const -> float override;
    void drawGlyph( microtex::u16 glyph, float x, float y ) override;
    bool beginPath( microtex::i32 id ) override;
    void moveTo( float x, float y ) override;
    void lineTo( float x, float y ) override;
    void cubicTo( float x1, float y1, float x2, float y2, float x3, float y3 ) override;
    void quadTo( float x1, float y1, float x2, float y2 ) override;
    void closePath() override;
    void fillPath( microtex::i32 id ) override;
    void drawText( const std::wstring& src, float x, float y );
    void drawLine( float x1, float y1, float x2, float y2 ) override;
    void drawRect( float x, float y, float w, float h ) override;
    void fillRect( float x, float y, float w, float h ) override;
    void drawRoundRect( float x, float y, float w, float h, float rx, float ry ) override;
    void fillRoundRect( float x, float y, float w, float h, float rx, float ry ) override;
  };

} // namespace Tac
