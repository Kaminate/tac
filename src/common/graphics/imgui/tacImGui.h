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


void TacImGuiBegin( const TacString& name, v2 size );
void TacImGuiEnd();

void TacImGuiSetGlobals(
  v2 mousePositionDesktopWindowspace,
  bool isWindowDirectlyUnderCursor,
  double elapsedSeconds,
  TacUI2DDrawData* ui2DDrawData);

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
bool TacImGuiDragInt( const TacString& str, int* value );
void TacImGuiDebugDraw();

struct TacImGuiIndentBlock {
  TacImGuiIndentBlock() { TacImGuiIndent(); }
  ~TacImGuiIndentBlock() { TacImGuiUnindent(); } };
#define TAC_IMGUI_INDENT_BLOCK TacImGuiIndentBlock indent##__LINE__;


