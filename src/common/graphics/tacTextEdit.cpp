#include "src/common/graphics/tacTextEdit.h"
#include "src/common/math/tacMath.h" // Min/Max
#include "src/common/tacString.h"

namespace Tac
{


  int                TextInputData::GetMinCaret()
  {
    return Min(
      mNumGlyphsBeforeCaret[ 0 ],
      mNumGlyphsBeforeCaret[ 1 ] );
  }

  int                TextInputData::GetMaxCaret()
  {
    return Max( mNumGlyphsBeforeCaret[ 0 ],
                mNumGlyphsBeforeCaret[ 1 ] );
  }

  void                TextInputData::OnClick( int numGlyphsBeforeCaret )
  {
    mNumGlyphsBeforeCaret[ 0 ] = numGlyphsBeforeCaret;
    mCaretCount = 1;
  }

  void                TextInputData::OnDrag( int numGlyphsBeforeCaret )
  {
    if( !mCaretCount || mNumGlyphsBeforeCaret[ 0 ] == numGlyphsBeforeCaret )
      return;
    mNumGlyphsBeforeCaret[ 1 ] = numGlyphsBeforeCaret;
    mCaretCount = 2;
  }

  void                TextInputData::OnArrowKeyPressed( bool canMoveDirection,
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

  void                TextInputData::OnDestructivePressed( int deletedCodepointsStartIndex )
  {
    if( !mCaretCount )
      return;

    int deletedCodepointCount = 1;
    if( mCaretCount > 1 )
    {
      deletedCodepointsStartIndex = GetMinCaret();
      deletedCodepointCount = GetMaxCaret() - GetMinCaret();
    }

    Vector< Codepoint > newCodepoints;
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

  void                TextInputData::OnKeyPressed( TextInputKey key )
  {
    switch( key )
    {
      case TextInputKey::RightArrow:
        OnArrowKeyPressed( mNumGlyphsBeforeCaret[ 0 ] < mCodepoints.size(), 1, GetMaxCaret() );
        break;
      case TextInputKey::LeftArrow:
        OnArrowKeyPressed( mNumGlyphsBeforeCaret[ 0 ] > 0, -1, GetMinCaret() );
        break;
      case TextInputKey::Backspace:
        OnDestructivePressed( mNumGlyphsBeforeCaret[ 0 ] - 1 );
        break;
      case TextInputKey::Delete:
        OnDestructivePressed( mNumGlyphsBeforeCaret[ 0 ] );
        break;
    }
  }

  void                TextInputData::OnCodepoint( Codepoint codepoint )
  {
    if( !mCaretCount ||
        codepoint == '\b' ) // handled by OnKeyPressed( TextInputKey::Backspace )
      return;
    if( mCaretCount == 2 )
      OnDestructivePressed( -1 ); // -1 will be overridden
    Vector< Codepoint > newCodepoints;
    int iCodepoint = 0;
    while( iCodepoint < mNumGlyphsBeforeCaret[ 0 ] )
      newCodepoints.push_back( mCodepoints[ iCodepoint++ ] );
    newCodepoints.push_back( codepoint );
    while( iCodepoint < mCodepoints.size() )
      newCodepoints.push_back( mCodepoints[ iCodepoint++ ] );
    mCodepoints = newCodepoints;
    mNumGlyphsBeforeCaret[ 0 ] ++;
  }

  void                TextInputData::SetCodepoints( CodepointView codepointView )
  {
    mCodepoints.resize( codepointView.size() );
    for( int i = 0; i < codepointView.size(); ++i )
      mCodepoints[ i ] = codepointView[ i ];
    mCaretCount = 0;
  }

  String              TextInputData::GetText()
  {
    const CodepointView codepointView( mCodepoints.data(), mCodepoints.size());
    const String text = CodepointsToUTF8( codepointView );
    return text;
  }

  void                TextInputData::SetText( StringView text )
  {
    const CodepointView codepointView = UTF8ToCodepoints( text );
    SetCodepoints( codepointView );
  }

}
