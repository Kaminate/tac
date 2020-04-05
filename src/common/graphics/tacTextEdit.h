#pragma once
#include "src/common/tacLocalization.h"
#include "src/common/containers/tacVector.h"

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
  int GetMinCaret();
  int GetMaxCaret();
  void OnClick( int numGlyphsBeforeCaret );
  void OnDrag( int numGlyphsBeforeCaret );
  void OnKeyPressed( TextInputKey key );
  void OnCodepoint( Codepoint codepoint );


  int mNumGlyphsBeforeCaret[ 2 ];
  int mCaretCount = 0;
  Vector< Codepoint > mCodepoints;

private:
  void OnArrowKeyPressed(
    bool canMoveDirection,
    int direction,
    int numGlyphsBeforeCaretEdge );
  void OnDestructivePressed( int deletedCodepointsStartIndex );
};
}
