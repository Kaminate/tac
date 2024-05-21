#include "tac_font_backend.h" // self-inc

#include "tac-engine-core/framememory/tac_frame_memory.h"
#include "tac-engine-core/thirdparty/stb_truetype.h"
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

namespace Tac
{

  static_assert( FontCellInnerSDFPadding, "cant get anything to render w/o padding" );

  // onedge_value     
  // value 0-255 to test the SDF against to reconstruct the character (i.e. the isocontour of the character)
  const unsigned char onedge_value{ 128 }; // [0,255]

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
  const float pixel_dist_scale{ onedge_value / ( float )FontCellInnerSDFPadding };

  // -----------------------------------------------------------------------------------------------

  static Language GetAssetPathLanguage( const AssetPathStringView& assetPath )
  {
    const String lowerAssetPath { ToLower( assetPath ) };

    for( int iLanguage {}; iLanguage < ( int )Language::Count; ++iLanguage )
    {
      const Language curLanguage { Language( iLanguage ) };
      const String lowerLanguage { ToLower( LanguageToStr( curLanguage ) ) };
      if( lowerAssetPath.contains( lowerLanguage ) )
        return curLanguage;
    }

    return Language::Count;
  }

  // -----------------------------------------------------------------------------------------------

  GlyphBytes::~GlyphBytes()
  {
     stbtt_FreeSDF( mBytes, nullptr );
  }

  // -----------------------------------------------------------------------------------------------

  FontFile::FontFile( const AssetPathStringView& filepath, Errors& errors )
  {
    mAssetPath = filepath;
    mFontMemory = TAC_CALL( LoadAssetPath( filepath, errors ) );

    stbtt_InitFont( &mFontInfo, ( const unsigned char* )mFontMemory.data(), 0 );
    const float scale{ stbtt_ScaleForPixelHeight( &mFontInfo, ( float )TextPxHeight ) };

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
  }

  GlyphBytes       FontFile::GetGlyphBytes( int glyphIndex )
  {
    // return value      --  a 2D array of bytes 0..255, width*height in size
    unsigned char* sdfBytes{
      stbtt_GetGlyphSDF( &mFontInfo,
                         mFontDims.mScale ,
                         glyphIndex,
                         FontCellInnerSDFPadding,
                         onedge_value,
                         pixel_dist_scale,
                         nullptr,
                         nullptr,
                         nullptr,
                         nullptr ) };

    return GlyphBytes
    {
      .mBytes { sdfBytes },
    };
  }

  GlyphMetrics     FontFile::GetGlyphMetrics( int glyphIndex )
  {

    // offset from the current horizontal position to the next horizontal position
    int unscaledAdvanceWidth;

    // offset from the current horizontal position to the left edge of the character
    int unscaledLeftSideBearing;
    stbtt_GetGlyphHMetrics( &mFontInfo,
                            glyphIndex,
                            &unscaledAdvanceWidth,
                            &unscaledLeftSideBearing );


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

    int sdfxoff {};
    int sdfyoff {};

    const float scale{ mFontDims.mScale };
    int ix0;
    int iy0;
    int ix1;
    int iy1;

    // get the bbox of the bitmap centered around the glyph origin; so the
    // bitmap width is ix1-ix0, height is iy1-iy0, and location to place
    // the bitmap top left is (leftSideBearing*scale,iy0).
    // (Note that the bitmap uses y-increases-down, but the shape uses
    // y-increases-up, so CodepointBitmapBox and CodepointBox are inverted.)
    stbtt_GetGlyphBitmapBox( &mFontInfo, glyphIndex, scale, scale, &ix0, &iy0, &ix1, &iy1 );

    // output height & width of the SDF bitmap (including padding)
    const int sdfWidth  { ix1 - ix0 };
    const int sdfHeight { iy1 - iy0 };

    return GlyphMetrics
    {
      .mUnscaledAdvanceWidth    { ( float )unscaledAdvanceWidth },
      .mUnscaledLeftSideBearing { ( float )unscaledLeftSideBearing },
      .mSDFxOffset              { ix0 },
      .mSDFyOffset              { iy0 },
      .mSDFWidth                { sdfWidth },
      .mSDFHeight               { sdfHeight },
    };
  }

  void             FontFile::DebugGlyph( int glyphIndex )
  {
#if TAC_DEBUGGING_ATLAS()
    static bool written;
    if( written )
      return;

    const GlyphBytes glyphBytes{ GetGlyphBytes( glyphIndex ) };
    if( !glyphBytes.mBytes )
      return;

    const FontAtlasCell::GlyphMetrics glyphMetrics{ GetGlyphMetrics( glyphIndex ) };

    const int w{ glyphMetrics.mSDFWidth };
    const int h{ glyphMetrics.mSDFHeight };
    const int write_result{ stbi_write_bmp( "deleteme.bmp", w, h, 1, glyphBytes.mBytes ) };
    TAC_ASSERT( write_result );
    written = true;
#endif
  }

  // -----------------------------------------------------------------------------------------------

  FontAtlas             FontAtlas::Instance;

  void                  FontAtlas::Load( Errors& errors )
  {
    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    const int maxCells { 1 };
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
      mDefaultFonts[ language ] = fontFile;
    }

    TAC_ASSERT( mDefaultFonts.size() == fontAssetPaths.size() );
    TAC_ASSERT( mFontFiles.size() == fontAssetPaths.size() );
    TAC_RAISE_ERROR_IF( mDefaultFonts.empty() || mFontFiles.empty(), "No fonts found" );

    // fill the atlas with a color other than black
    // so we can see the borders of cells as they get come in
    Vector< char > initialAtlasMemoryContainer( atlasVramByteCount );
    void* initialAtlasMemory { initialAtlasMemoryContainer.data() };

    const int totalCellCount { mCellRowCount * mCellColCount };

    const FormatByteSpec formatByteSpec
    {
      .mByteCount       { atlasVramByteCount },
      .mMinDenomination { 1024 * 1024 },
    };
    const String formattedBytes{ FormatBytes( formatByteSpec ) };

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
      .mFormat { Render::TexFmt::kR8_unorm },
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

  void                  FontAtlas::Uninit()
  {
    for( FontAtlasCell* fontAtlasCell : mCells )
    {
      TAC_DELETE fontAtlasCell;
    }

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };

    renderDevice->DestroyTexture( mTextureId );
  }

  FontAtlasCell*        FontAtlas::GetCharacter( Language defaultLanguage, Codepoint codepoint )
  {
    // For an example, see https://github.com/nothings/stb/blob/master/tests/sdf/sdf_test.c

    Render::IDevice* renderDevice{ Render::RenderApi::GetRenderDevice() };
    //LanguageStuff* languageStuff = mLanguageStuffs[ defaultLanguage ];
    //FontStyle fontStyle = FontStyle::NormalText;
    //FontFile* fontFile = languageStuff->mFontStylePaths[ fontStyle ];
    FontFile* fontFile { mDefaultFonts[ defaultLanguage ] };

    if( Optional< FontAtlasCell* > cell{ fontFile->mCells.FindVal( codepoint ) };
        cell && !FORCE_DRAW_EVERY_TIME )
      return cell.GetValue();

    const int glyphIndex { stbtt_FindGlyphIndex( &fontFile->mFontInfo, codepoint ) };
    if( !glyphIndex )
      return nullptr;

    const GlyphMetrics glyphMetrics{ fontFile->GetGlyphMetrics( glyphIndex ) };
    TAC_ASSERT( glyphMetrics.mSDFWidth <= FontCellPxWidth );
    TAC_ASSERT( glyphMetrics.mSDFHeight <= FontCellPxHeight );

    const GlyphBytes glyphBytes{ fontFile->GetGlyphBytes( glyphIndex ) };

    fontFile->DebugGlyph( glyphIndex );

    FontAtlasCell* cell{ FORCE_DRAW_EVERY_TIME
      ? *fontFile->mCells.FindVal( codepoint )
      : GetCell() };

    // cant use cell = FontAtlasCell{ .foo = bar, ... } because FontAtlasCell has other bookkeeping

    cell->mCodepoint = codepoint;
    cell->mCodepointAscii = codepoint < 128 ? ( char )codepoint : '?';
    cell->mOwner = fontFile;
    cell->mGlyphMetrics = glyphMetrics;
    cell->mFontCellUVs = ComputeTexCoords( cell->mFontCellPos, glyphMetrics );

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

    if( glyphMetrics.mSDFWidth && glyphMetrics.mSDFHeight )
    {
      const Render::Image src
      {
        .mWidth  { glyphMetrics.mSDFWidth },
        .mHeight { glyphMetrics.mSDFHeight },
        .mFormat { Render::TexFmt::kR8_unorm },
      };

      const Render::CreateTextureParams::Subresource srcSubresource
      {
        .mBytes { glyphBytes.mBytes },
        .mPitch { glyphMetrics.mSDFWidth },
      };

      const Render::UpdateTextureParams updateTextureParams
      {
        .mSrcImage            { src },
        .mSrcSubresource      { srcSubresource },
        .mDstSubresourceIndex { 0 },
        .mDstPos              { cell->mFontCellPos.mPxColumn, cell->mFontCellPos.mPxRow },
      };

      Render::IContext* context{};
      TAC_ASSERT( context );
      Errors errors;
      context->UpdateTexture( mTextureId, updateTextureParams, errors );
    }

    return cell;
  }

  FontCellPos           FontAtlas::CellIndexToPos( int cellIndex )
  {
    const int cellRow{ cellIndex / mCellRowCount };
    const int cellColumn{ cellIndex % mCellRowCount };

    return FontCellPos
    {
      .mPxRow      { cellRow * ( FontCellPxHeight + BilinearFilteringPadding ) },
      .mPxColumn   { cellColumn * ( FontCellPxWidth + BilinearFilteringPadding ) },
      .mCellRow    { cellRow },
      .mCellColumn { cellColumn },
    };
  }

  FontAtlasCell*        FontAtlas::GetCell()
  {
    if( const int cellCount { mCells.size() }; cellCount < mCellRowCount * mCellColCount )
    {
      FontAtlasCell* cell{ TAC_NEW FontAtlasCell };
      cell->mFontCellPos = CellIndexToPos( cellCount );
      mCells.push_back( cell );
      return cell;
    }
    else
    {
      FontAtlasCell* oldest{};
      for( FontAtlasCell* cell : mCells )
      {
        if( !cell->mOwner )
          return cell;

        if( !oldest || cell->mReadTime < oldest->mReadTime )
          oldest = cell;
      }

      FontFile* owner{ oldest->mOwner };
      owner->mCells.erase( oldest->mCodepoint );

      oldest->mOwner = nullptr;
      return oldest;
    }
  }


  void                  FontAtlas::FillAtlasRegion( void* initialAtlasMemory,
                                                    const TexelRegion& region,
                                                    u8 val )
  {
    auto mem{ ( u8* )initialAtlasMemory };
    u8* memRegionTL{ mem + ( std::ptrdiff_t )( region.mBeginColumn + region.mBeginRow * mPxStride ) };
    for( int i{}; i < region.mHeight; ++i )
      MemSet( memRegionTL + ( std::ptrdiff_t )( i * mPxStride ), val, region.mWidth );
  }

  void                  FontAtlas::InitAtlasCheckerboard( void* initialAtlasMemory,
                                                          u8 val0,
                                                          u8 val1 )
  {
    static_assert( BilinearFilteringPadding > 0 );
    const u8 bilinearFilterPaddingValue{ 0 };

    auto mem{ ( u8* )initialAtlasMemory };
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

    u8 vals[]{ val0, val1 };

    int pxRow{ 0 };
    for( int r{ 0 }; r < mCellRowCount; ++r )
    {
      int checkerboard{ r % 2 };
      int pxCol{ 0 };
      for( int c{ 0 }; c < mCellColCount; ++c )
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

  void                  FontAtlas::DebugImgui()
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

  float                 FontAtlas::GetSDFOnEdgeValue() const
  {
    return onedge_value / 255.0f;
  }

  float                 FontAtlas::GetSDFPixelDistScale() const
  {
    return pixel_dist_scale / 255.0f;
  }

  FontCellUVs           FontAtlas::ComputeTexCoords( const FontCellPos& mFontCellPos,
                                                     const GlyphMetrics& mGlyphMetrics )
  {
    // OGL vs DX Texture Coord Space
    //
    //        (1,1)   (0,0)
    //    ^---+         +--->
    //    |OGL|         |DX | 
    //    +--->         v---+
    // (0,0)               (1,1)

    FontAtlas& fontAtlas{ FontAtlas::Instance };

    const v2 sdfSize( ( float )mGlyphMetrics.mSDFWidth,
                      ( float )mGlyphMetrics.mSDFHeight );

    const v2 minDXTexCoord{ mFontCellPos.mPxColumn / sdfSize.x,
                            mFontCellPos.mPxRow / sdfSize.y };
    const v2 maxDXTexCoord{ minDXTexCoord + v2( sdfSize.x / ( float )fontAtlas.mPxWidth,
                                                sdfSize.y / ( float )fontAtlas.mPxHeight ) };
    return FontCellUVs
    {
      .mMinDXTexCoord { minDXTexCoord },
      .mMaxDXTexCoord { maxDXTexCoord },
    };
  }

  const FontDims*       FontAtlas::GetLanguageFontDims( Language language )
  {
    if( Optional< FontFile* > fontFile{ mDefaultFonts.FindVal( language ) }  )
      return &( ( *fontFile )->mFontDims );

    return nullptr;
  }

  Render::TextureHandle FontAtlas::GetTextureHandle()
  {
    return mTextureId;
  }
}

