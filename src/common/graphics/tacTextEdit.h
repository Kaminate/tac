#pragma once
#include "src/common/tacLocalization.h"
#include "src/common/containers/tacVector.h"

namespace Tac
{
  struct String;
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

    int                 mNumGlyphsBeforeCaret[ 2 ];

    //                  https://en.wikipedia.org/wiki/Cursor_(user_interface)
    int                 mCaretCount = 0;
    Vector< Codepoint > mCodepoints;

  private:
    void                OnArrowKeyPressed( bool canMoveDirection,
                                           int direction,
                                           int numGlyphsBeforeCaretEdge );
    void                OnDestructivePressed( int deletedCodepointsStartIndex );
  };
}
