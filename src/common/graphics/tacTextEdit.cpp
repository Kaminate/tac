#include "common/graphics/tacTextEdit.h"
#include "common/math/tacMath.h" // Min/Max

int TacTextInputData::GetMinCaret()
{
  return TacMin(
    mNumGlyphsBeforeCaret[ 0 ],
    mNumGlyphsBeforeCaret[ 1 ] );
}

int TacTextInputData::GetMaxCaret()
{
  return TacMax(
    mNumGlyphsBeforeCaret[ 0 ],
    mNumGlyphsBeforeCaret[ 1 ] );
}

void TacTextInputData::OnClick( int numGlyphsBeforeCaret )
{
  mNumGlyphsBeforeCaret[ 0 ] = numGlyphsBeforeCaret;
  mCaretCount = 1;
}

void TacTextInputData::OnDrag( int numGlyphsBeforeCaret )
{
  if( !mCaretCount || mNumGlyphsBeforeCaret[ 0 ] == numGlyphsBeforeCaret )
    return;
  mNumGlyphsBeforeCaret[ 1 ] = numGlyphsBeforeCaret;
  mCaretCount = 2;
}

void TacTextInputData::OnArrowKeyPressed(
  bool canMoveDirection,
  int direction,
  int numGlyphsBeforeCaretEdge )
{
  if( mCaretCount == 1 )
  {
    if( canMoveDirection )
      mNumGlyphsBeforeCaret[ 0 ] += direction;
  }
  else if( mCaretCount == 2 )
  {
    mNumGlyphsBeforeCaret[ 0 ] = numGlyphsBeforeCaretEdge;
    mCaretCount = 1;
  }
}

void TacTextInputData::OnDestructivePressed( int deletedCodepointsStartIndex )
{
  if( !mCaretCount )
    return;

  int deletedCodepointCount = 1;
  if( mCaretCount > 1 )
  {
    deletedCodepointsStartIndex = GetMinCaret();
    deletedCodepointCount = GetMaxCaret() - GetMinCaret();
  }

  TacVector< TacCodepoint > newCodepoints;
  for( int iCodepoint = 0; iCodepoint < mCodepoints.size(); ++iCodepoint )
  {
    if( iCodepoint >= deletedCodepointsStartIndex &&
      iCodepoint < deletedCodepointsStartIndex + deletedCodepointCount )
      continue;
    newCodepoints.push_back( mCodepoints[ iCodepoint ] );
  }

  if( newCodepoints.size() == mCodepoints.size() )
    return;
  mCodepoints = newCodepoints;
  mNumGlyphsBeforeCaret[ 0 ] = deletedCodepointsStartIndex;
  mCaretCount = 1;
}

void TacTextInputData::OnKeyPressed( TacTextInputKey key )
{
  switch( key )
  {
  case TacTextInputKey::RightArrow:
    OnArrowKeyPressed( mNumGlyphsBeforeCaret[ 0 ] < mCodepoints.size(), 1, GetMaxCaret() );
    break;
  case TacTextInputKey::LeftArrow:
    OnArrowKeyPressed( mNumGlyphsBeforeCaret[ 0 ] > 0, -1, GetMinCaret() );
    break;
  case TacTextInputKey::Backspace:
    OnDestructivePressed( mNumGlyphsBeforeCaret[ 0 ] - 1 );
    break;
  case TacTextInputKey::Delete:
    OnDestructivePressed( mNumGlyphsBeforeCaret[ 0 ] );
    break;
  }
}

void TacTextInputData::OnCodepoint( TacCodepoint codepoint )
{
  if( !mCaretCount ||
    codepoint == '\b' ) // handled by OnKeyPressed( TacTextInputKey::Backspace )
    return;
  if( mCaretCount == 2 )
    OnDestructivePressed( -1 ); // -1 will be overridden
  TacVector< TacCodepoint > newCodepoints;
  int iCodepoint = 0;
  while( iCodepoint < mNumGlyphsBeforeCaret[ 0 ] )
    newCodepoints.push_back( mCodepoints[ iCodepoint++ ] );
  newCodepoints.push_back( codepoint );
  while( iCodepoint < mCodepoints.size() )
    newCodepoints.push_back( mCodepoints[ iCodepoint++ ] );
  mCodepoints = newCodepoints;
  mNumGlyphsBeforeCaret[ 0 ] ++;
}
