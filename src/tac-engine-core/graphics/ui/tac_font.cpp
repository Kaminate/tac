#include "tac_font.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"
//#include "tac-engine-core/settings/tac_settings.h"
#include "tac-engine-core/thirdparty/stb_truetype.h"

//#include "tac-rhi/render3/tac_render_api.h"
#include "tac-rhi/render3/tac_render_api.h"

#include "tac-std-lib/error/tac_error_handling.h"
#include "tac-std-lib/containers/tac_map.h"
#include "tac-std-lib/filesystem/tac_asset.h"
#include "tac-std-lib/filesystem/tac_filesystem.h"
#include "tac-std-lib/math/tac_math.h"
#include "tac-std-lib/memory/tac_memory.h"
#include "tac-std-lib/memory/tac_memory_util.h"
#include "tac-std-lib/os/tac_os.h"
#include "tac-std-lib/string/tac_string_util.h"

#define TAC_DEBUGGING_ATLAS() 0

#if TAC_DEBUGGING_ATLAS()
#include "tac-engine-core/thirdparty/stb_image_write.h"
#endif

#if TAC_FONT_ENABLED()

namespace Tac
{
    static_assert( FontCellInnerSDFPadding, "cant get anything to render w/o padding" );

    // onedge_value     
    // value 0-255 to test the SDF against to reconstruct the character (i.e. the isocontour of the character)
    const unsigned char onedge_value { 128 }; // [0,255]



    // pixel_dist_scale
    //   what value the SDF should increase by when moving one SDF "pixel" away from the edge (on the 0..255 scale)
    //   if positive, > onedge_value is inside
    //   if negative, < onedge_value is inside
    // 
    // Example:
    //   onedge_value = 128
    //   FontCellInnerSDFPadding = 5
    //   pixel_dist_scale = onedge_value / ( float )FontCellInnerSDFPadding
    //                    = (128) / ( float )5
    //                    = 25.6
    //   If we look at the rendered SDF texture, pixels inside the boundary edge of the glyph will
    //   have values > 128.
    //   At 1 pixel away from boundary edge will have value 128 - ( 25.6 * 1 ) = 102
    //   At 2 pixel away from boundary edge will have value 128 - ( 25.6 * 2 ) = 76
    //   At 3 pixel away from boundary edge will have value 128 - ( 25.6 * 3 ) = 51
    //   At 4 pixel away from boundary edge will have value 128 - ( 25.6 * 4 ) = 25
    //   At 5 pixel away from boundary edge will have value 128 - ( 25.6 * 5 ) = 0
    const float pixel_dist_scale { onedge_value / ( float )FontCellInnerSDFPadding };

  struct FontFile
  {
    typedef Map< Codepoint, FontAtlasCell* > CellMap;

    FontFile( const AssetPathStringView&, Errors& );
    //Filesystem::Path mFilepath;
    AssetPathString  mAssetPath;
    CellMap          mCells;
    String           mFontMemory;
    stbtt_fontinfo   mFontInfo  {};
    FontDims         mFontDims;
  };

  // coords: (rows down, columns right)
  struct TexelRegion
  {
    int mBeginRow;
    int mBeginColumn;

    int mWidth;
    int mHeight;
  };

  struct FontAtlas
  {
    FontAtlas() = default;
    void                            Load( Errors& );
    void                            Uninit();
    void                            DebugImgui();
    FontAtlasCell*                  GetCharacter( Language , Codepoint );
    FontAtlasCell*                  GetCell();
    void                            InitAtlasCheckerboard( void*, u8, u8 );
    void                            FillAtlasRegion( void*, const TexelRegion&, u8 );// inclusive
    Render::TextureHandle           mTextureId;
    int                             mPxStride {};
    int                             mPxWidth {};
    int                             mPxHeight {};
    int                             mCellRowCount {};
    int                             mCellColCount {};
    Vector< FontAtlasCell* >        mCells;
    Vector< FontFile* >             mFontFiles;
    Map< Language, FontFile* >      mDefaultFonts;
    bool                            FORCE_DRAW_EVERY_TIME {};
  };

  FontAtlas gFontStuff;

  const Render::Format atlasFormat
  {
    .mElementCount        { 1 },
    .mPerElementByteCount { sizeof( u8 ) },
    .mPerElementDataType  { Render::GraphicsType::unorm},
  };

  const Render::Format& sdfFormat { atlasFormat };

  // -----------------------------------------------------------------------------------------------

  void           FontApi::Init( Errors& errors )
  {
    gFontStuff.Load( errors );
  }

  void           FontApi::Uninit()
  {
    gFontStuff.Uninit();
  }

  //FontFile*      FontApi::GetDefaultFontForLanguage( Language language)
  //{
  //  return gFontStuff.mDefaultFonts[ language ];
  //}

  const FontDims* FontApi::GetLanguageFontDims( Language language )
  {
    FontFile* fontFile { gFontStuff.mDefaultFonts.FindVal( language ).GetValueOr(nullptr) };
    return fontFile ? &fontFile->mFontDims : nullptr;
  }

  const FontAtlasCell* FontApi::GetFontAtlasCell( Language language, Codepoint codepoint )
  {
      return gFontStuff.GetCharacter( language, codepoint );
  }

  Render::TextureHandle FontApi::GetAtlasTextureHandle()
  {
    return gFontStuff.mTextureId;
  }

  void FontApi::SetForceRedraw(bool force)
  {
    gFontStuff.FORCE_DRAW_EVERY_TIME = force;
  }

  float                 FontApi::GetSDFOnEdgeValue() // [0,1]
  {
    return onedge_value / 255.0f;
  }

  float                 FontApi::GetSDFPixelDistScale() // [0,1]
  {
    return pixel_dist_scale / 255.0f;
  }

  // -----------------------------------------------------------------------------------------------

  FontFile::FontFile( const AssetPathStringView& filepath, Errors& errors )
  {
    //mFilepath = filepath;
    mAssetPath = filepath;
    mFontMemory = TAC_CALL( LoadAssetPath( filepath, errors ));

    stbtt_InitFont( &mFontInfo, ( const unsigned char* )mFontMemory.data(), 0 );
    const float scale { stbtt_ScaleForPixelHeight( &mFontInfo, ( float )TextPxHeight ) };

    // [ ] Q: Why are these not already floats?
    int ascent;
    int descent;
    int linegap;
    stbtt_GetFontVMetrics( &mFontInfo, &ascent, &descent, &linegap );

    mFontDims = FontDims
    {
      .mUnscaledAscent  { ( float )ascent },
      .mUnscaledDescent { ( float )descent },
      .mUnscaledLinegap { ( float )linegap },
      .mScale           { scale },
    };


    //mUISpaceAscent = ( float )ascent * mScale;
    //mUISpaceDescent = ( float )descent * mScale;
    //mUISpaceLinegap = ( float )linegap * mScale;
  }

  // -----------------------------------------------------------------------------------------------

  //FontAtlas::FontAtlas()
  //{
  //}

  void FontAtlas::Uninit()
  {
    for( FontAtlasCell* fontAtlasCell : mCells )
    {
      TAC_DELETE fontAtlasCell;
    }

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    renderDevice->DestroyTexture( mTextureId );
  }

  //void FontStuff::InitAtlasSolidColor(void* initialAtlasMemory, u8 val)
  //{
  //  const int bytesPerCell = FontCellWidth * FontCellHeight;
  //  const int atlasVramByteCount = mRowCount * mColCount * bytesPerCell;
  //  for( int i{}; i < atlasVramByteCount; ++i )
  //    ( ( u8* )initialAtlasMemory )[ i ] = val;
  //}

  void FontAtlas::FillAtlasRegion( void* initialAtlasMemory, const TexelRegion& region, u8 val )
  {
    auto mem { ( u8* )initialAtlasMemory };
    u8* memRegionTL { mem + (std::ptrdiff_t)(region.mBeginColumn + region.mBeginRow * mPxStride) };
    for( int i{}; i < region.mHeight; ++i )
      MemSet( memRegionTL + (std::ptrdiff_t)(i * mPxStride), val, region.mWidth );
  }

  void FontAtlas::InitAtlasCheckerboard(void* initialAtlasMemory, u8 val0, u8 val1 )
  {
    static_assert( BilinearFilteringPadding > 0 );
    const u8 bilinearFilterPaddingValue { 0 };

    auto mem { ( u8* )initialAtlasMemory };
    MemSet( mem, bilinearFilterPaddingValue, mPxWidth * mPxHeight );

    //auto FillCell = [ & ]( int rBegin, int cBegin, u8 cell_color )
    //  {
    //    int stride = mCellColCount * FontCellWidth;
    //    for( int r = 0; r < FontCellHeight; ++r )
    //    {
    //      for( int c = 0; c < FontCellWidth; ++c )
    //      {

    //        u8 val = cell_color;

    //        bool isBorder = r == 0 || r == FontCellHeight - 1 || c == 0 || c == FontCellWidth - 1;
    //        if( isBorder )
    //          val = bilinearFilterPaddingColor;

    //        int rCurr = rBegin + r;
    //        int cCurr = cBegin + c;
    //        int i = rCurr * stride + cCurr;
    //        mem[ i ] = val;
    //      }
    //    }
    //  };

    u8 vals[]  { val0, val1 };

    int pxRow { 0 };
    for( int r { 0 }; r < mCellRowCount; ++r )
    {
      int checkerboard { r % 2 };
      int pxCol { 0 };
      for( int c { 0 }; c < mCellColCount; ++c )
      {
        checkerboard = 1 - checkerboard;

        if( r == 6 && c == 8 )
          ++asdf;

        const u8 val { vals[ checkerboard ] };
        const int beginRow { r * ( FontCellPxHeight + BilinearFilteringPadding ) };
        const int beginColumn { c * ( FontCellPxWidth + BilinearFilteringPadding ) };
        const TexelRegion region
        {
          .mBeginRow    { beginRow },
          .mBeginColumn { beginColumn },
          .mWidth       { FontCellPxWidth },
          .mHeight      { FontCellPxHeight },
        };
        FillAtlasRegion( initialAtlasMemory, region, val );
      }
      pxRow += TextPxHeight;
    }
  }

  static Language GetAssetPathLanguage( const AssetPathStringView& assetPath )
  {
    const String lowerAssetPath { ToLower( assetPath ) };

    for( int iLanguage {  }; iLanguage < ( int )Language::Count; ++iLanguage )
    {
      const Language curLanguage { Language( iLanguage ) };
      const String lowerLanguage { ToLower( LanguageToStr( curLanguage ) ) };
      if( lowerAssetPath.contains( lowerLanguage ) )
        return curLanguage;
    }

    return Language::Count;
  }

  void FontAtlas::Load( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const int maxCells { 1000 };
    mCellRowCount = ( int )Sqrt( maxCells );
    mCellColCount = mCellRowCount;

    mPxWidth = mCellColCount * FontCellPxWidth + ( mCellColCount - 1 ) * BilinearFilteringPadding;
    mPxHeight = mCellRowCount * FontCellPxHeight + ( mCellRowCount - 1 ) * BilinearFilteringPadding;
    mPxStride = mPxWidth;

    const int atlasVramByteCount { mPxWidth * mPxHeight };

    TAC_CALL( const AssetPathStrings fontAssetPaths{
      IterateAssetsInDir( "assets/fonts", AssetIterateType::Default, errors ) } );

    for( const AssetPathString& assetPath : fontAssetPaths )
    {
      const Language language { GetAssetPathLanguage( assetPath ) };
      if( language == Language::Count )
        continue;

      TAC_CALL( FontFile* fontFile{ TAC_NEW FontFile( assetPath, errors ) } );

      mFontFiles.push_back( fontFile );

      // Tac::Hash( language ); <-- unused result?

      mDefaultFonts[ language ] = fontFile;
    }

    TAC_ASSERT( mDefaultFonts.size() == fontAssetPaths.size() );
    TAC_ASSERT( mFontFiles.size() == fontAssetPaths.size() );
    TAC_RAISE_ERROR_IF( mDefaultFonts.empty() || mFontFiles.empty(), "No fonts found" );

    // fill the atlas with a color other than black
    // so we can see the borders of cells as they get come in
    //void* initialAtlasMemory = Render::SubmitAlloc( atlasVramByteCount );
    Vector< char > initialAtlasMemoryContainer( atlasVramByteCount );
    void* initialAtlasMemory { initialAtlasMemoryContainer.data() };

    const int totalCellCount { mCellRowCount * mCellColCount };

    const String formattedBytes{ FormatBytes(
      FormatByteSpec
      {
        .mByteCount       { atlasVramByteCount },
        .mMinDenomination { 1024 * 1024 },
      } ) };

    String msg;
    msg += "Font atlas vram size: ";
    msg += formattedBytes;
    msg += ", Atlas dims (";
    msg += ToString( mPxWidth );
    msg += "x";
    msg += ToString( mPxHeight );
    msg += "), Cell Count: ";
    msg += ToString( totalCellCount );
    msg += ", Row Count: ";
    msg += ToString( mCellRowCount );
    msg += ", Col Count: ";
    msg += ToString( mCellColCount );
    OS::OSDebugPrintLine( msg );

    const u8 dark { ( u8 )( 0.25f * 255 ) };
    const u8 light { ( u8 )( 0.75f * 255 ) };
    InitAtlasCheckerboard( initialAtlasMemory, dark, light );

    const Render::Image image
    {
      .mWidth  { mPxWidth },
      .mHeight { mPxHeight },
      .mFormat { atlasFormat},
    };

    const Render::CreateTextureParams::Subresource subresource
    {
      .mBytes { initialAtlasMemory },
      .mPitch { mPxStride },
    };

    const Render::CreateTextureParams cmdData
    {
      .mImage        { image },
      .mMipCount     { 1 },
      .mSubresources { &subresource },
      .mBinding      { Render::Binding::ShaderResource },
      .mUsage        { Render::Usage::Default },
      .mCpuAccess    { Render::CPUAccess::Write },
      .mOptionalName { "text-atlas" },
      .mStackFrame   { TAC_STACK_FRAME },
    };
    mTextureId = TAC_CALL( renderDevice->CreateTexture( cmdData, errors ) );
  }

  FontAtlasCell* FontAtlas::GetCharacter( Language defaultLanguage, Codepoint codepoint )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    //LanguageStuff* languageStuff = mLanguageStuffs[ defaultLanguage ];
    //FontStyle fontStyle = FontStyle::NormalText;
    //FontFile* fontFile = languageStuff->mFontStylePaths[ fontStyle ];
    FontFile* fontFile { mDefaultFonts[ defaultLanguage ] };

    FontAtlasCell* result { fontFile->mCells.FindVal( codepoint ).GetValueOr( nullptr ) };
    if( result && !FORCE_DRAW_EVERY_TIME )
      return result;

    const int glyphIndex { stbtt_FindGlyphIndex( &fontFile->mFontInfo, codepoint ) };
    if( !glyphIndex )
      return nullptr;

    // offset from the current horizontal position to the next horizontal position
    int unscaledAdvanceWidth;

    // offset from the current horizontal position to the left edge of the character
    int unscaledLeftSideBearing;
    stbtt_GetGlyphHMetrics( &fontFile->mFontInfo, glyphIndex, &unscaledAdvanceWidth, &unscaledLeftSideBearing );

    // output height & width of the SDF bitmap (including padding)
    int sdfWidth {};
    int sdfHeight {};

    // sdf pixel bmp space
    //
    // (0,0)
    //   +--a-----+
    //   |        |
    //   |  |     |
    //   |  |     |
    //   b  +---  |
    //   |        |
    //   +--------+
    //          (w,h)
    //
    // For this example, the letter 'L' is rendered,
    // and the bottom part of the L lies on the baseline.
    // I think (-xoff,-yoff) = (a,b)
    //
    // ----------+-------------> baseline
    //    (cursorX, cursorY)
    //
    //   (TLx, TLy)
    //        +--------+
    //        |        |
    //        |  |     |
    //        |  |     |
    // -------+  +---  +-------> baseline
    //        |        |
    //        +--------+
    // TLx = cursorX + xOff * relativeScale
    // TLy = cursorY + yOff * relativeScale

    int sdfxoff {  };
    int sdfyoff {  };

    const float scale { fontFile->mFontDims.mScale };

    // For an example, see https://github.com/nothings/stb/blob/master/tests/sdf/sdf_test.c

    // return value      --  a 2D array of bytes 0..255, width*height in size
    unsigned char* sdfBytes{ stbtt_GetGlyphSDF( &fontFile->mFontInfo,
                                                 scale,
                                                 glyphIndex,
                                                 FontCellInnerSDFPadding,
                                                 onedge_value,
                                                 pixel_dist_scale,
                                                 &sdfWidth,
                                                 &sdfHeight,
                                                 &sdfxoff,
                                                 &sdfyoff ) };

#if TAC_DEBUGGING_ATLAS()
    static bool written;
    if( !written && sdfBytes )
    {
      written = true;
      int write_result { stbi_write_bmp("deleteme.bmp", sdfWidth, sdfHeight, 1, sdfBytes) };
      if( !write_result )
      {
        ++asdf;
        TAC_ASSERT_INVALID_CODE_PATH;
      }
    }
#endif


    TAC_ASSERT( sdfWidth <= FontCellPxWidth );
    TAC_ASSERT( sdfHeight <= FontCellPxHeight );

    TAC_ON_DESTRUCT(
       stbtt_FreeSDF( sdfBytes, nullptr );
    );


    FontAtlasCell* cell { result };
    if( !cell )
      cell = GetCell();

    //int atlasPxWidth = mCellRowCount * FontCellPxWidth;
    //int atlasPxHeight = mCellColCount * FontCellPxHeight;

    // OGL vs DX Texture Coord Space
    //
    //        (1,1)   (0,0)
    //    ^---+         +--->
    //    |OGL|         |DX | 
    //    +--->         v---+
    // (0,0)               (1,1)

    const v2 minDXTexCoord{ cell->mPxColumn / ( float )mPxWidth,
                            cell->mPxRow / ( float )mPxHeight };
    const v2 maxDXTexCoord{ minDXTexCoord + v2( sdfWidth / ( float )mPxWidth,
                                                sdfHeight / ( float )mPxWidth ) };

    // cant use cell = FontAtlasCell{ .foo = bar, ... } because FontAtlasCell has other bookkeeping

    cell->mCodepoint = codepoint;
    cell->mCodepointAscii = codepoint < 128 ? ( char )codepoint : '?';
    cell->mOwner = fontFile;
    cell->mUnscaledAdvanceWidth = ( float )unscaledAdvanceWidth;
    cell->mUnscaledLeftSideBearing = ( float )unscaledLeftSideBearing;
    cell->mSDFxOffset = sdfxoff;
    cell->mSDFyOffset = sdfyoff;
    cell->mSDFWidth = sdfWidth;
    cell->mSDFHeight = sdfHeight;
    cell->mMinDXTexCoord = minDXTexCoord;
    cell->mMaxDXTexCoord = maxDXTexCoord;

    fontFile->mCells[ codepoint ] = cell;

    // uhh, first clear out the whole cell because the bilinear padding aint gonna do shit
    // if the part of the cell unused by the sdf is colored.
    //
    // This is also useful to see atlas cells taken up by characters with no sdf (ie ' ')
    // because their cell will be black instead of checkerboard
#if TAC_DEBUGGING_ATLAS()
    {
      const u8 srcBytes[ FontCellPxWidth * FontCellPxHeight ]  {};

      const Render::Image src
      {
        .mWidth  { FontCellPxWidth },
        .mHeight { FontCellPxHeight },
        .mFormat { atlasFormat },
      };

      const Render::UpdateTextureParams data
      {
        .mSrc      { src },
        .mDstX     { cell->mPxColumn },
        .mDstY     { cell->mPxRow },
        .mSrcBytes { srcBytes },
        .mPitch    { FontCellPxWidth },
      };

      renderDevice->UpdateTexture( mTextureId, data );
    }
#endif

    if( sdfWidth && sdfHeight )
    {
      const Render::Image src
      {
        .mWidth  { sdfWidth },
        .mHeight { sdfHeight },
        .mFormat { sdfFormat },
      };

      const Render::CreateTextureParams::Subresource srcSubresource
      {
        .mBytes{ sdfBytes },
        .mPitch{ sdfWidth },
      };

      // TODO: this function de/allocates a temporary texture every time.
      // Instead, create a texture once, and write to it with D3D11_MAP_DISCARD
      const Render::UpdateTextureParams updateTextureParams
      {
        .mSrcImage            { src },
        .mSrcSubresource      { srcSubresource },
        .mDstSubresourceIndex { 0 },
        .mDstPos              { cell->mPxColumn, cell->mPxRow },
      };

      Render::IContext* context{};
      Errors errors;
      context->UpdateTexture( mTextureId, updateTextureParams, errors );
    }

    result = cell;
    return result;
  }

  FontAtlasCell* FontAtlas::GetCell()
  {
    const int cellCount { mCells.size() };
    if( cellCount < mCellRowCount * mCellColCount )
    {
      const int cellIndex { cellCount };
      const int cellRow { cellIndex / mCellRowCount };
      const int cellColumn { cellIndex % mCellRowCount };

      FontAtlasCell* cell{ TAC_NEW FontAtlasCell
      {
        .mPxRow      { cellRow * ( FontCellPxHeight + BilinearFilteringPadding ) },
        .mPxColumn   { cellColumn * ( FontCellPxWidth + BilinearFilteringPadding ) },
        .mCellRow    { cellRow },
        .mCellColumn { cellColumn },
      } };

      mCells.push_back( cell );
      return cell;
    }

    FontAtlasCell* oldest { nullptr };
    for( FontAtlasCell* cell : mCells )
    {
      if( !cell->mOwner )
        return cell;

      if( !oldest || cell->mReadTime < oldest->mReadTime )
        oldest = cell;
    }

    FontFile* owner { oldest->mOwner };
    owner->mCells.erase( oldest->mCodepoint );
    oldest->mOwner = nullptr;
    return oldest;
  }

  void FontAtlas::DebugImgui()
  {
    //if( !ImGui::CollapsingHeader( "Font Stuff" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //ImGui::InputInt( "Row count", &mRowCount );
    //if( ImGui::Button( "Clear cells" ) )
    //{
    //  for( auto cell : mCells )
    //  {
    //    cell->mOwner->mCells.erase( cell->mCodepoint );
    //    TAC_DELETE cell;
    //  }
    //  mCells.clear();
    //}


    //if( mTexture )
    //{
    //  auto imguitextureID = mTexture->GetImguiTextureID();
    //  auto size = ImVec2( ( float )300, ( float )300 );
    //  ImGui::Image( imguitextureID, size );
    //}
    //else
    //{
    //  ImGui::Text( "No font atlas" );
    //}

    //if( !ImGui::CollapsingHeader( "Language Stuffs" ) )
    //  return;
    //ImGui::Indent();
    //OnDestruct( ImGui::Unindent() );
    //for( int iLanguage = 0; iLanguage < ( int )Language::Count; ++iLanguage )
    //{
    //  auto language = ( Language )iLanguage;
    //  LanguageStuff* languageStuff = mLanguageStuffs[ language ];
    //  if( !languageStuff )
    //    continue;
    //  if( !ImGui::CollapsingHeader( LanguageToStr( language ).c_str() ) )
    //    continue;

    //  ImGui::Indent();
    //  OnDestruct( ImGui::Unindent() );

    //  ImGui::PushID( languageStuff );
    //  OnDestruct( ImGui::PopID() );

    //  for( int iFontStyle = 0; iFontStyle < ( int )FontStyle::Count; ++iFontStyle )
    //  {
    //    auto fontstyle = ( FontStyle )iFontStyle;
    //    auto& fontstylestr = FontStyleToString( fontstyle );
    //    auto fontFile = languageStuff->mFontStylePaths[ fontstyle ];
    //    if( !fontFile )
    //      continue;
    //    if( !ImGui::CollapsingHeader( fontstylestr.c_str() ) )
    //      continue;
    //    ImGui::Indent();
    //    OnDestruct( ImGui::Unindent() );
    //    ImGui::InputText( "Filepath", ( char* )fontFile->mFilepath.data(), ( size_t )fontFile->mFilepath.size() );
    //  }
    //}
  }

}
#endif
