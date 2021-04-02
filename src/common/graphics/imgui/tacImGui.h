// This file is responsible for containing the client facing imgui api

#pragma once

#include "src/common/math/tacVector2.h"

namespace Tac
{
  struct StringView;
  struct String;
  struct DesktopWindowHandle;
  struct Errors;

  struct ImGuiRect
  {
    static ImGuiRect FromPosSize( v2 pos, v2 size );
    static ImGuiRect FromMinMax( v2 mini, v2 maxi );
    float            GetWidth() const ;
    float            GetHeight() const ;
    bool             ContainsPoint( v2 ) const ;
    v2               GetDimensions() const ;
    v2               mMini = {};
    v2               mMaxi = {};
  };

  void ImGuiInit();
  void ImGuiUninit();
  void ImGuiSaveWindowSettings();
  bool ImGuiBegin( const StringView& );
  void ImGuiEnd();

  //void ImGuiSetGlobals( // v2 mousePositionDesktopWindowspace,
                        //bool isWindowDirectlyUnderCursor,
                        //double elapsedSeconds,
                        //UI2DDrawData* ui2DDrawData,
                        //int desktopWindowWidth,
                        //int desktopWindowHeight );

  void ImGuiBeginMenuBar();
  void ImGuiEndMenuBar();

  void ImGuiPushFontSize( int value );
  void ImGuiPopFontSize();

  void ImGuiBeginChild( const StringView& name, v2 size );
  void ImGuiEndChild();

  void ImGuiBeginGroup();
  void ImGuiEndGroup();

  void ImGuiIndent();
  void ImGuiUnindent();

	void ImGuiSetNextWindowMoveResize();
	void ImGuiSetNextWindowPosition( v2 );
	void ImGuiSetNextWindowStretch();
	void ImGuiSetNextWindowHandle( const DesktopWindowHandle& );
  void ImGuiSetNextWindowSize( v2 );
  bool ImGuiCollapsingHeader( const StringView& name );
  void ImGuiSameLine();
  void ImGuiText( const StringView& text );
  bool ImGuiInputText( const StringView& label, String& text );
  bool ImGuiSelectable( const StringView& str, bool selected );
  bool ImGuiButton( const StringView& str );
  bool ImGuiCheckbox( const StringView& str, bool* value );
  bool ImGuiDragFloat( const StringView& str, float* value );
  bool ImGuiDragFloat2( const StringView& str, float* value );
  bool ImGuiDragFloat3( const StringView& str, float* value );
  bool ImGuiDragFloat4( const StringView& str, float* value );
  bool ImGuiDragInt( const StringView& str, int* value );
  void ImGuiDebugDraw();
  void ImGuiSetCursorPos(v2 local);
  void ImGuiImage(int hTex, v2 size);

  // called by the main thread
  void ImGuiFrameBegin( double elapsedSeconds,
                        const DesktopWindowHandle& mouseHoveredWindow );
  void ImGuiFrameEnd( Errors& );

#define TAC_IMGUI_INDENT_BLOCK ImGuiIndent(); TAC_ON_DESTRUCT( ImGuiUnindent() );


}
