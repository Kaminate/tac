#include "tacFont.h"
#include "tacUtility.h"
#include "tacRenderer.h"
#include "common/math/tacMath.h"
#include "tacShell.h"
#include "tacSettings.h"
#include "tacMemory.h"

#include "imgui.h"

#include "stb_truetype.h"

const TacFormat atlasFormat = { 1, sizeof( uint8_t ), TacGraphicsType::unorm };

static v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
{
  return v4(
      colorAlphaUnassociated.x * colorAlphaUnassociated.w,
      colorAlphaUnassociated.y * colorAlphaUnassociated.w,
      colorAlphaUnassociated.z * colorAlphaUnassociated.w,
      colorAlphaUnassociated.w);
}

TacFontFile::TacFontFile( const TacString& filepath, TacErrors& errors )
{
  mFilepath = filepath;
  mFontMemory = TacTemporaryMemory( mFilepath, errors );
  TAC_HANDLE_ERROR( errors );

  stbtt_InitFont( &mFontInfo, ( const unsigned char* )mFontMemory.data(), 0 );
  mScale = stbtt_ScaleForPixelHeight( &mFontInfo, ( float )TacFontCellWidth );

  int ascent;
  int descent;
  int linegap;
  stbtt_GetFontVMetrics( &mFontInfo, &ascent, &descent, &linegap );
  mAscent = ( float )ascent;
  mDescent = ( float )descent;
  mLinegap = ( float )linegap;
}
float TacFontFile::GetLineDist()
{
  return mScale * ( mLinegap + mAscent - mDescent );
}

TacFontStuff::TacFontStuff()
{
  mOutlineGlyphs = false;
  mOutlineWidth = 3;
}
TacFontStuff::~TacFontStuff()
{
  for( auto fontAtlasCell : mCells )
  {
    delete fontAtlasCell;
  }
  delete mTexture;
}
void TacFontStuff::Load( TacSettings* settings, TacRenderer* renderer, int atlasVramBytes, TacErrors& errors )
{
  mRenderer = renderer;


  for( int iLanguage = 0; iLanguage < ( int )TacLanguage::Count; ++iLanguage )
  {
    auto language = TacLanguage( iLanguage );
    const TacString& languageString = TacLanguageToStr( language );
    TacString fontFilePathDefault = language == TacLanguage::English  ? "assets/english_srcpro.ttf" : "";
    TacString fontFilePath = settings->GetString( nullptr, { "defaultfonts",  languageString }, fontFilePathDefault, errors );
    if( fontFilePath.empty() )
      continue;

    auto fontFile = new TacFontFile( fontFilePath, errors );
    TAC_HANDLE_ERROR( errors );

    mFontFiles.push_back( fontFile );

    mDefaultFonts[ language ] = fontFile;
  }

  if( mDefaultFonts.empty() || mFontFiles.empty() )
  {
    errors = "Hey bud you didnt load any languages check your settings";
    return;
  }

  mRowCount = ( int )std::sqrt( atlasVramBytes ) / TacFontCellWidth;
  int size = mRowCount * TacFontCellWidth;
  // fill the atlas with a color other than black so we can see the borders of cells as they get come in
  TacVector< uint8_t > initialAtlas( TacSquare( size ), ( uint8_t )( 0.3f * 255 ) );

  TacImage image;
  image.mData = initialAtlas.data();
  image.mWidth = size;
  image.mHeight = size;
  image.mPitch = size;
  image.mFormat = atlasFormat;
  TacTextureData textureData;
  textureData.access = TacAccess::Dynamic;
  textureData.binding = { TacBinding::ShaderResource };
  textureData.cpuAccess = { TacCPUAccess::Write };
  textureData.mName = "texture atlas";
  textureData.mStackFrame = TAC_STACK_FRAME;
  textureData.myImage = image;
  renderer->AddTextureResource( &mTexture, textureData, errors );
  TAC_HANDLE_ERROR( errors );



#if 0




  mFontMemory = TacTemporaryMemory( fontPath, errors );
  TAC_HANDLE_ERROR( errors );

  stbtt_InitFont( &mFontInfo, ( const unsigned char* )mFontMemory.data(), 0 );
  mFontSize = 32;
  mScale = stbtt_ScaleForPixelHeight( &mFontInfo, mFontSize );

  int ascent;
  int descent;
  int linegap;
  stbtt_GetFontVMetrics( &mFontInfo, &ascent, &descent, &linegap );
  mAscent = ( float )ascent;
  mDescent = ( float )descent;
  mLinegap = ( float )linegap;

  stbtt_pack_context spc;
  int atlasWidth = 512;
  int atlasHeight = 512;
  // 256 kb
  auto atlasRawData = TacVector< char >( atlasWidth * atlasHeight );

  int stride_in_bytes = 0;
  int padding = 1;
  void *alloc_context = nullptr;
  int packBeginResult = stbtt_PackBegin(
    &spc,
    ( unsigned char* )atlasRawData.data(),
    atlasWidth,
    atlasHeight,
    stride_in_bytes,
    padding,
    alloc_context );
  if( packBeginResult == 0 )
  {
    errors = "packbegin failed";
    return;
  }

  for( auto codepoint : codepoints )
  {
    if( codepoint > ( TacCodepoint )std::numeric_limits< int >::max() )
    {
      // yeah idk what to do about this, because stbtt uses ints
      TacAssert( true );
    }

    mGlyphIndex = stbtt_FindGlyphIndex( &mFontInfo, codepoint );

    int firstUnicodeCharInRange = ( int )codepoint;
    int numCharsInRange = 1;
    int fontIndex = 0;

    stbtt_packedchar packedchar;
    int packFontRangesResult = stbtt_PackFontRange(
      &spc,
      ( unsigned char* )mFontMemory.data(),
      fontIndex,
      mFontSize,
      firstUnicodeCharInRange,
      numCharsInRange,
      &packedchar );
    if( packFontRangesResult == 0 )
    {
      errors = "pack font range failed";
      return;
    }

    auto perCodepoint = new TacPerCodepoint();
    perCodepoint->mPackedChar = packedchar;
    perCodepoint->uiSpaceCharHalfWidth = 0.5f * ( packedchar.x1 - packedchar.x0 );
    perCodepoint->uiSpaceCharHalfHeight = 0.5f * ( packedchar.y1 - packedchar.y0 );
    perCodepoint->glyphUVMin = v2(
      ( float )packedchar.x0 / atlasWidth,
      ( float )packedchar.y0 / atlasHeight );
    perCodepoint->glyphUVMax = v2(
      ( float )packedchar.x1 / atlasWidth,
      ( float )packedchar.y1 / atlasHeight );
    mCodepointData[ codepoint ] = perCodepoint;
  }

  stbtt_PackEnd( &spc );

  TacFormat format;
  format.mByteCount = 1;
  format.mCount = 1;
  format.mGraphicsType = TacGraphicsType::unorm;

  TacImage image;
  image.mData = atlasRawData.data();
  image.mWidth = atlasWidth;
  image.mHeight = atlasHeight;
  image.mPitch = atlasWidth * sizeof( unsigned char );
  image.mFormat = format;

  mTexture = renderer->AddTextureResource(
    image,
    TacAccess::Default,
    { TacBinding::ShaderResource },
    fontPath,
    errors );
  TAC_HANDLE_ERROR( errors );

#endif
}
void TacFontStuff::GetCharacter(
  TacLanguage defaultLanguage,
  TacCodepoint codepoint,
  TacFontAtlasCell** fontAtlasCell,
  TacErrors& errors )
{
  //TacLanguageStuff* languageStuff = mLanguageStuffs[ defaultLanguage ];
  //TacFontStyle fontStyle = TacFontStyle::NormalText;
  //TacFontFile* fontFile = languageStuff->mFontStylePaths[ fontStyle ];
  TacFontFile* fontFile = mDefaultFonts[ defaultLanguage ];
  auto cellIt = fontFile->mCells.find( codepoint );
  if( cellIt != fontFile->mCells.end() )
  {
    auto pair = *cellIt;
    *fontAtlasCell = pair.second;
    return;
  }

  auto glyphIndex = stbtt_FindGlyphIndex( &fontFile->mFontInfo, codepoint );
  if( !glyphIndex )
    return;

  int x0;
  int x1;
  int y0;
  int y1;
  // ( Comment copied from stb_truetype.h )
  //
  // get the bbox of the bitmap centered around the glyph origin; so the
  // bitmap width is ix1-ix0, height is iy1-iy0, and location to place
  // the bitmap top left is (leftSideBearing*scale,iy0).
  // (Note that the bitmap uses y-increases-down, but the shape uses
  // y-increases-up, so CodepointBitmapBox and CodepointBox are inverted.)
  stbtt_GetGlyphBitmapBox(
    &fontFile->mFontInfo,
    glyphIndex,
    fontFile->mScale,
    fontFile->mScale,
    &x0,
    &y0,
    &x1,
    &y1 );

  int bitmapWidthPx = x1 - x0;
  int bitmapHeightPx = y1 - y0;
  int advanceWidth;
  int leftSideBearing;
  // ( Comment copied from stb_truetype.h )
  //
  // leftSideBearing is the offset from the current horizontal position to the left edge of the character
  // advanceWidth is the offset from the current horizontal position to the next horizontal position
  //   these are expressed in unscaled coordinates
  stbtt_GetGlyphHMetrics( &fontFile->mFontInfo, glyphIndex, &advanceWidth, &leftSideBearing );

  TacVector< uint8_t > bitmapMemory( bitmapWidthPx * bitmapHeightPx );
  stbtt_MakeCodepointBitmap(
    &fontFile->mFontInfo,
    ( unsigned char* )bitmapMemory.data(),
    bitmapWidthPx,
    bitmapHeightPx,
    bitmapWidthPx,
    fontFile->mScale,
    fontFile->mScale,
    codepoint );
  if( mOutlineGlyphs )
  {
    for( int r = 0; r < bitmapHeightPx; ++r )
    {
      for( int c = 0; c < bitmapWidthPx; ++c )
      {
        bool isBorder =
          r < mOutlineWidth ||
          r > bitmapHeightPx - 1 - mOutlineWidth ||
          c < mOutlineWidth ||
          c > bitmapWidthPx - 1 - mOutlineWidth;
        if( !isBorder )
          continue;
        bitmapMemory[ r * bitmapWidthPx + c ] = 255;
      }
    }
  }

  TacFontAtlasCell* cell = GetCell();

  auto atlasPxWidth = mRowCount * TacFontCellWidth;

  v2 minGLTexCoord = {
    ( float )cell->mColumn / ( float )mRowCount,
    1 - ( float )cell->mRow / ( float )mRowCount - ( float )bitmapHeightPx / atlasPxWidth };
  v2 maxGLTexCoord = {
    ( float )cell->mColumn / ( float )mRowCount + ( float )bitmapWidthPx / atlasPxWidth,
    1 - ( float )cell->mRow / ( float )mRowCount };

  cell->mCodepoint = codepoint;
  cell->mCodepointAscii = codepoint < 128 ? ( char )codepoint : '?';
  cell->mOwner = fontFile;
  cell->mUISpaceAdvanceWidth = advanceWidth * fontFile->mScale;
  cell->mUISpaceLeftSideBearing = leftSideBearing * fontFile->mScale;
  cell->mBitmapWidth = bitmapWidthPx;
  cell->mBitmapHeight = bitmapHeightPx;
  cell->mMinGLTexCoord = minGLTexCoord;
  cell->mMaxGLTexCoord = maxGLTexCoord;
  cell->mUISpaceVerticalShift = ( float )-y1;
  fontFile->mCells[ codepoint ] = cell;

  if( bitmapWidthPx && bitmapHeightPx )
  {
    TacImage src;
    src.mData = bitmapMemory.data();
    src.mFormat = atlasFormat;
    src.mHeight = bitmapHeightPx;
    src.mWidth = bitmapWidthPx;
    src.mPitch = bitmapWidthPx;

    int x = cell->mColumn * TacFontCellWidth;
    int y = cell->mRow * TacFontCellWidth;

    // TODO: this function de/allocates a temporary texture every time.
    // Instead, create a texture once, and write to it with D3D11_MAP_DISCARD

    mRenderer->CopyTextureRegion( mTexture, src, x, y, errors );
    TAC_HANDLE_ERROR( errors );
  }
  *fontAtlasCell = cell;
}
TacFontAtlasCell* TacFontStuff::GetCell()
{
  if( mCells.size() < TacSquare( mRowCount ) )
  {
    auto cellIndex = ( int )mCells.size();
    auto cell = new TacFontAtlasCell();
    cell->mRow = cellIndex / mRowCount;
    cell->mColumn = cellIndex % mRowCount;
    mCells.push_back( cell );
    return cell;
  }
  TacFontAtlasCell* oldest = nullptr;
  for( auto cell : mCells )
  {
    if( !cell->mOwner )
      return cell;
    if( !oldest || cell->mReadTime < oldest->mReadTime )
      oldest = cell;
  }
  auto owner = oldest->mOwner;
  owner->mCells.erase( oldest->mCodepoint );
  oldest->mOwner = nullptr;
  return oldest;
}
void TacFontStuff::DebugImgui()
{
  if( !ImGui::CollapsingHeader( "Font Stuff" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  ImGui::InputInt( "Row count", &mRowCount );
  if( ImGui::Button( "Clear cells" ) )
  {
    for( auto cell : mCells )
    {
      cell->mOwner->mCells.erase( cell->mCodepoint );
      delete cell;
    }
    mCells.clear();
  }

  ImGui::Checkbox( "Outline Glyphs", &mOutlineGlyphs );
  ImGui::InputInt( "Outline Width", &mOutlineWidth );

  if( mTexture )
  {
    auto imguitextureID = mTexture->GetImguiTextureID();
    auto size = ImVec2( ( float )300, ( float )300 );
    ImGui::Image( imguitextureID, size );
  }
  else
  {
    ImGui::Text( "No font atlas" );
  }

  if( !ImGui::CollapsingHeader( "Language Stuffs" ) )
    return;
  ImGui::Indent();
  OnDestruct( ImGui::Unindent() );
  //for( int iLanguage = 0; iLanguage < ( int )TacLanguage::Count; ++iLanguage )
  //{
  //  auto language = ( TacLanguage )iLanguage;
  //  TacLanguageStuff* languageStuff = mLanguageStuffs[ language ];
  //  if( !languageStuff )
  //    continue;
  //  if( !ImGui::CollapsingHeader( TacLanguageToStr( language ).c_str() ) )
  //    continue;

  //  ImGui::Indent();
  //  OnDestruct( ImGui::Unindent() );

  //  ImGui::PushID( languageStuff );
  //  OnDestruct( ImGui::PopID() );

  //  for( int iFontStyle = 0; iFontStyle < ( int )TacFontStyle::Count; ++iFontStyle )
  //  {
  //    auto fontstyle = ( TacFontStyle )iFontStyle;
  //    auto& fontstylestr = TacFontStyleToString( fontstyle );
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
