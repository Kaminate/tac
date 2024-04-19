// This file contains the font atlas
// A big ass texture is allocated at the start of the program,
// and glyphs ( letters, numbers, symbols of any language )
// are rendered onto it. When it runs out of space, new glyphs
// replace the old glyphs

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-std-lib/math/tac_vector2.h" // v2
//#include "tac-rhi/render3/tac_render_api.h" // Render::TextureHandle
#include "tac-rhi/render3/tac_render_api.h"
//#include "tac-std-lib/tac_core.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"

namespace Tac
{
  struct FontFile;
  struct Errors;

  //        The height and width in pixels of a cell in the font atlas
  const int FontCellPxSize = 64;
  const int FontCellPxWidth = FontCellPxSize;
  const int FontCellPxHeight = FontCellPxSize;

  //        Padding inside a font cell for sdf effects like stroke borders, glow, and drop shadow
  const int FontCellInnerSDFPadding = 5;

   //       Number of pixels between font cells
  const int BilinearFilteringPadding = 1;

  //        This is the font size for a sdf glyph rendered into a cell
  const int TextPxHeight = FontCellPxSize - ( 2 * FontCellInnerSDFPadding );

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
    float         mUnscaledAscent = 0;

    //            descent is the coordinate below the baseline the font extends
    //            (i.e. it is typically negative)
    float         mUnscaledDescent = 0;

    //            lineGap is the spacing between one row's descent and the next row's ascent
    float         mUnscaledLinegap = 0;

    //            mScale is from stbtt_ScaleForPixelHeight( FontCellPxHeight )
    float         mScale = 0;
  };

  struct FontAtlasCell
  {
    Codepoint      mCodepoint = 0;

    //             mCodepointAscii = (char)mCodepoint.
    //             This parameter exists for debug convenience, so that you can see
    //             [mCodepointAscii 97'a' char] in the watch window instead of
    //             [mCodepoint 97 unsigned int] saving yourself a cast.
    char           mCodepointAscii = 0; 

    FontFile*      mOwner = nullptr;
    int            mPxRow = 0;
    int            mPxColumn = 0;
    int            mCellRow = 0;
    int            mCellColumn = 0;
    Timestamp      mWriteTime;
    Timestamp      mReadTime;
    
    //             DX uvs are better then GL uvs when rendering UI
    v2             mMinDXTexCoord = {};
    v2             mMaxDXTexCoord = {};

    float          mUnscaledAdvanceWidth = 0;
    float          mUnscaledLeftSideBearing = 0;

    int            mSDFxOffset = 0;
    int            mSDFyOffset = 0;

    //             the width of the sdf image in px, having been generated for a specific scale
    int            mSDFWidth = 0;
    int            mSDFHeight = 0;
  };

  struct FontApi
  {
    static void                  Init( Errors& );
    static void                  Uninit();
    static void                  SetForceRedraw(bool);
    static const FontDims*       GetLanguageFontDims( Language );
    static const FontAtlasCell*  GetFontAtlasCell( Language, Codepoint );
    static Render::TextureHandle GetAtlasTextureHandle();
    static float                 GetSDFOnEdgeValue(); // [0,1]
    static float                 GetSDFPixelDistScale(); // [0,1]
  };

} // namespace Tac
