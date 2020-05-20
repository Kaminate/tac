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


void ImGuiBegin( const StringView& name, v2 size );
void ImGuiEnd();

void ImGuiSetGlobals( v2 mousePositionDesktopWindowspace,
                      bool isWindowDirectlyUnderCursor,
                      double elapsedSeconds,
                      UI2DDrawData* ui2DDrawData,
                      int desktopWindowWidth,
                      int desktopWindowHeight );

void ImGuiBeginMenuBar();
//void ImGuiBeginMenu( const String& label );
//void ImGuiMenuItem( const String& label );
//void ImGuiEndMenu();
void ImGuiEndMenuBar();

void ImGuiPushFontSize( int value );
void ImGuiPopFontSize();

void ImGuiBeginChild( const StringView& name, v2 size );
void ImGuiEndChild();

void ImGuiBeginGroup();
void ImGuiEndGroup();

void ImGuiIndent();
void ImGuiUnindent();

void ImGuiSetNextWindowPos( v2 pos );
bool ImGuiCollapsingHeader( const StringView& name );
void ImGuiSameLine();
void ImGuiText( const StringView& text );
bool ImGuiInputText( const StringView& label, String& text );
bool ImGuiSelectable( const StringView& str, bool selected );
bool ImGuiButton( const StringView& str );
void ImGuiCheckbox( const StringView& str, bool* value );
bool ImGuiDragFloat( const StringView& str, float* value );
bool ImGuiDragInt( const StringView& str, int* value );
void ImGuiDebugDraw();

struct ImGuiIndentBlock {
  ImGuiIndentBlock() { ImGuiIndent(); }
  ~ImGuiIndentBlock() { ImGuiUnindent(); } };
#define TAC_IMGUI_INDENT_BLOCK ImGuiIndentBlock indent##__LINE__;


}
