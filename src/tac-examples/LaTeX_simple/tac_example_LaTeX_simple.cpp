#include "tac_example_LaTeX_simple.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_microtex.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"

#include <microtex/microtex.h>

namespace Tac
{
  static bool                  sInitialized                 {};
  static MicroTeXImGuiGraphics sGraphics2D                  {};

  // Integral equation (windowspace)
  static microtex::Render*     sWindowspaceEqRender         {};
  static bool                  sWindowspaceEqEnabled        { true };
  static const char*           sWindowspaceEqStr            { R"_(\int_{\textcolor{red}{a}}^b \frac{x}{\ln(x)} \, \mathrm{d}x)_" };
  static float                 sWindowspaceEqSizeMultipler  { 3 };
  static v2                    sWindowspaceEqPos            { 300, 250 };

  // Camera parameters (worldspace)
  static v2                    sWorldspaceCamPos            {};
  static float                 sWorldspaceCamHeight         { 10 };

  // Line equation (worldspace)
  static bool                  sWorldspaceEqEnabled         { true };
  static v2                    sWorldspaceEqPos             { 0, 0 };
  static float                 sWorldspaceEqSize            { 2 };
  static const char*           sWorldspaceEqStr             { "y=mx+b" };

  static v2 WorldToCamera( v2 p_world )
  {
    m3 worldToCameraTranslation{ m3::Translate( -sWorldspaceCamPos ) };
    return ( worldToCameraTranslation * v3( p_world, 1.0f ) ).xy();
  }

  static v2 CameraToWindow( v2 p_view )
  {
    const ImGuiRect contentRect{ ImGuiGetContentRect() };
    float px_per_world_unit = contentRect.GetHeight() / sWorldspaceCamHeight;
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
    if( auto drawData{ ImGuiGetDrawData() } )
    {
      const UI2DDrawData::Line xline
      {
        .mP0    { WorldToWindow( p0_world ) },
        .mP1    { WorldToWindow( p1_world ) },
        .mColor { color },
      };
      drawData->AddLine( xline );
    }
  }

  static void CameraControls()
  {
    ImGuiText( "Camera Controls:\n"
               "- Middle mouse to pan\n"
               "- Scroll wheel to zoom" );
    sWorldspaceCamHeight *= ( 1.0f - AppKeyboardApi::GetMouseWheelDelta() / 10.0f );
    sWorldspaceCamHeight = Max( sWorldspaceCamHeight, 0.1f );
    if( AppKeyboardApi::IsPressed( Key::MouseMiddle ) )
    {
      if( v2 px_delta{ AppKeyboardApi::GetMousePosDelta() }; px_delta != v2{} )
      {
        float px_per_world_unit{ ImGuiGetContentRect().GetHeight() / sWorldspaceCamHeight };
        sWorldspaceCamPos.x -= px_delta.x / px_per_world_unit;
        sWorldspaceCamPos.y += px_delta.y / px_per_world_unit;
      }
    }
  }

  static void UpdateEquation_worldspace( const char* eqStr, v2 pos_worldspace, float fontSize_worldspace )
  {
    const ImGuiRect contentRect{ ImGuiGetContentRect() };
    float px_per_world_unit{ contentRect.GetHeight() / sWorldspaceCamHeight };
    v2 pos_windowspace{ WorldToWindow( pos_worldspace ) };
    float fontSize_windowspace{ px_per_world_unit * fontSize_worldspace };
    const float width{}; // unlimited
    const float textSize{ fontSize_windowspace };
    const float lineSpace{}; // ???
    const microtex::color _color{ microtex::getColor( "white" ) };
    if( auto curRender{ microtex::MicroTeX::parse( eqStr, width, textSize, lineSpace, _color ) } )
    {
      curRender->draw( sGraphics2D, pos_windowspace.x, pos_windowspace.y );
      delete curRender;
    }
  }

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


    bool dirty{};
    if( ImGuiCollapsingHeader( "Equation 1 (screenspace)" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiCheckbox( "Equation 1 enabled", &sWindowspaceEqEnabled );
      ImGuiDragFloat2( "Equation 1 pos (screenspace)", sWindowspaceEqPos.data() );
      dirty |= ImGuiDragFloat( "Size Multiplier", &sWindowspaceEqSizeMultipler );
    }

    if( ImGuiCollapsingHeader( "Equation 2 (worldspace)" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiCheckbox( "Equation 2 enabled", &sWorldspaceEqEnabled );
      ImGuiDragFloat2( "equation 2 pos (worldspace)", sWorldspaceEqPos.data() );
      ImGuiDragFloat( "equation 2 size (worldspace)", &sWorldspaceEqSize );
    }


    if( ImGuiCollapsingHeader( "Camera" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiDragFloat2( "cam pos (worldspace)", sWorldspaceCamPos.data() );
      ImGuiDragFloat( "cam height (worldspace)", &sWorldspaceCamHeight );
    }

    if( dirty || !sWindowspaceEqRender )
    {
      const float width{}; // unlimited
      const float textSize{ ImGuiGetFontSize() * sWindowspaceEqSizeMultipler };
      const float lineSpace{}; // ???
      const microtex::color _color{ microtex::getColor( "white" ) };
      sWindowspaceEqRender = microtex::MicroTeX::parse( sWindowspaceEqStr, width, textSize, lineSpace, _color );
    }


    CameraControls();

    if( sWindowspaceEqEnabled && sWindowspaceEqRender )
    {
      sWindowspaceEqRender->draw( sGraphics2D, sWindowspaceEqPos.x, sWindowspaceEqPos.y );
      if( auto drawData = ImGuiGetDrawData() )
      {
        v2 mini{ sWindowspaceEqPos * 0.95f };
        v2 maxi{ sWindowspaceEqPos + 1.05f * v2( sWindowspaceEqRender->getWidth(), sWindowspaceEqRender->getHeight() ) }; //  *1.05f };
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

    if( sWorldspaceEqEnabled )
      UpdateEquation_worldspace( sWorldspaceEqStr, sWorldspaceEqPos, sWorldspaceEqSize );


  }
} // namespace Tac



