#include "tac_example_LaTeX_radiosity.h" // self-inc


#include "tac-engine-core/assetmanagers/tac_texture_asset_manager.h"
#include "tac-engine-core/graphics/ui/imgui/tac_imgui.h"
#include "tac-engine-core/graphics/ui/tac_font.h"
#include "tac-engine-core/graphics/ui/tac_microtex.h"
#include "tac-engine-core/hid/tac_app_keyboard_api.h"
#include "tac-engine-core/shell/tac_shell.h"
#include "tac-std-lib/containers/tac_vector_meta.h"
#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/math/tac_math_meta.h"
#include "tac-std-lib/math/tac_matrix3.h"
#include "tac-std-lib/meta/tac_meta_composite.h"
#include "tac-std-lib/meta/tac_meta_enum.h"
//#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_meta.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"

#include <microtex/microtex.h>

namespace Tac
{
  static bool                  sInitialized                 {};
  static MicroTeXImGuiGraphics sGraphics2D                  {};

  // Camera parameters (worldspace)
  static v2                    sWorldspaceCamPos            {};
  static float                 sWorldspaceCamHeight         { 10 };

  static bool                  sDrawOrigin                  {};

  enum TextStyle
  {
    kNormalText = 0,
    kHeading1,
    kCount,
  };

  using TextBlockID = i32;

  struct TextBlockDef
  {
    using Children = Vector< TextBlockID >;
    TextBlockID mID              { -1 };
    String      mLaTeX           {};
    v2          mPos_worldspace  {};
    TextStyle   mStyle           {};
    float       mScaleMultiplier { 1 };
    Children    mChildren        {};

    struct
    {
      bool mHovered{};
      bool mActivated{};
      float mAlphaCur{};
      float mAlphaTgt{ 0.2f };
      float mAlphaVel{};
      float mHoverAlphaCur{};
      float mHoverAlphaTgt{};
      float mHoverAlphaVel{};
      v2 mPos_windowspace;
      v2 mSize_windowspace;
    } mRuntimeData;
  };

  struct PresentationDef
  {
    using TextBlockDefs = Vector< TextBlockDef >;

    TextBlockDefs mTextBlockDefs                  {};
    float         mFontSize_worldspace_NormalText { 1 };
    float         mFontSize_worldspace_Heading1   { 2 };
    v2            mWorldspaceCamPos               {};
    float         mWorldspaceCamHeight            { 10 };
    TextBlockID   mIDCounter                      {};
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
  TAC_META_REGISTER_STRUCT_MEMBER( mScaleMultiplier );
  TAC_META_REGISTER_STRUCT_MEMBER( mStyle );
  TAC_META_REGISTER_STRUCT_MEMBER( mID );
  TAC_META_REGISTER_STRUCT_MEMBER( mChildren );
  TAC_META_REGISTER_STRUCT_END( TextBlockDef );

  TAC_META_REGISTER_STRUCT_BEGIN( PresentationDef );
  TAC_META_REGISTER_STRUCT_MEMBER( mTextBlockDefs );
  TAC_META_REGISTER_STRUCT_MEMBER( mFontSize_worldspace_NormalText );
  TAC_META_REGISTER_STRUCT_MEMBER( mFontSize_worldspace_Heading1 );
  TAC_META_REGISTER_STRUCT_MEMBER( mWorldspaceCamPos );
  TAC_META_REGISTER_STRUCT_MEMBER( mWorldspaceCamHeight );
  TAC_META_REGISTER_STRUCT_MEMBER( mIDCounter );
  TAC_META_REGISTER_STRUCT_END( PresentationDef );

  static void LoadPresentationDef( Errors& errors )
  {
    if( !Exists( sPresentationDefAssetPath ) )
      return;
    TAC_CALL( const String presentationJsonStr{ LoadAssetPath( sPresentationDefAssetPath, errors ) } );
    TAC_CALL( const Json presentationJson{ Json::Parse( presentationJsonStr, errors ) } );
    GetMetaType( sPresentationDef ).JsonDeserialize( &presentationJson, &sPresentationDef );

    for( TextBlockDef& textBlockDef : sPresentationDef.mTextBlockDefs )
      if( textBlockDef.mID == -1 )
        textBlockDef.mID = sPresentationDef.mIDCounter++;
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

    static String sStatusMessage;
    static Timestamp sStatusMessageEndTime;

    if( ImGuiCollapsingHeader( "Text Block Def", ImGuiNodeFlags_DefaultOpen ) )
    {
      TAC_IMGUI_INDENT_BLOCK;
      if( iSelectedTextBlock >=0 && iSelectedTextBlock < n )
      {
        TextBlockDef& def{ sPresentationDef.mTextBlockDefs[ iSelectedTextBlock ] };
        const MetaType& metaTextStyle{GetMetaType( def.mStyle )};
        ImGuiInputText( "LaTeX", def.mLaTeX );
        ImGuiDragFloat2( "Position", def.mPos_worldspace.data() );
        ImGuiText( "Style: " + metaTextStyle.ToString( &def.mStyle ) );
        for( TextStyle style{}; style < kCount; style = ( TextStyle )( style + 1 ) )
        {
          if( def.mStyle != style )
          {
            ImGuiSameLine();
            if( ImGuiButton(  "Set " + metaTextStyle.ToString( &style )  ) )
            {
              def.mStyle = style;
            }
          }
        }

        Errors dlgErr;
        if( ImGuiButton( "Set Image" ) )
        {
          AssetPathStringView asset{ AssetOpenDialog( dlgErr ) };
          if( dlgErr )
          {
            sStatusMessage = dlgErr.ToString();
            sStatusMessageEndTime = Timestep::GetElapsedTime() + TimestampDifference( 60.0f );
          }
          else if( Exists( asset ) )
          {
            def.mLaTeX = asset;
          }
        }
      }
    }



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

  static void DrawBorder(StringView eqStr, v2 pos_windowspace, v2 size_windowspace, float alpha )
  {
    UI2DDrawData* drawList{ ImGuiGetDrawData() };
    if( !drawList )
      return;
    v2 rectMini_windowspace{ pos_windowspace - v2( size_windowspace.x * 0.05f,
                                                   size_windowspace.y * 0.25f ) };
    v2 rectMaxi_windowspace{ pos_windowspace + v2( size_windowspace.x * 1.05f,
                                                   size_windowspace.y * 1.25f ) };
    ImGuiRect rect_windowspace{ ImGuiRect::FromMinMax( rectMini_windowspace, rectMaxi_windowspace ) };
    const bool hovered{ ImGuiIsRectHovered( rect_windowspace ) };
    if( !hovered )
      return;
    
    v2 TL{ rectMini_windowspace };
    v2 TR{ rectMaxi_windowspace.x, rectMini_windowspace.y };
    v2 BL{ rectMini_windowspace.x, rectMaxi_windowspace.y };
    v2 BR{ rectMaxi_windowspace };
    const v4 color{ 1, 1, 1, alpha };
    drawList->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ TR }, .mColor{color} } );
    drawList->AddLine( UI2DDrawData::Line{ .mP0{ TL }, .mP1{ BL }, .mColor{color} } );
    drawList->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ TR }, .mColor{color} } );
    drawList->AddLine( UI2DDrawData::Line{ .mP0{ BR }, .mP1{ BL }, .mColor{color} } );
  }

  static void UpdateEquation_worldspace( TextBlockDef& def)
  {
    const StringView eqStr{ def.mLaTeX };
    const v2 pos_worldspace{ def.mPos_worldspace };
    dynmc float fontSize_worldspace{ def.mScaleMultiplier };
    switch( def.mStyle )
    {
      case kHeading1: fontSize_worldspace *= sPresentationDef.mFontSize_worldspace_Heading1; break;
      case kNormalText: fontSize_worldspace *= sPresentationDef.mFontSize_worldspace_NormalText; break;
      default: TAC_ASSERT_INVALID_CASE( def.mStyle ); break;
    }

    const ImGuiRect contentRect{ ImGuiGetContentRect() };
    const float px_per_world_unit{ contentRect.GetHeight() / sWorldspaceCamHeight };
    const v2 pos_windowspace{ WorldToWindow( pos_worldspace ) };
    const float fontSize_windowspace{ px_per_world_unit * fontSize_worldspace };
    const float width{}; // unlimited
    const float lineSpace{}; // ???
    //const microtex::color _color{ microtex::getColor( "white" ) };

    const microtex::color _color{ microtex::argb( def.mRuntimeData.mAlphaCur, 1.f, 1.f, 1.f ) };

    const char* fallback{ "<nothing was drawn>" };

    try
    {
      if( microtex::Render* curRender{
        microtex::MicroTeX::parse( eqStr.c_str(), width, fontSize_windowspace, lineSpace, _color ) } )
      {
        curRender->draw( sGraphics2D, pos_windowspace.x, pos_windowspace.y );
        def.mRuntimeData.mPos_windowspace = pos_windowspace;
        def.mRuntimeData.mSize_windowspace = v2( curRender->getWidth(), curRender->getHeight() );
        DrawBorder( eqStr,
                    def.mRuntimeData.mPos_windowspace,
                    def.mRuntimeData.mSize_windowspace,
                    def.mRuntimeData.mHoverAlphaCur );
        if( curRender->getWidth() && curRender->getHeight() )
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
      if( UI2DDrawData * drawList{ ImGuiGetDrawData() } )
      {
        const microtex::color r{ microtex::color_r( _color ) };
        const microtex::color g{ microtex::color_g( _color ) };
        const microtex::color b{ microtex::color_b( _color ) };
        const microtex::color a{ microtex::color_a( _color ) };
        const v4 ui2Dcolor{ v4( r, g, b, a ) / 255.f };
        const UI2DDrawData::Text text
        {
          .mPos      { pos_windowspace },
          .mFontSize { fontSize_windowspace },
          .mUtf8     { fallback },
          .mColor    { ui2Dcolor },
        };
        drawList->AddText( text );
      }
    }
  }

  static void UpdateTexture_worldspace( TextBlockDef& def)
  {
    Errors errors;
    const AssetPathStringView assetPath{def.mLaTeX};
    Render::TextureHandle textureHandle{ TextureAssetManager::GetTexture( assetPath, errors ) };
    TAC_ASSERT( !errors );
    if( !textureHandle.IsValid() )
      return;

    v3i textureSize{ TextureAssetManager::GetTextureSize( assetPath, errors ) };
    TAC_ASSERT( !errors );
    if( textureSize == v3i{} )
      return;

    const float px_per_world_unit{ ImGuiGetContentRect().GetHeight() / sWorldspaceCamHeight };
    const float world_unit_per_image_texel{ 0.04f };
    const float texels_to_pixels{ world_unit_per_image_texel * def.mScaleMultiplier * px_per_world_unit };
    v2 pos_mini_windowspace{ WorldToWindow( def.mPos_worldspace ) };
    v2 pos_maxi_windowspace
    {
      pos_mini_windowspace.x + textureSize.x * texels_to_pixels,
      pos_mini_windowspace.y + textureSize.y * texels_to_pixels
    };

    if( UI2DDrawData * drawList{ ImGuiGetDrawData() } )
    {
      const v4 color{def.mRuntimeData.mAlphaCur};
      const UI2DDrawData::Box box
      {
        .mMini          { pos_mini_windowspace },
        .mMaxi          { pos_maxi_windowspace },
        .mColor         { color },
        .mTextureHandle { textureHandle },
      };
      drawList->AddBox( box );
    }
    def.mRuntimeData.mPos_windowspace = pos_mini_windowspace;
    def.mRuntimeData.mSize_windowspace = pos_maxi_windowspace - pos_mini_windowspace;
  }

  void ExampleLaTeXRadiosity::LazyInit( Errors& errors )
  {
    if( sInitialized )
      return;

    sInitialized = true;
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
  }

  void ExampleLaTeXRadiosity::Update( Errors& errors )
  {
    TAC_CALL( LazyInit( errors ) );

    for( TextBlockDef& def : sPresentationDef.mTextBlockDefs )
    {
      def.mRuntimeData.mPos_windowspace;
      def.mRuntimeData.mSize_windowspace;
      
    }


    for( TextBlockDef& def : sPresentationDef.mTextBlockDefs )
    {

      if( def.mLaTeX.ends_with( ".png" ) )
      {
        TAC_ASSERT( def.mLaTeX.starts_with( GetFileAssetPath( "" ) ) );
        UpdateTexture_worldspace( def);
      }
      else
      {
        UpdateEquation_worldspace( def);
      }

      ImGuiRect rect_windowspace{ ImGuiRect::FromPosSize( def.mRuntimeData.mPos_windowspace,
                                                          def.mRuntimeData.mSize_windowspace ) };
      const bool hovered{ ImGuiIsRectHovered( rect_windowspace ) };
      def.mRuntimeData.mHovered = hovered;
      if( def.mRuntimeData.mHovered && AppKeyboardApi::JustPressed( Key::MouseLeft ) )
      {
        def.mRuntimeData.mActivated = !def.mRuntimeData.mActivated ;
        if( def.mRuntimeData.mAlphaTgt > 0.9f )
          def.mRuntimeData.mAlphaTgt = 0.1f;
        else
          def.mRuntimeData.mAlphaTgt = 1.0f;
      }

      def.mRuntimeData.mHoverAlphaTgt = def.mRuntimeData.mHovered ? def.mRuntimeData.mAlphaTgt : 0;
      Spring( &def.mRuntimeData.mHoverAlphaCur,
              &def.mRuntimeData.mHoverAlphaVel,
              def.mRuntimeData.mHoverAlphaTgt,
              50.0f,
              1 / 60.0f );

      Spring( &def.mRuntimeData.mAlphaCur,
              &def.mRuntimeData.mAlphaVel,
              def.mRuntimeData.mAlphaTgt,
              80.0f,
              1 / 60.0f );
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


    if( ImGuiBegin( "Radiosity Demo Controls" ) )
    {
      ImGuiText( "Camera Controls:\n"
                 "- Middle mouse to pan\n"
                 "- Scroll wheel to zoom" );
      ImGuiCheckbox( "Draw Origin", &sDrawOrigin );

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



