#pragma once

#include "common/math/tacVector2.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"
#include <map>

struct TacUIRoot;
struct TacImGuiWindow;
struct TacUI2DDrawData;
struct TacKeyboardInput;
struct TacTextInputData;

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

// move some of the things to cpp?
struct TacImGuiGlobals
{
  TacImGuiWindow* FindWindow( const TacString& name );
  bool IsHovered( const TacImGuiRect& rect );

  v2 mNextWindowPos = {};
  // TODO: different space
  v2 mMousePositionDesktopWindowspace = {};
  bool mIsWindowDirectlyUnderCursor = false;
  double mElapsedSeconds = 0;
  TacVector< TacImGuiWindow* > mAllWindows;
  TacVector< TacImGuiWindow* > mWindowStack;
  TacImGuiWindow* mCurrentWindow = nullptr;
  TacUI2DDrawData* mUI2DDrawData = nullptr;
  TacKeyboardInput* mKeyboardInput = nullptr;
};
extern TacImGuiGlobals gTacImGuiGlobals;


void TacImGuiSetNextWindowPos( v2 pos );
void TacImGuiBegin( const TacString& name, v2 size );
void TacImGuiEnd();
bool TacImGuiCollapsingHeader( const TacString& name );
void TacImGuiPushFontSize( int value );
void TacImGuiPopFontSize();
void TacImGuiBeginChild( const TacString& name, v2 size );
void TacImGuiEndChild();
void TacImGuiBeginGroup();
void TacImGuiEndGroup();
void TacImGuiIndent();
void TacImGuiUnindent();
void TacImGuiSameLine();
void TacImGuiText( const TacString& text );
bool TacImGuiInputText( const TacString& label, TacString& text );
bool TacImGuiSelectable( const TacString& str, bool selected );
bool TacImGuiButton( const TacString& str );
void TacImGuiCheckbox( const TacString& str, bool* value );
void TacImGuiDragFloat( const TacString& str, float* value );

struct TacImGuiIndentBlock {
  TacImGuiIndentBlock() { TacImGuiIndent(); }
  ~TacImGuiIndentBlock() { TacImGuiUnindent(); } };
#define TAC_IMGUI_INDENT_BLOCK TacImGuiIndentBlock indent##__LINE__;

// move to cpp?
struct TacImGuiWindow
{
  TacImGuiWindow();
  ~TacImGuiWindow();
  void BeginFrame();
  void ItemSize( v2 size );
  void ComputeClipInfo( bool* clipped, TacImGuiRect* clipRect );
  void UpdateMaxCursorDrawPos( v2 pos );
  int GetID();

  TacString mName;
  TacImGuiWindow* mParent = nullptr;

  v2 mMaxiCursorDrawPos;
  v2 mCurrCursorDrawPos;
  v2 mPrevCursorDrawPos;

  // rename to mPosDesktopWindowspace
  v2 mPos = {};
  v2 mSize = {};
  TacImGuiRect mContentRect;
  float mCurrLineHeight = 0;
  float mPrevLineHeight = 0;

  float mScroll = 0;
  v2 mScrollMousePosScreenspaceInitial;
  bool mScrolling = false;

  v2 mGroupSavedCursorDrawPos;

  TacVector< float > mXOffsets;


  int mActiveID = -1;
  int mIDCounter = 0;

  TacTextInputData* inputData;
  std::map< int, bool > mCollapsingHeaderStates;
  std::map< int, bool > mDragFloatStates;
};
