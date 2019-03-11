#pragma once
#include "common/tacLocalization.h"
#include "common/containers/tacVector.h"

enum class TacTextInputKey
{
  LeftArrow,
  RightArrow,
  Backspace,
  Delete,
};

struct TacTextInputData
{
  int GetMinCaret();
  int GetMaxCaret();
  void OnClick( int numGlyphsBeforeCaret );
  void OnDrag( int numGlyphsBeforeCaret );
  void OnKeyPressed( TacTextInputKey key );
  void OnCodepoint( TacCodepoint codepoint );


  int mNumGlyphsBeforeCaret[ 2 ];
  int mCaretCount = 0;
  TacVector< TacCodepoint > mCodepoints;

private:
  void OnArrowKeyPressed(
    bool canMoveDirection,
    int direction,
    int numGlyphsBeforeCaretEdge );
  void OnDestructivePressed( int deletedCodepointsStartIndex );
};
