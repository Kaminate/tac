// This file is responsible for containing the client facing imgui api

#pragma once

#include "tac-engine-core/i18n/tac_localization.h" // Codepoint
#include "tac-engine-core/shell/tac_shell_timestep.h"
#include "tac-engine-core/window/tac_window_handle.h" // WindowHandle
#include "tac-engine-core/window/tac_sim_window_api.h"
#include "tac-engine-core/hid/tac_sim_keyboard_api.h"
#include "tac-engine-core/graphics/ui/tac_ui_2d.h" // UI2DDrawData
#include "tac-engine-core/settings/tac_settings_node.h"
#include "tac-std-lib/containers/tac_vector.h"
#include "tac-std-lib/math/tac_vector2i.h"
#include "tac-std-lib/math/tac_vector4.h"
#include "tac-std-lib/preprocess/tac_preprocessor.h"


namespace Tac
{
  struct SysWindowApi;

  struct ImGuiDesktopWindow
  {
    virtual ~ImGuiDesktopWindow() = default;

    WindowHandle mWindowHandle;
  };

  // +--> x
  // |
  // v
  // y
  struct ImGuiRect
  {
    static ImGuiRect FromPosSize( v2 pos, v2 size );
    static ImGuiRect FromMinMax( v2 mini, v2 maxi );
    float            GetWidth() const;
    float            GetHeight() const;
    v2               GetSize() const;
    bool             ContainsPoint( v2 ) const;
    bool             Contains( const ImGuiRect& ) const;
    bool             Overlaps( const ImGuiRect& ) const;

    v2               mMini = {};
    v2               mMaxi = {};
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
    kResizeNESW,
    kResizeNWSE,
    kCount,
  };

  ImGuiMouseCursor ImGuiGetMouseCursor();
  const char* ImGuiGetColName( ImGuiCol );

  enum ImGuiNodeFlags
  {
    ImGuiNodeFlags_None        = 0,
    ImGuiNodeFlags_DefaultOpen = 1 << 0
  };

  //TAC_ENUM_CLASS_BIT_OPS( ImGuiNodeFlags );

  struct UIStyle
  {
    UIStyle();
    float windowPadding = 8;

    //    what is itemspaceing?
    v2    itemSpacing = { 8, 4 };

    float fontSize = 23;
    float buttonPadding = 3.0f;
    v4    colors[ ( int )ImGuiCol::Count ] = {};
  };

  float     ImGuiGetFontSize();
  const v2& ImGuiGetItemSpacing();
  float     ImGuiGetButtonPadding();

  struct BeginFrameData
  {
    Timestamp                  mElapsedSeconds;
    const WindowHandle& mMouseHoveredWindow;
  };

  //struct ImGuiCreateWindowParams
  //{
  //  v2 mPos;
  //  v2 mSize;
  //};

  //using ImGuiSetWindowPos = void( * )( WindowHandle, v2i );
  //using ImGuiSetWindowSize = void( * )( WindowHandle, v2i );
  //using ImGuiCreateWindow = WindowHandle( * )( const ImGuiCreateWindowParams& );
  //using ImGuiDestroyWindow = void ( * )( WindowHandle );

  struct ImGuiInitParams
  {
    int                mMaxGpuFrameCount{};
    SimWindowApi*      mSimWindowApi{};
    SimKeyboardApi*    mSimKeyboardApi{};
    SettingsNode       mSettingsNode;
    //ImGuiSetWindowPos  mSetWindowPos{};
    //ImGuiSetWindowSize mSetWindowSize{};
    //ImGuiCreateWindow  mCreateWindow{};
    //ImGuiDestroyWindow mDestroyWindow{};
  };

  //   ImGui System Functions
  void ImGuiInit( const ImGuiInitParams& );
  void ImGuiUninit();
  void ImGuiSaveWindowSettings( WindowHandle );
  void ImGuiDebugDraw();
  // called by the main thread

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

	WindowHandle ImGuiGetWindowHandle();

  //   ImGuiSetNextWindow...
	void ImGuiSetNextWindowMoveResize();
	void ImGuiSetNextWindowPosition( v2 );
	void ImGuiSetNextWindowStretch();
	void ImGuiSetNextWindowHandle( const WindowHandle& );
  void ImGuiSetNextWindowSize( v2 );
  void ImGuiSetNextWindowDisableBG();


  //   ImGuiText
  void ImGuiText( const StringView& );
  
  //   ImGui... widgets
  bool ImGuiInputText( const StringView& label, String& text );
  bool ImGuiSelectable( const StringView& , bool );
  bool ImGuiButton( const StringView& );
  bool ImGuiCheckbox( const StringView& , bool* );
  void ImGuiImage( int hTex, const v2& size, const v4& color = { 1,1,1,1 } );
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

  v2   ImGuiGetCursorPos();
  void ImGuiSetCursorPos( v2 local );

  UIStyle&  ImGuiGetStyle();
  const v4& ImGuiGetColor( ImGuiCol );

  void      ImGuiSetColor( ImGuiCol, v4 );
  void      ImGuiDefaultColors();
  void      ImGuiDebugColors();

  UI2DDrawData* ImGuiGetDrawData();
  void ImGuiSetIsScrollbarEnabled( bool );
  

  struct ImGuiSimFrameDraws;
  ImGuiSimFrameDraws ImGuiGetSimFrameDraws();

  struct ImGuiSysDrawParams
  {
    ImGuiSimFrameDraws* mSimFrameDraws {};
    const SysWindowApi* mWindowApi     {};
    Timestamp           mTimestamp     {};
  };
  void               ImGuiPlatformRender( ImGuiSysDrawParams, Errors& );


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
