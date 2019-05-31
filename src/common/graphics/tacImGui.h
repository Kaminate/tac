#pragma once

#include "common/math/tacVector2.h"
#include "common/tacString.h"
#include "common/containers/tacVector.h"
#include <map>

struct TacDesktopWindow;
struct TacImGuiWindow;
struct TacKeyboardInput;
struct TacShell;
struct TacTextInputData;
struct TacUI2DDrawData;
struct TacUIRoot;

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

void TacImGuiBegin( const TacString& name, v2 size );
void TacImGuiEnd();

void TacImGuiBeginMenuBar();
//void TacImGuiBeginMenu( const TacString& label );
//void TacImGuiMenuItem( const TacString& label );
//void TacImGuiEndMenu();
void TacImGuiEndMenuBar();

void TacImGuiPushFontSize( int value );
void TacImGuiPopFontSize();

void TacImGuiBeginChild( const TacString& name, v2 size );
void TacImGuiEndChild();

void TacImGuiBeginGroup();
void TacImGuiEndGroup();

void TacImGuiIndent();
void TacImGuiUnindent();

void TacImGuiSetNextWindowPos( v2 pos );
bool TacImGuiCollapsingHeader( const TacString& name );
void TacImGuiSameLine();
void TacImGuiText( const TacString& text );
bool TacImGuiInputText( const TacString& label, TacString& text );
bool TacImGuiSelectable( const TacString& str, bool selected );
bool TacImGuiButton( const TacString& str );
void TacImGuiCheckbox( const TacString& str, bool* value );
bool TacImGuiDragFloat( const TacString& str, float* value );
void TacImGuiDebugDraw();
void TacImGuiSetGlobals(
  TacShell* shell,
  TacDesktopWindow* desktopWindow,
  TacUI2DDrawData* ui2DDrawData );

struct TacImGuiIndentBlock {
  TacImGuiIndentBlock() { TacImGuiIndent(); }
  ~TacImGuiIndentBlock() { TacImGuiUnindent(); } };
#define TAC_IMGUI_INDENT_BLOCK TacImGuiIndentBlock indent##__LINE__;


