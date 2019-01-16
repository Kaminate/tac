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
  float mUISpaceAdvanceWidth = 0;
  float mUISpaceLeftSideBearing = 0;
  float mUISpaceVerticalShift = 0;
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
  float mAscent = 0;
  float mDescent = 0;
  float mLinegap = 0;
  float mScale = 0;
  float GetLineDist();
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
