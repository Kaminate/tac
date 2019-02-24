// This file contains the font atlas
// A big ass texture is allocated at the start of the program,
// and glyphs ( letters, numbers, symbols of any language )
// are rendered onto it. When it runs out of space, new glyphs
// replace the old glyphs

#pragma once

#include "common/tacLocalization.h"
#include "common/containers/tacVector.h"
#include "common/math/tacVector2.h"
#include "common/math/tacVector4.h"
#include "common/tacErrorHandling.h"
#include "common/stb_truetype.h"

struct TacTexture;
struct TacRenderer;
struct TacFontFile;
struct TacTexture;
struct TacSettings;

// - This is the height and width in pixels of a cell in the font atlas
// - When a glyph is rendered into this cell, the height of the cell is used
//   as the distance between the font's ( highest ascender - lowest descender ).
// - In other words, this is the font size for glyphs in the atlas.
// - It's pretty much assumed that the glyph can fit in this cell.
//   are there any fonts with wide glyphs which would have their sides clipped?
const int TacFontCellWidth = 64;

const int TacFontAtlasDefaultVramByteCount = 40 * 1024 * 1024;

struct TacFontAtlasCell
{
  TacCodepoint mCodepoint = 0;
  char mCodepointAscii = 0; // For debugging purposes
  TacFontFile* mOwner = nullptr;
  int mRow = 0;
  int mColumn = 0;
  double mWriteTime = 0;
  double mReadTime = 0;
  v2 mMinGLTexCoord = {};
  v2 mMaxGLTexCoord = {};


  // todo:
  //  if foo is unscaled, rename it to unscaledFoo
  //  or if its in bitmap space, rename it to bitmapFoo
  //  delete the clones

  float mUISpaceAdvanceWidth = 0;
  float mAdvanceWidth = 0; // unscaled

  float mUISpaceLeftSideBearing = 0;
  float mLeftSideBearing = 0; // unscaled

  float mUISpaceVerticalShift = 0;

  // measured in pixels in bitmap space ( uispace )
  int mBitmapWidth = 0;
  int mBitmapHeight = 0;
};

struct TacFontFile
{
  TacFontFile( const TacString& filepath, TacErrors& errors );
  TacString mFilepath;
  std::map< TacCodepoint, TacFontAtlasCell* > mCells;
  TacVector< char > mFontMemory;
  stbtt_fontinfo mFontInfo = {};

  // y
  // ^
  // |_________________________________________
  // | | |              ^ ascent             ^
  // | |_| .       _    |                    | font
  // | | | | \  / / \   |                    | atlas
  // |_|_|_|__\/__\_/\__|_______ baseline    | cell
  // |        / |                            | height
  // |       /__v__descent___________________v_
  // |         ^
  // |         | linegap
  // |_________v_____
  // | |\           ^ ascent
  // | | |  _   _   |
  // | | | / \ / |  |
  // |_|/__\_/_\_|__|___________ baseline
  //             |
  //           \_/

  // ascent is the coordinate above the baseline the font extends
  float mAscent = 0;
  float mUISpaceAscent = 0;

  // descent is the coordinate below the baseline the font extends
  // (i.e. it is typically negative)
  float mDescent = 0;
  float mUISpaceDescent = 0;

  // lineGap is the spacing between one row's descent and the next row's ascent
  float mLinegap = 0;
  float mUISpaceLinegap = 0;

  // scaleFontToUI
  // scale = pixels / (ascent - descent)
  float mScale = 0;
};




// TODO: rename from TacFontStuff to TacFontAtlas
struct TacFontStuff
{
  TacFontStuff();
  ~TacFontStuff();
  void Load( TacSettings* settings, TacRenderer* renderer, int atlasVramBytes, TacErrors& errors );
  void DebugImgui();

  void GetCharacter(
    TacLanguage defaultLanguage,
    TacCodepoint codepoint,
    TacFontAtlasCell** fontAtlasCell,
    TacErrors& errors );
  TacFontAtlasCell* GetCell();

  TacTexture* mTexture = nullptr;
  TacRenderer* mRenderer = nullptr;
  int mRowCount = 0;
  TacVector< TacFontAtlasCell* > mCells;
  TacVector< TacFontFile* > mFontFiles;
  std::map< TacLanguage, TacFontFile* > mDefaultFonts;
  bool mOutlineGlyphs;
  int mOutlineWidth;
};
