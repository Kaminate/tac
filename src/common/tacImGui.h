#pragma once

#include "common/math/tacVector2.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"

struct TacUIRoot;

//extern TacUIRoot* mUIRoot;

struct TacImGuiRect
{
  static TacImGuiRect FromPosSize( v2 pos, v2 size );
  static TacImGuiRect FromMinMax( v2 mini, v2 maxi );
  float GetWidth();
  float GetHeight();
  v2 GetDimensions();
  v2 mMini = {};
  v2 mMaxi = {};
};

// move to cpp?
struct TacImGuiWindow
{
  void BeginFrame();
  void Text( const TacString& utf8 );
  bool InputText( const TacString& label, TacString& text );
  void Checkbox( const TacString& str, bool* value );
  bool Button( const TacString& str );
  bool Selectable( const TacString& str, bool selected );
  void ItemSize( v2 size );
  void ComputeClipInfo( bool* clipped, TacImGuiRect* clipRect );
  bool IsHovered( const TacImGuiRect& clipRect );
  void SameLine();
  void BeginGroup();
  void EndGroup();
  TacImGuiWindow* BeginChild( const TacString& name, v2 size );
  void EndChild();
  void UpdateMaxCursorDrawPos( v2 pos );

  TacUIRoot* mUIRoot = nullptr;
  TacString mName;
  //TacVector< TacImGuiWindow* > mChildren;
  TacImGuiWindow* mParent = nullptr;

  v2 mMaxiCursorDrawPos;
  v2 mCurrCursorDrawPos;
  v2 mPrevCursorDrawPos;

  // rename to mPosWindowspace
  v2 mPos = {};
  v2 mSize = {};
  TacImGuiRect mContentRect;
  float mCurrLineHeight;
  float mPrevLineHeight;

  float mScroll = 0;
  v2 mScrollMousePosScreenspaceInitial;
  bool mScrolling = false;

  v2 mGroupSavedCursorDrawPos;

  TacVector< float > mXOffsets;
};



