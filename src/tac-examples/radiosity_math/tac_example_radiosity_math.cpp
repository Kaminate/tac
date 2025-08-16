#include "tac_example_radiosity_math.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac_microtex.h"
#include "tac-engine-core/graphics/ui/tac_font.h"

#include <microtex/microtex.h>
//#include <microtex/env/env.h>

namespace Tac
{
  static bool              sInitialized          {};
  static microtex::Render* render                {};
  MicroTeXImGuiGraphics    sGraphics2D           {};
  static const char*       sExample_Simple       { R"_(\frac{\pi}{2})_" };
  static const char*       sExample_Integral     { R"_(\int_{\textcolor{red}{a}}^b \frac{x}{\ln(x)} \, \mathrm{d}x)_" };
  static const char*       sExample_Color        { R"_(\textcolor{red}{x}\textcolor{green}{y}z)_" };
  static const char*       sExample_Curr         {};
  static const char*       sExample_Next         { sExample_Integral };
  static float             sExampleSizeMultipler { 3 };

  void ExampleRadiosityMath::Update( Errors& errors )
  {
    if( !sInitialized )
    {
      const char* fontClmPath{ "assets/fonts/firamath/FiraMath-Regular.clm2" };
      const char* fontOtfPath{ "assets/fonts/firamath/FiraMath-Regular.otf" };
      const char* fontTtfPath{ "assets/fonts/firamath/FiraMath-Regular.ttf" };
      //const char* fontClmPath{ "assets/fonts/cmun/cmunss.clm1" };
      //const char* fontOtfPath{ "assets/fonts/cmun/cmunss.otf" };
      //const char* fontTtfPath{ "assets/fonts/cmun/cmunss.ttf" };
      microtex::FontSrcFile fontSrc( fontClmPath, fontOtfPath );
      microtex::MicroTeX::init( fontSrc );

      //ImGui::GetIO().Fonts->AddFontFromFileTTF( fontTtfPath );
      TAC_CALL( FontApi::LoadFont( Language::English, fontTtfPath, errors ) );

      microtex::PlatformFactory::registerFactory( "", std::make_unique< MicroTexImGuiPlatformFactory >() );
      MicroTeXImGuiGraphics::UnitTest();
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
    dirty |= ImGuiDragFloat( "Magic", &MicroTeXImGuiGraphics::sMagic );
    dirty |= ImGuiDragFloat( "Magic2", &MicroTeXImGuiGraphics::sMagic2 );
    dirty |= ( sExample_Curr != sExample_Next );
    sExample_Curr = sExample_Next;

    if( sExample_Curr && dirty )
    {
      float width { }; // unlimited
      float textSize{ ImGuiGetFontSize() * sExampleSizeMultipler };
      float lineSpace { }; // ???
      microtex::color _color{ microtex::getColor( "white" ) };
      render = microtex::MicroTeX::parse( sExample_Curr, width, textSize, lineSpace, _color );
    }

    static v2 sPos{ 200, 250 };
    ImGuiDragFloat2( "pos", sPos.data() );

    if( render )
    {
      render->draw( sGraphics2D, sPos.x, sPos.y );

      if( auto drawData = ImGuiGetDrawData() )
      {
        v2 mini { sPos };
        v2 maxi { sPos + v2( render->getWidth(), render->getHeight() ) * 1.05f };
        v2 TL{ mini };
        v2 TR{ maxi.x, mini.y };
        v2 BL{ mini.x, maxi.y };
        v2 BR{ maxi };

        //const UI2DDrawData::Box box
        //{
        //  .mMini          { sPos },
        //  .mMaxi          { sPos + v2( render->getWidth(), render->getHeight() ) },
        //  .mColor         { .2f, .2f, .2f, 1.0f },
        //};
        //drawData->AddBox( box );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ TR } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ BL } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ TR } } );
        drawData->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ BL } } );
        
      }

      //render->draw( sGraphics2D, 
                    //sPos.x * sExampleSizeMultipler * 1000 / 72.0f,
                    //sPos.y * sExampleSizeMultipler * 1000 / 72.0f );
      render->getWidth();
      render->getHeight();
    }

  }
} // namespace Tac



