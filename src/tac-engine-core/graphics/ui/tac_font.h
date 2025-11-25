// This file contains the font atlas
// A big ass texture is allocated at the start of the program,
// and glyphs ( letters, numbers, symbols of any language )
// are rendered onto it. When it runs out of space, new glyphs
// replace the old glyphs

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-std-lib/math/tac_vector2.h" // v2
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/shell/tac_shell_game_time.h"
#include "tac-engine-core/asset/tac_asset.h"

#define TAC_FONT_ENABLED() 1

#if TAC_FONT_ENABLED()

namespace Tac
{
  struct FontFile;
  struct Errors;

  //        The height and width in pixels of a cell in the font atlas
  const int FontCellPxSize           { 128 }; // 64 };
  const int FontCellPxWidth          { FontCellPxSize };
  const int FontCellPxHeight         { FontCellPxSize };

  //        Padding inside a font cell for sdf effects like stroke borders, glow, and drop shadow
  const int FontCellInnerSDFPadding  { 5 };

   //       Number of pixels between font cells
  const int BilinearFilteringPadding { 1 };

  //        This is the font size for a sdf glyph rendered into a cell
  const int TextPxHeight             { ( FontCellPxSize / 2 ) - ( 2 * FontCellInnerSDFPadding ) };

  struct FontDims
  {
    // y
    // ^
    // +--------------------------------------
    // | | |              ^                  ^
    // | |_|  _           |  ascent          |
    // | | | /_\ \  /     |                  | font
    // | | | \_   \/      |                  | atlas
    // +----------+-----+-+----> baseline    | cell
    // |         /      |                    | height
    // |        /       v  descent           v
    // |          ----------------------------
    // |          ^
    // |          | linegap
    // |          v
    // +----------------
    // | |\            ^ 
    // | | |  _   _    | ascent
    // | | | / \ / \   |
    // | |/  \_/ \_/   |
    // +----------/--+-+--------> baseline
    // |         /   |
    // |      \_/    v descent
    // |      --------

    //            ascent is the coordinate above the baseline the font extends
    float         mUnscaledAscent  {};

    //            descent is the coordinate below the baseline the font extends
    //            (i.e. it is typically negative)
    float         mUnscaledDescent {};

    //            lineGap is the spacing between one row's descent and the next row's ascent
    float         mUnscaledLinegap {};

    //            mScale is from stbtt_ScaleForPixelHeight( FontCellPxHeight )
    float         mScale           {};
  };

  struct FontCellPos
  {
    int            mPxRow                   {};
    int            mPxColumn                {};
    int            mCellRow                 {};
    int            mCellColumn              {};
  };

  struct FontCellUVs
  {
    //             DX uvs are better then GL uvs when rendering UI
    v2             mMinDXTexCoord           {};
    v2             mMaxDXTexCoord           {};
  };

  struct GlyphMetrics
  {
    //             offset from the current horizontal position to the next horizontal position
    float          mUnscaledAdvanceWidth    {};

    //             offset from the current horizontal position to the left edge of the character
    float          mUnscaledLeftSideBearing {};

    int            mSDFxOffset              {};
    int            mSDFyOffset              {};

    //             the width of the sdf image in px, having been generated for a specific scale
    int            mSDFWidth                {};
    int            mSDFHeight               {};
  };

  struct FontAtlasCell
  {
    Codepoint      mCodepoint               {};

    //             mCodepointAscii = (char)mCodepoint.
    //             This parameter exists for debug convenience, so that you can see
    //             [mCodepointAscii 97'a' char] in the watch window instead of
    //             [mCodepoint 97 unsigned int] saving yourself a cast.
    char           mCodepointAscii          {}; 

    FontFile*      mOwner                   {};
    GameTime      mWriteTime               {};
    GameTime      mReadTime                {};

    FontCellPos    mFontCellPos             {};
    FontCellUVs    mFontCellUVs             {};
    GlyphMetrics   mGlyphMetrics            {};
    bool           mNeedsGPUCopy            {};
  };

  struct FontApi
  {
    static void Init( Errors& );
    static void Uninit();
    static auto GetLanguageFontDims( Language ) -> const FontDims*;
    static auto GetFontAtlasCell( Language, Codepoint ) -> const FontAtlasCell*;
    static auto GetAtlasTextureHandle() -> Render::TextureHandle;
    static auto GetSDFOnEdgeValue() -> float; // [0,1]
    static auto GetSDFPixelDistScale() -> float; // [0,1]
    static void UpdateGPU( Errors& );
    static void LoadFont( Language, AssetPathStringView, Errors& );
    static bool IsFontLoaded( AssetPathStringView );
  };


} // namespace Tac
#endif


