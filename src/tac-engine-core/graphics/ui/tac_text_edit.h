#pragma once
#include "tac-std-lib/i18n/tac_localization.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/tac_core.h"

namespace Tac
{
  enum class TextInputKey
  {
    LeftArrow,
    RightArrow,
    Backspace,
    Delete,
  };

  struct TextInputData
  {
    int                 GetMinCaret();
    int                 GetMaxCaret();
    void                OnClick( int numGlyphsBeforeCaret );
    void                OnDrag( int numGlyphsBeforeCaret );
    void                OnKeyPressed( TextInputKey );
    void                OnCodepoint( Codepoint );
    void                SetCodepoints( CodepointView );

    String              GetText();
    void                SetText( StringView );
    CodepointView       GetCodepointView() const;

    int                 mNumGlyphsBeforeCaret[ 2 ] = {};

    //                  https://en.wikipedia.org/wiki/Cursor_(user_interface)
    int                 mCaretCount = 0;
    Vector< Codepoint > mCodepoints;

  private:
    void                OnArrowKeyPressed( bool canMoveDirection,
                                           int direction,
                                           int numGlyphsBeforeCaretEdge );
    void                OnDestructivePressed( int deletedCodepointsStartIndex );
  };

  // mousePos and textPos must be in the same space
  void TextInputDataUpdateKeys( TextInputData* inputData, const v2& mousePos, const v2& textPos );
  
} // namespace Tac
