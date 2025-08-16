#pragma once

#include "tac_font.h"
#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-std-lib/math/tac_vector2.h" // v2
#include "tac-std-lib/containers/tac_map.h"
#include "tac-engine-core/asset/tac_asset.h"
#include "tac-rhi/render3/tac_render_api.h"
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/thirdparty/stb/stb_truetype.h"

namespace Tac
{
  struct GlyphBytes
  {
    ~GlyphBytes();

    unsigned char* mBytes{};
  };

  struct FontFile
  {
    typedef Map< Codepoint, FontAtlasCell* > CellMap;

    FontFile( const AssetPathStringView&, Errors& );

    GlyphBytes     GetGlyphBytes( int glyphIndex ) const;
    GlyphMetrics   GetGlyphMetrics( int glyphIndex ) const;
    void           DebugGlyph( int glyphIndex );
    FontAtlasCell* TryFindFontAtlasCell( Codepoint codepoint ) const
    {
      auto it{ mCells.find( codepoint ) };
      if( it == mCells.end() )
        return nullptr;
      auto& [_, cell] {*it};
      return cell;
    }
    
    AssetPathString  mAssetPath  {};
    CellMap          mCells      {};
    String           mFontMemory {};
    stbtt_fontinfo   mFontInfo   {};
    FontDims         mFontDims   {};
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
    static FontAtlas Instance;
    FontAtlas() = default;

    void Load( Errors& );
    void Uninit();
    void UpdateGPU( Errors& );
    void DebugImgui();
    auto GetCharacter( Language, Codepoint ) -> FontAtlasCell*;
    auto GetCell() -> FontAtlasCell*;
    auto GetSDFOnEdgeValue() const -> float;
    auto GetSDFPixelDistScale() const -> float;
    auto GetLanguageFontDims( Language ) -> const FontDims*;
    auto GetTextureHandle() const -> Render::TextureHandle;

  private:
    void InitAtlasCheckerboard( void*, u8, u8 );
    void FillAtlasRegion( void*, const TexelRegion&, u8 ); // inclusive
    auto ComputeTexCoords( const FontCellPos&, const GlyphMetrics& ) -> FontCellUVs;
    auto CellIndexToPos( int ) -> FontCellPos;
    void UploadCellGPU( FontAtlasCell*, Errors& );

  public: // HACK, REMOVE ME
    Render::TextureHandle           mTextureId            {};
    int                             mPxStride             {};
    int                             mPxWidth              {};
    int                             mPxHeight             {};
    int                             mCellRowCount         {};
    int                             mCellColCount         {};
    FontAtlasCell*                  mCells                {};
    int                             mCellCount            {};
    int                             mCellCapacity         {};
    Vector< FontFile* >             mFontFiles            {};
    Map< Language, FontFile* >      mDefaultFonts         {}; // this belongs 1 level higher
    bool                            FORCE_DRAW_EVERY_TIME {};
  };

} // namespace Tac
