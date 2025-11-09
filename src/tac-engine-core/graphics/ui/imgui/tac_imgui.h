// This file is responsible for containing the client facing imgui api

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h" // WindowHandle
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/hid/tac_sys_keyboard_api.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h" // UI2DDrawData
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"


namespace Tac
{
  struct ImGuiID
  {
    ImGuiID( HashValue value = 0 ) : mValue( value ) {}
    bool        IsValid() const { return mValue; }
    operator    HashValue() { return mValue; }
    friend auto operator <=> ( const ImGuiID&, const ImGuiID& ) = default;

    HashValue mValue{};
  };

  struct ImGuiDesktopWindow
  {
    virtual ~ImGuiDesktopWindow() = default;

    WindowHandle    mWindowHandle;
    Optional< v2i > mRequestedPosition;
    Optional< v2i > mRequestedSize;
  };

  // +--> x
  // |
  // v
  // y
  struct ImGuiRect
  {
    static auto FromPosSize( v2 pos, v2 size ) -> ImGuiRect;
    static auto FromMinMax( v2 mini, v2 maxi ) -> ImGuiRect;
    auto GetWidth() const -> float;
    auto GetHeight() const -> float;
    auto GetSize() const -> v2;
    bool ContainsPoint( v2 ) const;
    bool Contains( const ImGuiRect& ) const;
    bool Overlaps( const ImGuiRect& ) const;
    v2 mMini  {};
    v2 mMaxi  {};
  };

  enum class ImGuiCol
  {
    Text,
    TextSelection,
    WindowBackground,
    ChildWindowBackground,
    FrameBG,
    FrameBGHovered,
    FrameBGActive,
    Scrollbar,
    ScrollbarHovered,
    ScrollbarActive,
    ScrollbarBG,
    Checkmark,

    Count,
  };

  enum class ImGuiMouseCursor
  {
    kNone,
    kArrow,
    kResizeNS,
    kResizeEW,
    kResizeNE_SW,
    kResizeNW_SE,
    kCount,
  };

  enum ImGuiNodeFlags
  {
    ImGuiNodeFlags_None        = 0,
    ImGuiNodeFlags_DefaultOpen = 1 << 0
  };

  enum class ImGuiCondition
  {
    kNone = 0,
    kAlways,
    kFirstUse
  };

  struct UIStyle
  {
    UIStyle();

    // outside of the padding is used for window resize
    float windowPadding                     { 8 };

    //    what is itemspaceing?
    v2    itemSpacing                       { 8, 4 };

    float fontSize                          { 23 };
    float buttonPadding                     { 3.0f };
    v4    colors[ ( int )ImGuiCol::Count ]  {};
  };

  struct BeginFrameData
  {
    Timestamp    mElapsedSeconds;
    WindowHandle mMouseHoveredWindow;
  };

  struct ImGuiInitParams
  {
    int                mMaxGpuFrameCount {};
    SettingsNode       mSettingsNode     {};
  };

  auto ImGuiGetMouseCursor() -> ImGuiMouseCursor;
  auto ImGuiGetColName( ImGuiCol ) -> const char*;
  auto ImGuiGetFontSize() -> float;
  auto ImGuiGetItemSpacing() -> const v2&;
  auto ImGuiGetButtonPadding() -> float;
  bool ImGuiIsRectHovered( ImGuiRect );

  //   ImGui System Functions
  void ImGuiInit( const ImGuiInitParams&, Errors& );
  void ImGuiUninit();
  void ImGuiSaveWindowSettings( WindowHandle );
  void ImGuiDebugDraw();

  //   ImGuiBegin functions
  bool ImGuiBegin( const StringView& );
  void ImGuiBeginMenuBar();
  void ImGuiBeginChild( const StringView& name, const v2& size );
  void ImGuiBeginGroup();
  void ImGuiBeginFrame( const BeginFrameData& );

  //   ImGuiEnd functions
  void ImGuiEnd();
  void ImGuiEndMenuBar();
  void ImGuiEndChild();
  void ImGuiEndGroup();
  void ImGuiEndFrame( Errors& );

  void ImGuiPushFontSize( float );
  void ImGuiPopFontSize();

  void ImGuiIndent();
  void ImGuiUnindent();

	auto ImGuiGetWindowHandle() -> WindowHandle;
	auto ImGuiGetWindowHandle( StringView ) -> WindowHandle;
  auto ImGuiGetWindowPos() -> v2;
  auto ImGuiGetWindowContentRegionMin() -> v2;
  auto ImGuiGetContentRect() -> ImGuiRect;
  auto ImGuiGetDrawData() -> UI2DDrawData*;
  void ImGuiSetIsScrollbarEnabled( bool );

  //   ImGuiSetNextWindow...
	void ImGuiSetNextWindowMoveResize();
	void ImGuiSetNextWindowPosition( v2, ImGuiCondition = ImGuiCondition::kNone );
	void ImGuiSetNextWindowStretch();
	void ImGuiSetNextWindowHandle( const WindowHandle& );
  void ImGuiSetNextWindowSize( v2, ImGuiCondition = ImGuiCondition::kNone );
  void ImGuiSetNextWindowDisableBG();

  // TODO: prefix with ImGui
  auto GetID( StringView ) -> ImGuiID;
  void PushID( StringView );
  void PopID();

  //   ImGuiText
  void ImGuiText( const StringView& );
  bool ImGuiInputText( const StringView& label, String& text );


  //   ImGuiButton
  bool ImGuiButton( const StringView&, v2 size = { 0, 0 } );
  bool ImGuiInvisibleButton( const StringView&, v2 size = { 0, 0 }             );

  bool ImGuiSelectable( const StringView& , bool );
  bool ImGuiCheckbox( const StringView& , bool* );
  void ImGuiImage( int hTex, const v2& size, const v4& color = { 1, 1, 1, 1 } );
  bool ImGuiCollapsingHeader( const StringView&, ImGuiNodeFlags = ImGuiNodeFlags_None );
  void ImGuiSameLine();

  //   ImGuiDragFloat
  bool ImGuiDragFloat( const StringView&, float* );
  bool ImGuiDragFloat2( const StringView&, float* );
  bool ImGuiDragFloat3( const StringView&, float* );
  bool ImGuiDragFloat4( const StringView&, float* );
       
  //   ImGuiDragInt
  bool ImGuiDragInt( const StringView&, int* );
  bool ImGuiDragInt2( const StringView&, int* );
  bool ImGuiDragInt3( const StringView&, int* );
  bool ImGuiDragInt4( const StringView&, int* );

  auto ImGuiGetCursorPos() -> v2;
  void ImGuiSetCursorPos( v2 local );

  auto ImGuiGetStyle() -> UIStyle&;
  auto ImGuiGetColor( ImGuiCol ) -> const v4&;

  void ImGuiSetColor( ImGuiCol, v4 );
  void ImGuiDefaultColors();
  void ImGuiDebugColors();

  
  void ImGuiPlatformRenderFrameBegin( Errors& );
  void ImGuiPlatformRender( Errors& );
  void ImGuiPlatformPresent( Errors& );

#define TAC_IMGUI_INDENT_BLOCK            ImGuiIndent();                          \
                                          TAC_ON_DESTRUCT( ImGuiUnindent() );

#define TAC_IMGUI_FONT_SIZE_BLOCK( size ) ImGuiPushFontSize( size );              \
                                          TAC_ON_DESTRUCT( ImGuiPopFontSize() );

#if 1 // todo: refactor
  struct TextInputData;
  void TextInputDataUpdateKeys( TextInputData* inputData,
                                const v2& mousePos,
                                const v2& textPos );

  //int GetCaret( const Vector< Codepoint >& codepoints,
  //              float mousePos );
  void TextInputDataDrawSelection( TextInputData* inputData,
                                   UI2DDrawData* drawData,
                                   const v2& textPos,
                                   const ImGuiRect* clipRect );
#endif


} // namespace Tac
