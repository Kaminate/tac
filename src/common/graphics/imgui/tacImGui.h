#pragma once

#include "src/common/math/tacVector2.h"
#include "src/common/tacString.h"
#include "src/common/containers/tacVector.h"
#include <map>

namespace Tac
{


struct DesktopWindow;
struct ImGuiWindow;
struct KeyboardInput;
struct Shell;
struct TextInputData;
struct UI2DDrawData;
struct UIRoot;

struct ImGuiRect
{
  static ImGuiRect FromPosSize( v2 pos, v2 size );
  static ImGuiRect FromMinMax( v2 mini, v2 maxi );
  float GetWidth();
  float GetHeight();
  v2 GetDimensions();
  v2 mMini = {};
  v2 mMaxi = {};
};


void ImGuiBegin( const String& name, v2 size );
void ImGuiEnd();

void ImGuiSetGlobals(
  v2 mousePositionDesktopWindowspace,
  bool isWindowDirectlyUnderCursor,
  double elapsedSeconds,
  UI2DDrawData* ui2DDrawData);

void ImGuiBeginMenuBar();
//void ImGuiBeginMenu( const String& label );
//void ImGuiMenuItem( const String& label );
//void ImGuiEndMenu();
void ImGuiEndMenuBar();

void ImGuiPushFontSize( int value );
void ImGuiPopFontSize();

void ImGuiBeginChild( const String& name, v2 size );
void ImGuiEndChild();

void ImGuiBeginGroup();
void ImGuiEndGroup();

void ImGuiIndent();
void ImGuiUnindent();

void ImGuiSetNextWindowPos( v2 pos );
bool ImGuiCollapsingHeader( const String& name );
void ImGuiSameLine();
void ImGuiText( const String& text );
bool ImGuiInputText( const String& label, String& text );
bool ImGuiSelectable( const String& str, bool selected );
bool ImGuiButton( const String& str );
void ImGuiCheckbox( const String& str, bool* value );
bool ImGuiDragFloat( const String& str, float* value );
bool ImGuiDragInt( const String& str, int* value );
void ImGuiDebugDraw();

struct ImGuiIndentBlock {
  ImGuiIndentBlock() { ImGuiIndent(); }
  ~ImGuiIndentBlock() { ImGuiUnindent(); } };
#define TAC_IMGUI_INDENT_BLOCK ImGuiIndentBlock indent##__LINE__;


}
