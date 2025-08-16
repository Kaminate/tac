#include "tac_example_LaTeX_simple.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_microtex.h"

#include <microtex/microtex.h>
//#include <microtex/env/env.h>

namespace Tac
{
  static bool              sInitialized          {};
  static microtex::Render* render                {};
  static MicroTeXImGuiGraphics    sGraphics2D           {};
  static const char*       sExample_Simple       { R"_(\frac{\pi}{2})_" };
  static const char*       sExample_Integral     { R"_(\int_{\textcolor{red}{a}}^b \frac{x}{\ln(x)} \, \mathrm{d}x)_" };
  static const char*       sExample_Color        { R"_(\textcolor{red}{x}\textcolor{green}{y}z)_" };
  static const char*       sExample_Curr         {};
  static const char*       sExample_Next         { sExample_Integral };
  static float             sExampleSizeMultipler { 3 };
  static v2                sPos                  { 200, 250 };

  void ExampleLaTeXSimple::Update( Errors& errors )
  {
    if( !sInitialized )
    {
      const char* fontClmPath{ "assets/fonts/firamath/FiraMath-Regular.clm2" };
      const char* fontOtfPath{ "assets/fonts/firamath/FiraMath-Regular.otf" };
      const char* fontTtfPath{ "assets/fonts/firamath/FiraMath-Regular.ttf" };
      if( !FontApi::IsFontLoaded( fontTtfPath ) )
      {
        microtex::FontSrcFile fontSrc( fontClmPath, fontOtfPath );
        microtex::MicroTeX::init( fontSrc );
        TAC_CALL( FontApi::LoadFont( Language::English, fontTtfPath, errors ) );
        microtex::PlatformFactory::registerFactory( "", std::make_unique< MicroTexImGuiPlatformFactory >() );
      }
      sInitialized = true;
    }

    const char* examples[]
    {
      sExample_Simple,
      sExample_Integral,
      sExample_Color,
    };

    for( const char* ex : examples )
      if( ImGuiButton( ex ) )
        sExample_Next = ex;

    bool dirty{};
    dirty |= ImGuiDragFloat( "Size Multiplier", &sExampleSizeMultipler );
    dirty |= ImGuiDragFloat( "Magic Text Scale", &MicroTeXImGuiGraphics::sMagicTextScale );
    dirty |= ImGuiDragFloat( "Magic Line Width", &MicroTeXImGuiGraphics::sMagicLineWidth );
    dirty |= ( sExample_Curr != sExample_Next );
    sExample_Curr = sExample_Next;

    if( sExample_Curr && dirty )
    {
      const float width{}; // unlimited
      const float textSize{ ImGuiGetFontSize() * sExampleSizeMultipler };
      const float lineSpace{}; // ???
      const microtex::color _color{ microtex::getColor( "white" ) };
      render = microtex::MicroTeX::parse( sExample_Curr, width, textSize, lineSpace, _color );
    }

    ImGuiDragFloat2( "pos", sPos.data() );

    if( render )
    {
      render->draw( sGraphics2D, sPos.x, sPos.y );
      if( auto drawData = ImGuiGetDrawData() )
      {
        v2 mini{ sPos * 0.95f };
        v2 maxi{ sPos + 1.05f * v2( render->getWidth(), render->getHeight() ) }; //  *1.05f };
        v2 TL{ mini };
        v2 TR{ maxi.x, mini.y };
        v2 BL{ mini.x, maxi.y };
        v2 BR{ maxi };
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ TR } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ BL } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ TR } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ BL } } );
      }
    }
  }
} // namespace Tac



