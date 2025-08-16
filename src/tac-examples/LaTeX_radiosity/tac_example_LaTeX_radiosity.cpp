#include "tac_example_LaTeX_radiosity.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_microtex.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"

#include <microtex/microtex.h>
//#include <microtex/env/env.h>

namespace Tac
{
  static bool              sInitialized          {};
  static microtex::Render* render                {};
  static MicroTeXImGuiGraphics    sGraphics2D           {};
  static const char*       sExample_Integral     { R"_(\int_{\textcolor{red}{a}}^b \frac{x}{\ln(x)} \, \mathrm{d}x)_" };
  static const char*       sExample_Curr         {};
  static const char*       sExample_Next         { sExample_Integral };
  static float             sExampleSizeMultipler { 3 };
  static v2                sPos                  { 300, 250 };

  static v2    sCamPos_worldspace    {};
  static float sCamHeight_worldspace { 10 }; // worldunits

  static v2           sTestEqPos_worldspace    { 0, 0 };
  static float        sTestEqSize_worldspace   { 2 };
  static const char*  sTestEqStr               { "y=mx+b" };

  static v2 WorldToCamera( v2 p_world )
  {
    m3 worldToCameraTranslation{ m3::Translate( -sCamPos_worldspace ) };
    return ( worldToCameraTranslation * v3( p_world, 1.0f ) ).xy();
  }

  static v2 CameraToWindow( v2 p_view )
  {
    const ImGuiRect contentRect{ ImGuiGetContentRect() };
    float px_per_world_unit = contentRect.GetHeight() / sCamHeight_worldspace;
    v2 p_window = p_view * px_per_world_unit;
    p_window.y *= -1;
    p_window.x += contentRect.GetWidth() / 2;
    p_window.y += contentRect.GetHeight() / 2;
    return p_window;
  }

  static v2 WorldToWindow( v2 p_world )
  {
    v2 p_view = WorldToCamera( p_world );
    v2 p_window = CameraToWindow( p_view );
    return p_window;
  }

  static void DrawLineWorldspace( v2 p0_world, v2 p1_world, v4 color )
  {
    v2 p0_window = WorldToWindow( p0_world );
    v2 p1_window = WorldToWindow( p1_world );


    if( auto drawData = ImGuiGetDrawData() )
    {
      const UI2DDrawData::Line xline
      {
        .mP0{ p0_window },
        .mP1{ p1_window },
        .mColor{ color },
      };
      drawData->AddLine( xline );
    }
  }

  static void CameraControls()
  {
    sCamHeight_worldspace -= AppKeyboardApi::GetMouseWheelDelta();
    sCamHeight_worldspace = Max( sCamHeight_worldspace, 1.0f );
    float px_per_world_unit = ImGuiGetContentRect().GetHeight() / sCamHeight_worldspace;
    if( AppKeyboardApi::IsPressed( Key::MouseMiddle ) )
    {
      if( v2 px_delta{ AppKeyboardApi::GetMousePosDelta() }; px_delta != v2{} )
      {
        sCamPos_worldspace.x -= px_delta.x / px_per_world_unit;
        sCamPos_worldspace.y += px_delta.y / px_per_world_unit;
      }
    }
  }

  static void DrawEquation_worldspace( v2 pos_worldspace, float fontSize_worldspace )
  {
    __debugbreak();
#if 0
    v2 pos_windowspace = WorldToWindow( pos_worldspace );
    float fontSize_windowspace = fontSize_worldspace * ??? ;
    const float width{}; // unlimited
    const float textSize{ fontSize_windowspace };
    const float lineSpace{}; // ???
    const microtex::color _color{ microtex::getColor( "white" ) };
    auto curRender = microtex::MicroTeX::parse( sExample_Curr, width, textSize, lineSpace, _color );
    curRender->draw( sGraphics2D, pos_windowspace.x, pos_windowspace.y );
    delete curRender;
#endif
  }

  void ExampleLaTeXRadiosity::Update( Errors& errors )
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

    sExample_Next = sExample_Integral;

    bool dirty{};
    dirty |= ImGuiDragFloat( "Size Multiplier", &sExampleSizeMultipler );
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

    ImGuiDragFloat2( "equation pos windowspace", sPos.data() );
    ImGuiDragFloat2( "cam pos (worldspace)", sCamPos_worldspace.data() );
    ImGuiDragFloat( "cam height (worldspace)", &sCamHeight_worldspace );

    CameraControls();

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


    if( auto drawData = ImGuiGetDrawData() )
    {
      DrawLineWorldspace( v2( 0, 0 ), v2( 1, 0 ), v4( 1, 0, 0, 1 ) );
      DrawLineWorldspace( v2( 0, 0 ), v2( 0, 1 ), v4( 0, 1, 0, 1 ) );
    }

    DrawEquation_worldspace( sTestEqPos_worldspace, sTestEqSize_worldspace );


  }
} // namespace Tac



