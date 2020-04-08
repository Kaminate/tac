#include "src/common/tacUtility.h"
#include "src/common/graphics/tacFont.h"
#include "src/common/graphics/tacRenderer.h"
#include "src/common/tacShell.h"
#include "src/common/tacSettings.h"
#include "src/common/tacMemory.h"

namespace Tac
{


const Format atlasFormat = { 1, sizeof( uint8_t ), GraphicsType::unorm };

static v4 ToColorAlphaPremultiplied( v4 colorAlphaUnassociated )
{
  return {
    colorAlphaUnassociated.x * colorAlphaUnassociated.w,
    colorAlphaUnassociated.y * colorAlphaUnassociated.w,
    colorAlphaUnassociated.z * colorAlphaUnassociated.w,
    colorAlphaUnassociated.w };
}

FontFile::FontFile( const String& filepath, Errors& errors )
{
  mFilepath = filepath;
  mFontMemory = TemporaryMemoryFromFile( mFilepath, errors );
  TAC_HANDLE_ERROR( errors );

  stbtt_InitFont( &mFontInfo, ( const unsigned char* )mFontMemory.data(), 0 );
  mScale = stbtt_ScaleForPixelHeight( &mFontInfo, ( float )FontCellWidth );

  int ascent;
  int descent;
  int linegap;
  stbtt_GetFontVMetrics( &mFontInfo, &ascent, &descent, &linegap );
  mAscent = ( float )ascent;
  mDescent = ( float )descent;
  mLinegap = ( float )linegap;

  mUISpaceAscent = ( float )ascent * mScale;
  mUISpaceDescent = ( float )descent * mScale;
  mUISpaceLinegap = ( float )linegap * mScale;
}

FontStuff* FontStuff::Instance = nullptr;
FontStuff::FontStuff()
{
  Instance = this;
  mOutlineGlyphs = false;
  mOutlineWidth = 3;
}
FontStuff::~FontStuff()
{
  for( auto fontAtlasCell : mCells )
  {
    delete fontAtlasCell;
  }
  Renderer::Instance->RemoveRendererResource( mTexture );
}
void FontStuff::Load( Errors& errors )
{
  const int atlasVramBytes = 40 * 1024 * 1024;

  for( int iLanguage = 0; iLanguage < ( int )Language::Count; ++iLanguage )
  {
    auto language = Language( iLanguage );
    const String& languageString = LanguageToStr( language );
    String fontFilePathDefault = language == Language::English ? "assets/fonts/english_srcpro.ttf" : "";
    String fontFilePath = Shell::Instance->mSettings->GetString(
      nullptr,
      { "defaultfonts", languageString },
      fontFilePathDefault,
      errors );
    if( fontFilePath.empty() )
      continue;

    auto fontFile = new FontFile( fontFilePath, errors );
    TAC_HANDLE_ERROR( errors );

    mFontFiles.push_back( fontFile );

    mDefaultFonts[ language ] = fontFile;
  }

  if( mDefaultFonts.empty() || mFontFiles.empty() )
  {
    errors = "Hey bud you didnt load any languages check your settings";
    return;
  }

  mRowCount = ( int )std::sqrt( atlasVramBytes ) / FontCellWidth;
  int size = mRowCount * FontCellWidth;
  // fill the atlas with a color other than black so we can see the borders of cells as they get come in
  Vector< uint8_t > initialAtlas( size * size, ( uint8_t )( 0.3f * 255 ) );

  Image image;
  image.mData = initialAtlas.data();
  image.mWidth = size;
  image.mHeight = size;
  image.mPitch = size;
  image.mFormat = atlasFormat;
  TextureData textureData;
  textureData.access = Access::Dynamic;
  textureData.binding = { Binding::ShaderResource };
  textureData.cpuAccess = { CPUAccess::Write };
  textureData.mName = "texture atlas";
  textureData.mFrame = TAC_STACK_FRAME;
  textureData.myImage = image;
  Renderer::Instance->AddTextureResource( &mTexture, textureData, errors );

  Render::;

  TAC_HANDLE_ERROR( errors );



  #if 0




  mFontMemory = TemporaryMemory( fontPath, errors );
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
  auto atlasRawData = Vector< char >( atlasWidth * atlasHeight );

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
    if( codepoint > ( Codepoint )std::numeric_limits< int >::max() )
    {
      // yeah idk what to do about this, because stbtt uses ints
      Assert( true );
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

    auto perCodepoint = new PerCodepoint();
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

  Format format;
  format.mByteCount = 1;
  format.mCount = 1;
  format.mGraphicsType = GraphicsType::unorm;

  Image image;
  image.mData = atlasRawData.data();
  image.mWidth = atlasWidth;
  image.mHeight = atlasHeight;
  image.mPitch = atlasWidth * sizeof( unsigned char );
  image.mFormat = format;

  mTexture = Renderer::Instance->AddTextureResource(
    image,
    Access::Default,
    { Binding::ShaderResource },
    fontPath,
    errors );
  TAC_HANDLE_ERROR( errors );

  #endif
}
void FontStuff::GetCharacter(
  Language defaultLanguage,
  Codepoint codepoint,
  FontAtlasCell** fontAtlasCell,
  Errors& errors )
{
  //LanguageStuff* languageStuff = mLanguageStuffs[ defaultLanguage ];
  //FontStyle fontStyle = FontStyle::NormalText;
  //FontFile* fontFile = languageStuff->mFontStylePaths[ fontStyle ];
  FontFile* fontFile = mDefaultFonts[ defaultLanguage ];
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

  int stride = bitmapWidthPx;
  Vector< uint8_t > bitmapMemory( stride * bitmapHeightPx );
  stbtt_MakeCodepointBitmap(
    &fontFile->mFontInfo,
    ( unsigned char* )bitmapMemory.data(),
    bitmapWidthPx,
    bitmapHeightPx,
    stride,
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

  FontAtlasCell* cell = GetCell();

  auto atlasPxWidth = mRowCount * FontCellWidth;

  v2 minGLTexCoord = {
    ( float )cell->mColumn / ( float )mRowCount,
    1 - ( float )cell->mRow / ( float )mRowCount - ( float )bitmapHeightPx / atlasPxWidth };
  v2 maxGLTexCoord = {
    ( float )cell->mColumn / ( float )mRowCount + ( float )bitmapWidthPx / atlasPxWidth,
    1 - ( float )cell->mRow / ( float )mRowCount };

  cell->mCodepoint = codepoint;
  cell->mCodepointAscii = codepoint < 128 ? ( char )codepoint : '?';
  cell->mOwner = fontFile;

  cell->mAdvanceWidth = ( float )advanceWidth;
  cell->mUISpaceAdvanceWidth = advanceWidth * fontFile->mScale;

  cell->mUISpaceLeftSideBearing = leftSideBearing * fontFile->mScale;
  cell->mLeftSideBearing = ( float )leftSideBearing;

  cell->mBitmapWidth = bitmapWidthPx;
  cell->mBitmapHeight = bitmapHeightPx;
  cell->mMinGLTexCoord = minGLTexCoord;
  cell->mMaxGLTexCoord = maxGLTexCoord;
  cell->mUISpaceVerticalShift = ( float )-y1;
  fontFile->mCells[ codepoint ] = cell;

  if( bitmapWidthPx && bitmapHeightPx )
  {
    Image src;
    src.mData = bitmapMemory.data();
    src.mFormat = atlasFormat;
    src.mHeight = bitmapHeightPx;
    src.mWidth = bitmapWidthPx;
    src.mPitch = bitmapWidthPx;

    int x = cell->mColumn * FontCellWidth;
    int y = cell->mRow * FontCellWidth;

    // TODO: this function de/allocates a temporary texture every time.
    // Instead, create a texture once, and write to it with D3D11_MAP_DISCARD

    // Renderer::Instance->CopyTextureRegion( mTexture, src, x, y, errors );

    Render::UpdateTextureRegion( mTextureId, src, x, y );

    TAC_HANDLE_ERROR( errors );
  }
  *fontAtlasCell = cell;
}
FontAtlasCell* FontStuff::GetCell()
{
  if( mCells.size() < mRowCount * mRowCount )
  {
    auto cellIndex = ( int )mCells.size();
    auto cell = new FontAtlasCell();
    cell->mRow = cellIndex / mRowCount;
    cell->mColumn = cellIndex % mRowCount;
    mCells.push_back( cell );
    return cell;
  }
  FontAtlasCell* oldest = nullptr;
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
void FontStuff::DebugImgui()
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
  //    delete cell;
  //  }
  //  mCells.clear();
  //}

  //ImGui::Checkbox( "Outline Glyphs", &mOutlineGlyphs );
  //ImGui::InputInt( "Outline Width", &mOutlineWidth );

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
