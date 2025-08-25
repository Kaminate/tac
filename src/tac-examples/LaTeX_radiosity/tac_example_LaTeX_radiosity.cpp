#include "tac_example_LaTeX_radiosity.h" // self-inc

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_microtex.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"

#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/meta/tac_meta_enum.h"
#include "tac-std-lib/string/tac_string_meta.h"
#include "tac-std-lib/containers/tac_vector_meta.h"

#include <microtex/microtex.h>

namespace Tac
{
  static bool                  sInitialized                 {};
  static MicroTeXImGuiGraphics sGraphics2D                  {};

  // Camera parameters (worldspace)
  static v2                    sWorldspaceCamPos            {};
  static float                 sWorldspaceCamHeight         { 10 };

  // Line equation (y=mx+b, worldspace)
  static bool                  sWorldspaceEqEnabled         { false };
  static v2                    sWorldspaceEqPos             { 0, 0 };
  static float                 sWorldspaceEqSize            { 2 };
  static const char*           sWorldspaceEqStr             { "y=mx+b" };

  static bool                  sDrawOrigin                  {};

  enum TextStyle
  {
    kNormalText = 0,
    kHeading1,
  };

  struct TextBlockDef
  {
    String      mLaTeX          {};
    v2          mPos_worldspace {};
    TextStyle   mStyle          {};
  };

  struct PresentationDef
  {
    using TextBlockDefs = Vector< TextBlockDef >;

    TextBlockDefs mTextBlockDefs                  {};
    float         mFontSize_worldspace_NormalText { 1 };
    float         mFontSize_worldspace_Heading1   { 2 };
    v2            mWorldspaceCamPos               {};
    float         mWorldspaceCamHeight            { 10 };
  };

  static PresentationDef sPresentationDef;
  static AssetPathString sPresentationDefAssetPath;


  TAC_META_REGISTER_ENUM_BEGIN( TextStyle );
  TAC_META_REGISTER_ENUM_VALUE( kNormalText );
  TAC_META_REGISTER_ENUM_VALUE( kHeading1 );
  TAC_META_REGISTER_ENUM_END( TextStyle );

  TAC_META_REGISTER_STRUCT_BEGIN( TextBlockDef );
  TAC_META_REGISTER_STRUCT_MEMBER( mLaTeX );
  TAC_META_REGISTER_STRUCT_MEMBER( mPos_worldspace );
  TAC_META_REGISTER_STRUCT_MEMBER( mStyle );
  TAC_META_REGISTER_STRUCT_END( TextBlockDef );

  TAC_META_REGISTER_STRUCT_BEGIN( PresentationDef );
  TAC_META_REGISTER_STRUCT_MEMBER( mTextBlockDefs );
  TAC_META_REGISTER_STRUCT_MEMBER( mFontSize_worldspace_NormalText );
  TAC_META_REGISTER_STRUCT_MEMBER( mFontSize_worldspace_Heading1 );
  TAC_META_REGISTER_STRUCT_MEMBER( mWorldspaceCamPos );
  TAC_META_REGISTER_STRUCT_MEMBER( mWorldspaceCamHeight );
  TAC_META_REGISTER_STRUCT_END( PresentationDef );

  static void LoadPresentationDef( Errors& errors )
  {
    if( !Exists( sPresentationDefAssetPath ) )
      return;
    TAC_CALL( const String presentationJsonStr{ LoadAssetPath( sPresentationDefAssetPath, errors ) } );
    TAC_CALL( const Json presentationJson{ Json::Parse( presentationJsonStr, errors ) } );
    GetMetaType( sPresentationDef ).JsonDeserialize( &presentationJson, &sPresentationDef );

    
  }

  static void SavePresentationDef( Errors& errors )
  {
    Json presentationJson;
    GetMetaType( sPresentationDef ).JsonSerialize( &presentationJson, &sPresentationDef );
    const String presentationJsonStr{ presentationJson.Stringify() };
    SaveToFile( sPresentationDefAssetPath, presentationJsonStr.data(), presentationJsonStr.size(), errors );

  }

  static void DebugImguiPresentationDef()
  {
    if( !ImGuiCollapsingHeader( "Presentation Def", ImGuiNodeFlags_DefaultOpen ) )
      return;

    TAC_IMGUI_INDENT_BLOCK;

    bool exists{ Exists( sPresentationDefAssetPath ) };
    ImGuiText( "Path: " + sPresentationDefAssetPath + "(Exists: " + ( exists ? "true" : "false" ) + ")" );

    static int iSelectedTextBlock{ -1 };
    const int n{sPresentationDef.mTextBlockDefs.size()};
    ImGuiText( "Num text blocks: " + ToString( n ) );
    if( ImGuiButton( "Add Text Block" ) )
    {
      sPresentationDef.mTextBlockDefs.resize( n + 1 );
      TextBlockDef& def{ sPresentationDef.mTextBlockDefs[ n ] };
      def.mLaTeX = "<empty>";
      def.mPos_worldspace = sWorldspaceCamPos;
      if( iSelectedTextBlock == -1 )
        iSelectedTextBlock = n;
    }

    if( ImGuiCollapsingHeader( "Text Block Defs", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      for( int i{}; i < n; ++i )
      {
        TextBlockDef& def{ sPresentationDef.mTextBlockDefs[ i ] };
        PushID( ToString( i ) );

        if( ImGuiSelectable( def.mLaTeX, iSelectedTextBlock == i ) )
          iSelectedTextBlock = i;


        PopID();
      }
    }

    if( ImGuiCollapsingHeader( "Text Block Def", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      if( iSelectedTextBlock >=0 && iSelectedTextBlock < n )
      {
        TextBlockDef& def{ sPresentationDef.mTextBlockDefs[ iSelectedTextBlock ] };
        ImGuiInputText( "LaTeX", def.mLaTeX );
        ImGuiDragFloat2( "Position", def.mPos_worldspace.data() );
        ImGuiText( "Style: " + GetMetaType( def.mStyle ).ToString( &def.mStyle ) );
        ImGuiSameLine();
        if( def.mStyle != kHeading1 && ImGuiButton( "Set H1" ) ) { def.mStyle = kHeading1; }
        if( def.mStyle != kNormalText && ImGuiButton( "Set NormalText" ) ) { def.mStyle = kNormalText; }
      }
    }


    static String sStatusMessage;
    static Timestamp sStatusMessageEndTime;

    if( ImGuiCollapsingHeader( "Style" ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      ImGuiDragFloat( "Heading1", &sPresentationDef.mFontSize_worldspace_Heading1 );
      ImGuiDragFloat( "Normal Text", &sPresentationDef.mFontSize_worldspace_NormalText );
    }

    bool saveRequested{};
    if( ImGuiButton( "Remember Camera" ) )
    {
      sPresentationDef.mWorldspaceCamHeight = sWorldspaceCamHeight;
      sPresentationDef.mWorldspaceCamPos = sWorldspaceCamPos;
      saveRequested = true;
    }

    saveRequested |= ImGuiButton( "Save" );
    
    if( saveRequested )
    {
      saveRequested = false;
      Errors saveErrors;
      SavePresentationDef( saveErrors );
      const Timestamp curTime{ Timestep::GetElapsedTime() };
      const TimestampDifference duration{ saveErrors ? 60.0f : 5.0f };
      sStatusMessage = saveErrors ? saveErrors.ToString() : "Saved!";
      sStatusMessageEndTime = curTime + duration;
    }

    if( Timestep::GetElapsedTime() < sStatusMessageEndTime )
      ImGuiText( sStatusMessage );

  }


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
    sWorldspaceCamHeight *= ( 1.0f - AppKeyboardApi::GetMouseWheelDelta() / 10.0f );
    sWorldspaceCamHeight = Max( sWorldspaceCamHeight, 0.1f );
    if( AppKeyboardApi::IsPressed( Key::MouseMiddle ) )
    {
      if( const v2 px_delta{ AppKeyboardApi::GetMousePosDelta() }; px_delta != v2{} )
      {
        const float px_per_world_unit{ ImGuiGetContentRect().GetHeight() / sWorldspaceCamHeight };
        sWorldspaceCamPos.x -= px_delta.x / px_per_world_unit;
        sWorldspaceCamPos.y += px_delta.y / px_per_world_unit;
      }
    }

  }

  static void DrawEquation_worldspace( const char* eqStr, v2 pos_worldspace, float fontSize_worldspace )
  {
    const ImGuiRect contentRect{ ImGuiGetContentRect() };
    float px_per_world_unit{ contentRect.GetHeight() / sWorldspaceCamHeight };
    v2 pos_windowspace{ WorldToWindow( pos_worldspace ) };
    float fontSize_windowspace{ px_per_world_unit * fontSize_worldspace };
    const float width{}; // unlimited
    const float lineSpace{}; // ???
    const microtex::color _color{ microtex::getColor( "white" ) };

    const char* fallback{ "<nothing was drawn>" };
    try
    {
      if( microtex::Render* curRender{
        microtex::MicroTeX::parse( eqStr, width, fontSize_windowspace, lineSpace, _color ) } )
      {
        curRender->draw( sGraphics2D, pos_windowspace.x, pos_windowspace.y );
        int drawnW = curRender->getWidth();
        int drawnH = curRender->getHeight();
        if( drawnW != 0 && drawnH != 0 )
          fallback = nullptr;

        delete curRender;
      }
    }
    catch( std::exception e )
    {
      if( e.what() )
        fallback = e.what();
    }

    if( fallback )
    {
      if( auto drawList = ImGuiGetDrawData() )
      {
        //ImGuiPushFontSize( fontSize_windowspace );

        v4 ui2Dcolor{ v4( microtex::color_r( _color ),
                          microtex::color_g( _color ),
                          microtex::color_b( _color ),
                          microtex::color_a( _color ) ) / 255.f };

        UI2DDrawData::Text text
        {
          .mPos      { pos_windowspace },
          .mFontSize { fontSize_windowspace },
          .mUtf8     { fallback },
          .mColor    { ui2Dcolor },
        };

        drawList->AddText( text );
        //ImGuiPopFontSize();
      }
    }
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

      sPresentationDefAssetPath = GetFileAssetPath( "PresentationDef.tac.json" );

      TAC_CALL( LoadPresentationDef( errors ) );
      sWorldspaceCamPos = sPresentationDef.mWorldspaceCamPos;
      sWorldspaceCamHeight = sPresentationDef.mWorldspaceCamHeight;

      sInitialized = true;
    }


    for( TextBlockDef& def : sPresentationDef.mTextBlockDefs )
    {
      float fontSize_worldspace{ 1 };
      switch( def.mStyle )
      {
        case kHeading1: fontSize_worldspace = sPresentationDef.mFontSize_worldspace_Heading1; break;
        case kNormalText: fontSize_worldspace = sPresentationDef.mFontSize_worldspace_NormalText; break;
        default: TAC_ASSERT_INVALID_CASE( def.mStyle ); break;
      }

      DrawEquation_worldspace( def.mLaTeX.data(), def.mPos_worldspace, fontSize_worldspace );
    }


    CameraControls();

    if( sDrawOrigin )
    {
      if( auto drawData = ImGuiGetDrawData() )
      {
        DrawLineWorldspace( v2( 0, 0 ), v2( 1, 0 ), v4( 1, 0, 0, 1 ) );
        DrawLineWorldspace( v2( 0, 0 ), v2( 0, 1 ), v4( 0, 1, 0, 1 ) );
      }
    }

    if( sWorldspaceEqEnabled )
      DrawEquation_worldspace( sWorldspaceEqStr, sWorldspaceEqPos, sWorldspaceEqSize );

    if( ImGuiBegin( "Radiosity Demo Controls" ) )
    {
      ImGuiText( "Camera Controls:\n"
                 "- Middle mouse to pan\n"
                 "- Scroll wheel to zoom" );
      ImGuiCheckbox( "Draw Origin", &sDrawOrigin );

      if( ImGuiCollapsingHeader( "Equation (worldspace)" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        ImGuiCheckbox( "Equation enabled", &sWorldspaceEqEnabled );
        ImGuiDragFloat2( "equation pos (worldspace)", sWorldspaceEqPos.data() );
        ImGuiDragFloat( "equation size (worldspace)", &sWorldspaceEqSize );
      }


      if( ImGuiCollapsingHeader( "Camera" ) )
      {
        TAC_IMGUI_INDENT_BLOCK;
        ImGuiDragFloat2( "cam pos (worldspace)", sWorldspaceCamPos.data() );
        ImGuiDragFloat( "cam height (worldspace)", &sWorldspaceCamHeight );

        if( ImGuiButton( "Reset Camera" ) )
        {
          sWorldspaceCamPos = {};
          sWorldspaceCamHeight = 10;
        }
      }

      DebugImguiPresentationDef();
      ImGuiEnd();
    }
  }
} // namespace Tac



